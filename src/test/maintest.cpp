// maintest.cpp : Defines the entry point for the test application.
//

#include "common/common.h"
#include "system/page_head.h"
#include "system/page_info.h"
#include "system/database.h"
#include "common/static_type.h"

#if !defined(SDL_DEBUG)
#error !defined(SDL_DEBUG)
#endif

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_CPP11_IS_ENUM
#include "third_party/catch/catch.hpp"

namespace {

using namespace sdl;

template<class T>
int trace_access_and_get_count(T & pa)
{
    int i = 0;
    for (auto & p : pa) {
        ++i;
        SDL_ASSERT(p.get());
    }
    return i;
}

// Catch tutorial: https://github.com/philsquared/Catch/blob/master/docs/tutorial.md

TEST_CASE( "base dataserver test", "" ) {

    SECTION( "create and destroy" ) {
        db::database db("./datasets/set1.mdf");

        if (!db.is_open())
            FAIL( "database failed" );
    }

    SECTION( "check system tables" ) {
        db::database db("./datasets/set1.mdf");

        if (!db.is_open())
            FAIL("database failed");

        REQUIRE( trace_access_and_get_count(db._sysallocunits) == 2 );
        REQUIRE( trace_access_and_get_count(db._sysschobjs) == 35 );
        REQUIRE( trace_access_and_get_count(db._syscolpars) == 16 );
        REQUIRE( trace_access_and_get_count(db._sysidxstats) == 4 );
        REQUIRE( trace_access_and_get_count(db._sysscalartypes) == 1 );
        REQUIRE( trace_access_and_get_count(db._sysobjvalues) == 23 );
        REQUIRE( trace_access_and_get_count(db._sysiscols) == 2 );
    }
}

} //namespace
