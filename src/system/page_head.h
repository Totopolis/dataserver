// page_head.h
//
#ifndef __SDL_SYSTEM_PAGE_HEAD_H__
#define __SDL_SYSTEM_PAGE_HEAD_H__

#pragma once

#include "page_type.h"
#include "page_meta.h"

//http://ugts.azurewebsites.net/data/UGTS/document/2/4/46.aspx
//http://www.sqlskills.com/blogs/paul/inside-the-storage-engine-anatomy-of-a-page/

namespace sdl { namespace db {

#pragma pack(push, 1) 

struct page_head // 96 bytes page header
{
    enum { page_size = kilobyte<8>::value }; // 8192 bytes - A database file at its simplest level is an array of 8KB pages
    enum { head_size = 96 };
    enum { body_size = page_size - head_size }; // 8096 bytes
    enum { body_limit = 8060+1 };               // size limit for a data record is 8060 bytes

    struct data_type {
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
        char raw[sizeof(data_type)];
    };

    bool is_null() const {
        return pageType::type::null == data.type;
    }
    bool is_data() const {
        return pageType::type::data == data.type;
    }
    static const char * begin(page_head const * head) {
        return reinterpret_cast<char const *>(head);
    }
    static const char * end(page_head const * head) {
        return page_head::begin(head) + page_size;
    }
    static const char * body(page_head const * head) {
        return page_head::begin(head) + head_size;
    }
};

/* http://ugts.azurewebsites.net/data/UGTS/document/2/4/46.aspx
Status Byte A - 1 byte - a bit mask with the following information: 
Bit 0: not used.
Bits 1-3: type of record:
    0 = data, 
    1 = forward, 
    2 = forwarding stub,
    3 = index,
    4 = blob or row overflow data,
    5 = ghost index record,
    6 = ghost data record,
    7 = not used.
Bit 4: record has a NULL bitmap.
Bit 5: record has variable length columns.
Bit 6: record has versioning info.
Bit 7: not used.*/

/* http://www.sqlskills.com/blogs/paul/inside-the-storage-engine-anatomy-of-a-record/
Byte 0 is the TagA byte of the record metadata.
It’s 0x30, which corresponds to 0x10 (bit 4) and 0x20 (bit 5). Bit 4 means the record has a null bitmap and bit 5 means the record has variable length columns.
If 0x40 (bit 6) was also set, that would indicate that the record has a versioning tag.
If 0x80 (bit 7) was also set, that would indicate that byte 1 has a value in it.
Bits 1-3 of byte 0 give the record type. The possible values are:
0 = primary record. A data record in a heap that hasn’t been forwarded or a data record at the leaf level of a clustered index.
1 = forwarded record
2 = forwarding record
3 = index record
4 = blob fragment
5 = ghost index record
6 = ghost data record
7 = ghost version record. A special 15-byte record containing a single byte record header plus a 14-byte versioning tag that is used in some circumstances (like ghosting a versioned blob record)
In our example, none of these bits are set which means the record is a primary record. If the record was an index record, byte 0 would have the value 0x36.
Remember that the record type starts on bit 1, not bit 0, and so the record type value from the enumeration above needs to be shifted left a bit (multiplied by two) 
to get its value in the byte.
Byte 1 is the TagB byte of the record metadata. It can either be 0x00 or 0x01. 
If it is 0x01, that means the record type is ghost forwarded record. In this case it’s 0x00, which is what we expect given the TagA byte value.
*/

enum class recordType {
    primary_record      = 0,
    forwarded_record    = 1,
    forwarding_record   = 2,
    index_record        = 3,
    blob_fragment       = 4,
    ghost_index         = 5,
    ghost_data          = 6,
    ghost_version       = 7,
};

//Fixed record header
struct row_head     // 4 bytes
{
    struct data_type {
        bitmask8    statusA;    // Status Byte A - 1 byte - a bit mask that contain information about the row, such as row type
        bitmask8    statusB;    // Status Byte B - 1 byte - Only present for data records - indicates if the record is a ghost forward record.
        uint16      fixedlen;   // 2 bytes - the offset to the end of the fixed length column data where the number of columns in the row is stored
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
    bool has_null() const       { return data.statusA.bit<4>(); }
    bool has_variable() const   { return data.statusA.bit<5>(); }
    bool has_version() const    { return data.statusA.bit<6>(); }   

    recordType get_type() const { // Bits 1-3 of byte 0 give the record type
        const int v = (data.statusA.byte & 0xE) >> 1;
        return static_cast<recordType>(v);
    }
    bool is_type(recordType t) const {
        return this->get_type() == t;
    }
    bool is_forwarded_record() const    { return this->is_type(recordType::forwarded_record); }
    bool is_forwarding_record() const   { return this->is_type(recordType::forwarding_record); }
    bool is_index_record() const        { return this->is_type(recordType::index_record); }
    
    mem_range_t fixed_data() const;// fixed length column data
    size_t fixed_size() const;

    static const char * begin(row_head const * p) {
        return reinterpret_cast<char const *>(p);
    }
};

struct lobtype // 2 bytes
{
    enum type
    {
        SMALL_ROOT          = 0,
        LARGE_ROOT          = 1,
        INTERNAL            = 2,
        DATA                = 3,
        LARGE_ROOT_SHILOH   = 4,
        LARGE_ROOT_YUKON    = 5,
        SUPER_LARGE_ROOT    = 6,
        _7                  = 7, // 7 Seems to exist but doesn't have a name. Unaware of it's usage.
        NONE                = 8  // 9+ are all INVALID AFAIK.
    };
    uint16  _16;

    operator type() const {
        static_assert(sizeof(*this) == 2, "");
        return static_cast<type>(_16);
    }
};

struct lob_head // 10 bytes
{
    uint64      blobID;     // 8 bytes
    lobtype     type;       // 2 bytes
};

struct LobSlotPointer // 12 bytes
{
    uint32      size;   // 4 bytes
    recordID    row;    // 8 bytes
};

struct LargeRootYukon
{
    lob_head    head;           // 10 bytes
    uint16      maxlinks;       // 2 bytes
    uint16      curlinks;       // 2 bytes
    uint16      level;          // 2 bytes
    uint32      _0x10;          // 4 bytes 
    LobSlotPointer data[1];     // [curlinks]
};

// Row-overflow page pointer structure
struct overflow_page // 24 bytes
{
    complextype     type;           // 0x00 : 2 bytes (2 = Row-overflow pointer; 4 = BLOB Inline Root; 5 = Sparse vector; 1024 = Forwarded record back pointer)
    uint16          _0x02;          // 0x02 : Link ?
    uint16          _0x04;          // 0x04 : UpdateSeg ?
    uint32          _0x06;          // 0x06 : timestamp
    uint16          _0x0A;          // 0x0A : two bytes always zero
    uint32          length;         // 0x0C : length of the data in bytes
    recordID        row;            // 0x10 : 8 bytes
};

// Text page pointer structure
struct text_pointer // 16 bytes
{
    uint32      timestamp;      // 4 bytes (TextTimeStamp)
    uint32      _0x04;          // 4 bytes
    recordID    row;            // 8 bytes (page locator plus a 2-byte slot index)
};

struct forwarding_stub // 9 bytes
{
    struct data_type {
        bitmask8    statusA;    // 1 byte
        recordID    row;        // 8 bytes
    };
    union {
        data_type data; // 9 bytes
        char raw[sizeof(data_type)];
    };
};

// back-pointer to the forwarding record
struct forwarded_stub // 10 bytes
{
    struct data_type {
        uint16      _16;    // 2 bytes
        recordID    row;    // 8 bytes
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
};


#pragma pack(pop)

// At the end of page is a slot array of 2-byte values, 
// each holding the offset to the start of the record. 
// The slot array grows backwards as records are added.
class slot_array : noncopyable {
    page_head const * const head;
public:
    typedef uint16 value_type;

    explicit slot_array(page_head const * h) : head(h) {
        SDL_ASSERT(head);
    }
    bool empty() const { 
        return 0 == size();
    }
    size_t size() const;
    uint16 operator[](size_t i) const;

    const uint16 * rbegin() const; // at last item
    const uint16 * rend() const;
private:
    std::vector<uint16> copy() const;
};

template<class T>
struct null_bitmap_traits {
    enum { value = 0 };
};

class null_bitmap : noncopyable {
    row_head const * const record;
    size_t const m_size; // # of columns
public:
    typedef uint16 column_num;
    explicit null_bitmap(row_head const *);

    template<class T> // T = row type
    explicit null_bitmap(T const * row): null_bitmap(&row->data.head) {
        static_assert(null_bitmap_traits<T>::value, "null_bitmap");
    }
    template<class T> // T = row type
    static bool has_null(T const * row) {
        static_assert(null_bitmap_traits<T>::value, "null_bitmap");
        return row->data.head.has_null();
    }
    bool empty() const {
        return 0 == size();
    }
    size_t size() const { return m_size; }
    bool operator[](size_t) const; // true if column in row contains a NULL value

    size_t count_reverse_null() const;
    size_t count_null() const;

    const char * begin() const; // at column_num field
    const char * end() const;

private:
    static size_t size(row_head const *); // # of columns
    static const char * begin(row_head const *); 
    // Variable number of bytes to store one bit per column in the record
    size_t col_bytes() const; // # bytes for columns (# of columns / 8, rounded up to the nearest whole number)
    const char * first_col() const; // at first item
};

template<class T>
struct variable_array_traits {
    enum { value = 0 };
};

/* http://ugts.azurewebsites.net/data/UGTS/document/2/4/46.aspx 
End Offsets of Variable length columns - 2 bytes for each variable length column. 
This stores the offset of the first byte past the end of the column data, relative to the start of the record.
If this two byte value has the high-bit set to 1, then the column is a complex column (such as a sparse vector),
otherwise it is a simple column.
Sparse vector columns are described in detail in the section on filtered indexes and sparse columns. */

class variable_array : noncopyable {
    row_head const * const record;
    size_t const m_size;// # of variable-length columns
public:
    static bool is_highbit(uint16 v) { return (v & (1 << 15)) != 0; }
    static uint16 highbit_off(uint16 v) { return v & 0x7fff; }
public:
    typedef uint16 column_num;
    explicit variable_array(row_head const *);

    template<class T> // T = row type
    explicit variable_array(T const * row): variable_array(&row->data.head) {
        static_assert(variable_array_traits<T>::value, "variable_array");
    }
    template<class T> // T = row type
    static bool has_variable(T const * row) {
        static_assert(variable_array_traits<T>::value, "variable_array");
        return row->data.head.has_variable();
    }
    bool empty() const {
        return 0 == size();
    }
    size_t size() const { return m_size; }

    typedef std::pair<uint16, bool> uint16_bool;
    uint16_bool operator[](size_t) const; // offset array with high-bit 

    uint16 offset(size_t) const; // offset array without high-bit

    static uint16 offset(uint16_bool const & d) {
        return highbit_off(d.first);
    }    
    bool is_complex(size_t i) const { // is a complex column
       return (*this)[i].second;
    }
    const char * begin() const; // start address of variable_array
    const char * end() const; // end address of variable_array

    mem_range_t back_var_data() const; // last variable-length column data
    mem_range_t var_data(size_t) const; // variable-length column data
    
    size_t var_data_bytes(size_t i) const; // variable-length column bytes
    size_t var_data_size() const; // variable-length data size

    bool is_overflow_page(size_t) const;
    bool is_text_pointer(size_t) const;

    complextype::type get_complextype(size_t) const;
    overflow_page const * get_overflow_page(size_t) const; // returns nullptr if wrong type
    text_pointer const * get_text_pointer(size_t) const; // returns nullptr if wrong type
private:
    static size_t size(row_head const *); // # of variable-length columns
    static const char * begin(row_head const *); 
    size_t col_bytes() const; // # bytes for columns
    const char * first_col() const; // at first item of uint16[]
};

class row_data: noncopyable {
    row_head const * const record;
public:
    null_bitmap const null;
    variable_array const variable;
public:
    explicit row_data(row_head const *);

    size_t size() const; // # of columns

    const char * begin() const;
    const char * end() const;

    bool is_null(size_t) const; // returns true if column is [NULL]    
    bool is_fixed(size_t) const; // returns false if column is [NULL] or variable

    mem_range_t data() const {
        return { begin(), end() };
    }
};

class forwarding_record: noncopyable {
    forwarding_stub const * const record;
public:
    explicit forwarding_record(row_head const *);
    recordID const & row() const {
        return record->data.row;
    }
};

namespace cast {

template<class T>
T const * page_body(page_head const * const p) {
    if (p) {
        A_STATIC_ASSERT_IS_POD(T);
        static_assert(sizeof(T) <= page_head::body_size, "");
        char const * body = page_head::body(p);
        return reinterpret_cast<T const *>(body);
    }
    SDL_ASSERT(0);
    return nullptr;
}

template<class T>
T const * page_row(page_head const * const p, slot_array::value_type const pos) {
    if (p) {
        A_STATIC_ASSERT_IS_POD(T);
        static_assert(sizeof(T) <= page_head::body_size, "");
        if ((pos >= page_head::head_size) && (pos < page_head::page_size)) {
            const char * row = page_head::begin(p) + pos;
            SDL_ASSERT(row < (const char *)slot_array(p).rbegin());
            return reinterpret_cast<T const *>(row);
        }
        SDL_ASSERT(!"bad_pos");
        return nullptr;
    }
    SDL_ASSERT(0);
    return nullptr;
}

} // cast

struct page_head_meta: is_static {

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
        ,flagBits
        ,indexId
        ,prevPage
        ,pminlen
        ,nextPage
        ,slotCnt
        ,objId
        ,freeCnt
        ,freeData
        ,pageId
        ,reservedCnt
        ,lsn
        ,xactReserved
        ,xdesId
        ,ghostRecCnt
        ,tornBits
    >::Type type_list;
};

struct row_head_meta: is_static {

    typedef_col_type_n(row_head, statusA);
    typedef_col_type_n(row_head, statusB);
    typedef_col_type_n(row_head, fixedlen);

    typedef TL::Seq<
        statusA
        ,statusB
        ,fixedlen
    >::Type type_list;
};

namespace unit {
    struct pageIndex{};
    struct fileIndex{};
}
typedef quantity<unit::pageIndex, uint32> pageIndex;
typedef quantity<unit::fileIndex, uint16> fileIndex;

inline pageIndex make_page(size_t i) {
    SDL_ASSERT(i < pageIndex::value_type(-1));
    static_assert(pageIndex::value_type(-1) == 4294967295, "");
    return pageIndex(static_cast<pageIndex::value_type>(i));
}

} // db
} // sdl

#endif // __SDL_SYSTEM_PAGE_HEAD_H__