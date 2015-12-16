// page_head.cpp
//
#include "common/common.h"
#include "page_head.h"
#include <time.h>       /* time_t, struct tm, time, localtime, strftime */

namespace sdl { namespace db {

const char meta::empty[] = "";

static_col_name(page_header_meta, headerVersion);
static_col_name(page_header_meta, type);
static_col_name(page_header_meta, typeFlagBits);
static_col_name(page_header_meta, level);
static_col_name(page_header_meta, flagBits);
static_col_name(page_header_meta, indexId);
static_col_name(page_header_meta, prevPage);
static_col_name(page_header_meta, pminlen);
static_col_name(page_header_meta, nextPage);
static_col_name(page_header_meta, slotCnt);
static_col_name(page_header_meta, objId);
static_col_name(page_header_meta, freeCnt);
static_col_name(page_header_meta, freeData);
static_col_name(page_header_meta, pageId);
static_col_name(page_header_meta, reservedCnt);
static_col_name(page_header_meta, lsn);
static_col_name(page_header_meta, xactReserved);
static_col_name(page_header_meta, xdesId);
static_col_name(page_header_meta, ghostRecCnt);
static_col_name(page_header_meta, tornBits);

static_col_name(row_head_meta, statusA);
static_col_name(row_head_meta, statusB);
static_col_name(row_head_meta, fixedlen);

//------------------------------------------------------------------------------

size_t slot_array::size() const
{
    return head->data.slotCnt;
}

const uint16 * slot_array::rbegin() const
{
    return this->rend() - this->size();
}

const uint16 * slot_array::rend() const
{
    const char * p = page_head::end(this->head);
    return reinterpret_cast<uint16 const *>(p);
}

uint16 slot_array::operator[](size_t i) const
{
    A_STATIC_ASSERT_TYPE(value_type, uint16);
    SDL_ASSERT(i < size());
    const uint16 * p = this->rend() - (i + 1);
    const uint16 val = *p;
    return val;
}

std::vector<uint16> slot_array::copy() const
{
    std::vector<uint16> val;
    if (const size_t sz = size()) {
        val.resize(sz);
        for (size_t i = 0; i < sz; ++i) {
            val[i] = (*this)[i];
        }
    }
    return val;
}

//----------------------------------------------------------------------

null_bitmap::null_bitmap(row_head const * h)
    : record(h)
    , m_size(null_bitmap::size(h))
{
    SDL_ASSERT(record);
    SDL_ASSERT(record->has_null());
    SDL_ASSERT(m_size);
}

const char * null_bitmap::begin(row_head const * record) 
{
    char const * p = reinterpret_cast<char const *>(record);
    SDL_ASSERT(record->data.fixedlen > 0);
    p += record->data.fixedlen;
    return p;
}

const char * null_bitmap::begin() const
{
    return null_bitmap::begin(this->record);
}

const char * null_bitmap::first_col() const // at first item
{
    static_assert(sizeof(column_num) == 2, "");
    return this->begin() + sizeof(column_num);
}

const char * null_bitmap::end() const
{
    return this->first_col() + col_bytes();
}

size_t null_bitmap::size(row_head const * record) // # of columns
{
    SDL_ASSERT(record);
    auto sz = *reinterpret_cast<column_num const *>(begin(record));
    A_STATIC_CHECK_TYPE(uint16, sz);
    return static_cast<size_t>(sz);
}

size_t null_bitmap::col_bytes() const // # bytes for columns
{
    return (this->size() + 7) >> 3;
}

bool null_bitmap::operator[](size_t const i) const
{
    SDL_ASSERT(i < this->size());
    const char * const p = this->first_col() + (i >> 3);
    SDL_ASSERT(p < this->end());
    const char mask = 1 << (i % 8);
    return (p[0] & mask) != 0;
}

size_t null_bitmap::count_last_null() const
{
    size_t count = 0;
    size_t i = size();
    while (i--) {
        if ((*this)[i])
            ++count;
        else
            break;
    }
    return count;
}

//----------------------------------------------------------------------

variable_array::variable_array(row_head const * h)
    : record(h)
    , m_size(variable_array::size(h))
{
    SDL_ASSERT(record);
    SDL_ASSERT(record->has_variable());
}

size_t variable_array::col_bytes() const // # bytes for columns
{
    return this->size() * sizeof(uint16);
}

const char * variable_array::begin(row_head const * record)
{
    return null_bitmap(record).end();
}

const char * variable_array::begin() const
{
    return variable_array::begin(this->record);
}

size_t variable_array::size(row_head const * record) // # of variable-length columns
{
    SDL_ASSERT(record);
    SDL_ASSERT(record->has_variable());
    auto sz = *reinterpret_cast<column_num const *>(variable_array::begin(record));
    A_STATIC_CHECK_TYPE(uint16, sz);
    return static_cast<size_t>(sz);
}

const char * variable_array::first_col() const // at first item
{
    return this->begin() + sizeof(column_num);
}

const char * variable_array::end() const
{
    return this->first_col() + col_bytes();
}

std::pair<uint16, bool>
variable_array::operator[](size_t const i) const
{
    SDL_ASSERT(i < this->size());
    const uint16 p = reinterpret_cast<const uint16 *>(this->first_col())[i];
    return { p, is_highbit(p) };
}

uint16 variable_array::offset(size_t const i) const
{
    auto p = highbit_off((*this)[i].first);
    SDL_ASSERT(p < page_head::body_limit);
    return p;
}

mem_range_t variable_array::var_data(size_t const i) const
{
    SDL_ASSERT(i < this->size());
    const char * const start = row_head::begin(record);
    if (i > 0) {
        return { start + this->offset(i-1), start + this->offset(i) };
    }
    mem_range_t const col(this->end(), start + this->offset(0));
    if (col.first < col.second) {
        return col;
    }
    SDL_ASSERT(!"var_data");  
    return mem_range_t(); // variable_array not exists or [NULL] column ?
}

/*
Like ROW_OVERFLOW data, there is a pointer to another piece of information called the LOB root structure,
which contains a set of the pointers to other data pages/rows. When LOB data is less than 32 KB and can fit into five
data pages, the LOB root structure contains the pointers to the actual chunks of LOB data. Otherwise, the LOB tree
starts to include an additional, intermediate levels of pointers, similar to the index B-Tree, which we will discuss in
Chapter 2, “Tables and Indexes: Internal Structure and Access Methods.”
*/

variable_array::row_overflow_t
variable_array::row_overflow(size_t const i) const // return nullptr if not complex column
{
    SDL_ASSERT(i < this->size());
    if (is_complex(i)) {
        auto const & d = this->var_data(i);
        size_t const len = (d.second - d.first);
        if (len && !(len % sizeof(overflow_page))) {
            // can be [ROW_OVERFLOW data] or [LOB root structure]
            return { reinterpret_cast<overflow_page const *>(d.first), len / sizeof(overflow_page) };
        }
        SDL_ASSERT(!"row_overflow");
    }
    return row_overflow_t();
}


//--------------------------------------------------------------

row_data::row_data(row_head const * h)
    : record(h), null(h), variable(h)
{
    SDL_ASSERT(record);
    SDL_ASSERT(record->has_null());
    SDL_ASSERT(record->has_variable());
}

size_t row_data::size() const
{
    return null.size();
}

const char * row_data::begin() const
{
    return reinterpret_cast<char const *>(this->record);
}

// Note. ignore versioning tag (14 bytes) at the row end
const char * row_data::end() const
{
    const char * p;
    if (const size_t sz = this->variable.size()) { // if variable-columns not empty
        auto const last = this->variable.offset(sz - 1);
        SDL_ASSERT(last < page_head::body_limit); // ROW_OVERFLOW data ?
        p = this->begin() + last;
    }
    else {
        p = this->variable.end(); 
    }
    SDL_ASSERT(this->begin() < p);
    SDL_ASSERT((p - this->begin()) < page_head::body_limit); // ROW_OVERFLOW data ?
    return p;
}

bool row_data::is_null(size_t const i) const
{
    SDL_ASSERT(i < this->size());
    return this->null[i];
}

// returns false if column is [NULL] or variable
bool row_data::is_fixed(size_t const i) const
{
    if (!is_null(i)) {
        const size_t sz = null.size() - null.count_last_null() - variable.size();
        SDL_ASSERT(sz <= this->size());
        return i < sz;
    }
    return false;
}

mem_range_t row_data::fixed_data() const
{
    const char * const p1 = this->begin() + sizeof(this->record);
    const char * const p2 = this->null.begin();
    SDL_ASSERT(p1 <= p2);
    return mem_range_t(p1, p2);
}

} // db
} // sdl

#if SDL_DEBUG
namespace sdl {
    namespace db {
        namespace {
            class unit_test {
            public:
                unit_test()
                {
                    SDL_TRACE(__FILE__);

                    SDL_ASSERT(IS_LITTLE_ENDIAN);

                    A_STATIC_ASSERT_IS_POD(page_head);

                    static_assert(page_head::page_size == 8 * 1024, "");
                    static_assert(page_head::body_size == 8 * 1024 - 96, "");
                    static_assert(page_head::body_limit < page_head::body_size, "");
                    static_assert(sizeof(page_head::data_type) == page_head::head_size, "");

                    static_assert(offsetof(page_head, data.headerVersion) == 0, "");
                    static_assert(offsetof(page_head, data.type) == 0x01, "");
                    static_assert(offsetof(page_head, data.typeFlagBits) == 0x02, "");
                    static_assert(offsetof(page_head, data.level) == 0x03, "");
                    static_assert(offsetof(page_head, data.flagBits) == 0x04, "");
                    static_assert(offsetof(page_head, data.indexId) == 0x06, "");
                    static_assert(offsetof(page_head, data.prevPage) == 0x08, "");
                    static_assert(offsetof(page_head, data.pminlen) == 0x0E, "");
                    static_assert(offsetof(page_head, data.nextPage) == 0x10, "");
                    static_assert(offsetof(page_head, data.slotCnt) == 0x16, "");
                    static_assert(offsetof(page_head, data.objId) == 0x18, "");
                    static_assert(offsetof(page_head, data.freeCnt) == 0x1C, "");
                    static_assert(offsetof(page_head, data.freeData) == 0x1E, "");
                    static_assert(offsetof(page_head, data.pageId) == 0x20, "");
                    static_assert(offsetof(page_head, data.reservedCnt) == 0x26, "");
                    static_assert(offsetof(page_head, data.lsn) == 0x28, "");
                    static_assert(offsetof(page_head, data.xactReserved) == 0x32, "");
                    static_assert(offsetof(page_head, data.xdesId) == 0x34, "");
                    static_assert(offsetof(page_head, data.ghostRecCnt) == 0x3A, "");
                    static_assert(offsetof(page_head, data.tornBits) == 0x3C, "");
                    static_assert(offsetof(page_head, data.reserved) == 0x40, "");
                    {
                        typedef page_header_meta T;
                        static_assert(T::headerVersion::offset == 0, "");
                        static_assert(T::type::offset == 1, "");
                        static_assert(std::is_same<T::headerVersion::type, uint8>::value, "");
                        static_assert(std::is_same<T::type::type, pageType>::value, "");
                        static_assert(std::is_same<T::tornBits::type, int32>::value, "");
                    }
                    SDL_ASSERT((page_head::end(nullptr) - page_head::begin(nullptr)) == 8 * 1024);
                    SDL_TRACE_2("sizeof(wchar_t) == ", sizeof(wchar_t)); // can be 2 or 4 bytes
                    SDL_TRACE_2("sizeof(void *) == ", sizeof(void *)); // must be 8 for 64-bit
                    SDL_TRACE_2("sizeof(size_t) == ", sizeof(size_t)); // must be 8 for 64-bit
                    A_STATIC_ASSERT_64_BIT;
                }
                A_STATIC_ASSERT_IS_POD(row_head);
                static_assert(sizeof(row_head) == 4, "");
                static_assert(sizeof(overflow_page) == 24, "");
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG

