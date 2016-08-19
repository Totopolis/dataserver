// page_head.inl
//
#pragma once
#ifndef __SDL_SYSTEM_PAGE_HEAD_INL__
#define __SDL_SYSTEM_PAGE_HEAD_INL__

namespace sdl { namespace db { namespace cast {

template<class T> inline
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

template<class T> inline
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

inline pageIndex make_page(size_t i) {
    SDL_ASSERT(i < pageIndex::value_type(-1));
    static_assert(pageIndex::value_type(-1) == 4294967295, "");
    return pageIndex(static_cast<pageIndex::value_type>(i));
}

template<class row_type> inline
std::string col_name_t(row_type const * p) {
    SDL_ASSERT(p);
    using info = typename row_type::info;
    return info::col_name(*p);
}

//----------------------------------------------------------------------

inline size_t slot_array::size() const
{
    return head->data.slotCnt;
}

inline const uint16 * slot_array::rbegin() const
{
    return this->rend() - this->size();
}

inline const uint16 * slot_array::rend() const
{
    const char * p = page_head::end(this->head);
    return reinterpret_cast<uint16 const *>(p);
}

inline uint16 slot_array::operator[](size_t i) const
{
    A_STATIC_ASSERT_TYPE(value_type, uint16);
    SDL_ASSERT(i < this->size());
    const uint16 * const p = this->rend() - (i + 1);
    return *p;
}

template<size_t i>
inline uint16 slot_array::get() const
{
    A_STATIC_ASSERT_TYPE(value_type, uint16);
    SDL_ASSERT(i < this->size());
    const uint16 * const p = this->rend() - (i + 1);
    return *p;
}

//----------------------------------------------------------------------

inline null_bitmap::null_bitmap(row_head const * h)
    : record(h)
    , m_size(null_bitmap::size(h))
{
    SDL_ASSERT(record);
    SDL_ASSERT(record->has_null());
    SDL_ASSERT(m_size);
}

inline const char * null_bitmap::begin(row_head const * record) 
{
    char const * p = reinterpret_cast<char const *>(record);
    SDL_ASSERT(record->data.fixedlen > 0);
    p += record->data.fixedlen;
    return p;
}

inline const char * null_bitmap::begin() const
{
    return null_bitmap::begin(this->record);
}

inline const char * null_bitmap::first_col() const // at first item
{
    static_assert(sizeof(column_num) == 2, "");
    return this->begin() + sizeof(column_num);
}

inline const char * null_bitmap::end() const
{
    return this->first_col() + col_bytes();
}

inline size_t null_bitmap::size(row_head const * record) // # of columns
{
    SDL_ASSERT(record);
    auto sz = *reinterpret_cast<column_num const *>(begin(record));
    A_STATIC_CHECK_TYPE(uint16, sz);
    return static_cast<size_t>(sz);
}

inline size_t null_bitmap::col_bytes() const // # bytes for columns
{
    return (this->size() + 7) >> 3;
}

inline bool null_bitmap::operator[](size_t const i) const
{
    SDL_ASSERT(i < this->size());
    const char * const p = this->first_col() + (i >> 3);
    SDL_ASSERT(p < this->end());
    const char mask = 1 << (i % 8);
    return (p[0] & mask) != 0;
}

//----------------------------------------------------------------------

inline variable_array::variable_array(row_head const * h)
    : record(h)
    , m_size(variable_array::size(h))
{
    SDL_ASSERT(record);
    SDL_ASSERT(record->has_variable());
}

inline size_t variable_array::col_bytes() const // # bytes for columns
{
    return this->size() * sizeof(uint16);
}

inline const char * variable_array::begin(row_head const * record)
{
    return null_bitmap(record).end();
}

inline const char * variable_array::begin() const
{
    return variable_array::begin(this->record);
}

inline size_t variable_array::size(row_head const * record) // # of variable-length columns
{
    SDL_ASSERT(record);
    SDL_ASSERT(record->has_variable());
    auto sz = *reinterpret_cast<column_num const *>(variable_array::begin(record));
    A_STATIC_CHECK_TYPE(uint16, sz);
    return static_cast<size_t>(sz);
}

inline const char * variable_array::first_col() const // at first item
{
    return this->begin() + sizeof(column_num);
}

inline const char * variable_array::end() const
{
    return this->first_col() + col_bytes();
}

inline std::pair<uint16, bool>
variable_array::operator[](size_t const i) const
{
    SDL_ASSERT(i < this->size());
    const uint16 p = reinterpret_cast<const uint16 *>(this->first_col())[i];
    return { p, is_highbit(p) };
}

inline uint16 variable_array::offset(size_t const i) const
{
    auto p = highbit_off((*this)[i].first);
    SDL_ASSERT(p < page_head::body_limit);
    return p;
}

inline size_t variable_array::var_data_bytes(size_t i) const 
{
    auto const & d = var_data(i);
    SDL_ASSERT(d.first <= d.second);
    return (d.second - d.first);
}

inline mem_range_t variable_array::back_var_data() const
{
    if (!empty()) {
        return var_data(size() - 1);
    }
    SDL_ASSERT(!"back_var_data");  
    return mem_range_t();
}

//----------------------------------------------------------------------

inline row_data::row_data(row_head const * h)
    : record(h), null(h), variable(h)
{
    SDL_ASSERT(record);
    SDL_ASSERT(record->has_null());
    SDL_ASSERT(record->has_variable());
}

inline size_t row_data::size() const
{
    return null.size();
}

inline const char * row_data::begin() const
{
    return reinterpret_cast<char const *>(this->record);
}

inline bool row_data::is_null(size_t const i) const
{
    SDL_ASSERT(i < this->size());
    return this->null[i];
}

//----------------------------------------------------------------------

inline forwarding_record::forwarding_record(row_head const * p)
    : record(reinterpret_cast<forwarding_stub const *>(p))
{
    SDL_ASSERT(record);
    SDL_ASSERT(p->is_type<recordType::forwarding_record>());
}

//----------------------------------------------------------------------

} // db
} // sdl

#endif // __SDL_SYSTEM_PAGE_HEAD_INL__