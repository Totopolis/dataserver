// page_type.h
//
#ifndef __SDL_SYSTEM_PAGE_TYPE_H__
#define __SDL_SYSTEM_PAGE_TYPE_H__

#pragma once

namespace sdl { namespace db {

#pragma pack(push, 1) 

struct pageType // 1 byte
{
    enum T {
        null = 0,
        data = 1,           //1 – data page. This holds data records in a heap or clustered index leaf-level.
        index = 2,          //2 – index page. This holds index records in the upper levels of a clustered index and all levels of non-clustered indexes.
        textmix = 3,        //3 – text mix page. A text page that holds small chunks of LOB values plus internal parts of text tree. These can be shared between LOB values in the same partition of an index or heap.
        texttree = 4,       //4 – text tree page. A text page that holds large chunks of LOB values from a single column value.
        sort = 7,           //7 – sort page. A page that stores intermediate results during a sort operation.
        GAM = 8,            //8 – GAM page. Holds global allocation information about extents in a GAM interval (every data file is split into 4GB chunks – the number of extents that can be represented in a bitmap on a single database page). Basically whether an extent is allocated or not. GAM = Global Allocation Map. The first one is page 2 in each file. More on these in a later post.
        SGAM = 9,           //9 – SGAM page. Holds global allocation information about extents in a GAM interval. Basically whether an extent is available for allocating mixed-pages. SGAM = Shared GAM. the first one is page 3 in each file. More on these in a later post.
        IAM = 10,           //10 – IAM page. Holds allocation information about which extents within a GAM interval are allocated to an index or allocation unit, in SQL Server 2000 and 2005 respectively. IAM = Index Allocation Map. More on these in a later post.
        PFS = 11,           //11 – PFS page. Holds allocation and free space information about pages within a PFS interval (every data file is also split into approx 64MB chunks – the number of pages that can be represented in a byte-map on a single database page. PFS = Page Free Space. The first one is page 1 in each file. More on these in a later post.
        boot = 13,          //13 – boot page. Holds information about the database. There’s only one of these in the database. It’s page 9 in file 1.
        fileheader = 15,    //15 – file header page.Holds information about the file.There’s one per file and it’s page 0 in the file.
        diffmap = 16,       //16 – diff map page. Holds information about which extents in a GAM interval have changed since the last full or differential backup. The first one is page 6 in each file.
        MLmap = 17,         //17 – ML map page.Holds information about which extents in a GAM interval have changed while in bulk - logged mode since the last backup.This is what allows you to switch to bulk - logged mode for bulk - loads and index rebuilds without worrying about breaking a backup chain.The first one is page 7 in each file.
        deallocated = 18,   //18 – a page that’s be deallocated by DBCC CHECKDB during a repair operation.
        temporary = 19,     //19 – the temporary page that ALTER INDEX … REORGANIZE(or DBCC INDEXDEFRAG) uses when working on an index.
        preallocated = 20,  //20 – a page pre - allocated as part of a bulk load operation, which will eventually be formatted as a ‘real’ page.
    };
    uint8 val;
    operator T() const {
        static_assert(sizeof(*this) == 1, "");
        return static_cast<T>(val);
    }
    pageType & operator=(T t) {
        val = static_cast<decltype(val)>(t);
        return *this;
    }
    pageType() = default;
    pageType(T t) { 
        *this = t;
    }
};

struct guid_t // 16 bytes
{
    uint32 a;
    uint16 b;
    uint16 c;
    uint8 d;
    uint8 e;
    uint8 f;
    uint8 g;
    uint8 h;
    uint8 i;
    uint8 j;
    uint8 k;
};

struct pageFileID // 6 bytes
{
    uint32 pageId;  // 4 bytes : PageID
    uint16 fileId;  // 2 bytes : FileID
};

struct pageLSN // 10 bytes
{
    uint32 lsn1;
    uint32 lsn2;
    uint16 lsn3;
};

struct pageXdesID // 6 bytes 
{
    uint32 id2;
    uint16 id1;
};

struct nchar_t // 2 bytes
{
    uint16 c;
};

/*
Datetime Data Type

The datetime data type is a packed byte array which is composed of two integers - the number of days since 1900-01-01 (a signed integer value),
and the number of clock ticks since midnight (where each tick is 1/300th of a second), as explored on this blog and this Microsoft article.
http://www.sql-server-performance.com/2004/datetime-datatype/
https://msdn.microsoft.com/en-us/library/aa175784(v=sql.80).aspx

This gives the interesting result that a zero datetime value with all bytes zero is equal to 1900-01-01 at midnight. 
It also tells us that the datetime structure is a very inefficient way to store time (the datetime2 data type was created to address this concern), 
except that it is excellent at defaulting to a reasonable zero point, and that the date and time parts can be split apart very easily by SQL server.
Note that while it is capable of storing days up to the year plus or minus 58 million, it is limited by rule to only go between 1753-01-01 and 9999-12-31.
And note that while the clock ticks part is a 32-bit number, in practice the highest value used will be 25919999.
Since the datatime clock ticks are 1/300ths of a second, while they display accuracy to the millisecond, 
the will actually be rounded to the nearest 0, 3, 7, or 10 millisecond boundary in all conversions and comparisons.
*/
struct datetime_t // 8 bytes
{
    uint32 t;   // clock ticks since midnight (where each tick is 1/300th of a second)
    int32 d;    // days since 1900-01-01

    enum { u_date_diff = 25567 }; // = SELECT DATEDIFF(d, '19000101', '19700101');

    // convert to number of seconds that have elapsed since 00:00:00 UTC, 1 January 1970
    static size_t get_unix_time(datetime_t const & src);
    size_t get_unix_time() const {
        return get_unix_time(*this);
    }
    static datetime_t set_unix_time(size_t);

    bool is_null() const {
        return !d && !t;
    }
    bool is_valid() const {
        return d >= u_date_diff;
    }
};

#pragma pack(pop)

} // db
} // sdl

#endif // __SDL_SYSTEM_PAGE_TYPE_H__