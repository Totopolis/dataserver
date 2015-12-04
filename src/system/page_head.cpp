// page_head.cpp
//
#include "common/common.h"
#include "page_head.h"
#include <time.h>       /* time_t, struct tm, time, localtime, strftime */

namespace sdl { namespace db {

const char meta::empty[] = "";

static_col_name(page_header_meta, headerVersion);
static_col_name(page_header_meta, type);
static_col_name(page_header_meta, typeFlagBits);
static_col_name(page_header_meta, level);
static_col_name(page_header_meta, flagBits);
static_col_name(page_header_meta, indexId);
static_col_name(page_header_meta, prevPage);
static_col_name(page_header_meta, pminlen);
static_col_name(page_header_meta, nextPage);
static_col_name(page_header_meta, slotCnt);
static_col_name(page_header_meta, objId);
static_col_name(page_header_meta, freeCnt);
static_col_name(page_header_meta, freeData);
static_col_name(page_header_meta, pageId);
static_col_name(page_header_meta, reservedCnt);
static_col_name(page_header_meta, lsn);
static_col_name(page_header_meta, xactReserved);
static_col_name(page_header_meta, xdesId);
static_col_name(page_header_meta, ghostRecCnt);
static_col_name(page_header_meta, tornBits);

//------------------------------------------------------------------------------

size_t slot_array::size() const
{
    return head->data.slotCnt;
}

const uint16 * slot_array::rbegin() const
{
    return this->rend() - this->size();
}

const uint16 * slot_array::rend() const
{
    const char * p = page_head::end(this->head);
    return reinterpret_cast<uint16 const *>(p);
}

uint16 slot_array::operator[](size_t i) const
{
    A_STATIC_ASSERT_TYPE(value_type, uint16);
    SDL_ASSERT(i < size());
    const uint16 * p = this->rend() - (i + 1);
    const uint16 val = *p;
#if 0 //SDL_DEBUG
    if (0) {
        std::cerr 
            << "slot[" << i << "] = 0x"
            << std::hex << val << " ("
            << std::dec << val << ")"
            << std::endl;
        SDL_ASSERT(!"slot_array");
    }
#endif
    return val;
}

std::vector<uint16> slot_array::copy() const
{
    std::vector<uint16> val;
    if (const size_t sz = size()) {
        val.resize(sz);
        for (size_t i = 0; i < sz; ++i) {
            val[i] = (*this)[i];
        }
    }
    return val;
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

                    SDL_ASSERT(IS_LITTLE_ENDIAN);

                    A_STATIC_ASSERT_IS_POD(page_head);

                    static_assert(page_head::page_size == 8 * 1024, "");
                    static_assert(page_head::body_size == 8 * 1024 - 96, "");

                    static_assert(offsetof(page_head, data.headerVersion) == 0, "");
                    static_assert(offsetof(page_head, data.type) == 0x01, "");
                    static_assert(offsetof(page_head, data.typeFlagBits) == 0x02, "");
                    static_assert(offsetof(page_head, data.level) == 0x03, "");
                    static_assert(offsetof(page_head, data.flagBits) == 0x04, "");
                    static_assert(offsetof(page_head, data.indexId) == 0x06, "");
                    static_assert(offsetof(page_head, data.prevPage) == 0x08, "");
                    static_assert(offsetof(page_head, data.pminlen) == 0x0E, "");
                    static_assert(offsetof(page_head, data.nextPage) == 0x10, "");
                    static_assert(offsetof(page_head, data.slotCnt) == 0x16, "");
                    static_assert(offsetof(page_head, data.objId) == 0x18, "");
                    static_assert(offsetof(page_head, data.freeCnt) == 0x1C, "");
                    static_assert(offsetof(page_head, data.freeData) == 0x1E, "");
                    static_assert(offsetof(page_head, data.pageId) == 0x20, "");
                    static_assert(offsetof(page_head, data.reservedCnt) == 0x26, "");
                    static_assert(offsetof(page_head, data.lsn) == 0x28, "");
                    static_assert(offsetof(page_head, data.xactReserved) == 0x32, "");
                    static_assert(offsetof(page_head, data.xdesId) == 0x34, "");
                    static_assert(offsetof(page_head, data.ghostRecCnt) == 0x3A, "");
                    static_assert(offsetof(page_head, data.tornBits) == 0x3C, "");
                    static_assert(offsetof(page_head, data.reserved) == 0x40, "");
                    {
                        typedef page_header_meta T;
                        static_assert(T::headerVersion::offset == 0, "");
                        static_assert(T::type::offset == 1, "");
                        static_assert(std::is_same<T::headerVersion::type, uint8>::value, "");
                        static_assert(std::is_same<T::type::type, pageType>::value, "");
                        static_assert(std::is_same<T::tornBits::type, int32>::value, "");
                    }
                    SDL_ASSERT((page_head::end(nullptr) - page_head::begin(nullptr)) == 8 * 1024);
                    SDL_TRACE_2("sizeof(wchar_t) = ", sizeof(wchar_t)); // can be 2 or 4 bytes
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG

