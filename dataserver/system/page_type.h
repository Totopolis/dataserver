// page_type.h
//
#pragma once
#ifndef __SDL_SYSTEM_PAGE_TYPE_H__
#define __SDL_SYSTEM_PAGE_TYPE_H__

#include "dataserver/system/mem_utils.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

struct pageType // 1 byte
{
    enum class type {
        null = 0,
        data = 1,           //1 - data page. This holds data records in a heap or clustered index leaf-level.
        index = 2,          //2 - index page. This holds index records in the upper levels of a clustered index and all levels of non-clustered indexes.
        textmix = 3,        //3 - text mix page. A text page that holds small chunks of LOB values plus internal parts of text tree. These can be shared between LOB values in the same partition of an index or heap.
        texttree = 4,       //4 - text tree page. A text page that holds large chunks of LOB values from a single column value.
        _5 = 5,             //5 - reserved
        _6 = 6,             //6 - reserved
        sort = 7,           //7 - sort page. A page that stores intermediate results during a sort operation.
        GAM = 8,            //8 - GAM page. Holds global allocation information about extents in a GAM interval (every data file is split into 4GB chunks – the number of extents that can be represented in a bitmap on a single database page). Basically whether an extent is allocated or not. GAM = Global Allocation Map. The first one is page 2 in each file. More on these in a later post.
        SGAM = 9,           //9 - SGAM page. Holds global allocation information about extents in a GAM interval. Basically whether an extent is available for allocating mixed-pages. SGAM = Shared GAM. the first one is page 3 in each file. More on these in a later post.
        IAM = 10,           //10 - IAM page. Holds allocation information about which extents within a GAM interval are allocated to an index or allocation unit, in SQL Server 2000 and 2005 respectively. IAM = Index Allocation Map. More on these in a later post.
        PFS = 11,           //11 - PFS page. Holds allocation and free space information about pages within a PFS interval (every data file is also split into approx 64MB chunks – the number of pages that can be represented in a byte-map on a single database page. PFS = Page Free Space. The first one is page 1 in each file. More on these in a later post.
        _12 = 12,           //12 - reserved
        boot = 13,          //13 - boot page. Holds information about the database. There’s only one of these in the database. It’s page 9 in file 1.
        _14 = 14,           //14 - reserved. Instance Header - present only in the master database at 1:10. Stores the various settings you see when getting properties on an SQL server instance.
        fileheader = 15,    //15 - file header page. Holds information about the file. There’s one per file and it’s page 0 in the file.
        diffmap = 16,       //16 - diff map page. Holds information about which extents in a GAM interval have changed since the last full or differential backup. The first one is page 6 in each file.
        MLmap = 17,         //17 - ML map page.Holds information about which extents in a GAM interval have changed while in bulk - logged mode since the last backup.This is what allows you to switch to bulk - logged mode for bulk - loads and index rebuilds without worrying about breaking a backup chain.The first one is page 7 in each file.
        deallocated = 18,   //18 - a page that’s be deallocated by DBCC CHECKDB during a repair operation.
        temporary = 19,     //19 - the temporary page that ALTER INDEX … REORGANIZE(or DBCC INDEXDEFRAG) uses when working on an index.
        preallocated = 20,  //20 - a page pre - allocated as part of a bulk load operation, which will eventually be formatted as a ‘real’ page.
        _end,
        _begin = data
    };
    enum { size = int(type::_end) };
    uint8 value;

    operator type() const {
        SDL_ASSERT(static_cast<type>(value) < type::_end);
        return static_cast<type>(value);
    }
    static pageType init(type i) {
        return { static_cast<uint8>(i) };
    }
    static bool reserved(type t) {
        switch (t) {
        case type::_5:
        case type::_6:
        case type::_12:
        case type::_14:
            return true;
        default:
            SDL_ASSERT(t != type::null);
            SDL_ASSERT(t != type::_end);
            return false;
        }
    }
};

template<pageType::type T> 
using pageType_t = Val2Type<pageType::type, T>;

struct dataType // 1 byte
{
    enum class type {
        null = 0,
        IN_ROW_DATA = 1,
        LOB_DATA = 2,
        ROW_OVERFLOW_DATA = 3,
        _end,
        _begin = IN_ROW_DATA
    };
    enum { size = int(type::_end) };
    uint8 value;
    operator type() const {
        SDL_ASSERT(static_cast<type>(value) < type::_end);
        return static_cast<type>(value);
    }
};

/* Schema Objects / Type
Every object type has a char(2) type code:
AF - AGGREGATE_FUNCTION (2005) - A user-defined aggregate function using CLR code.
C - CHECK_CONSTRAINT - A constraint on a single table to validate column values.
D - DEFAULT_CONSTRAINT - A short SQL procedure to run to generate a default value for a column when a new row is added, and the value for the column is not specified.
F - FOREIGN_KEY_CONSTRAINT - A constraint between two tables which enforces referential integrity.
FN - SQL_SCALAR_FUNCTION (2000) - A user-defined-function (UDF) which returns a single value.
FS - CLR_SCALAR_FUNCTION (2005) - CLR based scalar function
FT - CLR_TABLE_VALUED_FUNCTION (2005) - CLR based table valued function
IF - SQL_INLINE_TABLE_VALUED_FUNCTION (2000) - SQL inline table-valued function where the function is a single SQL statement.  The advantage of this over a multi statement UDF is that the query optimizer can 'understand' how the inline function works and know how many rows of data to expect ahead of time so that the query optimizer knows what to do without having to use temporary results in a table variable and/or a RECOMPILE or a FORCE ORDER option.
IT - INTERNAL_TABLE (2005) - An automatically created persistent table similar to an index which holds results such as XML nodes, transactional data, service queue messages, etc...  These tables will usually have a unique number appended to the name, and are transparent for the most part to your code, except that they take up space in the database, and need to be there to enable the functionality they provide.
P - SQL_STORED_PROCEDURE - A stored procedure which is capable of running any T-SQL, and can return an integer, output parameters, and multiple result sets.
PC - CLR_STORED_PROCEDURE (2005) - CLR based stored procedure
PG - PLAN_GUIDE (2008) - A query hint that can exist separately from the query that it is targeted to optimize, useful when you need to modify the query optimization on a vendor database where you are not allowed to change the objects directly (usually because they might be overwritten during the next upgrade of the product).
PK - PRIMARY_KEY_CONSTRAINT - A primary key on a table
R - RULE - an old-style stand-alone rule which has been deprecated in favor of CHECK_CONSTRAINT starting with SQL 2000.
RF - REPLICATION_FILTER_PROCEDURE (2005) - A replication-filter procedure
S - SYSTEM_TABLE - A system base table
SN - SYNONYM (2005) - An alias name for a database object
SQ - SERVICE_QUEUE (2005) - An asynchronous message queue
TA - CLR_TRIGGER (2005) - CLR based DML trigger
TF - SQL_TABLE_VALUED_FUNCTION (2000) - A user-defined-function (UDF) which returns a table.
TR - SQL_TRIGGER - DML trigger
TT - TYPE_TABLE (2008) - A user defined table type which can be passed as a parameter to a function or procedure
U - USER_TABLE - A user-defined table
UQ - UNIQUE_CONSTRAINT - Unique constraint on a set of columns
V - VIEW - A user-defined view
X - EXTENDED_STORED_PROCEDURE - An extended stored procedure
*/
struct obj_code // 2 bytes
{
    enum class type {
        AGGREGATE_FUNCTION,
        CHECK_CONSTRAINT,
        DEFAULT_CONSTRAINT,
        FOREIGN_KEY_CONSTRAINT,
        SQL_SCALAR_FUNCTION,
        CLR_SCALAR_FUNCTION,
        CLR_TABLE_VALUED_FUNCTION,
        SQL_INLINE_TABLE_VALUED_FUNCTION,
        INTERNAL_TABLE,
        SQL_STORED_PROCEDURE,
        CLR_STORED_PROCEDURE,
        PLAN_GUIDE,
        PRIMARY_KEY_CONSTRAINT,
        RULE,
        REPLICATION_FILTER_PROCEDURE,
        SYSTEM_TABLE,
        SYNONYM,
        SERVICE_QUEUE,
        CLR_TRIGGER,
        SQL_TABLE_VALUED_FUNCTION,
        SQL_TRIGGER,
        TYPE_TABLE,
        USER_TABLE,
        UNIQUE_CONSTRAINT,
        VIEW,
        EXTENDED_STORED_PROCEDURE,
        //
        _end  // unknown type
    };
    union {
        char c[2];
        uint16 u;
    };
    static const char * get_name(type);
    static const char * get_name(obj_code);
    static obj_code get_code(type);
    const char * name() const {
        return get_name(*this);
    }
};

struct scalartype // 4 bytes
{
    enum type
    {
        t_none              = 0,
        t_image             = 34,
        t_text              = 35,
        t_uniqueidentifier  = 36,
        t_date              = 40,
        t_time              = 41,
        t_datetime2         = 42,
        t_datetimeoffset    = 43,
        t_tinyint           = 48,
        t_smallint          = 52,
        t_int               = 56,
        t_smalldatetime     = 58, 
        t_real              = 59, 
        t_money             = 60, 
        t_datetime          = 61, 
        t_float             = 62, 
        t_sql_variant       = 98, 
        t_ntext             = 99,
        t_bit               = 104, 
        t_decimal           = 106, 
        t_numeric           = 108, 
        t_smallmoney        = 122, 
        t_bigint            = 127,
        t_hierarchyid       = 128,
        t_geometry          = 129,
        t_geography         = 130,
        t_varbinary         = 165,
        t_varchar           = 167,
        t_binary            = 173,
        t_char              = 175,
        t_timestamp         = 189,
        t_nvarchar          = 231,
        t_nchar             = 239,
        t_xml               = 241,
        t_sysname           = 256,
        //
        _end,
        null = t_none
    };

    uint32 _32;

    operator type() const {
        const type ret = static_cast<type>(_32);
        SDL_ASSERT(ret < type::_end);
        return (ret < type::_end) ? ret : t_none;
    }
    static const char * get_name(type);
    static bool is_fixed(type);
    const char * name() const { 
        return get_name(*this);
    }
    static constexpr bool is_text(type const t) {
        return (scalartype::t_text == t)
            || (scalartype::t_char == t)
            || (scalartype::t_varchar == t);
    }
    static constexpr bool is_ntext(type const t) {
        return (scalartype::t_ntext == t)
            || (scalartype::t_nchar == t)
            || (scalartype::t_nvarchar == t);
    }
};

template<scalartype::type T>
struct scalartype_trait {
    static constexpr scalartype::type value = T;
    static constexpr bool is_text = scalartype::is_text(T);
    static constexpr bool is_ntext = scalartype::is_ntext(T);
};

struct scalarlen // 2 bytes
{
    int16 _16;

    bool is_var() const { // variable length
        SDL_ASSERT((_16 == -1) || (_16 >= 0));
        return (_16 == -1);
    }
};

struct complextype // 2 bytes
{
    enum type
    {
        none = 0,                   // unknown type
        row_overflow = 2,
        blob_inline_root = 4,       
        sparse_vector = 5,
        forwarded = 1024
    };

    uint16 _16;

    operator type() const {
        SDL_ASSERT(is_found((type)_16, {row_overflow, blob_inline_root, sparse_vector, forwarded}));
        return static_cast<type>(_16);
    }
    static const char * get_name(type);
    const char * name() const {
        return get_name(*this);
    }
};

struct idxtype // 1 byte
{
    enum type {
        heap = 0,
        clustered = 1,
        nonclustered = 2,
        xml = 3,
        spatial = 4,
        //
        _end  // unknown type
    };
    uint8 _8;

    operator type() const {
        SDL_ASSERT(static_cast<type>(_8) < type::_end);
        return static_cast<type>(_8);
    }
    bool is_clustered() const {
        return static_cast<type>(*this) == type::clustered;
    }
    bool is_spatial() const {
        return static_cast<type>(*this) == type::spatial;
    }
    static const char * get_name(type);
    const char * name() const {
        return get_name(*this);
    }
};

struct idxstatus // 4 bytes
{
    uint32 _32;

    bool IsUnique() const           { return 0 != (_32 & 0x8); }
    bool IsPrimaryKey() const       { return 0 != (_32 & 0x20); }
    bool IsUniqueConstraint() const { return 0 != (_32 & 0x40); }
    bool IsPadded() const           { return 0 != (_32 & 0x10); }
    bool IsDisabled() const         { return 0 != (_32 & 0x80); }
    bool IsHypothetical() const     { return 0 != (_32 & 0x100); }
    bool HasFilter() const          { return 0 != (_32 & 0x20000); }
    bool AllowRowLocks() const      { return 0 != (1 - (_32 & 512) / 512); }
    bool AllowPageLocks() const     { return 0 != (1 - (_32 & 1024) / 1024); }
};

enum class sortorder {
    NONE = 0,
    ASC,
    DESC
};

template<sortorder T> 
using sortorder_t = Val2Type<sortorder, T>;

template<sortorder s>
struct invert_sortorder {
    static_assert(s != sortorder::NONE, "");
    static constexpr sortorder value = (s == sortorder::DESC) ? sortorder::ASC : sortorder::DESC;
};

// 4 bytes - bit mask : 0x1 appears to always be set, 0x2 for index, 0x4 for a descending index column(is_descending_key).
struct iscolstatus
{
    uint32 _32;

    bool is_index() const           { return 0 != (_32 & 0x2); }
    bool is_descending() const      { return 0 != (_32 & 0x4); }

    sortorder index_order() const {
        if (is_index())
            return is_descending() ? sortorder::DESC : sortorder::ASC;
        return sortorder::NONE;
    }
};

struct guid_t // 16 bytes
{
    uint32 a;
    uint16 b;
    uint16 c;
    uint8 d;
    uint8 e;
    uint8 f; // 1
    uint8 g; // 2
    uint8 h; // 3
    uint8 i; // 4
    uint8 j; // 5
    uint8 k; // 6

    // SQL Server behavior : in the last six bytes of a value are most significant.
    static int compare(guid_t const & x, guid_t const & y) {
        return ::memcmp(&x.f, &y.f, 6);
    }
};

inline bool operator < (guid_t const & x, guid_t const & y) {
    return guid_t::compare(x, y) < 0;
}

template<int size>
struct numeric_t {
    static_assert((size == 5) || (size == 9) || (size == 13) || (size == 17), "");
    using type = char[size]; 
    type data;
};
template<> struct numeric_t<9> {
    uint8 _8;
    int64 _64;
};
using numeric9 = numeric_t<9>;

struct decimal5 // 5 bytes
{
    uint8 _8;
    int32 _32;
};

struct bitmask8 // 1 byte
{
    uint8 byte;

    template<size_t i>
    bool bit() const {
        static_assert(i < 8, "");
        return (byte & (1 << i)) != 0;
    }
};

struct pageFileID // 6 bytes
{
    uint32 pageId;  // 4 bytes : PageID
    uint16 fileId;  // 2 bytes : FileID

    bool is_null() const {
        SDL_ASSERT(fileId || !pageId); // 0:0 if is_null
        return 0 == fileId;
    }
    explicit operator bool() const {
        return !is_null();
    }
    int compare(pageFileID const & y) const {
        if (fileId < y.fileId) return -1;
        if (y.fileId < fileId) return 1;
        if (pageId < y.pageId) return -1;
        return (y.pageId < pageId) ? 1 : 0;
    }
};

// An RID value (Row ID, also called a row locator, record locator, or Record ID) is a page locator plus a 2 byte slot index
struct recordID // 8 bytes
{
    using slot_type = uint16;

    pageFileID  id;         // 6 bytes
    slot_type   slot;       // 2 bytes

    bool is_null() const {
        SDL_ASSERT(id || !slot);
        return id.is_null();
    }
    explicit operator bool() const {
        return !is_null();
    }
    static recordID init(pageFileID const & i, size_t const s = 0){
        SDL_ASSERT(s <= (uint16)(-1));
        return { i, static_cast<slot_type>(s) };
    }
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

struct smalldatetime_t // 4 bytes
{
    uint16 min;
    uint16 day;
};

struct gregorian_t
{
    int year;
    int month;
    int day;
};

struct clocktime_t
{
    int hour;  // hours since midnight - [0, 23]
    int min;   // minutes after the hour - [0, 59]
    int sec;   // seconds after the minute - [0, 60] including leap second
    int milliseconds; // < 1 second
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
    uint32 ticks;   // clock ticks since midnight (where each tick is 1/300th of a second)
    int32 days;     // days since 1900-01-01

    static constexpr int32 u_date_diff = 25567; // = SELECT DATEDIFF(d, '19000101', '19700101');

    // convert to number of seconds that have elapsed since 00:00:00 UTC, 1 January 1970
    size_t get_unix_time() const;
    static datetime_t set_unix_time(size_t);

    bool is_null() const {
        return !days && !ticks;
    }
    bool unix_epoch() const {
        return days >= u_date_diff;
    }
    bool before_epoch() const {
        return !unix_epoch();
    }
    static datetime_t init(int32 days, uint32 ticks) {
        datetime_t d;
        d.ticks = ticks;
        d.days = days;
        return d;
    }
    int milliseconds() const {
        return (ticks % 300) * 1000 / 300; // < 1 second
    }
    gregorian_t gregorian() const;
    clocktime_t clocktime() const;
};

struct auid_t // 8 bytes
{
    union {
        struct {
            uint16 lo;  // 0x00 : The lowest 16 bits appear to always be 0.
            uint32 id;  // 0x02 : The mid 32 bits is the Allocation Unit's ObjectID (which is an auto-increment number), 
            uint16 hi;  // 0x06 : The top 16 bits of this ID is 0 - 4, 255 or 256.  
        } d;
        uint64 _64;
    };
    bool is_null() const {
        return 0 == _64;
    }
    explicit operator bool() const {
        return !is_null();
    }
};

struct schobj_id // 4 bytes - the unique ID for the object (sysschobjs_row)
{
    using type = int32;
    type _32;  // signed to be compatible with SQL Management Studio
};

inline schobj_id _schobj_id(schobj_id::type i) {
    return { i };
}

struct nsid_id // 4 bytes - the schema ID of this object (sysschobjs_row)
{
    using type = int32;
    type _32;
};

inline nsid_id _nsid_id(nsid_id::type i) {
    return { i };
}

struct index_id // 4 bytes - the index_id (1 for the clustered index, larger numbers for non-clustered indexes)
{
    using type = int32;
    type _32;

    bool is_index() const {
        return (_32 > 0);
    }
    bool is_clustered() const {
        return (1 == _32);
    }
    // _32 == 384000 for idxtype::spatial ?
};

inline index_id _index_id(index_id::type i) {
    return { i };
}

struct column_xtype // 1 byte - ID for the data type of this column. This references the system table sys.sysscalartypes.xtype
{
    uint8 _8;
};

struct column_id // 4 bytes - the unique id of the column within this object: syscolpars_row.colid, sysiscols_row.intprop
{
    uint32 _32;
};

enum class pfs_full
{
    PCT_FULL_0  = 0,    // completely empty
    PCT_FULL_50 = 1,    // 1-50 % full
    PCT_FULL_80 = 2,    // 51-80 % full
    PCT_FULL_95 = 3,    // 80-95 % full
    PCT_FULL_100 = 4,   // 96-100 % full
};

/*enum class pfs_flags
{
    Empty           = 0x0,
    UpTo50          = 0x1,
    UpTo80          = 0x2,
    UpTo95          = 0x3,
    UpTo100         = 0x4,
    GhostRecords    = 0x8,
    IAM             = 0x10,
    MixedExtent     = 0x20,
    Allocated       = 0x40,
};*/

struct pfs_byte // 1 byte
{
    struct bitfields {
        unsigned char freespace  : 3;    // bit 0-2
        unsigned char ghost      : 1;    // bit 3: 1 = page contains ghost records
        unsigned char iam        : 1;    // bit 4: 1 = page is an IAM page
        unsigned char mixed      : 1;    // bit 5: 1 = page belongs to a mixed extent
        unsigned char allocated  : 1;    // bit 6: 1 = page is allocated
        unsigned char unknown    : 1;    // bit 7: appears to be ignored and is always 0.
    };
    union {
        bitfields b;
        uint8 byte;
    };
    pfs_full fullness() const {
        SDL_ASSERT(this->b.freespace <= 4);
        return static_cast<pfs_full>(this->b.freespace);
    }
    void set_fullness(pfs_full f) {
        this->b.freespace = static_cast<unsigned char>(f);
    }
    bool is_allocated() const {
        return b.allocated != 0;
    }
};

template<class T1, class T2 = T1>
struct pair_key
{
    T1 first;
    T2 second;
};

#pragma pack(pop)

inline bool operator == (auid_t x, auid_t y) { return x._64 == y._64; }
inline bool operator != (auid_t x, auid_t y) { return x._64 != y._64; }

inline bool operator == (obj_code x, obj_code y) { return x.u == y.u; }
inline bool operator != (obj_code x, obj_code y) { return x.u != y.u; }

inline bool operator == (schobj_id x, schobj_id y) { return x._32 == y._32; }
inline bool operator != (schobj_id x, schobj_id y) { return x._32 != y._32; }
inline bool operator < (schobj_id x, schobj_id y) { return x._32 < y._32; }

inline bool operator == (schobj_id x, int32 y) { return x._32 == y; }
inline bool operator != (schobj_id x, int32 y) { return x._32 != y; }
inline bool operator == (int32 x, schobj_id y) { return x == y._32; }
inline bool operator != (int32 x, schobj_id y) { return x != y._32; }

inline bool operator == (index_id x, index_id y) { return x._32 == y._32; }
inline bool operator != (index_id x, index_id y) { return x._32 != y._32; }
inline bool operator < (index_id x, index_id y) { return x._32 < y._32; }

inline bool operator == (column_xtype x, column_xtype y) { return x._8 == y._8; }
inline bool operator != (column_xtype x, column_xtype y) { return x._8 != y._8; }

inline bool operator == (column_id x, column_id y) { return x._32 == y._32; }
inline bool operator != (column_id x, column_id y) { return x._32 != y._32; }

inline bool operator == (scalartype x, scalartype y) { return x._32 == y._32; }
inline bool operator != (scalartype x, scalartype y) { return x._32 != y._32; }
inline bool operator < (scalartype x, scalartype y) { return x._32 < y._32; }

inline bool operator < (pageFileID const & x, pageFileID const & y) {
    if (x.fileId < y.fileId) return true;
    if (y.fileId < x.fileId) return false;
    return (x.pageId < y.pageId);
}
inline bool operator == (pageFileID const & x, pageFileID const & y) { 
    return (x.pageId == y.pageId) && (x.fileId == y.fileId);
}
inline bool operator != (pageFileID const & x, pageFileID const & y) {
    return !(x == y);
}
inline bool operator == (recordID const & x, recordID const & y) { 
    return (x.id == y.id) && (x.slot == y.slot);
}
inline bool operator != (recordID const & x, recordID const & y) {
    return !(x == y);
}
inline bool operator < (recordID const & x, recordID const & y) {
    const int i = x.id.compare(y.id);
    if (i < 0) return true;
    if (i > 0) return false;
    return x.slot < y.slot;
}

template<scalartype::type T>
class var_mem_t : public var_mem { // movable
public:
    static_assert(T != scalartype::t_none, "");
    static constexpr scalartype::type unit_type = T;
    using var_mem::var_mem;
    bool empty_or_whitespace() const;   // see system/page_info.h
    bool exists() const {
        return !empty_or_whitespace();
    }
    std::string str() const;            // see system/page_info.h
    std::string str_utf8() const;       // see system/type_utf.h
    std::wstring str_wide() const;      // see system/type_utf.h
};

//-----------------------------------------------------------------

template<class T>
struct enum_trait : is_static
{
    static bool reserved(T) { return false; }
};

template<>
struct enum_trait<pageType::type> : is_static
{
    static bool reserved(pageType::type t) { 
        return pageType::reserved(t);
    }
};

template<class T>
struct enum_iter : is_static
{
    static T& increment(T& t) {
        SDL_ASSERT(t != T::null);
        SDL_ASSERT(t != T::_end);
        t = static_cast<T>(int(t) + 1);
        return t;
    }
    static int distance(T const first, T const last) {
        return int(last) - int(first);
    }
    template<class fun_type> static 
    void for_each(fun_type && fun) {
        for (auto t = T::_begin; t != T::_end; ++t) {
            if (!enum_trait<T>::reserved(t)) {
                fun(t);
            }
        }
    }
};

//-----------------------------------------------------------------

inline pageType::type& operator++(pageType::type& t) {
    return enum_iter<pageType::type>::increment(t);
}

inline int distance(pageType::type first, pageType::type last) {
    return enum_iter<pageType::type>::distance(first, last);
}

template<class fun_type> inline
void for_pageType(fun_type && fun) {
    enum_iter<pageType::type>::for_each(std::forward<fun_type>(fun));
}

//-----------------------------------------------------------------

inline dataType::type& operator++(dataType::type& t) {
    return enum_iter<dataType::type>::increment(t);
}

inline int distance(dataType::type first, dataType::type last) {
    return enum_iter<dataType::type>::distance(first, last);
}

template<class fun_type> inline
void for_dataType(fun_type && fun) {
    enum_iter<dataType::type>::for_each(std::forward<fun_type>(fun));
}

//-----------------------------------------------------------------

inline scalartype::type& operator++(scalartype::type& t) {
    return enum_iter<scalartype::type>::increment(t);
}

inline int distance(scalartype::type first, scalartype::type last) {
    return enum_iter<scalartype::type>::distance(first, last);
}

template<class fun_type> inline
void for_scalartype(fun_type && fun) {
    enum_iter<scalartype::type>::for_each(std::forward<fun_type>(fun));
}

inline std::ostream & operator <<(std::ostream & out, schobj_id id) {
    out << id._32;
    return out;
}

} // db
} // sdl

#endif // __SDL_SYSTEM_PAGE_TYPE_H__