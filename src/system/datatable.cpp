// datatable.cpp
//
#include "common/common.h"
#include "datatable.h"
#include "database.h"
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

//--------------------------------------------------------------------------

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

//------------------------------------------------------------------

datatable::record_type::record_type(datatable const * p, row_head const * row, const recordID & id)
    : table(p)
    , record(row)
    , this_id(id) // can be empty during find_record
{
    // A null bitmap is always present in data rows in heap tables or clustered index leaf rows
    SDL_ASSERT(table && record);
    SDL_ASSERT(record->fixed_size() == table->ut().fixed_size());
    throw_error_if<record_error>(!record->has_null(), "null bitmap missing");
    if (table->ut().size() != null_bitmap(record).size()) {
        // When you create a non unique clustered index, SQL Server creates a hidden 4 byte uniquifier column that ensures that all rows in the index are distinctly identifiable
        throw_error<record_error>("uniquifier column?");
    }
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

size_t datatable::record_type::count_fixed() const
{
    const size_t s = table->ut().count_fixed();
    SDL_ASSERT(s <= size());
    return s;
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

mem_range_t datatable::record_type::fixed_memory(column const & col, size_t const i) const
{
    mem_range_t const m = record->fixed_data();
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
        //case scalartype::t_geometry:
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
    return table->db->get_variable(record, table->ut().var_offset(col_index), col.type);
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

