// bitmap_cell.cpp
//
#include "common/common.h"
#include "bitmap_cell.h"
#include <map>

namespace sdl { namespace db {

struct bitmap_cell : noncopyable { //FIXME: not implemented

	A_STATIC_ASSERT_TYPE(uint8, spatial_cell::id_type);

#pragma pack(push, 1) 

    struct mask_256 {
		uint8 byte[32];
		void set_bit(uint8 const b) {
			byte[b >> 3] |= 1 << (b & 0x7);
		}
		bool bit(uint8 const b) const {
			return !!(byte[b >> 3] & (1 << (b & 0x7)));
		}
    };

    struct node_type {
        mask_256 cell_id;
    };
	
#pragma pack(pop)

public:
	void insert(spatial_cell);
};


void bitmap_cell::insert(spatial_cell const cell) {
	SDL_ASSERT(cell.data.depth);
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
                    static_assert(sizeof(bitmap_cell::mask_256) == 256 / 8, "");
					bitmap_cell test;
					test.insert(spatial_cell::min());
					test.insert(spatial_cell::max());
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG
