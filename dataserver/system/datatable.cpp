// datatable.cpp
//
#include "common/common.h"
#include "datatable.h"
#include "database.h"
#include "page_info.h"

namespace sdl { namespace db {

datatable::datatable(database const * const p, shared_usertable const & t)
    : base_datatable(p, t)
    , _datarow(this)
    , _record(this)
    , _head(this)
{
    if ((m_primary_key = this->db->get_primary_key(this->get_id()))) {
        if ((m_cluster_index = this->db->get_cluster_index(this->schema))) {
            m_index_tree = std::make_shared<index_tree>(this->db, m_cluster_index);
        }
    }
}

datatable::head_access::head_access(base_datatable const * p)
    : table(p)
    , _datarow(p, dataType::type::IN_ROW_DATA, pageType::type::data)
{
    SDL_ASSERT(table);
}

//------------------------------------------------------------------
#if 0 // reserved
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
#endif

//------------------------------------------------------------------

datatable::record_type::record_type(base_datatable const * const p, row_head const * const row
#if SDL_DEBUG_RECORD_ID
    , const recordID & id
#endif
)
    : table(p)
    , record(row)
#if SDL_DEBUG_RECORD_ID
    , this_id(id) // can be empty during find_record
#endif
{
    SDL_ASSERT(table && record);
    if (!record->has_null()) {
        // A null bitmap is always present in data rows in heap tables or clustered index leaf rows
        throw_error<record_error>("null bitmap missing");
    }
    else if (table->ut().size() != null_bitmap(record).size()) {
        // When you create a non unique clustered index, SQL Server creates a hidden 4 byte uniquifier column that ensures that all rows in the index are distinctly identifiable
        throw_error<record_error>("uniquifier column?");
    }
    SDL_ASSERT(record->fixed_size() == table->ut().fixed_size()); //FIXME: need to rebuild database ?
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

std::string datatable::record_type::type_fixed_col(mem_range_t const & m, column const & col)
{
    SDL_ASSERT(mem_size(m) == col.fixed_size());

    if (auto pv = scalartype_cast<scalartype::t_int>(m, col)) {
        return to_string::type(*pv);
    }
    if (auto pv = scalartype_cast<scalartype::t_bigint>(m, col)) {
        return to_string::type(*pv);
    }
    if (auto pv = scalartype_cast<scalartype::t_smallint>(m, col)) {
        return to_string::type(*pv);
    }
    if (auto pv = scalartype_cast<scalartype::t_real>(m, col)) {
        return to_string::type(*pv);
    }
    if (auto pv = scalartype_cast<scalartype::t_float>(m, col)) {
        return to_string::type(*pv);
    }
    if (auto pv = scalartype_cast<scalartype::t_numeric>(m, col)) {
        SDL_ASSERT(pv->_8 == 1);
        return to_string::type(*pv);
    }
    if (auto pv = scalartype_cast<scalartype::t_smalldatetime>(m, col)) {
        return to_string::type(*pv);
    }
    if (auto pv = scalartype_cast<scalartype::t_datetime>(m, col)) {
        return to_string::type(*pv);
    }   
    if (auto pv = scalartype_cast<scalartype::t_uniqueidentifier>(m, col)) {
        return to_string::type(*pv);
    }
    if (col.type == scalartype::t_nchar) {
        return to_string::type(make_nchar_checked(m));
    }
    if (col.type == scalartype::t_char) {
        return std::string(m.first, m.second); // can be Windows-1251
    }
    return to_string::dump_mem(m); // FIXME: not implemented
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
        case scalartype::t_geometry:
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
    SDL_ASSERT(!null_bitmap(record)[table->ut().place(col_index)]); // already checked
#if 0 //(SDL_DEBUG > 1)
    if (is_geography(col_index)) {
        auto test = table->db->get_geography(this->record, table->ut().var_offset(col_index)); // test API
    }
#endif
    return table->db->var_data(record, table->ut().var_offset(col_index), col.type);
}

//Note. null_bitmap relies on real columns order in memory, which can differ from table schema order
bool datatable::record_type::is_null(col_size_t const i) const
{
    SDL_ASSERT(i < this->size());
    return null_bitmap(record)[table->ut().place(i)];
}

bool datatable::record_type::is_geography(col_size_t const i) const
{
    return this->usercol(i).is_geography();
}

std::string datatable::record_type::STAsText(col_size_t const i) const
{
    if (is_null(i)) {
        SDL_ASSERT(is_geography(i));
        return {};
    }
    if (is_geography(i)) {
        return geo_mem(this->data_col(i)).STAsText();
    }
    SDL_ASSERT(0);
    return {};
}

bool datatable::record_type::STContains(col_size_t const i, spatial_point const & p) const
{
    if (is_null(i)) {
        SDL_ASSERT(is_geography(i));
        return false;
    }
    if (is_geography(i)) {
        return geo_mem(this->data_col(i)).STContains(p);
    }
    SDL_ASSERT(0);
    return false;
}

Meters datatable::record_type::STDistance(col_size_t const i, spatial_point const & where, spatial_rect const * const bbox) const
{
    if (is_null(i)) {
        SDL_ASSERT(is_geography(i));
        return 0;
    }
    if (is_geography(i)) {
        return geo_mem(this->data_col(i)).STDistance(where, bbox);
    }
    SDL_ASSERT(0);
    return 0;
}

spatial_type datatable::record_type::geo_type(col_size_t const i) const
{
    if (is_null(i)) {
        SDL_ASSERT(is_geography(i));
        return spatial_type::null;
    }
    if (is_geography(i)) {
        return geo_mem(this->data_col(i)).type();
    }
    SDL_ASSERT(0);
    return spatial_type::null;
}

geo_mem datatable::record_type::geography(col_size_t const i) const
{
    if (is_null(i)) {
        SDL_ASSERT(is_geography(i));
        return {};
    }
    if (is_geography(i)) {
        return geo_mem(this->data_col(i));
    }
    SDL_ASSERT(0);
    return{};
}

std::string datatable::record_type::type_col(col_size_t const i) const
{
    SDL_ASSERT(i < this->size());
    if (is_null(i)) {
        return {};
    }
    column const & col = usercol(i);
    if (col.is_fixed()) {
#if 0 //(SDL_DEBUG > 1)
        if (col.type == scalartype::t_int) {
            SDL_ASSERT(cast_fixed_col<scalartype::t_int>(i));
        }
#endif
        return type_fixed_col(fixed_memory(col, i), col);
    }
    return type_var_col(col, i);
}

vector_mem_range_t datatable::record_type::data_col(col_size_t const i) const
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
        append(m, data_col(index.col_ind(i)));
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

datatable::sysalloc_access::sysalloc_access(base_datatable const * p, dataType::type const t)
    : sysalloc(p->db->find_sysalloc(p->get_id(), t))
{
    SDL_ASSERT(t != dataType::type::null);
    SDL_ASSERT(sysalloc);
}

//--------------------------------------------------------------------------

datatable::datapage_access::datapage_access(base_datatable const * p, 
    dataType::type const t1, pageType::type const t2)
    : page_access(p->db->find_datapage(p->get_id(), t1, t2))
{
    SDL_ASSERT(t1 != dataType::type::null);
    SDL_ASSERT(t2 != pageType::type::null);
    SDL_ASSERT(page_access);
}

datatable::column_order
datatable::get_PrimaryKeyOrder() const
{
    if (m_primary_key) {
        if (auto col = this->ut().find_col(m_primary_key->primary()).first) {
            SDL_ASSERT(m_primary_key->first_order() != sortorder::NONE);
            return { col, m_primary_key->first_order() };
        }
    }
    return { nullptr, sortorder::NONE };
}

spatial_tree_idx datatable::find_spatial_tree() const {
    return this->db->find_spatial_tree(this->get_id());
}

unique_spatial_tree
datatable::get_spatial_tree() const 
{
    if (auto const tree = find_spatial_tree()) {
        if (m_primary_key) {
            A_STATIC_ASSERT_TYPE(int64, spatial_tree::pk0_type);
            constexpr scalartype::type spatial_scalartype = key_to_scalartype<spatial_tree::pk0_type>::value;
            auto const & pk0 = m_primary_key;
            if ((1 == pk0->size()) && (pk0->first_type() == spatial_scalartype)) {
                return sdl::make_unique<spatial_tree>(this->db, tree.pgroot, pk0, tree.idx);
            }
            SDL_WARNING(!"not implemented");
        }
        else {
            SDL_ASSERT(0);
        }
    }
    return {};
}

template<class ret_type, class fun_type>
ret_type datatable::find_row_head_impl(key_mem const & key, fun_type const & fun) const
{
    SDL_ASSERT(mem_size(key));
    if (m_index_tree) {
        if (auto const id = m_index_tree->find_page(key)) {
            if (page_head const * const h = db->load_page_head(id)) {
                SDL_ASSERT(h->is_data());
                const datapage data(h);
                if (!data.empty()) {
                    index_tree const * const tr = m_index_tree.get();
                    size_t const slot = data.lower_bound(
                        [this, tr, key](row_head const * const row) {
                        return tr->key_less(
                            record_type(this, row).get_cluster_key(tr->index()),
                            key);
                    });
                    if (slot < data.size()) {
                        if (!tr->key_less(key, record_type(this, data[slot]).get_cluster_key(tr->index()))) {
                            return fun(data[slot]
#if SDL_DEBUG_RECORD_ID
                                , recordID::init(id, slot)
#endif
                            );
                        }
                    }
                    return ret_type{};
                }
            }
            SDL_ASSERT(0);
        }
    }
    return ret_type{};
}

row_head const *
datatable::find_row_head(key_mem const & key) const {
    return find_row_head_impl<row_head const *>(key, [](row_head const * head
#if SDL_DEBUG_RECORD_ID
        , const recordID &
#endif
        ) {
        return head;
    });
}

datatable::record_type
datatable::find_record(key_mem const & key) const {
    return find_row_head_impl<record_type>(key, [this](row_head const * head
#if SDL_DEBUG_RECORD_ID
        , const recordID & id
#endif
        ) {
        return record_type(this, head
#if SDL_DEBUG_RECORD_ID
            , id
#endif
            );
    });
}

} // db
} // sdl

#if 0 //SDL_DEBUG
namespace sdl { namespace db { namespace {
class unit_test {
public:
    unit_test()
    {
        if (0) {
            struct clustered {
                struct T0 {
                    using type = int;
                };
                struct key_type {
                    T0::type _0;
                    using this_clustered = clustered;
                };
            };
            make::index_tree<clustered::key_type> test(nullptr, nullptr);
            if (test.min_page()){}
            if (test.max_page()){}
        }
    }
};
static unit_test s_test;
}
} // db
} // sdl
#endif //#if SV_DEBUG