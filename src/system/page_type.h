// page_type.h
//
#ifndef __SDL_SYSTEM_PAGE_TYPE_H__
#define __SDL_SYSTEM_PAGE_TYPE_H__

#pragma once

namespace sdl { namespace db {

#pragma pack(push, 1) 

struct pageType // 1 byte
{
    enum class type {
        null = 0,
        data = 1,           //1 – data page. This holds data records in a heap or clustered index leaf-level.
        index = 2,          //2 – index page. This holds index records in the upper levels of a clustered index and all levels of non-clustered indexes.
        textmix = 3,        //3 – text mix page. A text page that holds small chunks of LOB values plus internal parts of text tree. These can be shared between LOB values in the same partition of an index or heap.
        texttree = 4,       //4 – text tree page. A text page that holds large chunks of LOB values from a single column value.
        _5 = 5,             //5 - reserved
        _6 = 6,             //6 - reserved
        sort = 7,           //7 – sort page. A page that stores intermediate results during a sort operation.
        GAM = 8,            //8 – GAM page. Holds global allocation information about extents in a GAM interval (every data file is split into 4GB chunks – the number of extents that can be represented in a bitmap on a single database page). Basically whether an extent is allocated or not. GAM = Global Allocation Map. The first one is page 2 in each file. More on these in a later post.
        SGAM = 9,           //9 – SGAM page. Holds global allocation information about extents in a GAM interval. Basically whether an extent is available for allocating mixed-pages. SGAM = Shared GAM. the first one is page 3 in each file. More on these in a later post.
        IAM = 10,           //10 – IAM page. Holds allocation information about which extents within a GAM interval are allocated to an index or allocation unit, in SQL Server 2000 and 2005 respectively. IAM = Index Allocation Map. More on these in a later post.
        PFS = 11,           //11 – PFS page. Holds allocation and free space information about pages within a PFS interval (every data file is also split into approx 64MB chunks – the number of pages that can be represented in a byte-map on a single database page. PFS = Page Free Space. The first one is page 1 in each file. More on these in a later post.
        _12 = 12,           //12 - reserved
        boot = 13,          //13 – boot page. Holds information about the database. There’s only one of these in the database. It’s page 9 in file 1.
        _14 = 14,           //14 - reserved
        fileheader = 15,    //15 – file header page.Holds information about the file.There’s one per file and it’s page 0 in the file.
        diffmap = 16,       //16 – diff map page. Holds information about which extents in a GAM interval have changed since the last full or differential backup. The first one is page 6 in each file.
        MLmap = 17,         //17 – ML map page.Holds information about which extents in a GAM interval have changed while in bulk - logged mode since the last backup.This is what allows you to switch to bulk - logged mode for bulk - loads and index rebuilds without worrying about breaking a backup chain.The first one is page 7 in each file.
        deallocated = 18,   //18 – a page that’s be deallocated by DBCC CHECKDB during a repair operation.
        temporary = 19,     //19 – the temporary page that ALTER INDEX … REORGANIZE(or DBCC INDEXDEFRAG) uses when working on an index.
        preallocated = 20,  //20 – a page pre - allocated as part of a bulk load operation, which will eventually be formatted as a ‘real’ page.
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
        _end
    };

    uint32 _32;

    operator type() const {
        SDL_ASSERT(static_cast<type>(_32) < type::_end);
        return static_cast<type>(_32);
    }
    static const char * get_name(type);
    static bool is_fixed(type);
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
        SDL_ASSERT(get_name(static_cast<type>(_16))[0]);
        return static_cast<type>(_16);
    }
    static const char * get_name(type);
};

struct indexType // 1 byte
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
    static const char * get_name(type);
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
};

// An RID value (Row ID, also called a row locator, record locator, or Record ID) is a page locator plus a 2 byte slot index
struct recordID // 8 bytes
{
    using slot_type = uint16;

    pageFileID  id;         // 6 bytes
    slot_type   slot;       // 2 bytes

    bool is_null() const {
        return id.is_null();
    }
    explicit operator bool() const {
        return !is_null();
    }
    static recordID init(pageFileID const & i, slot_type s){
        return { i, s };
    }
    static recordID init(pageFileID const & i, size_t s){
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

struct nchar_t // 2 bytes
{
    uint16 c;
};

struct smalldatetime_t // 4 bytes
{
    uint16 min;
    uint16 day;
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
};

struct schobj_id // 4 bytes - the unique ID for the object (sysschobjs_row)
{
    int32 _32;
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

#pragma pack(pop)

inline bool operator == (auid_t x, auid_t y) { return x._64 == y._64; }
inline bool operator != (auid_t x, auid_t y) { return x._64 != y._64; }

inline bool operator == (obj_code x, obj_code y) { return x.u == y.u; }
inline bool operator != (obj_code x, obj_code y) { return x.u != y.u; }

inline bool operator == (nchar_t x, nchar_t y) { return x.c == y.c; }
inline bool operator != (nchar_t x, nchar_t y) { return x.c != y.c; }

inline bool operator == (schobj_id x, schobj_id y) { return x._32 == y._32; }
inline bool operator != (schobj_id x, schobj_id y) { return x._32 != y._32; }
inline bool operator < (schobj_id x, schobj_id y) { return x._32 < y._32; }

inline bool operator == (scalartype x, scalartype y) { return x._32 == y._32; }
inline bool operator != (scalartype x, scalartype y) { return x._32 != y._32; }
inline bool operator < (scalartype x, scalartype y) { return x._32 < y._32; }

inline bool operator < (pageFileID x, pageFileID y) {
    if (x.fileId < y.fileId) return true;
    if (y.fileId < x.fileId) return false;
    return (x.pageId < y.pageId);
}

typedef std::pair<nchar_t const *, nchar_t const *> nchar_range;

nchar_t const * reverse_find(
    nchar_t const * const begin,
    nchar_t const * const end,
    nchar_t const * const buf,
    size_t const buf_size);

template<size_t buf_size> inline
nchar_t const * reverse_find(nchar_range const & s, nchar_t const(&buf)[buf_size]) {
    return reverse_find(s.first, s.second, buf, buf_size);
}

typedef std::pair<const char *, const char *> mem_range_t;
using vector_mem_range_t = std::vector<mem_range_t>;

inline size_t mem_size(mem_range_t const & m) {
    SDL_ASSERT(m.first <= m.second);
    return (m.first < m.second) ? (m.second - m.first) : 0;
}

inline bool mem_empty(mem_range_t const & m) {
    return 0 == mem_size(m);
}

template<class T>
inline size_t mem_size_n(T const & array) {
    size_t size = 0;
    for (auto const & m : array) {
        SDL_ASSERT(!mem_empty(m)); // warning
        size += mem_size(m);
    }
    return size;
}

inline nchar_range make_nchar(mem_range_t const & m) {
    SDL_ASSERT(m.first <= m.second);
    SDL_ASSERT(!(mem_size(m) % sizeof(nchar_t)));
    return {
        reinterpret_cast<nchar_t const *>(m.first),
        reinterpret_cast<nchar_t const *>(m.second) };
}

inline nchar_range make_nchar_checked(mem_range_t const & m) {
    if (!(mem_size(m) % sizeof(nchar_t))) {
        return {
            reinterpret_cast<nchar_t const *>(m.first),
            reinterpret_cast<nchar_t const *>(m.second) };
    }
    return {};
}

template<class T>
class mem_array_t {
public:
    const mem_range_t data;
    mem_array_t() = default;
    explicit mem_array_t(mem_range_t const & d) : data(d) {
        SDL_ASSERT(!((data.second - data.first) % sizeof(T)));
        SDL_ASSERT((end() - begin()) == size());
    }
    mem_array_t(const T * b, const T * e)
        : data((const char *)b, (const char *)e)
    {
        SDL_ASSERT(b <= e);
    }
    bool empty() const {
        return (data.first == data.second); // return 0 == size();
    }
    size_t size() const {
        return (data.second - data.first) / sizeof(T);
    }
    T const & operator[](size_t i) const {
        SDL_ASSERT(i < size());
        return *(begin() + i);
    }
    T const * begin() const {
        return reinterpret_cast<T const *>(data.first);
    }
    T const * end() const {
        return reinterpret_cast<T const *>(data.second);
    }
};

//-----------------------------------------------------------------

template<class T>
struct enum_trait : is_static
{
    static bool reserved(T t) { return false; }
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
    static int distance(T first, T last) {
        return int(last) - int(first);
    }
    template<class fun_type> static 
    void for_each(fun_type & fun) {
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
void for_pageType(fun_type fun) {
    enum_iter<pageType::type>::for_each(fun);
}

//-----------------------------------------------------------------

inline dataType::type& operator++(dataType::type& t) {
    return enum_iter<dataType::type>::increment(t);
}

inline int distance(dataType::type first, dataType::type last) {
    return enum_iter<dataType::type>::distance(first, last);
}

template<class fun_type> inline
void for_dataType(fun_type fun) {
    enum_iter<dataType::type>::for_each(fun);
}

//-----------------------------------------------------------------

} // db
} // sdl

#endif // __SDL_SYSTEM_PAGE_TYPE_H__