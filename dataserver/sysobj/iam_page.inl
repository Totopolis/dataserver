// iam_page.inl
//
#pragma once
#ifndef __SDL_SYSOBJ_IAM_PAGE_INL__
#define __SDL_SYSOBJ_IAM_PAGE_INL__

namespace sdl { namespace db {

inline iam_page_row const *
iam_page::first() const 
{
    if (!empty()) {
        return cast::page_row<iam_page_row>(this->head, this->slot[0]);
    }
    SDL_ASSERT(0);
    return nullptr;
}

template<class allocated_fun>
void iam_page::allocated_extents(allocated_fun const & fun) const
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

template<class allocated_fun>
void iam_page::allocated_pages(database const * const db, allocated_fun const & fun) const 
{
    SDL_ASSERT(db);
    if (iam_page_row const * const p = this->first()) {
        for (pageFileID const & id : *p) {
            if (id && fwd::is_allocated(db, id)) {
                fun(id);
            }
        }
        allocated_extents([db, fun](pageFileID const & start) {
            if (fwd::is_allocated(db, start)) {
                fun(start);
                for (uint32 i = 1; i < 8; ++i) { // Eight consecutive pages form an extent
                    pageFileID id = start;
                    A_STATIC_SAME_TYPE(i, id.pageId);
                    id.pageId += i;
                    if (fwd::is_allocated(db, id)) {
                        fun(id);
                    }
                }
            }
        });
    }
}

} // db
} // sdl

#endif // __SDL_SYSOBJ_IAM_PAGE_INL__