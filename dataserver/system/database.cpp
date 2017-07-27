// database.cpp
//
#include "dataserver/system/database.h"
#include "dataserver/system/overflow.h"
#include "dataserver/system/database_fwd.h"
#include "dataserver/system/database_impl.h"

namespace sdl { namespace db {

#if SDL_USE_BPOOL
database::scoped_thread_lock::scoped_thread_lock(database const & db, const bool f)
    : m_db(db), remove_id(f)
#if SDL_DEBUG
    , m_id(std::this_thread::get_id())
#endif
{
}

database::scoped_thread_lock::~scoped_thread_lock() {
    SDL_ASSERT(std::this_thread::get_id() == m_id);
    m_db.unlock_thread(remove_id);
}
#endif // SDL_USE_BPOOL

database::database(const std::string & fname)
    : m_data(std::make_unique<shared_data>(fname))
{
    init_database();
}

database::~database()
{
}

void database::init_database()
{
    SDL_TRACE_FUNCTION;
    
    _usertables.init(get_usertables());
    _internals.init(get_internals());
    _datatables.init(get_datatables());

    for (auto const & ut : _usertables) {
        init_datatable(ut);
    }
    for (auto const & ut : _internals) {
        init_datatable(ut);
    }
    m_data->initialized = true;
}

void database::init_datatable(shared_usertable const & schema)
{
    SDL_ASSERT(!m_data->initialized);
    schobj_id const id = schema->get_id();
    for_dataType([this, id](dataType::type const t){
        this->find_sysalloc(id, t);
    });
    for_pageType([this, id](pageType::type const t){
        this->load_pg_index(id, t);
    });
    get_primary_key(id);
    get_cluster_index(schema);
    find_spatial_tree(id);
}

const std::string & database::filename() const
{
    return m_data->filename;
}

bool database::is_open() const
{
    return m_data->const_pool().is_open();
}

void const * database::start_address() const // diagnostic
{
    return m_data->const_pool().start_address();
}

void const * database::memory_offset(void const * p) const // diagnostic
{
    char const * p1 = (char const *)this->start_address();
    char const * p2 = (char const *)p;
    SDL_ASSERT(p2 >= p1);
    void const * offset = reinterpret_cast<void const *>(p2 - p1);    
    return offset;
}

std::string database::dbi_dbname() const
{
    if (auto boot = get_bootpage()) {
        return to_string::trim(to_string::type(boot->row->data.dbi_dbname));
    }
    SDL_ASSERT(0);
    return{};
}

size_t database::page_count() const
{
    return m_data->const_pool().page_count();
}

size_t database::page_allocated() const
{
    size_t allocated = 0;
    size_t count = page_count();
    pageFileID id = pageFileID::init(0);
    while (count--) {
        SDL_ASSERT(id.pageId < (uint32)page_count());
        if (auto h = load_page_head(pfs_page::pfs_for_page(id))) {
            if (pfs_page(h)[id].b.allocated) {
                ++allocated;
            }
        }
        ++(id.pageId);
    }
    return allocated;
}

break_or_continue
database::scan_checksum(checksum_fun fun) const
{
    pageFileID id = pageFileID::init(0);
    size_t count = page_count();
    while (count--) {
        if (is_allocated(id)) {
            if (page_head const * const p = load_page_head(id)) {
                if (p->data.tornBits) {
                    if (page_head::checksum(p) != p->data.tornBits) {
                        if (!fun(p)) {
                            return break_or_continue::break_;
                        }
                    }
                }
                else {
                    SDL_TRACE_WARNING("empty tornBits ", to_string::type(id));
                }
            }
            else {
                throw_error<database_error>("cannot load page");
                return break_or_continue::break_;
            }
        }
        ++(id.pageId);
    }
    return break_or_continue::continue_;
}

break_or_continue 
database::scan_checksum() const {
    return scan_checksum([](page_head const * const){
        SDL_ASSERT(!"scan_checksum");
        return false;
    });
}

#if SDL_USE_BPOOL

size_t database::unlock_thread(std::thread::id const id, bool const remove_id) const {
    return m_data->pool().unlock_thread(id, remove_id);
}

size_t database::unlock_thread(bool const remove_id) const {
    return m_data->pool().unlock_thread(remove_id);
}

bool database::unlock_page(pageIndex const pageId) const {
    return m_data->pool().unlock_page(pageId);
}

bool database::unlock_page(pageFileID const & id) const {
    if (id) {
        return m_data->pool().unlock_page(id.pageId);
    }
    return false;
}

bool database::unlock_page(page_head const * const p) const {
    if (p) {
        return m_data->pool().unlock_page(p->data.pageId.pageId);
    }
    return false;
}

bpool::lock_page_head
database::auto_lock_page(pageIndex const i) const {
    return m_data->pool().auto_lock_page(i);
}

bpool::lock_page_head
database::auto_lock_page(pageFileID const & id) const {
    if (id) {
        return m_data->pool().auto_lock_page(id.pageId);
    }
    return{};
}

#endif // SDL_USE_BPOOL

page_head const *
database::load_page_head(pageIndex const i) const
{
    return m_data->pool().lock_page(i);
}

page_head const *
database::load_page_head(pageFileID const & id) const
{
    if (id) {
        return m_data->pool().lock_page(id.pageId);
    }
    return nullptr;
}

database::page_row
database::load_page_row(recordID const & row) const
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
database::load_page_list(page_head const * p) const
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

pageType database::get_pageType(pageFileID const & id) const
{
    if (auto p = load_page_head(id)) {
        SDL_ASSERT(is_allocated(p));
        return p->data.type;
    }
    return pageType::init(pageType::type::null);
}

bool database::is_pageType(pageFileID const & id, pageType::type const t) const
{
    SDL_ASSERT(t != pageType::type::null);
    if (auto p = load_page_head(id)) {
        return p->data.type == t;
    }
    SDL_ASSERT(0);
    return false;
}

pageFileID database::nextPageID(pageFileID const & id) const // diagnostic
{
    if (auto p = load_page_head(id)) {
        return p->data.nextPage;
    }
    SDL_ASSERT(id.is_null());
    return {};
}

pageFileID database::prevPageID(pageFileID const & id) const // diagnostic
{
    if (auto p = load_page_head(id)) {
        return p->data.prevPage;
    }
    SDL_ASSERT(id.is_null());
    return {};
}

database::page_ptr<bootpage>
database::get_bootpage() const
{
    page_head const * const h = load_page_head(sysPage::boot_page);
    if (h) {
        return std::make_unique<bootpage>(h, cast::page_body<bootpage_row>(h));
    }
    return {};
}

database::page_ptr<datapage>
database::get_datapage(pageIndex const i) const
{
    page_head const * const h = load_page_head(i);
    if (h) {
        return std::make_unique<datapage>(h);
    }
    return {};
}

database::page_ptr<fileheader>
database::get_fileheader() const
{
    page_head const * const h = load_page_head(0);
    if (h) {
        return std::make_unique<fileheader>(h);
    }
    return {};
}

page_head const * database::sysallocunits_head() const
{
    auto boot = get_bootpage();
    if (boot) {
        auto & id = boot->row->data.dbi_firstSysIndexes;
        page_head const * const h = this->load_page_head(id); // m_data->pm.load_page(id);
        SDL_ASSERT(h->is_data());
        return h;
    }
    SDL_ASSERT(0);
    return nullptr;
}

database::page_ptr<sysallocunits>
database::get_sysallocunits() const
{
    if (auto p = sysallocunits_head()) {
        return std::make_unique<sysallocunits>(p);
    }
    SDL_ASSERT(0);
    return {};
}

database::page_ptr<pfs_page>
database::get_pfs_page() const
{
    page_head const * const h = load_page_head(sysPage::PFS);
    if (h) {
        return std::make_unique<pfs_page>(h);
    }
    SDL_ASSERT(0);
    return {};
}

page_head const *
database::load_sys_obj(const sysObj id) const 
{
    if (auto const h = sysallocunits_head()) {
#if SDL_DEBUG
        if (0) {
            const uint32 auid_id = static_cast<uint32>(id);
            sysallocunits(h).scan_auid(auid_id,
                [auid_id](sysallocunits::const_pointer p){
                SDL_ASSERT(p->data.auid.d.id == auid_id);
                return bc::continue_;
            });
        }
#endif
        if (auto row = sysallocunits(h).find_auid(static_cast<uint32>(id))) {
            return load_page_head(row->data.pgfirst);
        }
    }
    SDL_ASSERT(0);
    return nullptr;
}

//---------------------------------------------------------

page_head const * database::load_next_head(page_head const * const p) const
{
    if (p) {
        auto next = this->load_page_head(p->data.nextPage);
        SDL_ASSERT(!next || (next->data.type == p->data.type));
        return next;
    }
    SDL_ASSERT(0);
    return nullptr;
}

page_head const * database::load_prev_head(page_head const * const p) const
{
    if (p) {
        auto prev = this->load_page_head(p->data.prevPage);
        SDL_ASSERT(!prev || (prev->data.type == p->data.type));
        return prev;
    }
    SDL_ASSERT(0);
    return nullptr;
}

page_head const * database::load_last_head(page_head const * p) const
{
    page_head const * next;
    while ((next = load_next_head(p)) != nullptr) {
        p = next;
    }
    return p;
}

page_head const * database::load_first_head(page_head const * p) const
{
    page_head const * prev;
    while ((prev = load_prev_head(p)) != nullptr) {
        p = prev;
    }
    return p;
}

recordID database::load_next_record(recordID const & r) const
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

recordID database::load_prev_record(recordID const & r) const
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
unique_datatable database::find_table_if(fun_type && fun) const
{
    for (auto const & p : _usertables) {
        const usertable & d = *p.get();
        if (fun(d)) {
            return std::make_unique<datatable>(this, p);
        }
    }
    return {};
}

template<class fun_type>
unique_datatable database::find_internal_if(fun_type && fun) const
{
    for (auto const & p : _internals) {
        const usertable & d = *p.get();
        if (fun(d)) {
            return std::make_unique<datatable>(this, p);
        }
    }
    return {};
}

unique_datatable database::find_table(const std::string & name) const
{
    SDL_ASSERT(!name.empty());
    return find_table_if([&name](const usertable & d) {
        return d.name() == name;
    });
}

unique_datatable database::find_table(schobj_id const id) const
{
    return find_table_if([id](const usertable & d) {
        return d.get_id() == id;
    });
}

unique_datatable database::find_internal(const std::string & name) const
{
    SDL_ASSERT(!name.empty());
    return find_internal_if([&name](const usertable & d) {
        return d.name() == name;
    });
}

unique_datatable database::find_internal(schobj_id const id) const
{
    return find_internal_if([id](const usertable & d) {
        return d.get_id() == id;
    });
}

shared_usertable database::find_table_schema(schobj_id const id) const
{
    for (auto const & p : _usertables) {
        if (p->get_id() == id) {
            return p;
        }
    }
    throw_error<database_error>("cannot find table schema");
    return {};
}

shared_usertable database::find_internal_schema(schobj_id const id) const
{
    for (auto const & p : _internals) {
        if (p->get_id() == id) {
            return p;
        }
    }
    throw_error<database_error>("cannot find internal schema");
    return {};
}

template<class fun_type>
void database::for_USER_TABLE(fun_type const & fun) const {
    for_row(_sysschobjs, [&fun](sysschobjs::const_pointer row){
        if (row->is_USER_TABLE()) {
            fun(row);
        }
    });
}

template<class fun_type>
void database::for_INTERNAL_TABLE(fun_type const & fun) const {
    for_row(_sysschobjs, [&fun](sysschobjs::const_pointer row){
        if (row->is_INTERNAL_TABLE()) {
            fun(row);
        }
    });
}

template<class fun_type>
void database::get_tables(vector_shared_usertable & m_ut, fun_type const & is_table) const
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

database::shared_usertables
database::get_usertables() const
{
    if (!m_data->empty_usertable()) {
        return m_data->usertable();
    }
    SDL_ASSERT(!m_data->initialized);
    shared_usertables ut(new vector_shared_usertable);
    get_tables(*ut, [](sysschobjs::const_pointer row){
        return row->is_USER_TABLE();
    });
    m_data->usertable() = ut;
    return ut;
}

database::shared_usertables
database::get_internals() const
{
    if (!m_data->empty_internal()) {
        return m_data->internal();
    }
    SDL_ASSERT(!m_data->initialized);
    shared_usertables ut(new vector_shared_usertable);
    get_tables(*ut, [](sysschobjs::const_pointer row){
        return row->is_INTERNAL_TABLE();
    });
    m_data->internal() = ut;
    return ut;
}

database::shared_datatables
database::get_datatables() const
{
    if (!m_data->empty_datatable()) {
        return m_data->datatable();
    }
    SDL_ASSERT(!m_data->initialized);
    auto const ut = this->get_usertables();
    shared_datatables dt(new vector_shared_datatable);
    dt->reserve(ut->size());

    for (auto & p : *ut) {
        dt->push_back(std::make_shared<datatable>(this, p));
    }
    using table_type = vector_shared_datatable::value_type;
    std::sort(dt->begin(), dt->end(),
        [](table_type const & x, table_type const & y){
        return x->name() < y->name();
    });

    m_data->datatable() = dt;
    return dt;
}

database::shared_sysallocunits
database::find_sysalloc(schobj_id const id, dataType::type const data_type) const // FIXME: scanPartition ?
{
    {
        auto const found = m_data->find_sysalloc(id, data_type);
        if (found.second) {
            return found.first;
        }
    }
    shared_sysallocunits shared_result(new vector_sysallocunits_row);
    auto & result = *shared_result;
    for_row(_sysidxstats, [this, id, data_type, &result](sysidxstats::const_pointer idx) {
        if ((idx->data.id == id) && !idx->data.rowset.is_null()) {
            for_row(_sysallocunits, 
                [this, idx, data_type, &result](sysallocunits::const_pointer row) {
                    if (row->data.pgfirstiam) {
                        if ((row->data.ownerid == idx->data.rowset) && (row->data.type == data_type)) {
                            if (!algo::is_find(result, row)) {
                                result.push_back(row);
                            }
                            else {
                                SDL_ASSERT(!"push unique"); // to be tested
                            }
                        }
                    }
                    else {
                        //SDL_TRACE("\nfind_sysalloc id=", idx->data.id._32, " name=", col_name_t(idx));
                    }
            });
        }
    });
    m_data->set_sysalloc(id, data_type, shared_result);
    return shared_result;
}

database::pgroot_pgfirst 
database::load_pg_index(schobj_id const id, pageType::type const page_type) const
{
    {
        auto const found = m_data->load_pg_index(id, page_type);
        if (found.second) {
            return found.first;
        }
    }
    pgroot_pgfirst result{};
    auto const & sysalloc = find_sysalloc(id, dataType::type::IN_ROW_DATA);
    for (auto const alloc : *sysalloc) {
        A_STATIC_CHECK_TYPE(sysallocunits_row const * const, alloc);
        SDL_ASSERT(alloc->data.pgfirstiam);
        if (alloc->data.pgroot && alloc->data.pgfirst) { // root page of the index tree
            if (is_allocated(alloc->data.pgroot) && is_allocated(alloc->data.pgfirst)) { // it is possible that pgfirst is not allocated (and not empty)
                auto const pgroot = load_page_head(alloc->data.pgroot); // load index page
                if (pgroot) {
                    auto const pgfirst = load_page_head(alloc->data.pgfirst); // ask for data page
                    SDL_ASSERT(pgfirst);
                    if (pgfirst && (pgfirst->data.type == page_type)) {
                        if (pgroot->is_index()) {
                            //SDL_ASSERT(pgroot != pgfirst); it is possible (TSQL2012, dbo_Categories)
                            result = { pgroot, pgfirst };
                            break;
                        }
                        //if (pgroot->is_data() && (pgroot == pgfirst)) { address can change if page_bpool::free_unlock_blocks tested 
                        if (pgroot->is_data() && (alloc->data.pgroot == alloc->data.pgfirst)) {
                            result = { pgroot, pgfirst };
                            break;
                        }
                        SDL_ASSERT(!"load_pg_index");
                    }
                }
            }
        }
    }
    m_data->set_pg_index(id, page_type, result);
    return result;
}

database::shared_page_head_access
database::find_datapage(schobj_id const id, 
                        dataType::type const data_type,
                        pageType::type const page_type) const
{
    using class_clustered_access = page_head_access_t<clustered_access>;
    using class_forward_access   = page_head_access_t<forward_access>;
    using class_heap_access      = page_head_access_t<heap_access>;
    {
        auto const found = m_data->find_datapage(id, data_type, page_type);
        if (found.second) {
            return found.first;
        }
    }
    shared_page_head_access result;
    //TODO: Before we can scan either heaps or indices, we need to know the compression level as that's set at the partition level, and not at the record/page level.
    //TODO: We also need to know whether the partition is using vardecimals.
    if ((data_type == dataType::type::IN_ROW_DATA) && (page_type == pageType::type::data)) {
        if (auto const index = get_cluster_index(id)) { // use cluster index if possible
            if (index->is_root_index()) {
                const index_tree tree(this, index);
                page_head const * const min_page = load_page_head(tree.min_page());
                page_head const * const max_page = load_page_head(tree.max_page());
                if (min_page && max_page) {
                    reset_shared<class_clustered_access>(result, this, min_page, max_page);
                    m_data->set_datapage(id, data_type, page_type, result);
                    return result;
                }
                SDL_ASSERT(0);
            }
            else {
                SDL_ASSERT(index->is_root_data());
                if (page_head const * p = load_pg_index(id, page_type).pgfirst()) {
                    reset_shared<class_forward_access>(result, this, p);
                    m_data->set_datapage(id, data_type, page_type, result);
                    return result;
                }
            }
        }
    }
    // Heap tables won't have root pages
    vector_page_head heap_pages;
    vector_sysallocunits_row const & sysalloc = *find_sysalloc(id, data_type);
    for (auto alloc : sysalloc) {
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
    reset_shared<class_heap_access>(result, this, std::move(heap_pages));
    m_data->set_datapage(id, data_type, page_type, result);
    return result;
}

bool database::is_allocated(pageFileID const & id) const
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

bool database::is_allocated(page_head const * const p) const
{
     if (p && !p->is_null()) {
         return is_allocated(p->data.pageId);
     }
     return false;
}

shared_iam_page database::load_iam_page(pageFileID const & id) const
{
    if (is_allocated(id)) {
        if (auto p = load_page_head(id)) {
            return std::make_shared<iam_page>(p);
        }
    }
    return {};
}

shared_primary_key
database::make_primary_key(schobj_id const table_id) const
{
    shared_primary_key result;
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
                                return {};
                            }
                        }
                        else {
                            SDL_ASSERT(!"_sysscalartypes");
                            return {};
                        }
                    }
                    else {
                        SDL_ASSERT(!"_syscolpars");
                        return {};
                    }
                }
                SDL_ASSERT(idx_col.size() == idx_stat.size());
                if (slot_array::size(pg.pgroot())) {
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

shared_primary_key
database::get_primary_key(schobj_id const table_id) const
{
    {
        auto const found = m_data->get_primary_key(table_id);
        if (found.second) {
            return found.first;
        }
    }
    if (shared_primary_key result = make_primary_key(table_id)) {
        sysidxstats_row const * const idxstat = result->idxstat;
        SDL_ASSERT(idxstat->is_clustered());
        SDL_ASSERT(idxstat->IsUnique());
        SDL_WARNING(idxstat->IsPrimaryKey()); // possible case
        if (idxstat->is_clustered() && 
            idxstat->IsUnique() && 
            idxstat->IsPrimaryKey()) {
            m_data->set_primary_key(table_id, result);
            return result;
        }
    }
    m_data->set_primary_key(table_id, shared_primary_key());
    return {};
}

shared_cluster_index
database::get_cluster_index(shared_usertable const & schema) const
{
    if (!schema) {
        SDL_ASSERT(0);
        return{};
    }
    schobj_id const schema_id = schema->get_id();
    {
        auto const found = m_data->get_cluster_index(schema_id);
        if (found.second) {
            return found.first;
        }
    }
    shared_cluster_index result;
    if (auto p = get_primary_key(schema_id)) {
        SDL_ASSERT(p->idxstat->is_clustered());
        SDL_ASSERT(p->idxstat->IsUnique());
        SDL_ASSERT(p->idxstat->IsPrimaryKey());
        if (p->is_index() || p->is_data()) { // is_data() if not enough records for index tree (TSQL2012, dbo_Categories) 
            cluster_index::column_index pos(p->size());
            for (size_t i = 0; i < p->size(); ++i) {
                const auto & col = schema->find_col(p->colpar[i]);
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
    m_data->set_cluster_index(schema_id, result);
    return result;
}

shared_cluster_index
database::get_cluster_index(schobj_id const id) const  
{
    for (auto & p : _usertables) {
        if (p->get_id() == id) {
            return get_cluster_index(p);
        }
    }
    return{};
}

page_head const *
database::get_cluster_root(schobj_id const id) const
{
    if (auto p = get_cluster_index(id)) {
        return p->root();
    }
    return nullptr;
}

vector_mem_range_t
database::var_data(row_head const * const row, size_t const i, scalartype::type const col_type) const
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
            return{}; // possible case (sdlSQL.mdf)
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
                            auto const link = reinterpret_cast<overflow_link const *>(page + 1);
                            size_t const link_count = (len - sizeof(overflow_page)) / sizeof(overflow_link);
                            varchar_overflow_page varchar(this, page);
                            SDL_ASSERT(varchar.length() == page->length);
                            auto total_length = page->length;
                            for (size_t i = 0; i < link_count; ++i) {
                                auto const nextlink = link + i;
                                varchar_overflow_link next(this, page, nextlink);
                                SDL_ASSERT(next.length() == (nextlink->size - total_length));
                                append(varchar.data(), next.detach());
                                SDL_ASSERT(total_length < nextlink->size);
                                total_length = nextlink->size;
                                SDL_ASSERT(mem_size_n(varchar.data()) == total_length);
                            }
                            throw_error_if_not<database_error>(
                                mem_size_n(varchar.data()) == total_length,
                                "bad varchar_overflow_page"); //FIXME: to be tested
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

geo_mem database::get_geography(row_head const * const row, size_t const i) const
{
    return geo_mem(this->var_data(row, i, scalartype::t_geography));
}

vector_sysidxstats_row
database::index_for_table(schobj_id const id) const
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

sysidxstats_row const * database::find_spatial_type(const std::string & index_name, idxtype::type const type) const
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
database::find_spatial_alloc(const std::string & index_name) const
{
    if (!index_name.empty()) {
        if (auto const idx = find_spatial_type(index_name, idxtype::clustered)) {
            auto const & palloc = find_sysalloc(idx->data.id, dataType::type::IN_ROW_DATA);
            auto const & alloc = *palloc;
            if (!alloc.empty()) {
                SDL_ASSERT(alloc.size() == 1);
                SDL_ASSERT(alloc[0] != nullptr);
                return alloc[0];
            }
            SDL_TRACE_WARNING("find_spatial_alloc[", index_name, "] empty");
            return nullptr; // spatial index is not allocated (table is empty ?)
        }
    }
    SDL_ASSERT(0);
    return nullptr;
}

sysidxstats_row const * database::find_spatial_idx(schobj_id const table_id) const
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
database::find_spatial_root(schobj_id const table_id) const
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
database::find_spatial_tree(schobj_id const table_id) const
{
    {
        auto const found = m_data->find_spatial_tree(table_id);
        if (found.second) {
            return found.first;
        }
    }
    spatial_tree_idx result{};
    auto const sroot = find_spatial_root(table_id);
    if (sroot.first) {
        SDL_ASSERT(sroot.second);
        A_STATIC_CHECK_TYPE(sysidxstats_row const *, sroot.second);
        sysallocunits_row const * const root = sroot.first;
        if (root->data.pgroot && root->data.pgfirst) {
            SDL_ASSERT(is_allocated(root->data.pgroot));
            SDL_ASSERT(is_allocated(root->data.pgfirst));
            SDL_ASSERT(get_pageType(root->data.pgroot) == pageType::type::index);
            SDL_ASSERT(get_pageType(root->data.pgfirst) == pageType::type::data);
            if (auto const pk0 = get_primary_key(table_id)) {
                if (1 == pk0->size()) {
                    if (pk0->is_index()) {
                        if (auto const pgroot = load_page_head(root->data.pgroot)) {
                            result.pgroot = pgroot;
                            result.idx = sroot.second;
                            m_data->set_spatial_tree(table_id, result);
                            return result;
                        }
                    }
                    else {
                        SDL_ASSERT(pk0->is_data()); // possible for small tables
                    }
                }
                else {
                    SDL_ASSERT(pk0->size() > 1);
                    SDL_TRACE("\nspatial_tree not implemented for composite pk0, id = ", table_id._32);
                    SDL_WARNING(0);
                }
                return {};
            }
        }
        SDL_ASSERT(!"find_spatial_tree"); // to be tested
    }
    return {};
}

//----------------------------------------------------------
// database_fwd.h
page_head const * fwd::load_page_head(database const * d, pageFileID const & it) {
    return d->load_page_head(it);
}
page_head const * fwd::load_next_head(database const * d, page_head const * p) {
    return d->load_next_head(p);
}
page_head const * fwd::load_prev_head(database const * d,page_head const * p) {
    return d->load_prev_head(p);
}
recordID fwd::load_next_record(database const * d, recordID const & it) {
    return d->load_next_record(it);
}
recordID fwd::load_prev_record(database const * d, recordID const & it) {
    return d->load_prev_record(it);
}
pageFileID fwd::nextPageID(database const * d, pageFileID const & it) {
    return d->nextPageID(it);
}
pageFileID fwd::prevPageID(database const * d, pageFileID const & it) {
    return d->prevPageID(it);
}
bool fwd::is_allocated(database const * d, pageFileID const & it) {
    return d->is_allocated(it);
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
#endif //#if SDL_DEBUG
