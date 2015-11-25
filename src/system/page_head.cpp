// page_head.cpp
//
#include "common/common.h"
#include "page_head.h"

namespace sdl { namespace db {

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

                    SDL_ASSERT(IS_LITTLE_ENDIAN);
                    static_assert(sizeof(uint8) == 1, "");
                    static_assert(sizeof(int16) == 2, "");
                    static_assert(sizeof(uint16) == 2, "");
                    static_assert(sizeof(int32) == 4, "");
                    static_assert(sizeof(uint32) == 4, "");
                    static_assert(sizeof(int64) == 8, "");
                    static_assert(sizeof(uint64) == 8, "");

                    A_STATIC_ASSERT_IS_POD(page_header);
                    A_STATIC_ASSERT_IS_POD(guid_t);

                    static_assert(sizeof(page_header) == 96, "");
                    static_assert(sizeof(pageFileID) == 6, "");
                    static_assert(sizeof(pageLSN) == 10, "");
                    static_assert(sizeof(pageXdesID) == 6, "");
                    static_assert(sizeof(guid_t) == 16, "");

                    static_assert(page_header::page_size == 8 * 1024, "");
                    static_assert(page_header::body_size == 8 * 1024 - 96, "");

                    static_assert(offsetof(page_header, data.headerVersion) == 0, "");
                    static_assert(offsetof(page_header, data.type) == 0x01, "");
                    static_assert(offsetof(page_header, data.typeFlagBits) == 0x02, "");
                    static_assert(offsetof(page_header, data.level) == 0x03, "");
                    static_assert(offsetof(page_header, data.flagBits) == 0x04, "");
                    static_assert(offsetof(page_header, data.indexId) == 0x06, "");
                    static_assert(offsetof(page_header, data.prevPage) == 0x08, "");
                    static_assert(offsetof(page_header, data.pminlen) == 0x0E, "");
                    static_assert(offsetof(page_header, data.nextPage) == 0x10, "");
                    static_assert(offsetof(page_header, data.slotCnt) == 0x16, "");
                    static_assert(offsetof(page_header, data.objId) == 0x18, "");
                    static_assert(offsetof(page_header, data.freeCnt) == 0x1C, "");
                    static_assert(offsetof(page_header, data.freeData) == 0x1E, "");
                    static_assert(offsetof(page_header, data.pageId) == 0x20, "");
                    static_assert(offsetof(page_header, data.reservedCnt) == 0x26, "");
                    static_assert(offsetof(page_header, data.lsn) == 0x28, "");
                    static_assert(offsetof(page_header, data.xactReserved) == 0x32, "");
                    static_assert(offsetof(page_header, data.xdesId) == 0x34, "");
                    static_assert(offsetof(page_header, data.ghostRecCnt) == 0x3A, "");
                    static_assert(offsetof(page_header, data.tornBits) == 0x3C, "");
                    static_assert(offsetof(page_header, data.reserved) == 0x40, "");

                    static_assert(std::is_same<pageId_t::value_type, uint32>::value, "");
                    static_assert(std::is_same<fileId_t::value_type, uint16>::value, "");
                    {
                        typedef page_header_meta T;
                        static_assert(T::headerVersion::offset == 0, "");
                        static_assert(T::type::offset == 1, "");
                        static_assert(std::is_same<T::headerVersion::type, uint8>::value, "");
                        static_assert(std::is_same<T::type::type, pageType>::value, "");
                        static_assert(std::is_same<T::tornBits::type, int32>::value, "");
                    }
                    SDL_TRACE_2(__FILE__, " end");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG

