// database.cpp
//
#include "common/common.h"
#include "database.h"
#include "page_map.h"
#include "overflow.h"
#include "database_fwd.h"
#include "common/map_enum.h"
#include <map>
#include <algorithm>

namespace sdl { namespace db {
   
class database::data_t : noncopyable {
    using shared_page_head_access = std::shared_ptr<page_head_access>;
    using map_sysalloc = compact_map<schobj_id, vector_sysallocunits_row>;
    using map_datapage = compact_map<schobj_id, shared_page_head_access>;
    using map_index = compact_map<schobj_id, pgroot_pgfirst>;
    using map_primary = compact_map<schobj_id, shared_primary_key>;
    using map_cluster = compact_map<schobj_id, shared_cluster_index>;
    using map_spatial_tree = compact_map<schobj_id, spatial_tree_idx>;
public:
    explicit data_t(const std::string & fname): pm(fname){}
    PageMapping pm;    
    vector_shared_usertable shared_usertable;
    vector_shared_usertable shared_internal; // INTERNAL_TABLE
    vector_shared_datatable shared_datatable;
    map_enum_1<map_sysalloc, dataType> sysalloc;
    map_enum_2<map_datapage, dataType, pageType> datapage;
    map_enum_1<map_index, pageType> index;
    map_primary primary;
    map_cluster cluster;
    map_spatial_tree spatial_tree;
};

database::database(const std::string & fname)
    : m_data(sdl::make_unique<data_t>(fname))
{
}

database::~database()
{
}

const std::string & database::filename() const
{
    return m_data->pm.filename;
}

bool database::is_open() const
{
    return m_data->pm.is_open();
}

void const * database::start_address() const
{
    return m_data->pm.start_address();
}

void const * database::memory_offset(void const * p) const
{
    char const * p1 = (char const *)start_address();
    char const * p2 = (char const *)p;
    SDL_ASSERT(p2 >= p1);
    void const * offset = reinterpret_cast<void const *>(p2 - p1);    
    return offset;
}

size_t database::page_count() const
{
    return m_data->pm.page_count();
}

page_head const *
database::load_page_head(pageIndex i)
{
    return m_data->pm.load_page(i);
}

page_head const *
database::load_page_head(pageFileID const & id)
{
    return m_data->pm.load_page(id);
}

page_head const * 
database::load_page_head(sysPage i)
{
    return load_page_head(static_cast<pageIndex::value_type>(i));
}

database::page_row
database::load_page_row(recordID const & row)
{
    if (page_head const * const h = load_page_head(row.id)) {
        const datapage data(h);
        if (row.slot < data.size()) {
            if (row_head const * const r = data[row.slot]) {
                return { h, r };
            }
        }
    }
    SDL_ASSERT(0);
    return{};
}

std::vector<page_head const *>
database::load_page_list(page_head const * p)
{
    std::vector<page_head const *> vec;
    if (p) {
        auto prev = p;
        while ((prev = load_prev_head(prev)) != nullptr) {
            vec.push_back(prev);
        }
        std::reverse(vec.begin(), vec.end());
        vec.push_back(p);
        while ((p = load_next_head(p)) != nullptr) {
            vec.push_back(p);
        }
    }
    return vec;
}

pageType database::get_pageType(pageFileID const & id)
{
    if (auto p = load_page_head(id)) {
        SDL_ASSERT(is_allocated(p));
        return p->data.type;
    }
    return pageType::init(pageType::type::null);
}

bool database::is_pageType(pageFileID const & id, pageType::type const t)
{
    SDL_ASSERT(t != pageType::type::null);
    if (auto p = load_page_head(id)) {
        return p->data.type == t;
    }
    SDL_ASSERT(0);
    return false;
}

pageFileID database::nextPageID(pageFileID const & id) // diagnostic
{
    if (auto p = load_page_head(id)) {
        return p->data.nextPage;
    }
    SDL_ASSERT(id.is_null());
    return {};
}

pageFileID database::prevPageID(pageFileID const & id) // diagnostic
{
    if (auto p = load_page_head(id)) {
        return p->data.prevPage;
    }
    SDL_ASSERT(id.is_null());
    return {};
}

database::page_ptr<bootpage>
database::get_bootpage()
{
    page_head const * const h = load_page_head(sysPage::boot_page);
    if (h) {
        return make_unique<bootpage>(h, cast::page_body<bootpage_row>(h));
    }
    return {};
}

database::page_ptr<datapage>
database::get_datapage(pageIndex i)
{
    page_head const * const h = load_page_head(i);
    if (h) {
        return make_unique<datapage>(h);
    }
    return {};
}

database::page_ptr<fileheader>
database::get_fileheader()
{
    page_head const * const h = load_page_head(0);
    if (h) {
        return make_unique<fileheader>(h);
    }
    return {};
}

page_head const * database::sysallocunits_head()
{
    auto boot = get_bootpage();
    if (boot) {
        auto & id = boot->row->data.dbi_firstSysIndexes;
        page_head const * h = m_data->pm.load_page(id);
        SDL_ASSERT(h->is_data());
        return h;
    }
    SDL_ASSERT(0);
    return nullptr;
}

database::page_ptr<sysallocunits>
database::get_sysallocunits()
{
    if (auto p = sysallocunits_head()) {
        return make_unique<sysallocunits>(p);
    }
    SDL_ASSERT(0);
    return {};
}

database::page_ptr<pfs_page>
database::get_pfs_page()
{
    page_head const * const h = load_page_head(sysPage::PFS);
    if (h) {
        return make_unique<pfs_page>(h);
    }
    SDL_ASSERT(0);
    return {};
}

page_head const *
database::load_sys_obj(const sysObj id)
{
    if (auto h = sysallocunits_head()) {
        if (auto row = sysallocunits(h).find_auid(static_cast<uint32>(id))) {
            return load_page_head(row->data.pgfirst);
        }
    }
    SDL_ASSERT(0);
    return nullptr;
}

//---------------------------------------------------------

page_head const * database::load_next_head(page_head const * const p)
{
    if (p) {
        auto next = m_data->pm.load_page(p->data.nextPage);
        SDL_ASSERT(!next || (next->data.type == p->data.type));
        return next;
    }
    SDL_ASSERT(0);
    return nullptr;
}

page_head const * database::load_prev_head(page_head const * const p)
{
    if (p) {
        auto prev = m_data->pm.load_page(p->data.prevPage);
        SDL_ASSERT(!prev || (prev->data.type == p->data.type));
        return prev;
    }
    SDL_ASSERT(0);
    return nullptr;
}

page_head const * database::load_last_head(page_head const * p)
{
    page_head const * next;
    while ((next = load_next_head(p)) != nullptr) {
        p = next;
    }
    return p;
}

page_head const * database::load_first_head(page_head const * p)
{
    page_head const * prev;
    while ((prev = load_prev_head(p)) != nullptr) {
        p = prev;
    }
    return p;
}

recordID database::load_next_record(recordID const & r)
{
    SDL_ASSERT(r.id);
    if (page_head const * const h = load_page_head(r.id)) {
        size_t const size = slot_array(h).size();
        SDL_ASSERT(r.slot < size);
        if ((size_t)r.slot + 1 < size) {
            return recordID::init(r.id, r.slot + 1);
        }
        if (auto const next = load_next_head(h)) {
            if (slot_array(next).size()) {
                return recordID::init(next->data.pageId);
            }
            SDL_ASSERT(0);
        }
    }
    else {
        SDL_ASSERT(0);
    }
    return{};
}

recordID database::load_prev_record(recordID const & r)
{
    SDL_ASSERT(r.id);
    if (page_head const * const h = load_page_head(r.id)) {
        SDL_ASSERT(r.slot <= slot_array(h).size());
        if (r.slot) {
            return recordID::init(r.id, r.slot - 1);
        }
        if (auto const prev = load_prev_head(h)) {
            size_t const size = slot_array(prev).size();
            if (size) {
                return recordID::init(prev->data.pageId, size - 1);
            }
            SDL_ASSERT(0);
        }
    }
    else {
        SDL_ASSERT(0);
    }
    return{};
}

template<class fun_type>
unique_datatable database::find_table_if(fun_type fun)
{
    for (auto & p : _usertables) {
        const usertable & d = *p.get();
        if (fun(d)) {
            return sdl::make_unique<datatable>(this, p);
        }
    }
    return {};
}

template<class fun_type>
unique_datatable database::find_internal_if(fun_type fun)
{
    for (auto & p : _internals) {
        const usertable & d = *p.get();
        if (fun(d)) {
            return sdl::make_unique<datatable>(this, p);
        }
    }
    return {};
}

unique_datatable database::find_table(const std::string & name)
{
    SDL_ASSERT(!name.empty());
    return find_table_if([&name](const usertable & d) {
        return d.name() == name;
    });
}

unique_datatable database::find_table(schobj_id const id)
{
    return find_table_if([id](const usertable & d) {
        return d.get_id() == id;
    });
}

unique_datatable database::find_internal(const std::string & name)
{
    SDL_ASSERT(!name.empty());
    return find_internal_if([&name](const usertable & d) {
        return d.name() == name;
    });
}

unique_datatable database::find_internal(schobj_id const id)
{
    return find_internal_if([id](const usertable & d) {
        return d.get_id() == id;
    });
}

shared_usertable database::find_table_schema(schobj_id const id)
{
    for (auto const & p : _usertables) {
        if (p->get_id() == id) {
            return p;
        }
    }
    SDL_ASSERT(0);
    return {};
}

shared_usertable database::find_internal_schema(schobj_id const id)
{
    for (auto const & p : _internals) {
        if (p->get_id() == id) {
            return p;
        }
    }
    SDL_ASSERT(0);
    return {};
}

template<class fun_type>
void database::for_USER_TABLE(fun_type fun) {
    for_row(_sysschobjs, [&fun](sysschobjs::const_pointer row){
        if (row->is_USER_TABLE()) {
            fun(row);
        }
    });
}

template<class fun_type>
void database::for_INTERNAL_TABLE(fun_type fun) {
    for_row(_sysschobjs, [&fun](sysschobjs::const_pointer row){
        if (row->is_INTERNAL_TABLE()) {
            fun(row);
        }
    });
}

template<class fun_type>
void database::get_tables(vector_shared_usertable & m_ut, fun_type is_table)
{
    SDL_ASSERT(m_ut.empty());
    vector_shared_usertable ret;
    for_row(_sysschobjs, [&ret, &is_table, this](sysschobjs::const_pointer schobj_row){
        if (is_table(schobj_row)) {
            const schobj_id table_id = schobj_row->data.id;
            usertable::columns cols;
            for_row(_syscolpars, [&cols, table_id, this](syscolpars::const_pointer colpar_row) {
                if (colpar_row->data.id == table_id) {
                    const scalartype utype = colpar_row->data.utype;
                    if (auto scalar_row = find_if(_sysscalartypes, [utype](sysscalartypes::const_pointer p) {
                        return (p->data.id == utype);
                    })) {
                        usertable::emplace_back(cols, colpar_row, scalar_row);
                    }
                }
            });
            if (!cols.empty()) {
                primary_key const * const PK = get_primary_key(table_id).get();             
                auto ut = std::make_shared<usertable>(schobj_row, std::move(cols), PK);
                SDL_ASSERT(schobj_row->data.id == ut->get_id());
                ret.push_back(std::move(ut));
            }
        }
    });
    if (!ret.empty()) {
        using table_type = vector_shared_usertable::value_type;
        std::sort(ret.begin(), ret.end(),
            [](table_type const & x, table_type const & y){
            return x->name() < y->name();
        });    
        ret.swap(m_ut);
        m_ut.shrink_to_fit(); 
    }
}

vector_shared_usertable const &
database::get_usertables()
{
    auto & m_ut = m_data->shared_usertable;
    if (m_ut.empty()) {
        get_tables(m_ut, [](sysschobjs::const_pointer row){
            return row->is_USER_TABLE();
        });
    }
    return m_ut;
}

vector_shared_usertable const &
database::get_internals()
{
    auto & m_ut = m_data->shared_internal;
    if (m_ut.empty()) {
        get_tables(m_ut, [](sysschobjs::const_pointer row){
            return row->is_INTERNAL_TABLE();
        });
    }
    return m_ut;
}

vector_shared_datatable const &
database::get_datatable()
{
    auto & m_dt = m_data->shared_datatable;
    if (!m_dt.empty())
        return m_dt;

    auto & ut = this->get_usertables();
    m_dt.reserve(ut.size());

    for (auto & p : ut) {
        m_dt.push_back(std::make_shared<datatable>(this, p));
    }
    using table_type = vector_shared_datatable::value_type;
    std::sort(m_dt.begin(), m_dt.end(),
        [](table_type const & x, table_type const & y){
        return x->name() < y->name();
    });
    m_dt.shrink_to_fit(); 
    return m_dt;
}

database::vector_sysallocunits_row const &
database::find_sysalloc(schobj_id const id, dataType::type const data_type) // FIXME: scanPartition ?
{
    if (auto found = m_data->sysalloc.find(id, data_type)) {
        return *found;
    }
    vector_sysallocunits_row & result = m_data->sysalloc(id, data_type);
    SDL_ASSERT(result.empty());
    auto push_back = [data_type, &result](sysallocunits_row const * const row) {
        if (row->data.type == data_type) {
            if (std::find(result.begin(), result.end(), row) == result.end()) {
                result.push_back(row);
            }
            else {
                SDL_ASSERT(0);
            }
        }
    };
    for_row(_sysidxstats, [this, id, push_back](sysidxstats::const_pointer idx) {
        if ((idx->data.id == id) && !idx->data.rowset.is_null()) {
            for_row(_sysallocunits, 
                [idx, push_back](sysallocunits::const_pointer row) {
                    if (row->data.ownerid == idx->data.rowset) {
                        push_back(row);
                    }
            });
        }
    });
    return result; // returns pointers to mapped memory
}

database::pgroot_pgfirst 
database::load_pg_index(schobj_id const id, pageType::type const page_type)
{
    if (auto found = m_data->index.find(id, page_type)) {
        return *found;
    }
    pgroot_pgfirst & result = m_data->index(id, page_type);
    for (auto alloc : this->find_sysalloc(id, dataType::type::IN_ROW_DATA)) {
        A_STATIC_CHECK_TYPE(sysallocunits_row const *, alloc);
        if (alloc->data.pgroot && alloc->data.pgfirst) { // root page of the index tree
            auto const pgroot = load_page_head(alloc->data.pgroot); // load index page
            if (pgroot) {
                auto const pgfirst = load_page_head(alloc->data.pgfirst); // ask for data page
                if (pgfirst && (pgfirst->data.type == page_type)) {
                    SDL_ASSERT(is_allocated(alloc->data.pgroot));
                    SDL_ASSERT(is_allocated(alloc->data.pgfirst));
                    if (pgroot->is_index()) {
                        SDL_ASSERT(pgroot != pgfirst);
                        return result = { pgroot, pgfirst };
                    }
                    if (pgroot->is_data() && (pgroot == pgfirst)) {
                        return result = { pgroot, pgfirst };
                    }
                    SDL_WARNING(0); // to be tested
                }
            }
        }
        else {
            SDL_ASSERT(!alloc->data.pgroot); // expect pgfirst if pgroot exists
        }
    }
    SDL_ASSERT(!result);
    return result;
}

database::page_head_access &
database::find_datapage(schobj_id const id, 
                        dataType::type const data_type,
                        pageType::type const page_type)
{
    using class_clustered_access = page_head_access_t<clustered_access>;
    using class_forward_access   = page_head_access_t<forward_access>;
    using class_heap_access      = page_head_access_t<heap_access>;

    if (auto found = m_data->datapage.find(id, data_type, page_type)) {
        return *(found->get());
    }
    auto & result = m_data->datapage(id, data_type, page_type);

    //TODO: Before we can scan either heaps or indices, we need to know the compression level as that's set at the partition level, and not at the record/page level.
    //TODO: We also need to know whether the partition is using vardecimals.
    if ((data_type == dataType::type::IN_ROW_DATA) && (page_type == pageType::type::data)) {
        if (auto index = get_cluster_index(id)) { // use cluster index if possible
            const index_tree tree(this, index);
            page_head const * const min_page = load_page_head(tree.min_page());
            page_head const * const max_page = load_page_head(tree.max_page());
            if (min_page && max_page) {
                return * reset_shared<class_clustered_access>(result, this, min_page, max_page);
            }
            SDL_ASSERT(0);
        }
        else {
            if (page_head const * p = load_pg_index(id, page_type).pgfirst()) {
                return * reset_shared<class_forward_access>(result, this, p);
            }
        }
    }
    // Heap tables won't have root pages
    vector_page_head heap_pages;
    for (auto alloc : this->find_sysalloc(id, data_type)) {
        A_STATIC_CHECK_TYPE(sysallocunits_row const *, alloc);
        SDL_ASSERT(alloc->data.type == data_type);
        for (auto const & page : iam_access(this, alloc)) {
            A_STATIC_CHECK_TYPE(shared_iam_page const &, page);
            page->allocated_pages(this, [this, page_type, &heap_pages](pageFileID const & id) {
                SDL_ASSERT(id);
                if (auto p = this->load_page_head(id)) {
                    if (p->data.type == page_type) {
                        heap_pages.push_back(p);
                    }
                }
                else {
                    SDL_ASSERT(0);
                }
            });
        }
    }
    if (1) {
        std::sort(heap_pages.begin(), heap_pages.end(), 
            [](page_head const * x, page_head const * y){
            return (x->data.pageId < y->data.pageId);
        });
    }
    return * reset_shared<class_heap_access>(result, this, std::move(heap_pages));
}

bool database::is_allocated(pageFileID const & id)
{
    if (!id.is_null()) {
        if (id.pageId < (uint32)page_count()) { // check range
            if (auto h = load_page_head(pfs_page::pfs_for_page(id))) {
                return pfs_page(h)[id].b.allocated;
            }
        }
        else {
            SDL_ASSERT(0);
        }
    }
    return false;
}

bool database::is_allocated(page_head const * const p)
{
     if (p && !p->is_null()) {
         return is_allocated(p->data.pageId);
     }
     return false;
}

shared_iam_page database::load_iam_page(pageFileID const & id)
{
    if (is_allocated(id)) {
        if (auto p = load_page_head(id)) {
            return std::make_shared<iam_page>(p);
        }
    }
    return {};
}

shared_primary_key
database::get_primary_key(schobj_id const table_id)
{
    {
        auto const found = m_data->primary.find(table_id);
        if (found != m_data->primary.end()) {
            return found->second;
        }
    }
    auto & result = m_data->primary[table_id];
    if (auto const pg = load_pg_index(table_id, pageType::type::data)) {
        sysidxstats_row const * idx = 
            find_if(_sysidxstats, [table_id](sysidxstats::const_pointer p) {
                if ((p->data.id == table_id) && p->data.indid.is_clustered()) {
                    return p->data.status.IsPrimaryKey();
                }
                return false;
            });
        if (!idx) {
            idx = find_if(_sysidxstats, [table_id](sysidxstats::const_pointer p) {
                if ((p->data.id == table_id) && p->data.indid.is_clustered()) {
                    return p->data.status.IsUnique();
                }
                return false;
            });
        }
        if (idx) {
            SDL_ASSERT(idx->data.status.IsPrimaryKey() || idx->data.status.IsUnique());
            SDL_ASSERT(idx->data.type.is_clustered());
            SDL_ASSERT(idx->data.indid.is_clustered());            
            
            std::vector<sysiscols_row const *> idx_stat;
            for_row(_sysiscols, [table_id, idx, &idx_stat](sysiscols::const_pointer stat) {
                if ((stat->data.idmajor == table_id) && (stat->data.idminor == idx->data.indid)) {
                    idx_stat.push_back(stat);
                }
            });
            if (!idx_stat.empty()) {
                SDL_ASSERT(idx_stat.size() < 256); // we use sysiscols_row.tinyprop1 (1 byte) to sort columns
                std::sort(idx_stat.begin(), idx_stat.end(), 
                    [](sysiscols_row const * x, sysiscols_row const * y) {
                        SDL_ASSERT(!(x->data.tinyprop2 || x->data.tinyprop3));
                        SDL_ASSERT(!(y->data.tinyprop2 || y->data.tinyprop3));
                        //return x->data.subid < y->data.subid; not always correct
                        return x->data.tinyprop1 < y->data.tinyprop1;
                });
                primary_key::colpars idx_col;
                primary_key::scalars idx_scal;
                primary_key::orders idx_ord;
                idx_col.reserve(idx_stat.size());
                idx_scal.reserve(idx_stat.size());
                idx_ord.reserve(idx_stat.size());
                for (sysiscols_row const * stat : idx_stat) {
                    SDL_ASSERT(stat->data.status.is_index());
                    if (syscolpars_row const * const col = find_if(_syscolpars, 
                        [table_id, stat](syscolpars::const_pointer p) {
                            return (p->data.id == table_id) && (p->data.colid == stat->data.intprop);
                        }))
                    {
                        const scalartype utype = col->data.utype;
                        if (auto scal = find_if(_sysscalartypes, [utype](sysscalartypes::const_pointer p) {
                            return (p->data.id == utype);
                        })) 
                        {
                            if (usertable::column::is_fixed(col, scal)) {
                                idx_col.push_back(col);
                                idx_scal.push_back(scal);
                                idx_ord.push_back(stat->data.status.index_order());
                            }
                            else { //FIXME: support only fixed columns as key
                                SDL_ASSERT(!result);
                                return result;
                            }
                        }
                        else {
                            SDL_ASSERT(!"_sysscalartypes");
                            return result;
                        }
                    }
                    else {
                        SDL_ASSERT(!"_syscolpars");
                        return result;
                    }
                }
                SDL_ASSERT(idx_col.size() == idx_stat.size());
                if (slot_array::size( pg.pgroot())) {
                    reset_new(result, pg.pgroot(), idx,
                        std::move(idx_col),
                        std::move(idx_scal),
                        std::move(idx_ord),
                        table_id);
                }
                SDL_ASSERT(result);
            }
        }
    }
    return result;
}

shared_cluster_index
database::get_cluster_index(shared_usertable const & schema)
{
    if (!schema) {
        SDL_ASSERT(0);
        return{};
    }
    {
        auto const found = m_data->cluster.find(schema->get_id());
        if (found != m_data->cluster.end()) {
            return found->second;
        }
    }
    auto & result = m_data->cluster[schema->get_id()];
    if (auto p = get_primary_key(schema->get_id())) {
        if (p->is_index()) {
            cluster_index::column_index pos(p->size());
            for (size_t i = 0; i < p->size(); ++i) {
                const auto col = schema->find_col(p->colpar[i]);
                if (col.first) {
                    pos[i] = col.second;
                }
                else {
                    SDL_ASSERT(0);
                    return nullptr;
                }
            }
            SDL_ASSERT(pos.size() == p->colpar.size());
            reset_new(result, p, schema, std::move(pos));
        }
        SDL_ASSERT(result);
    }
    return result;
}

shared_cluster_index
database::get_cluster_index(schobj_id const id) 
{
    for (auto & p : _usertables) {
        if (p->get_id() == id) {
            return get_cluster_index(p);
        }
    }
    return{};
}

page_head const *
database::get_cluster_root(schobj_id const id)
{
    if (auto p = get_cluster_index(id)) {
        return p->root();
    }
    return nullptr;
}

vector_mem_range_t
database::var_data(row_head const * const row, size_t const i, scalartype::type const col_type)
{
    if (row->has_variable()) {
        const variable_array data(row);
        if (i >= data.size()) {
            SDL_ASSERT(!"wrong var_offset");
            return{};
        }
        const mem_range_t m = data.var_data(i);
        const size_t len = mem_size(m);
        if (!len) {
            SDL_ASSERT(!"wrong var_data");
            return{};
        }
        if (data.is_complex(i)) {
            // If length == 16 then we're dealing with a LOB pointer, otherwise it's a regular complex column
            if (len == sizeof(text_pointer)) { // 16 bytes
                auto const tp = reinterpret_cast<text_pointer const *>(m.first);
                if ((col_type == scalartype::t_text) || 
                    (col_type == scalartype::t_ntext)) {
                    return text_pointer_data(this, tp).detach();
                }
            }
            else {
                const auto type = data.var_complextype(i);
                SDL_ASSERT(type != complextype::none);
                if (type == complextype::row_overflow) {
                    if (len == sizeof(overflow_page)) { // 24 bytes
                        auto const overflow = reinterpret_cast<overflow_page const *>(m.first);
                        SDL_ASSERT(overflow->type == type);
                        if (col_type == scalartype::t_varchar) {
                            return varchar_overflow_page(this, overflow).detach();
                        }
                    }
                }
                else if (type == complextype::blob_inline_root) {
                    if (len == sizeof(overflow_page)) { // 24 bytes
                        auto const overflow = reinterpret_cast<overflow_page const *>(m.first);
                        SDL_ASSERT(overflow->type == type);
                        if (col_type == scalartype::t_geography) {
                            varchar_overflow_page varchar(this, overflow);
                            SDL_ASSERT(varchar.length() == overflow->length);
                            return varchar.detach();
                        }
                    }
                    if (len > sizeof(overflow_page)) { // 24 bytes + 12 bytes * link_count 
                        SDL_ASSERT(!((len - sizeof(overflow_page)) % sizeof(overflow_link)));
                        if (col_type == scalartype::t_geography) {
                            auto const page = reinterpret_cast<overflow_page const *>(m.first);
                            size_t const link_count = (len - sizeof(overflow_page)) / sizeof(overflow_link);
                            auto const link = reinterpret_cast<overflow_link const *>(page + 1);
                            varchar_overflow_page varchar(this, page);
                            SDL_ASSERT(varchar.length() == page->length);
                            for (size_t i = 0; i < link_count; ++i) {
                                const varchar_overflow_link next(this, page, link + i);
                                append(varchar.data(), next.begin(), next.end());
                                SDL_ASSERT(mem_size_n(varchar.data()) == link[i].size);
                            }
                            return varchar.detach();
                        }
                    }
                }
            }
            if (col_type == scalartype::t_varbinary) {
                return { m };
            }
            SDL_ASSERT(!"unknown data type");
            return { m };
        }
        else { // in-row-data
            return { m };
        }
    }
    SDL_ASSERT(0);
    return {};
}

geography_t database::get_geography(row_head const * const row, size_t const i)
{
    return this->var_data(row, i, scalartype::t_geography);
}

vector_sysidxstats_row
database::index_for_table(schobj_id const id)
{
    using T = vector_sysidxstats_row;
    T result;
    for_row(_sysidxstats, [this, id, &result](sysidxstats::const_pointer idx) {
        if ((idx->data.id == id) && idx->data.indid.is_index()) {
            switch (idx->data.type) {
            case idxtype::clustered:
            case idxtype::nonclustered:
            case idxtype::spatial:
                result.push_back(idx);
                break;
            default:
                break; // _WA_Sys_00000002_182C9B23 (used for statistics)
            }
        }
    });
    std::sort(result.begin(), result.end(),
        [](T::value_type const & x, T::value_type const & y){
        return x->data.indid < y->data.indid;
    });  
    return result;
}

sysidxstats_row const * database::find_spatial_type(const std::string & index_name, idxtype::type const type)
{
    SDL_ASSERT(!index_name.empty());
    sysidxstats_row const * const idx = 
    find_if(_sysidxstats, [this, &index_name, type](sysidxstats::const_pointer idx) {
        if ((idx->data.type == type) && (idx->name() == index_name)) {
            SDL_ASSERT_1((idx->data.indid._32 == 1) == (type == idxtype::clustered));
            SDL_ASSERT_1((idx->data.indid._32 == 384000) == (type == idxtype::spatial));
            return true;
        }
        return false;
    });
    return idx;
}

sysallocunits_row const *
database::find_spatial_alloc(const std::string & index_name)
{
    if (!index_name.empty()) {
        if (auto const idx = find_spatial_type(index_name, idxtype::clustered)) {
            auto const & alloc = find_sysalloc(idx->data.id, dataType::type::IN_ROW_DATA);
            if (!alloc.empty()) {
                SDL_ASSERT(alloc.size() == 1);
                SDL_ASSERT(alloc[0] != nullptr);
                return alloc[0];
            }
        }
    }
    SDL_ASSERT(0);
    return nullptr;
}

sysidxstats_row const * database::find_spatial_idx(schobj_id const table_id)
{
    sysidxstats_row const * const idx = 
    find_if(_sysidxstats, [this, table_id](sysidxstats::const_pointer idx) {
        if ((idx->data.type == idxtype::spatial) && (idx->data.id == table_id)) {
            return true;
        }
        return false;
    });
    if (idx) {
        SDL_ASSERT(idx->data.id == table_id);
        return idx;
    }
    return nullptr;
}

database::spatial_root
database::find_spatial_root(schobj_id const table_id)
{
    if (sysidxstats_row const * const idx = find_spatial_idx(table_id)) {
        SDL_ASSERT(idx->is_spatial());
        //FIXME: use id instead of idx->name() ? found duplicated name SPATIAL_LINK
        if (sysallocunits_row const * const root = find_spatial_alloc(idx->name())) {
            return { root, idx };
        }
    }
    return{};
}

spatial_tree_idx
database::find_spatial_tree(schobj_id const table_id) {
    {
        auto const found = m_data->spatial_tree.find(table_id);
        if (found != m_data->spatial_tree.end()) {
            return found->second;
        }
    }
    auto & result = m_data->spatial_tree[table_id];
    auto const sroot = find_spatial_root(table_id);
    if (sroot.first) {
        SDL_ASSERT(sroot.second);
        A_STATIC_CHECK_TYPE(sysidxstats_row const *, sroot.second);
        sysallocunits_row const * const root = sroot.first;
        if (root->data.pgroot && root->data.pgfirst) {
            SDL_ASSERT(get_pageType(root->data.pgroot) == pageType::type::index);
            SDL_ASSERT(get_pageType(root->data.pgfirst) == pageType::type::data);
            if (auto const pk0 = get_primary_key(table_id)) {
                if (auto const pgroot = load_page_head(root->data.pgroot)) {
                    SDL_ASSERT(1 == pk0->size()); // to be tested
                    //SDL_ASSERT(pk0->first_type() == scalartype::t_bigint);  
                    result.pgroot = pgroot;
                    result.idx = sroot.second;
                    return result;
                }
            }
        }
        SDL_WARNING(0); //FIXME: SDL_ASSERT(0);
    }
    return result;
}

//----------------------------------------------------------
// database_fwd.h
page_head const * fwd::load_page_head(database * d, pageFileID const & it) {
    return d->load_page_head(it);
}
page_head const * fwd::load_next_head(database * d, page_head const * p) {
    return d->load_next_head(p);
}
page_head const * fwd::load_prev_head(database * d,page_head const * p) {
    return d->load_prev_head(p);
}
recordID fwd::load_next_record(database * d, recordID const & it) {
    return d->load_next_record(it);
}
recordID fwd::load_prev_record(database * d, recordID const & it) {
    return d->load_prev_record(it);
}
pageFileID fwd::nextPageID(database * d, pageFileID const & it) {
    return d->nextPageID(it);
}
pageFileID fwd::prevPageID(database * d, pageFileID const & it) {
    return d->prevPageID(it);
}
//----------------------------------------------------------

} // db
} // sdl

#if 0
SQL Server tracks the pages and extents used by the different types of pages (in-row, row-overflow, and LOB pages), 
that belong to the object with another set of the allocation map pages, called Index Allocation Map (IAM).
Every table/index has its own set of IAM pages, which are combined into separate linked lists called IAM chains.
Each IAM chain covers its own allocation unit—IN_ROW_DATA, ROW_OVERFLOW_DATA, and LOB_DATA.
Each IAM page in the chain covers a particular GAM interval and represents the bitmap where each bit indicates
if a corresponding extent stores the data that belongs to a particular allocation unit for a particular object. 
In addition, the first IAM page for the object stores the actual page addresses for the first eight object pages, 
which are stored in mixed extents.
------------------------------------------------------
PFS_page
The first PFS page is at *:1 in each database file, 
and it stores the statistics for pages 0-8087 in that database file. 
There will be one PFS page for just about every 64MB of file size (8088 bytes * 8KB per page).
A large database file will use a long chain of PFS pages, linked together using the LastPage and NextPage pointers in the 96 byte header. 
------------------------------------------------------
linked list of pages using the PrevPage, ThisPage, and NextPage page locators, when one page is not enough to hold all the data.
------------------------------------------------------
#endif

#if 0 //SDL_DEBUG
namespace sdl {
    namespace db {
        namespace {
            class unit_test {
            public:
                unit_test()
                {
                    SDL_TRACE_FILE;
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG
