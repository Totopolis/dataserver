// overflow.cpp
//
#include "common/common.h"
#include "overflow.h"
#include "database.h"
#include "page_info.h"

namespace sdl { namespace db { namespace {

template<class root_type>
mem_range_t load_slot_t(database const * const db, root_type const * const root, size_t const slot)
{
    SDL_ASSERT(db && root);
    SDL_ASSERT(slot < root->curlinks);
    auto const & p = root->data[slot];
    if (p.size && p.row) {
        auto const page_row = db->load_page_row(p.row);
        if (page_row.first && page_row.second) {
            SDL_ASSERT(page_row.first->data.type == pageType::type::textmix);
            mem_range_t const m = page_row.second->fixed_data();
            size_t const sz = mem_size(m);
            if (sz > sizeof(lob_head)) {
                lob_head const * const lob = reinterpret_cast<lob_head const *>(m.first);
                if (lob->type == lobtype::DATA) {
                    SDL_ASSERT(root->head.blobID == lob->blobID);
                    const char * const p1 = m.first + sizeof(lob_head);
                    if (p1 <= m.second) {
                        return { p1, m.second };
                    }
                }
                else {
                    SDL_ASSERT(0);
                }
            }
        }
        SDL_ASSERT(0);
    }
    SDL_ASSERT(0);
    return {};
}

template<class root_type>
vector_mem_range_t
load_root_t(database const * const db, root_type const * const root)
{
    SDL_ASSERT(db && root);
    if (root->curlinks > 0) {
        vector_mem_range_t result(root->curlinks);
        size_t offset = 0;
        for (size_t i = 0; i < root->curlinks; ++i) {
            auto & d = result[i];
            d = load_slot_t(db, root, i);
            SDL_ASSERT(mem_size(d));
            offset += mem_size(d);
            SDL_ASSERT(offset == root->data[i].size);
        }
        SDL_ASSERT(mem_size_n(result) == offset);
        return result;
    }
    SDL_ASSERT(0);
    return{};
}

} // namespace

std::string mem_range_page::text() const {
    return to_string::make_text(m_data);
}
std::string mem_range_page::ntext() const {
    return to_string::make_ntext(m_data);
}

//----------------------------------------------------------------------

varchar_overflow_page::varchar_overflow_page(
    database const * const db,
    overflow_page const * const page_over)
{
    SDL_ASSERT(db && page_over && page_over->row);
    SDL_ASSERT(page_over->length);

    auto const page_row = db->load_page_row(page_over->row);
    if (page_row.first && page_row.second) {
        if (page_row.first->data.type == pageType::type::textmix) {
            mem_range_t const m = page_row.second->fixed_data();
            if (mem_size(m) == page_over->length + sizeof(lob_head)) {
                lob_head const * const lob = reinterpret_cast<lob_head const *>(m.first);
                SDL_ASSERT(lob->blobID == page_over->timestamp);
                if (lob->type == lobtype::DATA) {
                    m_data.emplace_back(m.first + sizeof(lob_head), m.second);
                }
                else {
                    SDL_ASSERT(0);
                }
            }
        }
        else if (page_row.first->data.type == pageType::type::texttree) {
            mem_range_t const m = page_row.second->fixed_data();
            const size_t sz = mem_size(m);
            if (sz > sizeof(lob_head)) {
                lob_head const * const lob = reinterpret_cast<lob_head const *>(m.first);
                SDL_ASSERT(lob->blobID == page_over->timestamp);
                if (lob->type == lobtype::INTERNAL) {
                    // LOB root structure, which contains a set of the pointers to other data pages/rows.
                    // When LOB data is less than 32 KB and can fit into five data pages, 
                    // the LOB root structure contains the pointers to the actual chunks of LOB data
                    if (sz >= sizeof(TextTreeInternal)) {
                        TextTreeInternal const * const root = reinterpret_cast<TextTreeInternal const *>(m.first);
                        SDL_ASSERT(root->head.blobID == lob->blobID);
                        SDL_ASSERT(root->curlinks <= root->maxlinks);
                        if (root->curlinks && (sz >= root->length())) {
                            m_data = load_root_t(db, root);
                        }
                        else {
                            SDL_ASSERT(0);
                        }
                    }
                }
            }
        }
    }
    SDL_ASSERT(mem_size_n(m_data));
}

//----------------------------------------------------------------------

varchar_overflow_link::varchar_overflow_link(
    database const * const db, 
    overflow_page const * const page_over,
    overflow_link const * const page_link)
{
    SDL_ASSERT(db);
    SDL_ASSERT(page_over && page_over->row);
    SDL_ASSERT(page_link && page_link->row);  
    SDL_ASSERT(page_over->length);
    SDL_ASSERT(page_link->size);

    auto const page_row = db->load_page_row(page_link->row);
    if (page_row.first && page_row.second) {
        if (page_row.first->data.type == pageType::type::textmix) {
            mem_range_t const m = page_row.second->fixed_data();
            auto const sz = mem_size(m);
            if (sz > sizeof(lob_head)) {
                lob_head const * const lob = reinterpret_cast<lob_head const *>(m.first);
                SDL_ASSERT(lob->blobID == page_over->timestamp);
                if (lob->type == lobtype::DATA) {
                    m_data.emplace_back(m.first + sizeof(lob_head), m.second);
                }
                else {
                    SDL_ASSERT(0);
                }
            }
            else {
                SDL_ASSERT(0);
            }
        }
    }
    SDL_WARNING(mem_size_n(m_data)); //FIXME: dbo_COUNTRY.Geoinfo
}

//------------------------------------------------------------------

text_pointer_data::text_pointer_data(
    database const * const db, 
    text_pointer const * const text_ptr)
{
    SDL_ASSERT(db && text_ptr && text_ptr->row);

    auto const page_row = db->load_page_row(text_ptr->row);
    if (page_row.first && page_row.second) {
        // textmix(3) stores multiple LOB values and indexes for LOB B-trees
        SDL_ASSERT(page_row.first->data.type == pageType::type::textmix);
        mem_range_t const m = page_row.second->fixed_data();
        const size_t sz = mem_size(m);
        if (sz > sizeof(lob_head)) {
            lob_head const * const lob = reinterpret_cast<lob_head const *>(m.first);
            if (lob->type == lobtype::LARGE_ROOT_YUKON) {
                // LOB root structure, which contains a set of the pointers to other data pages/rows.
                // When LOB data is less than 32 KB and can fit into five data pages, 
                // the LOB root structure contains the pointers to the actual chunks of LOB data
                if (sz >= sizeof(LargeRootYukon)) {
                    LargeRootYukon const * const root = reinterpret_cast<LargeRootYukon const *>(m.first);
                    SDL_ASSERT(root->head.blobID == lob->blobID);
                    SDL_ASSERT(root->curlinks <= root->maxlinks);
                    SDL_ASSERT(root->maxlinks == 5);
                    if (root->curlinks && (sz >= root->length())) {
                        m_data = load_root_t(db, root);
                    }
                    else {
                        SDL_ASSERT(0);
                    }
                }
            }
            else if (lob->type == lobtype::SMALL_ROOT) {
                SDL_ASSERT(sz > sizeof(LobSmallRoot));
                if (sz > sizeof(LobSmallRoot)) {
                    LobSmallRoot const * const root = reinterpret_cast<LobSmallRoot const *>(m.first);      
                    const char * const p1 = m.first + sizeof(LobSmallRoot);
                    const char * const p2 = p1 + root->length;
                    if (p2 <= m.second) {
                        m_data.emplace_back(p1, p2);
                    }
                    else {
                        SDL_ASSERT(0);
                    }
                }
            }
        }
    }
    SDL_ASSERT(mem_size_n(m_data));
}

} // db
} // sdl

