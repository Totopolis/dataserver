// bitmap_cell.cpp
//
#include "common/common.h"
#include "bitmap_cell.h"
#include <map>

namespace sdl { namespace db {

class bitmap_cell : noncopyable { // prototype, experimental

#pragma pack(push, 1) 

    struct mask_256 {
        static const size_t size = 256;
		uint8 byte[size >> 3];
		void set_bit(uint8 const b) {
			byte[b >> 3] |= 1 << (b & 0x7);
		}
		bool bit(uint8 const b) const {
			return !!(byte[b >> 3] & (1 << (b & 0x7)));
		}
        void fill(uint8 const b) {
            memset_pod(byte, b);
        }
        bool bit_all() const {
            for (uint8 const b : byte) {
                if (!b) return false;
            }
            return true;
        } 
    };
    struct node_type {
        mask_256 m_used;
        mask_256 m_full;
        node_type(): m_used(), m_full(){}
        void set_full() {
            m_used.fill(1);
            m_full.fill(1);
        }
        bool is_full() const {
            return m_full.bit_all();
        }        
    };
	
#pragma pack(pop)

    class allocator : noncopyable {
        using key_type = spatial_cell;
        using map_type = std::map<key_type, node_type>;
        map_type m_map;
    public:
        node_type & operator[](key_type id){
            return m_map[id];
        }
        void erase(key_type id) {
            auto it = m_map.find(id);
            if (it != m_map.end()) {
                m_map.erase(it);
            }
        }
        node_type * find(key_type id) {
            auto it = m_map.find(id);
            if (it != m_map.end()) {
                return &(it->second);
            }
            return nullptr;
        }
    };
    allocator alloc;
public:
	void insert(spatial_cell); 
private:
    void remove_used(node_type const *, spatial_cell);
};

void bitmap_cell::insert(spatial_cell const cell)
{
    if (cell.data.depth == 1) {
        if (node_type * p = alloc.find(cell)) {
            if (!p->is_full()) {
                remove_used(p, cell);
                p->set_full();
            }
        }
        else {
            alloc[cell].set_full();
        }
    }
    else {
        SDL_ASSERT(0);
    }
}

void bitmap_cell::remove_used(node_type const * const node, spatial_cell const cell)
{
    SDL_ASSERT(node);
    SDL_ASSERT(cell.data.depth < 4);
    spatial_cell id = cell;
    ++(id.data.depth);
    for (size_t i = 0; i < mask_256::size; ++i) {
        uint8 const b = static_cast<uint8>(i);
        if (node->m_used.bit(b)) {
            id[cell.data.depth] = b;
            if (id.data.depth < 3) {
                remove_used(alloc.find(id), id);
            }
            alloc.erase(id); // may be optimized
        }
    }
}

} // db
} // sdl

#if 0 //SDL_DEBUG
namespace sdl {
    namespace db {
        namespace {
            class unit_test {
            public:
                unit_test()
                {
                    static_assert(sizeof(bitmap_cell::mask_256) == 256 / 8, "");
                    static_assert(sizeof(bitmap_cell::node_type) == 64, "");
                    if (0)
                    {
    					bitmap_cell test;
	    				test.insert(spatial_cell::min());
		    			test.insert(spatial_cell::max());
                    }
                    if (0)
                    {
                        std::vector<int> data(10);
                        std::make_heap(data.begin(), data.end());
                    }
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG
