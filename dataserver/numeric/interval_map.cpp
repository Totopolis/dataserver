// interval_map.cpp
//
#include "dataserver/numeric/interval_map.h"

#if SDL_DEBUG
namespace sdl { namespace {

class unit_test {
    void test1();
public:
    unit_test() {
        if (0) {
            test1();
        }
    }
};
static unit_test s_test;

void unit_test::test1()
{
	typedef int key_t;
	typedef int val_t;
	typedef interval_map<key_t, val_t> map_type;

	map_type test(-1);
	enum { N = 111 };
	enum { M = 1000 };
	enum { M2 = 1 };
	for (int j = 0; j < M; ++j) {
	for (int j2 = 0; j2 < M2; ++j2)	{
		typedef std::vector<key_t> vector_key_t;
		typedef std::vector<val_t> vector_val_t;
		vector_key_t dkeyBegin, dkeyEnd;
		vector_val_t dval;
		for (int i = 0; i < N; ++i) {
			const key_t keyBegin = rand() % 100;
			const key_t keyEnd = keyBegin + rand() % 100;
			const val_t val = rand() % 100;
			dkeyBegin.push_back(keyBegin);
			dkeyEnd.push_back(keyEnd);
			dval.push_back(val);
			test.assign(keyBegin, keyEnd, val);
		}
        if (!test.map().empty()) {
            const auto end = test.map().end();
		    auto it = test.map().begin();
            ++it;
		    for (; it != end; ++it) {
				auto it2 = it; --it2;
				SDL_ASSERT(it2->second != it->second);
				SDL_ASSERT(it2->first < it->first);
		    }
        }
    }}
}

}} // sdl
#endif //#if SDL_DEBUG
