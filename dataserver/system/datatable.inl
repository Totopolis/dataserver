// datatable.inl
//
#pragma once
#ifndef __SDL_SYSTEM_DATATABLE_INL__
#define __SDL_SYSTEM_DATATABLE_INL__

namespace sdl { namespace db {

inline datatable::sysalloc_access
datatable::get_sysalloc(dataType::type t1) { 
    return sysalloc_access(this, t1);
}

inline datatable::datapage_access
datatable::get_datapage(dataType::type t1, pageType::type t2) {
    return datapage_access(this, t1, t2);
}

inline datatable::datarow_access
datatable::get_datarow(dataType::type t1, pageType::type t2) {
    return datarow_access(this, t1, t2);
}

inline shared_primary_key const &
datatable::get_PrimaryKey() const {
    return m_primary_key;
}

inline shared_cluster_index const &
datatable::get_cluster_index() const {
    return m_cluster_index;
}

inline shared_index_tree const &
datatable::get_index_tree() const {
    return m_index_tree;
}

//----------------------------------------------------------------------

inline void datatable::datarow_access::load_next(page_slot & p) const
{
    SDL_ASSERT(!is_end(p));
    if (++p.second >= slot_array(*p.first).size()) {
        p.second = 0;
        ++p.first;
    }
}

inline bool datatable::datarow_access::is_begin(page_slot const & p) const
{
    if (!p.second && (p.first == _datapage.begin())) {
        return true;
    }
    return false;
}

inline bool datatable::datarow_access::is_end(page_slot const & p) const
{
    if (p.first == _datapage.end()) {
        SDL_ASSERT(!p.second);
        return true;
    }
    return false;
}

inline bool datatable::datarow_access::is_empty(page_slot const & p) const
{
    return datapage(*p.first).empty();
}

inline row_head const * datatable::datarow_access::dereference(page_slot const & p)
{
    const datapage page(*p.first);
    return page.empty() ? nullptr : page[p.second];
}

inline datatable::datarow_access::iterator
datatable::datarow_access::begin() const
{
    return iterator(this, page_slot(_datapage.begin(), 0));
}

inline datatable::datarow_access::iterator
datatable::datarow_access::end() const
{
    return iterator(this, page_slot(_datapage.end(), 0));
}

inline page_head const * 
datatable::datarow_access::get_page(iterator const & it)
{
    A_STATIC_CHECK_TYPE(page_slot, it.current);
    return *(it.current.first);
}

inline recordID datatable::datarow_access::get_id(iterator const & it)
{
    if (page_head const * page = get_page(it)) {
        A_STATIC_CHECK_TYPE(page_slot, it.current);
        return recordID::init(page->data.pageId, it.current.second);
    }
    return {};
}

//----------------------------------------------------------------------

inline datatable::head_access::iterator
datatable::head_access::end() const
{
    return iterator(this, _datarow.end());
}

inline bool datatable::head_access::_is_end(datarow_iterator const & it) const
{
    return (it == _datarow.end());
}

inline bool datatable::head_access::use_record(datarow_iterator const & it)
{
    if (row_head const * const p = *it) {
        return p->use_record();
    }
    return false;
}

inline void datatable::head_access::load_next(datarow_iterator & it) const
{
    SDL_ASSERT(it != _datarow.end());
    for (;;) {
        ++it;
        if (it == _datarow.end())
            break;
        if (use_record(it))
            break;
    }
}

//----------------------------------------------------------------------

inline datatable::record_type::col_size_t
datatable::record_type::size() const
{
    return table->ut().size();
}

inline datatable::record_type::column const & 
datatable::record_type::usercol(col_size_t i) const
{
    return table->ut()[i];
}

inline size_t datatable::record_type::fixed_size() const
{
    return record->fixed_size();
}

inline size_t datatable::record_type::var_size() const
{
    if (record->has_variable()) {
        return variable_array(record).var_data_size();
    }
    return 0;
}

inline size_t datatable::record_type::count_null() const
{
    return null_bitmap(record).count_null();
}

inline bool datatable::record_type::is_forwarded() const
{ 
    return record->is_forwarded_record();
}

template<scalartype::type type> inline
scalartype_t<type> const *
datatable::scalartype_cast(mem_range_t const & m, usertable::column const & col)
{
    using T = scalartype_t<type>;
    SDL_ASSERT(col.type == type);
    SDL_ASSERT(col.fixed_size() == mem_size(m));
    if (mem_size(m) == sizeof(T)) {
        return reinterpret_cast<const T *>(m.first);
    }
    SDL_ASSERT(col.type == scalartype::t_numeric); //FIXME: not all types supported
    SDL_ASSERT_DEBUG_2(0);
    return nullptr; 
}

template<scalartype::type type>
scalartype_t<type> const *
datatable::record_type::cast_fixed_col(col_size_t const i) const
{
    SDL_ASSERT(i < this->size());
    if (is_null(i)) {
        return nullptr;
    }
    column const & col = usercol(i);
    if (col.is_fixed()) {
        mem_range_t const m = fixed_memory(col, i);
        if (mem_size(m)) {
            return datatable::scalartype_cast<type>(m, col);
        }
    }
    SDL_ASSERT(0);
    return nullptr;
}

//----------------------------------------------------------------------

template<typename pk0_type> unique_spatial_tree_t<pk0_type>
datatable::get_spatial_tree(identity<pk0_type>) const {
    if (auto const tree = this->find_spatial_tree()) {
        if (auto const pk0 = this->get_PrimaryKey()) {
            SDL_ASSERT(pk0->first_type() == key_to_scalartype<pk0_type>::value);
            return sdl::make_unique<spatial_tree_t<pk0_type>>(this->db, tree.pgroot, pk0, tree.idx);
        }
        SDL_ASSERT(!"get_spatial_tree");
    }
    return{};
}

template<class T, class fun_type>
void datatable::for_datarow(T && data, fun_type && fun) {
    A_STATIC_ASSERT_TYPE(datarow_access, remove_reference_t<T>);
    for (row_head const * row : data) {
        if (row) { 
            fun(*row);
        }
    }
}

//----------------------------------------------------------------------

inline datatable::record_type
datatable::find_record(vector_mem_range_t const & v) const {
    auto buf = make_vector(v);
    return find_record(make_mem_range(buf));
} 

inline datatable::record_iterator
datatable::find_record_iterator(vector_mem_range_t const & v) const {
    auto buf = make_vector(v);
    return find_record_iterator(make_mem_range(buf));
}

//----------------------------------------------------------------------

} // db
} // sdl

#endif // __SDL_SYSTEM_DATATABLE_INL__
