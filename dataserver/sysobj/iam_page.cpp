// iam_page.cpp
//
#include "dataserver/sysobj/iam_page.h"
#include "dataserver/system/database.h"

namespace sdl { namespace db {

#if 0
using allocated_fun = std::function<void(pageFileID const &)>;
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

void iam_page::_allocated_pages(database const * const db, allocated_fun fun) const 
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
#endif

} // db
} // sdl
