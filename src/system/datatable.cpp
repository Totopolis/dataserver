// datatable.cpp
//
#include "common/common.h"
#include "datatable.h"
#include "database.h"
#include "overflow_page.h"
#include "page_info.h"

namespace sdl { namespace db {

datatable::datatable(database * p, shared_usertable const & t)
    : db(p), schema(t)
{
    SDL_ASSERT(db && schema);
}

datatable::~datatable()
{
}

//------------------------------------------------------------------

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

page_head const * 
datatable::datarow_access::get_page(iterator it)
{
    SDL_ASSERT(it != this->end());
    A_STATIC_CHECK_TYPE(page_slot, it.current);
    return *(it.current.first);
}

recordID datatable::datarow_access::get_id(iterator it)
{
    if (page_head const * page = get_page(it)) {
        A_STATIC_CHECK_TYPE(page_slot, it.current);
        return recordID::init(page->data.pageId, it.current.second);
    }
    return {};
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
    if (row_head const * const p = *it) {
        if (p->is_forwarding_record()) { // skip forwarding records 
            return false;
        }
        if (p->get_type() == recordType::ghost_data) { // skip ghosted records
            return false;
        }
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
    return record_type(table, *p, _datarow.get_id(p));
}

//------------------------------------------------------------------

datatable::record_type::record_type(datatable const * p, row_head const * row, const recordID & id)
    : table(p)
    , record(row)
    , this_id(id) // can be empty during find_record
{
    SDL_ASSERT(table && record);
    SDL_ASSERT(table->ut().size() == null_bitmap(record).size()); // A null bitmap is always present in data rows in heap tables or clustered index leaf rows
    SDL_ASSERT(record->fixed_size() == table->ut().fixed_size());
}

size_t datatable::record_type::size() const
{
    return table->ut().size();
}

datatable::record_type::column const & 
datatable::record_type::usercol(size_t i) const
{
    return table->ut()[i];
}

size_t datatable::record_type::fixed_size() const
{
    return record->fixed_size();
}

mem_range_t datatable::record_type::fixed_data() const
{
    return record->fixed_data();
}

size_t datatable::record_type::var_size() const
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

bool datatable::record_type::is_forwarded() const
{ 
    return record->is_forwarded_record();
}

bool datatable::record_type::is_null(size_t i) const
{
    SDL_ASSERT(i < this->size());
    if (record->has_null()) {
        return null_bitmap(record)[i];
    }
    SDL_ASSERT(0);
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

mem_range_t datatable::record_type::fixed_memory(column const & col, size_t const i) const
{
    mem_range_t const m = fixed_data();
    const char * const p1 = m.first + table->ut().fixed_offset(i);
    const char * const p2 = p1 + col.fixed_size();
    if (p2 <= m.second) {
        return { p1, p2 };
    }
    SDL_ASSERT(!"bad offset");
    return{};
}

namespace {

template<typename T, scalartype::type type> inline
T const * scalartype_cast(mem_range_t const & m, usertable::column const & col) {
    if (col.type == type) {
        SDL_ASSERT(col.fixed_size() == sizeof(T));
        if (mem_size(m) == sizeof(T)) {
            return reinterpret_cast<const T *>(m.first);
        }
        SDL_ASSERT(0);
    }
    return nullptr; 
}

} // namespace

std::string datatable::record_type::type_fixed_col(mem_range_t const & m, column const & col)
{
    SDL_ASSERT(mem_size(m) == col.fixed_size());

    if (auto pv = scalartype_cast<int, scalartype::t_int>(m, col)) {
        return to_string::type(*pv);
    }
    if (auto pv = scalartype_cast<int64, scalartype::t_bigint>(m, col)) {
        return to_string::type(*pv);
    }
    if (auto pv = scalartype_cast<int16, scalartype::t_smallint>(m, col)) {
        return to_string::type(*pv);
    }
    if (auto pv = scalartype_cast<float, scalartype::t_real>(m, col)) {
        return to_string::type(*pv);
    }
    if (auto pv = scalartype_cast<double, scalartype::t_float>(m, col)) {
        return to_string::type(*pv);
    }
    if (col.type == scalartype::t_numeric) {
        if ((mem_size(m) == (sizeof(int64) + 1)) && ((*m.first) == 0x01))  {
            auto pv = reinterpret_cast<int64 const *>(m.first + 1);
            return to_string::type(*pv);
        }
        SDL_ASSERT(0);
        return to_string::dump_mem(m); // FIXME: not implemented
    }
    if (auto pv = scalartype_cast<smalldatetime_t, scalartype::t_smalldatetime>(m, col)) {
        return to_string::type(*pv);
    }
    if (auto pv = scalartype_cast<guid_t, scalartype::t_uniqueidentifier>(m, col)) {
        return to_string::type(*pv);
    }
    if (col.type == scalartype::t_nchar) {
        return to_string::type(make_nchar_checked(m));
    }
    if (col.type == scalartype::t_char) {
        return std::string(m.first, m.second); // can be Windows-1251
    }
    SDL_ASSERT(0);
    return "?"; // FIXME: not implemented
}

std::string datatable::record_type::type_var_col(column const & col, size_t const col_index) const
{
    auto const m = data_var_col(col, col_index);
    if (!m.empty()) {
        switch (col.type) {
        case scalartype::t_text:
        case scalartype::t_varchar:
            return to_string::make_text(m);
        case scalartype::t_ntext:
        case scalartype::t_nvarchar:
            return to_string::make_ntext(m);
        case scalartype::t_geography:
        case scalartype::t_varbinary:
            return to_string::dump_mem(m);
        default:
            SDL_ASSERT(!"unknown data type");
            return to_string::dump_mem(m);
        }
    }
    return {};
}

vector_mem_range_t
datatable::record_type::data_var_col(column const & col, size_t const col_index) const
{
    const size_t i = table->ut().var_offset(col_index);
    if (i < count_var()) {
        const variable_array data(record);
        const mem_range_t m = data.var_data(i);
        const size_t len = mem_size(m);
        if (!len) {
            SDL_ASSERT(0);
            return{};
        }
        if (data.is_complex(i)) {
            // If length == 16 then we're dealing with a LOB pointer, otherwise it's a regular complex column
            if (len == sizeof(text_pointer)) { // 16 bytes
                auto const tp = reinterpret_cast<text_pointer const *>(m.first);
                if ((col.type == scalartype::t_text) || 
                    (col.type == scalartype::t_ntext)) {
                    return text_pointer_data(table->db, tp).data();
                }
            }
            else {
                const auto type = data.var_complextype(i);
                SDL_ASSERT(type != complextype::none);
                if (type == complextype::row_overflow) {
                    if (len == sizeof(overflow_page)) { // 24 bytes
                        auto const overflow = reinterpret_cast<overflow_page const *>(m.first);
                        SDL_ASSERT(overflow->type == type);
                        if (col.type == scalartype::t_varchar) {
                            return varchar_overflow_page(table->db, overflow).data();
                        }
                    }
                }
                else if (type == complextype::blob_inline_root) {
                    if (len == sizeof(overflow_page)) { // 24 bytes
                        auto const overflow = reinterpret_cast<overflow_page const *>(m.first);
                        SDL_ASSERT(overflow->type == type);
                        if (col.type == scalartype::t_geography) {
                            const varchar_overflow_page varchar(table->db, overflow);
                            SDL_ASSERT(varchar.length() == overflow->length);
                            return varchar.data();
                        }
                    }
                    if (len > sizeof(overflow_page)) { // 24 bytes + 12 bytes * link_count 
                        SDL_ASSERT(!((len - sizeof(overflow_page)) % sizeof(overflow_link)));
                        if (col.type == scalartype::t_geography) {
                            auto const page = reinterpret_cast<overflow_page const *>(m.first);
                            size_t const link_count = (len - sizeof(overflow_page)) / sizeof(overflow_link);
                            auto const link = reinterpret_cast<overflow_link const *>(page + 1);
                            const varchar_overflow_page varchar(table->db, page);
                            SDL_ASSERT(varchar.length() == page->length);
                            auto memory = varchar.data();
                            for (size_t i = 0; i < link_count; ++i) {
                                const varchar_overflow_link next(table->db, page, link + i);
                                memory.insert(memory.end(), next.data().begin(), next.data().end());
                                SDL_ASSERT(mem_size_n(memory) == link[i].size);
                            }
                            return memory;
                        }
                    }
                }
            }
            if (col.type == scalartype::t_varbinary) {
                return { m };
            }
            SDL_ASSERT(!"unknown data type");
            return { m };
        }
        else { // in-row-data
            return { m };
        }
    }
    throw_error<record_error>("bad var_offset");
    return {};
}

std::string datatable::record_type::type_col(size_t const i) const
{
    SDL_ASSERT(i < this->size());

    if (is_null(i)) {
        return {};
    }
    column const & col = usercol(i);
    if (col.is_fixed()) {
        return type_fixed_col(fixed_memory(col, i), col);
    }
    return type_var_col(col, i);
}

vector_mem_range_t datatable::record_type::data_col(size_t const i) const
{
    SDL_ASSERT(i < this->size());

    if (is_null(i)) {
        return {};
    }
    column const & col = usercol(i);
    if (col.is_fixed()) {
        return { fixed_memory(col, i) };
    }
    return data_var_col(col, i);
}

vector_mem_range_t
datatable::record_type::get_cluster_key(cluster_index const & index) const
{
    vector_mem_range_t m;
    for (size_t i = 0; i < index.size(); ++i) {
        vector_mem_range_t m2 = data_col(index.col_index[i]);
        m.insert(m.end(), m2.begin(), m2.end());
    }
    if (m.size() == index.size()) {
        if (mem_size(m) == index.key_length()) {
            return m;
        }
        SDL_ASSERT(0);
    }
    // keys values are splitted ?
    throw_error<record_error>("get_cluster_key");
    return {}; 
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

datatable::datapage_order::datapage_order(datatable * p, dataType::type t1, pageType::type t2)
    : ordered(datapage_access(p, t1, t2).find_datapage())
{
    std::sort(ordered.begin(), ordered.end(), 
        [](page_head const * x, page_head const * y){
        return (x->data.pageId < y->data.pageId);
    });
}

//--------------------------------------------------------------------------

shared_primary_key
datatable::get_PrimaryKey() const
{
    return db->get_PrimaryKey(this->get_id());
}

datatable::column_order
datatable::get_PrimaryKeyOrder() const
{
    if (auto p = get_PrimaryKey()) {
        if (auto col = this->schema->find_col(p->primary()).first) {
            SDL_ASSERT(p->first_order() != sortorder::NONE);
            return { col, p->first_order() };
        }
    }
    return { nullptr, sortorder::NONE };
}

unique_cluster_index
datatable::get_cluster_index() const
{
    return db->get_cluster_index(this->schema);
}

unique_index_tree
datatable::get_index_tree() const
{
    if (auto p = get_cluster_index()) {
        return sdl::make_unique<index_tree>(this->db, std::move(p));
    }
    return {};
}

datatable::unique_record
datatable::find_record(key_mem const & key) const
{
    SDL_ASSERT(mem_size(key));
    if (auto tree = get_index_tree()) {
        if (auto const id = tree->find_page(key)) {
            if (page_head const * const h = db->load_page_head(id)) {
                SDL_ASSERT(h->is_data());
                const datapage data(h);
                if (!data.empty()) {
                    index_tree const * const tr = tree.get();
                    size_t const slot = data.lower_bound(
                        [this, tr, key](row_head const * const row, size_t) {
                        return tr->key_less(
                            record_type(this, row).get_cluster_key(tr->index()),
                            key);
                    });
                    if (slot < data.size()) {
                        if (!tr->key_less(key, record_type(this, data[slot]).get_cluster_key(tr->index()))) {
                            return sdl::make_unique<record_type>(this, data[slot], recordID::init(id, slot));
                        }
                    }
                    return {};
                }
            }
            SDL_ASSERT(0);
        }
    }
    return {};
}

} // db
} // sdl

