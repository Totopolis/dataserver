// database_impl.cpp
//
#include "dataserver/system/database.h"
#include "dataserver/system/database_impl.h"

namespace sdl { namespace db {

#if SDL_DEBUG
bool database_PageMapping::assert_page(pageFileID const & id) {
    if (id) {
        return m_pool.assert_page(id.pageId);
    }
    return true;
}
#endif

} // db
} // sdl