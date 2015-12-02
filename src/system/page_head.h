// page_head.h
//
#ifndef __SDL_SYSTEM_PAGE_HEAD_H__
#define __SDL_SYSTEM_PAGE_HEAD_H__

#pragma once

#include "page_type.h"
#include "common/type_seq.h"
#include "common/static_type.h"

//http://ugts.azurewebsites.net/data/UGTS/document/2/4/46.aspx
//http://www.sqlskills.com/blogs/paul/inside-the-storage-engine-anatomy-of-a-page/

namespace sdl { namespace db {

#pragma pack(push, 1) 

struct page_head // 96 bytes page header
{
    enum { page_size = kilobyte<8>::value }; // A database file at its simplest level is an array of 8KB pages (8192 bytes)
    enum { head_size = 96 };
    enum { body_size = page_size - head_size }; // 8096 bytes

    struct data_type { // IS_LITTLE_ENDIAN
        uint8       headerVersion;  //0x00 : Header Version(m_headerVersion) - 1 byte - 0x01 for SQL Server up to 2008 R2
        pageType    type;           //0x01 : PageType(m_type) - 1 byte - as described above, this will be 0x01 for data pages, or 0x02 for index pages.
        uint8       typeFlagBits;   //0x02 : TypeFlagBits(m_typeFlagBits) - 1 byte - for PFS pages, this will be 1 in any of the pages in the interval have ghost records.For all other page types, this field is ignored.
        uint8       level;          //0x03 : Level(m_level) - 1 byte - for a B - Tree, this is the level in the tree, with 0 being the leaf nodes.For a heap(which is just a flat list), this value is ignored.
        uint16      flagBits;       //0x04 : FlagBits(m_flagBits) - 2 bytes - various page flags : 0x200 means the page has a page checksum stored in the TornBits field.  0x100 means torn page protection is on, and has detected an error.
        uint16      indexId;        //0x06 : IndexID(m_indexId(AllocUnitId.idInd)) - 2 bytes - the idInd member on the allocation unit(similar to index_id on sysindexes, but not the same, see the section on allocation units for details)
        pageFileID  prevPage;       //0x08 : PrevPage - 6 bytes - PageID : FileID of the previous page at the same level in this B - tree, 0 : 0 if this is the first page.
        uint16      pminlen;        //0x0E : PMinLen(pminlen) - 2 bytes - the size in bytes of the fixed length part of the data records on this page.
        pageFileID  nextPage;       //0x10 : NextPage - 6 bytes - PageID : FileID of the next page at the same level in this B - tree, 0 : 0 if this is the last page.
        uint16      slotCnt;        //0x16 : SlotCount - 2 bytes - the number of entries in the slot array(though some of them may be deallocated).
        uint32      objId;          //0x18 : ObjectID(m_objId(AllocUnitId.idObj)) - 4 bytes - in SQL 2000 and earlier, this held the ObjectID.In SQL 2005 and higher, this holds the idObj member of an allocation unit ID, which is not usually the same as sys.objects.object_id.For system tables, it this is the same, but for user defined tables, idObj continues to be sequentially assigned, whereas object_id is a random 32 - bit number.See the section on allocation units for details.
        uint16      freeCnt;        //0x1C : FreeCount(m_freeCnt) - 2 bytes - the total number of bytes of free space on this page, not necessarily contiguous.
        uint16      freeData;       //0x1E : FreeData(m_freeData) - 2 bytes - the offset to the next available position to store row data on this page.
        pageFileID  pageId;         //0x20 : ThisPage(m_pageId) - 6 bytes - PageID : FileID of this page - not really necessary because the position in the file determines this, but helps to detect database corruption.
        uint16      reservedCnt;    //0x26 : ReservedCount(m_reservedCnt) - 2 bytes - the number of bytes that have been reserved by open transactions to allow for rollback and to prevent that space for being used for any other purpose.
        pageLSN     lsn;            //0x28 : LSN(m_lsn) - 10 bytes - the LSN of the last log record that changed this page.
        uint16      xactReserved;   //0x32 : XactReserved(m_xactReserved) - 2 bytes - the amount that was last added to m_reservedCnt.
        pageXdesID  xdesId;         //0x34 : TransactionID - XdesID - 6 bytes - the ID of the last transaction that affected ReservedCount.
        uint16      ghostRecCnt;    //0x3A : GhostRecord(m_ghostRecCnt) - 2 bytes - the number of ghost records on this page.
        int32       tornBits;       //0x3C : TornBits(m_tornBits) - 4 bytes - this will either hold a page or torn bits checksum, which is used to check for corrupted pages due to interrupted disk I / O operations.This is described in greater detail later on.
        uint8       reserved[32];   //0x40 : Reserved - 32 bytes - these don't appear to be used for anything, and are usually zero.
    };
    union {
        data_type data;
        char raw[head_size];
    };
    bool is_null() const {
        return pageType::null == data.type;
    }
    static const char * begin(page_head const * head) {
        return reinterpret_cast<char const *>(head);
    }
    static const char * end(page_head const * head) {
        return page_head::begin(head) + page_size;
    }
};

#pragma pack(pop)

template<class T>
inline T const * page_body(page_head const * p) {
    if (p) {
        A_STATIC_ASSERT_IS_POD(T);
        static_assert(sizeof(T) <= page_head::body_size, "");
        char const * body = ((char const *)p) + page_head::head_size;
        return (T const *)body;
    }
    SDL_ASSERT(0);
    return nullptr;
}

// At the end of page is a slot array of 2-byte values, 
// each holding the offset to the start of the record. 
// The slot array grows backwards as records are added.
class slot_array : noncopyable {
    page_head const * const head;
public:
    explicit slot_array(page_head const * h) : head(h) {
        SDL_ASSERT(head);
    }
    bool empty() const { 
        return 0 == size();
    }
    size_t size() const;
    uint16 operator[](size_t i) const;

    std::vector<uint16> copy() const;

    const uint16 * rbegin() const; // at last item
    const uint16 * rend() const;
private:
    uint16 max_offset() const;
};

namespace meta {
    extern const char empty[];
    template<size_t _offset, class T, const char* _name>
    struct col_type {
        enum { offset = _offset };
        typedef T type;
        static const char * name() { return _name; }
    };
} // meta

#define typedef_col_type(pagetype, member) \
    typedef meta::col_type<offsetof(pagetype, data.member), decltype(pagetype().data.member), meta::empty> member

#define typedef_col_type_n(pagetype, member) \
    static const char c_##member[]; \
    typedef meta::col_type<offsetof(pagetype, data.member), decltype(pagetype().data.member), c_##member> member

#define static_col_name(pagetype, member) \
    const char pagetype::c_##member[] = #member

struct page_header_meta {

    typedef_col_type_n(page_head, headerVersion);
    typedef_col_type_n(page_head, type);
    typedef_col_type_n(page_head, typeFlagBits);
    typedef_col_type_n(page_head, level);
    typedef_col_type_n(page_head, flagBits);
    typedef_col_type_n(page_head, indexId);
    typedef_col_type_n(page_head, prevPage);
    typedef_col_type_n(page_head, pminlen);
    typedef_col_type_n(page_head, nextPage);
    typedef_col_type_n(page_head, slotCnt);
    typedef_col_type_n(page_head, objId);
    typedef_col_type_n(page_head, freeCnt);
    typedef_col_type_n(page_head, freeData);
    typedef_col_type_n(page_head, pageId);
    typedef_col_type_n(page_head, reservedCnt);
    typedef_col_type_n(page_head, lsn);
    typedef_col_type_n(page_head, xactReserved);
    typedef_col_type_n(page_head, xdesId);
    typedef_col_type_n(page_head, ghostRecCnt);
    typedef_col_type_n(page_head, tornBits);

    typedef TL::Seq<
        headerVersion
        ,type
        ,typeFlagBits
        ,level
        , flagBits
        , indexId
        , prevPage
        , pminlen
        , nextPage
        , slotCnt
        , objId
        , freeCnt
        , freeData
        , pageId
        , reservedCnt
        , lsn
        , xactReserved
        , xdesId
        , ghostRecCnt
        ,tornBits
    >::Type type_list;
};

} // db
} // sdl

#endif // __SDL_SYSTEM_PAGE_HEAD_H__