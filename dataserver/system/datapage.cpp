// datapage.cpp
//
#include "dataserver/system/datapage.h"
#include "dataserver/system/page_info.h"
#include "dataserver/system/database.h"

namespace sdl { namespace db {

pfs_page::pfs_page(page_head const * const p)
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
                    SDL_ASSERT(is_str_valid(fileheader::name()));
                    SDL_ASSERT(is_str_valid(sysallocunits::name()));
                    SDL_ASSERT(is_str_valid(sysschobjs::name()));
                    SDL_ASSERT(is_str_valid(syscolpars::name()));
                    SDL_ASSERT(is_str_valid(sysidxstats::name()));
                    SDL_ASSERT(is_str_valid(sysscalartypes::name()));
                    SDL_ASSERT(is_str_valid(sysobjvalues::name()));
                    SDL_ASSERT(is_str_valid(sysiscols::name()));
                    SDL_ASSERT(is_str_valid(iam_page::name()));
                    SDL_ASSERT(is_str_valid(sysrowsets::name()));
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
                        SDL_TRACE(sysrowsets::name());
                    }
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SDL_DEBUG

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

