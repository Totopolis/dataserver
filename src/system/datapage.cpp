// datapage.cpp
//
#include "common/common.h"
#include "datapage.h"
#include "page_info.h"

namespace sdl { namespace db {

row_head const * datapage::get_row_head(size_t i) const
{
    return cast::page_row<row_head>(this->head, this->slot[i]);
}

sysallocunits::find_result
sysallocunits::find_auid(uint32 const id) const
{
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
                    SDL_TRACE(__FILE__);
                    SDL_TRACE_2("uint32(-1) = ", uint32(-1));
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

