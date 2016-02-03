// datapage.cpp
//
#include "common/common.h"
#include "datapage.h"
#include "page_info.h"
#include "database.h"

namespace sdl { namespace db {

pfs_page::pfs_page(page_head const * p)
    : head(p)
    , row(cast::page_body<pfs_page_row>(p))
{
    SDL_ASSERT(head);
    SDL_ASSERT(row);
    SDL_ASSERT(is_pfs(head->data.pageId));
    static_assert(pfs_size == 8088, "");
}

bool pfs_page::is_pfs(pageFileID const & id) const
{
    return (id.fileId > 0) && ((id.pageId == 1) || !(id.pageId % pfs_size));
}

pageFileID pfs_page::pfs_for_page(pageFileID const & id)
{
    pageFileID loc = id;
    if (!loc.is_null()) {
        loc.pageId = a_max(loc.pageId / pfs_size * pfs_size, uint32(1));
    }
    else {
        SDL_ASSERT(0);
    }
    return loc;
}

pfs_byte pfs_page::operator[](pageFileID const & id) const
{
    SDL_ASSERT(!id.is_null());
    return (*row)[id.pageId % pfs_size];
}

//---------------------------------------------------------------------------

datapage::const_pointer
datapage::operator[](size_t i) const
{
    return cast::page_row<row_head>(this->head, this->slot[i]);
}

sysallocunits::const_pointer
sysallocunits::find_auid(uint32 const id) const
{
    A_STATIC_CHECK_TYPE(decltype(auid_t().d.id) const, id);
    A_STATIC_ASSERT_TYPE(const_pointer, sysallocunits_row const *);
    return find_if([id](sysallocunits_row const * const p) {
        return (p->data.auid.d.id == id);
    });
}

//---------------------------------------------------------------------------

iam_page_row const *
iam_page::first() const 
{
    if (!empty()) {
        return cast::page_row<iam_page_row>(this->head, this->slot[0]);
    }
    SDL_ASSERT(0);
    return nullptr;
}

void iam_page::_allocated_extents(allocated_fun fun) const
{
    if (_extent.empty())
        return;

    iam_extent_row const & row = _extent.first();
    SDL_ASSERT(&row == *_extent.begin());

    pageFileID const start_pg = this->first()->data.start_pg;
    pageFileID allocated = start_pg; // copy id.fileId

    const size_t row_size = row.size();
    for (size_t i = 0; i < row_size; ++i) {
        const uint8 b = row[i];
        for (size_t j = 0; j < 8; ++j) {
            if (b & (1 << j)) { // extent is allocated ?
                const size_t pageId = start_pg.pageId + (((i << 3) + j) << 3);
                SDL_ASSERT(pageId < uint32(-1));
                allocated.pageId = static_cast<uint32>(pageId);
                SDL_ASSERT(!(allocated.pageId % 8));
                fun(allocated);
            }
        }
    }
}

void iam_page::_allocated_pages(database * const db, allocated_fun fun) const 
{
    SDL_ASSERT(db);
    if (iam_page_row const * const p = this->first()) {
        for (pageFileID const & id : *p) {
            if (id && db->is_allocated(id)) {
                fun(id);
            }
        }
        _allocated_extents([db, fun](pageFileID const & start) {
            if (db->is_allocated(start)) {
                fun(start);
                for (uint32 i = 1; i < 8; ++i) { // Eight consecutive pages form an extent
                    pageFileID id = start;
                    A_STATIC_SAME_TYPE(i, id.pageId);
                    id.pageId += i;
                    if (db->is_allocated(id)) {
                        fun(id);
                    }
                }
            }
        });
    }
}

//---------------------------------------------------------------------------

#define static_datapage_name(classname) \
    template<> const char * datapage_t<classname##_row>::name() { return #classname; }

static_datapage_name(fileheader)
static_datapage_name(sysallocunits)
static_datapage_name(sysschobjs)
static_datapage_name(syscolpars)
static_datapage_name(sysidxstats)
static_datapage_name(sysscalartypes)
static_datapage_name(sysobjvalues)
static_datapage_name(sysiscols)

//---------------------------------------------------------------------------

} // db
} // sdl

#if SDL_DEBUG
namespace sdl {
    namespace db {
        namespace {
            class unit_test {
            public:
                unit_test()
                {
                    SDL_TRACE_FILE;
                    if (0) {
                        SDL_TRACE(fileheader::name());
                        SDL_TRACE(sysallocunits::name());
                        SDL_TRACE(sysschobjs::name());
                        SDL_TRACE(syscolpars::name());
                        SDL_TRACE(sysidxstats::name());
                        SDL_TRACE(sysscalartypes::name());
                        SDL_TRACE(sysobjvalues::name());
                        SDL_TRACE(sysiscols::name());
                        SDL_TRACE(iam_page::name());
                    }
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG

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

