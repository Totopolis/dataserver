// page_head.cpp
//
#include "common/common.h"
#include "page_head.h"
#include <time.h>       /* time_t, struct tm, time, localtime, strftime */

namespace sdl { namespace db {

const char meta::empty[] = "";

static_col_name(page_head_meta, headerVersion);
static_col_name(page_head_meta, type);
static_col_name(page_head_meta, typeFlagBits);
static_col_name(page_head_meta, level);
static_col_name(page_head_meta, flagBits);
static_col_name(page_head_meta, indexId);
static_col_name(page_head_meta, prevPage);
static_col_name(page_head_meta, pminlen);
static_col_name(page_head_meta, nextPage);
static_col_name(page_head_meta, slotCnt);
static_col_name(page_head_meta, objId);
static_col_name(page_head_meta, freeCnt);
static_col_name(page_head_meta, freeData);
static_col_name(page_head_meta, pageId);
static_col_name(page_head_meta, reservedCnt);
static_col_name(page_head_meta, lsn);
static_col_name(page_head_meta, xactReserved);
static_col_name(page_head_meta, xdesId);
static_col_name(page_head_meta, ghostRecCnt);
static_col_name(page_head_meta, tornBits);

static_col_name(row_head_meta, statusA);
static_col_name(row_head_meta, statusB);
static_col_name(row_head_meta, fixedlen);

//------------------------------------------------------------------------------

mem_range_t row_head::fixed_data() const // fixed length column data
{
    static_assert(sizeof(row_head) == 4, "");
    row_head const * const p = this;
    SDL_ASSERT(sizeof(row_head) <= p->data.fixedlen);
    return mem_range_t(
        row_head::begin(p) + sizeof(row_head),
        row_head::begin(p) + p->data.fixedlen);
}

size_t row_head::fixed_size() const
{
    SDL_ASSERT(sizeof(row_head) <= data.fixedlen);
    return data.fixedlen - sizeof(row_head);
}

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
    SDL_ASSERT(i < this->size());
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

size_t null_bitmap::count_reverse_null() const
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

size_t null_bitmap::count_null() const
{
    size_t count = this->size();
    size_t const sz = count;
    for (size_t i = 0; i < sz; ++i) {
        if (!((*this)[i]))
            --count;
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
    if (col.first <= col.second) {
        return col;
    }
    SDL_ASSERT(!"var_data");  
    return mem_range_t(); // variable_array not exists or [NULL] column ?
}

mem_range_t variable_array::back_var_data() const
{
    if (!empty()) {
        return var_data(size() - 1);
    }
    SDL_ASSERT(!"back_var_data");  
    return mem_range_t();
}

size_t variable_array::var_data_bytes(size_t i) const 
{
    auto const & d = var_data(i);
    SDL_ASSERT(d.first <= d.second);
    return (d.second - d.first);
}

size_t variable_array::var_data_size() const // variable-length data size
{
    size_t bytes = 0;
    for (size_t i = 0; i < size(); ++i) {
        bytes += var_data_bytes(i);
    }
    return bytes;
}

complextype::type
variable_array::var_complextype(size_t const i) const
{
    if (is_complex(i)) {
         const mem_range_t m = var_data(i);
         if (mem_size(m) > sizeof(complextype)) { // expect data after complextype
             complextype const * p = reinterpret_cast<complextype const *>(m.first);
             return static_cast<complextype::type>(*p);
         }
    }
    return complextype::none;
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
        const size_t sz = null.size() - null.count_reverse_null() - variable.size();
        SDL_ASSERT(sz <= this->size());
        return i < sz;
    }
    return false;
}

//--------------------------------------------------------------

forwarding_record::forwarding_record(row_head const * p)
    : record(reinterpret_cast<forwarding_stub const *>(p))
{
    SDL_ASSERT(record);
    SDL_ASSERT(p->is_type<recordType::forwarding_record>());
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
                    SDL_TRACE_FILE;

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
                        typedef page_head_meta T;
                        static_assert(T::headerVersion::offset == 0, "");
                        static_assert(T::type::offset == 1, "");
                        static_assert(std::is_same<T::headerVersion::type, uint8>::value, "");
                        static_assert(std::is_same<T::type::type, pageType>::value, "");
                        static_assert(std::is_same<T::tornBits::type, int32>::value, "");
                    }
                    SDL_ASSERT((page_head::end(nullptr) - page_head::begin(nullptr)) == 8 * 1024);
                    if (0) {
                        SDL_TRACE_2("sizeof(wchar_t) == ", sizeof(wchar_t)); // can be 2 or 4 bytes
                        SDL_TRACE_2("sizeof(void *) == ", sizeof(void *)); // must be 8 for 64-bit
                        SDL_TRACE_2("sizeof(size_t) == ", sizeof(size_t)); // must be 8 for 64-bit
                    }
                    A_STATIC_ASSERT_64_BIT;
                }
                A_STATIC_ASSERT_IS_POD(row_head);
                A_STATIC_ASSERT_IS_POD(overflow_page);
                A_STATIC_ASSERT_IS_POD(overflow_link);

                static_assert(sizeof(row_head) == 4, "");
                static_assert(sizeof(overflow_page) == 24, "");
                static_assert(sizeof(overflow_link) == 12, "");
                static_assert(sizeof(text_pointer) == 16, "");    

                static_assert(offsetof(overflow_page, _0x02) == 0x02, "");
                static_assert(offsetof(overflow_page, _0x0A) == 0x0A, "");
                static_assert(offsetof(overflow_page, length) == 0x0C, "");
                static_assert(offsetof(overflow_page, row) == 0x10, "");

                A_STATIC_ASSERT_IS_POD(forwarding_stub);
                static_assert(sizeof(forwarding_stub) == 9, "");
                static_assert(offsetof(forwarding_stub, data.row) == 0x1, "");

                A_STATIC_ASSERT_IS_POD(forwarded_stub);
                static_assert(sizeof(forwarded_stub) == 10, "");
                static_assert(offsetof(forwarded_stub, data.row) == 0x2, "");

                A_STATIC_ASSERT_IS_POD(lob_head);
                A_STATIC_ASSERT_IS_POD(lobtype);
                A_STATIC_ASSERT_IS_POD(LargeRootYukon);
                A_STATIC_ASSERT_IS_POD(LobSlotPointer);
                A_STATIC_ASSERT_IS_POD(LobSmallRoot);
                A_STATIC_ASSERT_IS_POD(TextTreeInternal);
                A_STATIC_ASSERT_IS_POD(InternalLobSlotPointer);

                static_assert(sizeof(lob_head) == 10, "");
                static_assert(sizeof(lobtype) == 2, "");
                static_assert(sizeof(LobSlotPointer) == 12, "");
                static_assert(sizeof(LargeRootYukon) == 32, "");
                static_assert(offsetof(LargeRootYukon, _0x10) == 0x10, "");
                static_assert(offsetof(LargeRootYukon, data) == 20, "");
                static_assert(sizeof(LobSmallRoot) == 16, "");
                static_assert(offsetof(LobSmallRoot, _0x0C) == 0x0C, ""); 
                static_assert(sizeof(InternalLobSlotPointer) == 16, "");
                static_assert(sizeof(TextTreeInternal) == 32, "");
                static_assert(offsetof(TextTreeInternal, data) == 16, "");
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG

