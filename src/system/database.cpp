// database.cpp
//
#include "common/common.h"
#include "database.h"
#include "page_map.h"

#include <algorithm>
#include <sstream>

namespace sdl { namespace db {
    
class database::data_t : noncopyable {
public:
    PageMapping pm;
    vector_shared_usertable ut;
    vector_shared_datatable dt;
    explicit data_t(const std::string & fname): pm(fname){}
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

pageType database::get_pageType(pageFileID const & id)
{
    if (auto p = load_page_head(id)) {
        return p->data.type;
    }
    return pageType::init(pageType::type::null);
}

pageFileID database::nextPage(pageFileID const & id) // diagnostic
{
    if (auto p = load_page_head(id)) {
        return p->data.nextPage;
    }
    SDL_ASSERT(id.is_null());
    return {};
}

pageFileID database::prevPage(pageFileID const & id) // diagnostic
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

database::page_ptr<pfs_page>
database::get_pfs_page()
{
    page_head const * const h = load_page_head(sysPage::PFS);
    if (h) {
        return make_unique<pfs_page>(h);
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

database::page_ptr<sysallocunits>
database::get_sysallocunits()
{
    auto boot = get_bootpage();
    if (boot) {
        auto & id = boot->row->data.dbi_firstSysIndexes;
        page_head const * const h = m_data->pm.load_page(id);
        if (h) {
            return make_unique<sysallocunits>(h);
        }
    }
    return {};
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
        return sdl::make_unique<T>(h);
    }
    return {};
}

template<class T, database::sysObj id> 
database::page_ptr<T>
database::get_sys_obj()
{
    return get_sys_obj<T, id>(get_sysallocunits().get());
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
        return m_data->pm.load_page(p->data.nextPage);
    }
    SDL_ASSERT(0);
    return nullptr;
}

page_head const * database::load_prev_head(page_head const * p)
{
    if (p) {
        return m_data->pm.load_page(p->data.prevPage);
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
            reset_new(p, h);
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
            reset_new(p, h);
        }
        else {
            SDL_ASSERT(0);
            p.reset();
        }
    }
}

template<class fun_type>
database::unique_datatable
database::find_table_if(fun_type fun)
{
    for (auto & p : _usertables) {
        const usertable & d = *p.get();
        if (fun(d)) {
            return sdl::make_unique<datatable>(this, p);
        }
    }
    return {};
}

void database::load_page(page_ptr<sysallocunits> & p)   { p = get_sysallocunits(); }
void database::load_page(page_ptr<sysschobjs> & p)      { p = get_sysschobjs(); }
void database::load_page(page_ptr<syscolpars> & p)      { p = get_syscolpars(); }
void database::load_page(page_ptr<sysidxstats> & p)     { p = get_sysidxstats(); }
void database::load_page(page_ptr<sysscalartypes> & p)  { p = get_sysscalartypes(); }
void database::load_page(page_ptr<sysobjvalues> & p)    { p = get_sysobjvalues(); }
void database::load_page(page_ptr<sysiscols> & p)       { p = get_sysiscols(); }
void database::load_page(page_ptr<pfs_page> & p)        { p = get_pfs_page(); }

void database::load_next(page_ptr<sysallocunits> & p)   { load_next_t(p); }
void database::load_next(page_ptr<sysschobjs> & p)      { load_next_t(p); }
void database::load_next(page_ptr<syscolpars> & p)      { load_next_t(p); }
void database::load_next(page_ptr<sysidxstats> & p)     { load_next_t(p); }
void database::load_next(page_ptr<sysscalartypes> & p)  { load_next_t(p); }
void database::load_next(page_ptr<sysobjvalues> & p)    { load_next_t(p); }
void database::load_next(page_ptr<sysiscols> & p)       { load_next_t(p); }
void database::load_next(page_ptr<pfs_page> & p)        { load_next_t(p); }

void database::load_prev(page_ptr<sysallocunits> & p)   { load_prev_t(p); }
void database::load_prev(page_ptr<sysschobjs> & p)      { load_prev_t(p); }
void database::load_prev(page_ptr<syscolpars> & p)      { load_prev_t(p); }
void database::load_prev(page_ptr<sysidxstats> & p)     { load_prev_t(p); }
void database::load_prev(page_ptr<sysscalartypes> & p)  { load_prev_t(p); }
void database::load_prev(page_ptr<sysobjvalues> & p)    { load_prev_t(p); }
void database::load_prev(page_ptr<sysiscols> & p)       { load_prev_t(p); }
void database::load_prev(page_ptr<pfs_page> & p)        { load_prev_t(p); }

void database::load_next(shared_datapage & p) { load_next_t(p); }
void database::load_prev(shared_datapage & p) { load_prev_t(p); }

void database::load_next(shared_iam_page & p) { load_next_t(p); }
void database::load_prev(shared_iam_page & p) { load_prev_t(p); }

database::vector_shared_usertable const &
database::get_usertables()
{
    auto & m_ut = m_data->ut;
    if (!m_ut.empty())
        return m_ut;

    vector_shared_usertable ret;
    for_USER_TABLE([&ret, this](sysschobjs::const_pointer schobj_row)
    {        
        auto utable = std::make_shared<usertable>(schobj_row, col_name_t(schobj_row));
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
                                    col_name_t(colpar_row)));
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
    using table_type = vector_shared_usertable::value_type;
    std::sort(ret.begin(), ret.end(),
        [](table_type const & x, table_type const & y){
        return x->name() < y->name();
    });    
    ret.swap(m_ut);
    return m_ut;
}

database::vector_shared_datatable const &
database::get_datatable()
{
    auto & m_dt = m_data->dt;
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
        return x->ut().name() < y->ut().name();
    });   
    return m_dt;
}

database::unique_datatable
database::find_table_name(const std::string & name)
{
    return find_table_if([&name](const usertable & d) {
        return d.name() == name;
    });
}

database::vector_sysallocunits_row
database::find_sysalloc(schobj_id const id)
{
    vector_sysallocunits_row result; // returns pointers to mapped memory
    for_row(_sysidxstats, [this, id, &result](sysidxstats::const_pointer idx) {
        if ((idx->data.id == id) && !idx->data.rowset.is_null()) {
            for_row(_sysallocunits, 
                [idx, &result](sysallocunits::const_pointer row) {
                    if (row->data.ownerid == idx->data.rowset) {
                        if (std::find(result.begin(), result.end(), row) == result.end()) {
                            result.push_back(row); // add unique sysallocunits_row
                        }
                    }
            });
        }
    });
    SDL_ASSERT(!result.empty());
    return result;
}

bool database::is_valid(pageFileID const & id) const
{
    return !id.is_null() && ((size_t)id.pageId < page_count());
}

bool database::is_allocated(pageFileID const & id)
{
    if (is_valid(id)) {
        if (auto h = load_page_head(pfs_page::pfs_for_page(id))) {
            return pfs_page(h)[id].b.allocated;
        }
    }
    SDL_ASSERT(0);
    return false;
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

