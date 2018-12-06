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
    SDL_TRACE_FUNCTION;

	typedef int key_t;
	typedef int val_t;
	typedef interval_map<key_t, val_t> map_type;

	map_type test(-1);
	enum { N = 111 };
	enum { M = 1000 };
	enum { M2 = 1 };
	for (int j = 0; j < M; ++j)
	for (int j2 = 0; j2 < M2; ++j2)
	{
		typedef std::vector<key_t> vector_key_t;
		typedef std::vector<val_t> vector_val_t;
		vector_key_t dkeyBegin, dkeyEnd;
		vector_val_t dval;
		const bool print = (0 == j) && (0 == j2);
		if (print) { 
			std::cout << "assign:\n";
		}
		for (int i = 0; i < N; ++i)
		{
			const key_t keyBegin = rand() % 100;
			const key_t keyEnd = keyBegin + rand() % 100;
			const val_t val = rand() % 100;
			dkeyBegin.push_back(keyBegin);
			dkeyEnd.push_back(keyEnd);
			dval.push_back(val);
			if (print) {
				std::cout << i << ": (" << keyBegin << "," << keyEnd << "," << val << ")\n";
			}
			test.assign(keyBegin, keyEnd, val);
		}
		if (print) {
			std::cout << "trace map:\n";
		}
		auto it = test.map().begin();
		for (; it != test.map().end(); ++it) {
			if (print) {
				std::cout
					<< "key = " << it->first << ", "
					<< "val = " << it->second << "\n";
			}
			if (it != test.map().begin()) {
				auto it2 = it; --it2;
				if (it2->second == it->second) // error !
				{
					map_type test2(-1);
					for (size_t k = 0; k < dkeyBegin.size(); ++k)
					{
						std::cout << k << ": (" << dkeyBegin[k] << "," << dkeyEnd[k] << "," << dval[k] << ")\n";
						test2.assign(dkeyBegin[k], dkeyEnd[k], dval[k]);
					}
				}
				assert(it2->second != it->second);
				assert(it2->first < it->first);
			}
		}
	}
}

}} // sdl
#endif //#if SDL_DEBUG
