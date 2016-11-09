// datatable.cpp
//
#include "dataserver/system/datatable.h"
#include "dataserver/system/database.h"
#include "dataserver/system/page_info.h"
#include "dataserver/utils/conv.h"

namespace sdl { namespace db {

datatable::datatable(database const * const p, shared_usertable const & t)
    : base_datatable(p, t)
    , _datarow(this)
    , _record(this)
    , _head(this)
{
    m_primary_key = this->db->get_primary_key(this->get_id());
    if (m_primary_key) {
        m_cluster_index = this->db->get_cluster_index(this->schema);
        if (m_cluster_index && m_cluster_index->is_root_index()) {
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

datatable::head_access::iterator
datatable::head_access::begin() const
{
    datarow_iterator it = _datarow.begin();
    while (it != _datarow.end()) {
        if (head_access::use_record(it))
            break;
        ++it;
    }
    return iterator(this, std::move(it));
}

datatable::page_head_access::iterator
datatable::page_head_access::make_iterator(datatable const * const tab, pageFileID const & id) const
{
    SDL_ASSERT(tab && id);
    SDL_ASSERT_DEBUG_2(tab->db->find_datapage(tab->get_id(), dataType::type::IN_ROW_DATA, pageType::type::data).get() == this);
    if (page_head const * h = tab->db->load_page_head(id)) {
        return iterator(this, page_pos(h, 0));
    }
    SDL_ASSERT(0);
    return this->end();
}

datatable::datapage_access::iterator
datatable::datapage_access::make_iterator(datatable const * p, pageFileID const & id) const
{
    return page_access->make_iterator(p, id);
}

datatable::datarow_access::iterator
datatable::datarow_access::make_iterator(datatable const * p, recordID const & rec) const
{
    return iterator(this, page_slot(_datapage.make_iterator(p, rec.id), rec.slot));
}

datatable::head_access::iterator
datatable::head_access::make_iterator(datatable const * p, recordID const & rec) const
{
    auto it = _datarow.make_iterator(p, rec);
    SDL_ASSERT(use_record(it));
    return iterator(this, std::move(it));
}

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

#if !defined(case_scalartype_cast)
#define case_scalartype_cast(SCALAR) \
    case SCALAR: \
        if (auto pv = scalartype_cast<SCALAR>(m, col)) \
            return to_string::type(*pv); \
        break;
#else
#error case_scalartype_cast
#endif

std::string datatable::record_type::type_fixed_col(mem_range_t && m, column const & col)
{
    SDL_ASSERT(mem_size(m) == col.fixed_size());
    switch (col.type) {
    case_scalartype_cast(scalartype::t_int)
    case_scalartype_cast(scalartype::t_bigint)
    case_scalartype_cast(scalartype::t_smallint)
    case_scalartype_cast(scalartype::t_real)
    case_scalartype_cast(scalartype::t_float)
    case_scalartype_cast(scalartype::t_numeric);
    case_scalartype_cast(scalartype::t_smalldatetime)
    case_scalartype_cast(scalartype::t_datetime)
    case_scalartype_cast(scalartype::t_uniqueidentifier)
    case scalartype::t_nchar:
        return to_string::type(make_nchar_checked(m));
    case scalartype::t_char:
        return std::string(m.first, m.second); // can be Windows-1251
    case scalartype::t_geography: 
        return geo_mem(std::move(m)).STAsText();
    default:
        break;
    }
    SDL_ASSERT_DEBUG_2(!"type_fixed_col");
    return to_string::dump_mem(m); // FIXME: not implemented
}

#undef case_scalartype_cast

std::string datatable::record_type::type_var_col(column const & col, size_t const col_index) const
{
    auto m = data_var_col(col, col_index);
    A_STATIC_CHECK_TYPE(vector_mem_range_t, m);
    if (!m.empty()) {
        switch (col.type) {
        case scalartype::t_text:
        case scalartype::t_varchar:
            return to_string::make_text(m);
        case scalartype::t_ntext:
        case scalartype::t_nvarchar:
            return to_string::make_ntext(m);
        case scalartype::t_geography:
            return geo_mem(std::move(m)).STAsText();
        case scalartype::t_geometry:
        case scalartype::t_varbinary:
            return to_string::dump_mem(m);
        default:
            SDL_ASSERT_DEBUG_2(!"type_var_col");
            return to_string::dump_mem(m);
        }
    }
    return {};
}

vector_mem_range_t
datatable::record_type::data_var_col(column const & col, size_t const col_index) const
{
    SDL_ASSERT(!null_bitmap(record)[table->ut().place(col_index)]); // already checked
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

Meters datatable::record_type::STDistance(col_size_t const i, spatial_point const & where) const
{
    if (is_null(i)) {
        SDL_ASSERT(is_geography(i));
        return 0;
    }
    if (is_geography(i)) {
        return geo_mem(this->data_col(i)).STDistance(where);
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
        return type_fixed_col(fixed_memory(col, i), col);
    }
    return type_var_col(col, i);
}

std::string datatable::record_type::type_col_utf8(col_size_t const i) const
{
    SDL_ASSERT(i < this->size());
    if (is_null(i)) {
        return {};
    }
    column const & col = usercol(i);
    if (scalartype::is_text(col.type)) {
        return conv::cp1251_to_utf8(type_col(i));
    }
    if (scalartype::is_ntext(col.type)) {
        return conv::nchar_to_utf8(data_col(i));
    }
    std::string s = type_col(i);
    SDL_ASSERT(conv::is_utf8(s));
    return s;
}

std::wstring datatable::record_type::type_col_wide(col_size_t const i) const
{
    SDL_ASSERT(i < this->size());
    if (is_null(i)) {
        return {};
    }
    column const & col = usercol(i);
    if (scalartype::is_text(col.type)) {
        return conv::cp1251_to_wide(type_col(i));
    }
    if (scalartype::is_ntext(col.type)) {
        return conv::nchar_to_wide(data_col(i));
    }
    std::string s = type_col(i);
    SDL_ASSERT(conv::is_utf8(s));
    return conv::utf8_to_wide(s);
}

size_t datatable::record_type::col_LEN(col_size_t const i) const
{
    SDL_ASSERT(i < this->size());
    if (is_null(i)) {
        return 0;
    }
    column const & col = usercol(i);
    if (col.is_fixed()) {
        switch (col.type) {
        case scalartype::t_char:
            return to_string::length_text(fixed_memory(col, i));
        case scalartype::t_nchar:
            return to_string::length_ntext(fixed_memory(col, i));
        default:
            SDL_ASSERT(0);
            break;
        }
    }
    else {
        const auto m = data_var_col(col, i);
        switch (col.type) {
        case scalartype::t_text:
        case scalartype::t_varchar:
            return to_string::length_text(m);
        case scalartype::t_ntext:
        case scalartype::t_nvarchar:
            return to_string::length_ntext(m);
        default:
            SDL_ASSERT(0);
            break;
        }
    }
    SDL_ASSERT(0);
    return type_col(i).size();
}

std::string datatable::record_type::operator[](const std::string & col_name) const
{
    SDL_ASSERT(!col_name.empty());
    const col_size_t i = table->ut().find(col_name);
    if (i < table->ut().size()) {
        return type_col(i);
    }
    SDL_ASSERT(0);
    return{};
}

std::string datatable::record_type::operator[](const char * const col_name) const
{
    SDL_ASSERT(is_str_valid(col_name));
    const col_size_t i = table->ut().find(col_name);
    if (i < table->ut().size()) {
        return type_col(i);
    }
    SDL_ASSERT(0);
    return{};
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

usertable::col_index
datatable::get_PrimaryKeyCol() const
{
    if (m_primary_key) {
        SDL_WARNING(m_primary_key->size() == 1);
        return this->ut().find_col(m_primary_key->primary());
    }
    return{};
}

spatial_tree_idx datatable::find_spatial_tree() const
{
    return this->db->find_spatial_tree(this->get_id());
}

namespace {
    struct make_spatial_tree : noncopyable {
        using ret_type = spatial_tree;
        datatable const * const this_;
        spatial_tree_idx const * const tree;
        make_spatial_tree(datatable const * p, spatial_tree_idx const * t): this_(p), tree(t) {}
        template<typename T> // T = scalartype_to_key
        ret_type operator()(T) const {
            return spatial_tree::make<typename T::type>(
                this_->db, tree->pgroot,
                this_->get_PrimaryKey(),
                tree->idx); 
        }
    };
}

spatial_tree
datatable::get_spatial_tree() const 
{
    if (auto const tree = find_spatial_tree()) {
        if (m_primary_key) {
            return case_scalartype_to_key::find(
                    m_primary_key->first_type(),
                    make_spatial_tree(this, &tree));
        }
        SDL_ASSERT(!"get_spatial_tree");
    }
    return {};
}

datatable::record_iterator
datatable::scan_table_with_record_key(key_mem const & key) const
{
    SDL_ASSERT(!m_index_tree); // scan small table without index tree
    if (shared_cluster_index const & index = get_cluster_index()) {
        SDL_ASSERT(mem_size(key) == index->key_length());
        auto const last = _record.end();
        for (auto it = _record.begin(); it != last; ++it) {
            auto buf = mem_utils::make_vector((*it).get_cluster_key(*index));
            mem_range_t const it_key = make_mem_range(buf);
            SDL_ASSERT(mem_size(it_key) == mem_size(key));
            if (!mem_compare(it_key, key)) {
                return it;
            }
        }
        return last;
    }
    else {
        SDL_ASSERT(0);
    }
    return _record.end();
}

template<class ret_type, class fun_type>
ret_type datatable::find_row_head_impl(key_mem const & key, fun_type const & fun) const
{
    SDL_ASSERT(mem_size(key) == cluster_key_length());
    SDL_ASSERT(is_index_tree());
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
                            return fun(data[slot], recordID::init(id, slot)); //FIXME: skip recordID::init ?
                        }
                    }
                    return ret_type();
                }
            }
            SDL_ASSERT(0);
        }
    }
    return ret_type();
}

size_t datatable::cluster_key_length() const
{
    if (m_cluster_index) {
        return m_cluster_index->key_length();
    }
    return 0;
}

datatable::record_iterator
datatable::find_record_iterator(key_mem const & key) const
{
    SDL_ASSERT(cluster_key_length() == mem_size(key));
    if (m_index_tree) {
        if (auto const found = find_row_head_impl<recordID>(key,
            [](row_head const *, recordID const & id) {
                return id; }))
        {
            return _record.make_iterator(this, found);
        }
        SDL_ASSERT(!"find_record_iterator");
        return _record.end();
    }
    return scan_table_with_record_key(key);
}

row_head const *
datatable::find_row_head(key_mem const & key) const
{
    if (m_index_tree) {
        return find_row_head_impl<row_head const *>(key, [](row_head const * head, recordID const &) {
            return head;
        });
    }
    else {
        const auto it = scan_table_with_record_key(key);
        if (it != _record.end()) {
            return (*it).head();
        }
        return nullptr;
    }
}

datatable::record_type
datatable::find_record(key_mem const & key) const
{
    if (m_index_tree) {
        return find_row_head_impl<record_type>(key, [this](row_head const * head, recordID const & id) {
            return record_type(this, head
    #if SDL_DEBUG_RECORD_ID
                , id
    #endif
                );
        });
    }
    else {
        const auto it = scan_table_with_record_key(key);
        if (it != _record.end()) {
            return *it;
        }
        return {};
    }
}

datatable::record_iterator
datatable::find_record_iterator(vector_mem_range_t const & v) const {
    auto buf = mem_utils::make_vector(v); // lvalue to avoid expiring buf
    return find_record_iterator(make_mem_range(buf));
}

datatable::record_type
datatable::find_record(vector_mem_range_t const & v) const {
    auto buf = mem_utils::make_vector(v); // lvalue to avoid expiring buf
    return find_record(make_mem_range(buf));
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