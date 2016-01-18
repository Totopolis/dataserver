// database.cpp
//
#include "common/common.h"
#include "database.h"
#include "file_map.h"

#include <algorithm>
#include <sstream>

namespace sdl { namespace db {

class database::data_t : noncopyable
{
    enum { page_size = page_head::page_size };
public:
    const std::string filename;
    explicit data_t(const std::string & fname);

    bool is_open() const
    {
        return m_fmap.IsFileMapped();
    }
    void const * start_address() const
    {
        return m_fmap.GetFileView();
    }
    size_t page_count() const
    {   
        return m_pageCount;
    }
    page_head const * load_page(pageIndex) const;
    page_head const * load_page(pageFileID const &) const;
private:
    size_t m_pageCount;
    FileMapping m_fmap;
};

database::data_t::data_t(const std::string & fname)
    : filename(fname)
    , m_pageCount(0)
{
    static_assert(page_size == 8 * 1024, "");
    if (m_fmap.CreateMapView(fname.c_str())) {
        uint64 sz = m_fmap.GetFileSize();
        uint64 pp = sz / page_size;
        SDL_ASSERT(!(sz % page_size));
        SDL_ASSERT(pp < size_t(-1));
        m_pageCount = static_cast<size_t>(pp);
    }
    else {
        SDL_WARNING(false);
    }
}

page_head const *
database::data_t::load_page(pageIndex const i) const
{
    const size_t pageIndex = i.value();
    if (pageIndex < m_pageCount) {
        const char * const data = static_cast<const char *>(m_fmap.GetFileView());
        const char * p = data + pageIndex * page_size;
        return reinterpret_cast<page_head const *>(p);
    }
    SDL_TRACE_4("\n*** load_page failed: ", pageIndex, " of ", m_pageCount);
    //FIXME: SDL_ASSERT(0);
    SDL_WARNING(0);
    return nullptr;
}

page_head const *
database::data_t::load_page(pageFileID const & id) const
{
    if (id.is_null()) {
        return nullptr;
    }
    return load_page(pageIndex(id.pageId));
}

//----------------------------------------------------------------------------

database::database(const std::string & fname)
    : m_data(sdl::make_unique<data_t>(fname))
{
}

database::~database()
{
}

const std::string & database::filename() const
{
    return m_data->filename;
}

bool database::is_open() const
{
    return m_data->is_open();
}

void const * database::start_address() const
{
    return m_data->start_address();
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
    return m_data->page_count();
}

page_head const *
database::load_page_head(pageIndex i)
{
    return m_data->load_page(i);
}

page_head const *
database::load_page_head(pageFileID const & id)
{
    return m_data->load_page(id);
}

page_head const * 
database::load_page_head(sysPage i)
{
    return load_page_head(static_cast<pageIndex::value_type>(i));
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
        vec.shrink_to_fit();
    }
    return vec;
}

database::page_ptr<bootpage>
database::get_bootpage()
{
    page_head const * const h = load_page_head(sysPage::boot_page);
    if (h) {
        return make_unique<bootpage>(h, cast::page_body<bootpage_row>(h));
    }
    return std::unique_ptr<bootpage>();
}

database::page_ptr<datapage>
database::get_datapage(pageIndex i)
{
    page_head const * const h = load_page_head(i);
    if (h) {
        return make_unique<datapage>(h);
    }
    return std::unique_ptr<datapage>();
}

database::page_ptr<fileheader>
database::get_fileheader()
{
    page_head const * const h = load_page_head(0);
    if (h) {
        return make_unique<fileheader>(h);
    }
    return std::unique_ptr<fileheader>();
}

database::page_ptr<sysallocunits>
database::get_sysallocunits()
{
    auto boot = get_bootpage();
    if (boot) {
        auto & id = boot->row->data.dbi_firstSysIndexes;
        page_head const * const h = m_data->load_page(id);
        if (h) {
            return make_unique<sysallocunits>(h);
        }
    }
    return std::unique_ptr<sysallocunits>();
}

page_head const *
database::load_sys_obj(sysallocunits const * p, const sysObj id)
{
    if (p) {
        if (auto row = p->find_auid(static_cast<uint32>(id))) {
            return load_page_head(row->data.pgfirst);
        }
    }
    return nullptr;
}

//-----------------------------------------------------------------------

template<class T, database::sysObj id> 
database::page_ptr<T>
database::get_sys_obj(sysallocunits const * p)
{
    if (auto h = load_sys_obj(p, id)) {
        return make_pointer<page_ptr<T>>(h);
    }
    return page_ptr<T>();
}

template<class T, database::sysObj id> 
database::page_ptr<T>
database::get_sys_obj()
{
    return get_sys_obj<T, id>(get_sysallocunits().get());
}

template<class T> 
database::vector_page_ptr<T>
database::get_sys_list(page_ptr<T> && p)
{
    vector_page_ptr<T> vec;
    if (p) {
        auto page_head_list = load_page_list(p->head);
        if (!page_head_list.empty()) {
            vec.reserve(page_head_list.size());
            for (auto h : page_head_list) {
                vec.push_back(make_pointer<page_ptr<T>>(h));
            }
        }
    }
    return vec;
}

template<class T, database::sysObj id> 
database::vector_page_ptr<T>
database::get_sys_list()
{
    return get_sys_list(get_sys_obj<T, id>());
}

//-----------------------------------------------------------------------

database::page_ptr<sysschobjs>
database::get_sysschobjs()
{
    return get_sys_obj<sysschobjs, sysObj::sysschobjs>();
}

database::page_ptr<syscolpars>
database::get_syscolpars()
{
    return get_sys_obj<syscolpars, sysObj::syscolpars>();
}

database::page_ptr<sysidxstats>
database::get_sysidxstats()
{
    return get_sys_obj<sysidxstats, sysObj::sysidxstats>();
}

database::page_ptr<sysscalartypes>
database::get_sysscalartypes()
{
    return get_sys_obj<sysscalartypes, sysObj::sysscalartypes>();
}

database::page_ptr<sysobjvalues>
database::get_sysobjvalues()
{
    return get_sys_obj<sysobjvalues, sysObj::sysobjvalues>();
}

database::page_ptr<sysiscols>
database::get_sysiscols()
{
    return get_sys_obj<sysiscols, sysObj::sysiscols>();
}

//---------------------------------------------------------

page_head const * database::load_next_head(page_head const * p)
{
    if (p) {
        return m_data->load_page(p->data.nextPage);
    }
    SDL_ASSERT(0);
    return nullptr;
}

page_head const * database::load_prev_head(page_head const * p)
{
    if (p) {
        return m_data->load_page(p->data.prevPage);
    }
    SDL_ASSERT(0);
    return nullptr;
}

//---------------------------------------------------------

template<class T>
void database::load_next_t(page_ptr<T> & p)
{
    SDL_ASSERT(p);
    if (p) {
        A_STATIC_CHECK_TYPE(page_head const * const, p->head);
        if (auto h = load_next_head(p->head)) {
            A_STATIC_CHECK_TYPE(page_head const *, h);
            p = make_pointer<page_ptr<T>>(h);
        }
        else {
            p.reset();
        }
    }
}

template<class T>
void database::load_prev_t(page_ptr<T> & p)
{
    SDL_ASSERT(p);
    if (p) {
        A_STATIC_CHECK_TYPE(page_head const * const, p->head);
        if (auto h = load_prev_head(p->head)) {
            A_STATIC_CHECK_TYPE(page_head const *, h);
            p = make_pointer<page_ptr<T>>(h);
        }
        else {
            SDL_ASSERT(0);
            p.reset();
        }
    }
}

void database::load_page(page_ptr<sysallocunits> & p)   { p = get_sysallocunits(); }
void database::load_page(page_ptr<sysschobjs> & p)      { p = get_sysschobjs(); }
void database::load_page(page_ptr<syscolpars> & p)      { p = get_syscolpars(); }
void database::load_page(page_ptr<sysidxstats> & p)     { p = get_sysidxstats(); }
void database::load_page(page_ptr<sysscalartypes> & p)  { p = get_sysscalartypes(); }
void database::load_page(page_ptr<sysobjvalues> & p)    { p = get_sysobjvalues(); }
void database::load_page(page_ptr<sysiscols> & p)       { p = get_sysiscols(); }

void database::load_next(page_ptr<sysallocunits> & p)   { load_next_t(p); }
void database::load_next(page_ptr<sysschobjs> & p)      { load_next_t(p); }
void database::load_next(page_ptr<syscolpars> & p)      { load_next_t(p); }
void database::load_next(page_ptr<sysidxstats> & p)     { load_next_t(p); }
void database::load_next(page_ptr<sysscalartypes> & p)  { load_next_t(p); }
void database::load_next(page_ptr<sysobjvalues> & p)    { load_next_t(p); }
void database::load_next(page_ptr<sysiscols> & p)       { load_next_t(p); }

void database::load_prev(page_ptr<sysallocunits> & p)   { load_prev_t(p); }
void database::load_prev(page_ptr<sysschobjs> & p)      { load_prev_t(p); }
void database::load_prev(page_ptr<syscolpars> & p)      { load_prev_t(p); }
void database::load_prev(page_ptr<sysidxstats> & p)     { load_prev_t(p); }
void database::load_prev(page_ptr<sysscalartypes> & p)  { load_prev_t(p); }
void database::load_prev(page_ptr<sysobjvalues> & p)    { load_prev_t(p); }
void database::load_prev(page_ptr<sysiscols> & p)       { load_prev_t(p); }

void database::load_next(shared_datapage & p)
{
    SDL_ASSERT(p);
    if (p) {
        if (auto h = this->load_next_head(p->head)) {
            p = std::make_shared<datapage>(h);
        }
        else {
            p.reset();
        }
    }
}

void database::load_prev(shared_datapage & p)
{
    SDL_ASSERT(p);
    if (p) {
        if (auto h = this->load_prev_head(p->head)) {
            p = std::make_shared<datapage>(h);
        }
        else {
            SDL_ASSERT(0);
            p.reset();
        }
    }
}

database::vector_usertable const &
database::get_usertables()
{
    if (!m_ut.empty())
        return m_ut;

    vector_usertable ret;

    for_USER_TABLE([&ret, this](sysschobjs::const_pointer schobj_row)
    {        
        auto utable = make_pointer<shared_usertable>(schobj_row, schobj_row->col_name());
        auto ut = utable.get();
        {
            SDL_ASSERT(schobj_row->data.id == ut->get_id());
            for (auto & colpar : _syscolpars) {
                colpar->for_row([ut, this](syscolpars::const_pointer colpar_row) {
                    if (colpar_row->data.id == ut->get_id()) {
                        auto const utype = colpar_row->data.utype;
                        for (auto const & scalar : _sysscalartypes) {
                            auto const s = scalar->find_if([utype](sysscalartypes_row const * const p) {
                                return (p->data.id == utype);
                            });
                            if (s) {
                                ut->push_back(sdl::make_unique<tablecolumn>(colpar_row, s,
                                    colpar_row->col_name()));
                            }
                        }
                    }
                });
            }
            if (!utable->empty()) {
                ret.push_back(std::move(utable));
            }
        }
    });
    using table_type = vector_usertable::value_type;
    std::sort(ret.begin(), ret.end(),
        [](table_type const & x, table_type const & y){
        return x->name() < y->name();
    });    
    ret.swap(m_ut);
    return m_ut;
}

database::vector_datatable const &
database::get_datatable()
{
    if (!m_dt.empty())
        return m_dt;

    auto & ut = this->get_usertables();
    m_dt.reserve(ut.size());

    for (auto & p : ut) {
        m_dt.push_back(std::make_shared<datatable>(this, p));
    }
    return m_dt;
}

database::datatable_ptr
database::make_datatable(shared_usertable const & p)
{
    SDL_ASSERT(p);
    return make_pointer<datatable_ptr>(this, p);
}

page_head const *
database::find_pgfirst(schobj_id const id)
{
    sysidxstats_row const * const idx = find_row_if(_sysidxstats, 
        [id](sysidxstats::const_pointer row) {
        return (row->data.id == id) && !row->data.rowset.is_null();
    });
    if (idx) {
        sysallocunits_row const * const alloc = find_row_if(_sysallocunits, 
            [idx](sysallocunits::const_pointer row){
            return row->data.ownerid == idx->data.rowset;
        });
        if (alloc) {
            return load_page_head(alloc->data.pgfirst);
        }
    }
    SDL_ASSERT(0);
    return nullptr;
}

//----------------------------------------------------------------------------

class datatable::data_t: noncopyable {
    shared_usertable const table;
public:
    database & db;
    data_t(database * p, shared_usertable const & t): db(*p), table(t) {
        SDL_ASSERT(p && table);
    }
    const usertable & ut() const {
        return *table.get();
    }
    page_head const * pgfirst() {
        return db.find_pgfirst(ut().get_id());
    }
};

datatable::datatable(database * p, shared_usertable const & t)
    : m_data(sdl::make_unique<data_t>(p, t))
    , _pages(this, p)
{
}

datatable::~datatable()
{
}

datatable::data_t & datatable::data()
{
    return *m_data.get();
}

const usertable & datatable::ut() const
{
    return m_data->ut();
} 

datatable::page_access::iterator
datatable::page_access::begin()
{
    page_head const * p = table.data().pgfirst();
    if (p) {
        return iterator(this, std::make_shared<datapage>(p));
    }
    //FIXME: [ForwardingPointers]
    //FIXME: pgfirstiam = 1:372 (740100000100)
    return this->end(); 
}

datatable::page_access::iterator
datatable::page_access::end() 
{
    return iterator(this);
}

} // db
} // sdl

#if 0
------------------------------------------------------
PFS_page
The first PFS page is at *:1 in each database file, 
and it stores the statistics for pages 0-8087 in that database file. 
There will be one PFS page for just about every 64MB of file size (8088 bytes * 8KB per page).
A large database file will use a long chain of PFS pages, linked together using the LastPage and NextPage pointers in the 96 byte header. 
------------------------------------------------------
linked list of pages using the PrevPage, ThisPage, and NextPage page locators, when one page is not enough to hold all the data.
------------------------------------------------------
------------------------------------------------------
------------------------------------------------------
#endif

