// datatable.inl
//
#pragma once
#ifndef __SDL_SYSTEM_DATATABLE_INL__
#define __SDL_SYSTEM_DATATABLE_INL__

namespace sdl { namespace db {

//----------------------------------------------------------------------

inline void datatable::datarow_access::load_next(page_slot & p)
{
    SDL_ASSERT(!is_end(p));
    if (++p.second >= slot_array(*p.first).size()) {
        p.second = 0;
        ++p.first;
    }
}

inline bool datatable::datarow_access::is_begin(page_slot const & p)
{
    if (!p.second && (p.first == _datapage.begin())) {
        return true;
    }
    return false;
}

inline bool datatable::datarow_access::is_end(page_slot const & p)
{
    if (p.first == _datapage.end()) {
        SDL_ASSERT(!p.second);
        return true;
    }
    return false;
}

inline bool datatable::datarow_access::is_empty(page_slot const & p)
{
    return datapage(*p.first).empty();
}

inline row_head const * datatable::datarow_access::dereference(page_slot const & p)
{
    const datapage page(*p.first);
    return page.empty() ? nullptr : page[p.second];
}

inline datatable::datarow_access::iterator
datatable::datarow_access::begin()
{
    return iterator(this, page_slot(_datapage.begin(), 0));
}

inline datatable::datarow_access::iterator
datatable::datarow_access::end()
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

inline datatable::head_access::head_access(datatable * p)
    : table(p)
    , _datarow(p, dataType::type::IN_ROW_DATA, pageType::type::data)
{
    SDL_ASSERT(table);
}

inline datatable::head_access::iterator
datatable::head_access::begin()
{
    datarow_iterator it = _datarow.begin();
    while (it != _datarow.end()) {
        if (head_access::use_record(it))
            break;
        ++it;
    }
    return iterator(this, std::move(it));
}


inline datatable::head_access::iterator
datatable::head_access::end()
{
    return iterator(this, _datarow.end());
}

inline bool datatable::head_access::_is_end(datarow_iterator const & it)
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

inline void datatable::head_access::load_next(datarow_iterator & it)
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

inline size_t datatable::record_type::size() const
{
    return table->ut().size();
}

inline datatable::record_type::column const & 
datatable::record_type::usercol(size_t i) const
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

//----------------------------------------------------------------------

} // db
} // sdl

#endif // __SDL_SYSTEM_DATATABLE_INL__
