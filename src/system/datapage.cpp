// datapage.cpp
//
#include "common/common.h"
#include "datapage.h"
#include "page_info.h"

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
                    //SDL_TRACE_2("pageIndex::value_type(-1) = ", pageIndex::value_type(-1));
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

