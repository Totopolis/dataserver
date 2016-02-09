// datatable.cpp
//
#include "common/common.h"
#include "datatable.h"
#include "database.h"
#include "page_info.h"

namespace sdl { namespace db {
   
void datatable::datarow_access::load_next(page_slot & p)
{
    SDL_ASSERT(!is_end(p));
    if (++p.second >= slot_array(*p.first).size()) {
        p.second = 0;
        ++p.first;
    }
}

void datatable::datarow_access::load_prev(page_slot & p)
{
    if (p.second > 0) {
        SDL_ASSERT(!is_end(p));
        --p.second;
    }
    else {
        SDL_ASSERT(!is_begin(p));
        --p.first;
        const size_t size = slot_array(*p.first).size(); // slot_array can be empty
        p.second = size ? (size - 1) : 0;
    }
    SDL_ASSERT(!is_end(p));
}

bool datatable::datarow_access::is_begin(page_slot const & p)
{
    if (!p.second && (p.first == _datapage.begin())) {
        return true;
    }
    return false;
}

bool datatable::datarow_access::is_end(page_slot const & p)
{
    if (p.first == _datapage.end()) {
        SDL_ASSERT(!p.second);
        return true;
    }
    return false;
}

bool datatable::datarow_access::is_empty(page_slot const & p)
{
    return datapage(*p.first).empty();
}

row_head const * datatable::datarow_access::dereference(page_slot const & p)
{
    const datapage page(*p.first);
    return page.empty() ? nullptr : page[p.second];
}

datatable::datarow_access::iterator
datatable::datarow_access::begin()
{
    return iterator(this, page_slot(_datapage.begin(), 0));
}

datatable::datarow_access::iterator
datatable::datarow_access::end()
{
    return iterator(this, page_slot(_datapage.end(), 0));
}

bool datatable::datarow_access::is_same(page_slot const & p1, page_slot const & p2)
{ 
    return p1 == p2;
}   

page_head const * 
datatable::datarow_access::get_page(iterator it)
{
    SDL_ASSERT(it != this->end());
    page_slot const & current = it.current;
    auto p = *current.first;
    A_STATIC_CHECK_TYPE(page_head const *, p);
    SDL_ASSERT(p);
    return p;
}

//--------------------------------------------------------------------------

datatable::record_access::record_access(datatable * p)
    : table(p)
    , _datarow(p, dataType::type::IN_ROW_DATA, pageType::type::data)
{
    SDL_ASSERT(table);
}

datatable::record_access::iterator
datatable::record_access::begin()
{
    datarow_iterator it = _datarow.begin();
    while (it != _datarow.end()) {
        if (use_record(it))
            break;
        ++it;
    }
    return iterator(this, std::move(it));
}

void datatable::record_access::load_next(datarow_iterator & it)
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

bool datatable::record_access::use_record(datarow_iterator const & it)
{
    if (row_head const * p = *it) {
        if (!p->is_forwarding_record()) // skip forwarding records 
            return true;
    }
    return false;
}

datatable::record_access::iterator
datatable::record_access::end()
{
    return iterator(this, _datarow.end());
}

bool datatable::record_access::is_end(datarow_iterator const & it)
{
    return (it == _datarow.end());
}

bool datatable::record_access::is_same(datarow_iterator const & p1, datarow_iterator const & p2)
{
    return p1 == p2;
}

datatable::record_type
datatable::record_access::dereference(datarow_iterator const & p)
{
    A_STATIC_CHECK_TYPE(row_head const *, *p);
    return record_type(table, *p, _datarow.get_page(p));
}

//--------------------------------------------------------------------------

datatable::record_type::record_type(datatable * p, row_head const * r, page_head const * h)
    : table(p), record(r), mypage(h)
{
    SDL_ASSERT(table && record && mypage);
}

size_t datatable::record_type::size() const
{
    return schema().size();
}

size_t datatable::record_type::fixed_data_size() const
{
    return record->fixed_size();
}

size_t datatable::record_type::var_data_size() const
{
    if (record->has_variable()) {
        return variable_array(record).var_data_size();
    }
    return 0;
}

size_t datatable::record_type::count_null() const
{
    if (record->has_null()) {
        return null_bitmap(record).count_null();
    }
    return 0;
}

bool datatable::record_type::is_null(size_t i) const
{
    SDL_ASSERT(i < this->size());
    if (record->has_null()) {
        return null_bitmap(record)[i];
    }
    return false;
}

size_t datatable::record_type::count_var() const
{
    if (record->has_variable()) {
        const size_t s = variable_array(record).size();
        // trailing NULL variable-length columns in the row are not stored
        // forwarded records adds forwarded_stub as variable column
        SDL_ASSERT(is_forwarded() || (s <= size()));
        SDL_ASSERT(is_forwarded() || (s <= table->ut().count_var()));
        SDL_ASSERT(!is_forwarded() || forwarded());
        return s;
    }
    return 0;
}

forwarded_stub const *
datatable::record_type::forwarded() const
{
    if (is_forwarded()) {
        if (record->has_variable()) {
            auto const m = variable_array(record).back_var_data();
            if (mem_size(m) == sizeof(forwarded_stub)) {
                forwarded_stub const * p = reinterpret_cast<forwarded_stub const *>(m.first);
                return p;
            }
        }
        SDL_ASSERT(0);
    }
    return nullptr;
}

size_t datatable::record_type::count_fixed() const
{
    const size_t s = table->ut().count_fixed();
    SDL_ASSERT(s <= size());
    return s;
}

namespace {

    template<typename T, scalartype::type type>
    T const * scalartype_t(const char * begin, const char * end, usertable::column const & col)
    {
        if (col.type == type) {
            if (((end - begin) == sizeof(T)) && (col.fixed_size() == sizeof(T))) {
                return reinterpret_cast<const T *>(begin);
            }
            SDL_ASSERT(0);
        }
        return nullptr;
    }

} // namespace

std::string datatable::record_type::type_col(size_t const i) const
{
    if (is_null(i)) {
        return "NULL";
    }
    size_t offset = 0;
    int j = 0;
    for (auto & col : schema()) {
        if (col.is_fixed()) {
            if (j == i) {
                mem_range_t const m = record->fixed_data();
                const char * const p1 = m.first + offset;
                const char * const p2 = p1 + col.fixed_size();
                if (p2 <= m.second) {
                    if (auto pv =  scalartype_t<int, scalartype::t_int>(p1, p2, col)) {
                        return to_string::type(*pv);
                    }
                    if (auto pv =  scalartype_t<float, scalartype::t_real>(p1, p2, col)) {
                        return to_string::type(*pv);
                    }
                }
                else {
                    SDL_ASSERT(!"bad offset");
                }
                break;
            }
            ++j;
            offset += col.fixed_size();
        }
    }
    return "?";
}

std::string datatable::record_type::type_pageId() const
{
    return to_string::type_less(mypage->data.pageId);
}

//--------------------------------------------------------------------------

datatable::sysalloc_access::vector_data const & 
datatable::sysalloc_access::find_sysalloc() const
{
    return table->db->find_sysalloc(table->get_id(), data_type);
}

datatable::datapage_access::vector_data const &
datatable::datapage_access::find_datapage() const
{
    return table->db->find_datapage(table->get_id(), data_type, page_type);
}

//--------------------------------------------------------------------------

} // db
} // sdl

//----------------------------------------------------------------------------------------------------------
// page iterator -> type, row[]
// row iterator -> column[] -> column type, name, length, value 
// iam_chain(IN_ROW_DATA|LOB_DATA|ROW_OVERFLOW_DATA) -> iam_page[] -> datapage, index_page => row[] => col[]
//----------------------------------------------------------------------------------------------------------
// forwarded row = 16 bytes or 9 bytes ?
// forwarding_stub = 9 bytes
// forwarded_stub = 10 bytes
// overflow_page = 24 bytes
// text_pointer = 16 bytes
// ROW_OVERFLOW_DATA = 24 bytes
// LOB_DATA (text, ntext, image columns) = 16 bytes
//----------------------------------------------------------------------------------------------------------
