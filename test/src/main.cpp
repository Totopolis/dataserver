#include "common/common.h"
#include "common/static_type.h"
#include "system/page_head.h"
#include "system/page_info.h"
#include "system/database.h"

//#include "set1db.h"

// generated from MDF
#include "dstest.h"

// must be included last
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

/*
#if !defined(SDL_DEBUG)
#error !defined(SDL_DEBUG)
#endif*/

using namespace std;

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

/*
TEST_CASE("base test", "") {
    if (false) {
        SECTION("create and destroy") {
            db::database db("./datasets/set1.mdf");

            if (!db.is_open())
                FAIL("database failed");
        }

        SECTION("check system tables") {
            db::database db("./datasets/set1.mdf");

            if (!db.is_open())
                FAIL("database failed");

            REQUIRE(trace_access_and_get_count(db._sysallocunits) == 2);
            REQUIRE(trace_access_and_get_count(db._sysschobjs) == 35);
            REQUIRE(trace_access_and_get_count(db._syscolpars) == 16);
            REQUIRE(trace_access_and_get_count(db._sysidxstats) == 4);
            REQUIRE(trace_access_and_get_count(db._sysscalartypes) == 1);
            REQUIRE(trace_access_and_get_count(db._sysobjvalues) == 23);
            REQUIRE(trace_access_and_get_count(db._sysiscols) == 2);
        }
    }
}

TEST_CASE("queries test", "") {
    if (false) {
        SECTION("simple select") {
            db::database db("./datasets/set1.mdf");

            if (!db.is_open())
                FAIL("database failed");

            using namespace sdl::db::make::set1;

            if (auto ptab = db.make_table<dbo_DataRows>()) {
                auto & tab = *ptab;
                using namespace sdl::db::make::where_;
                using T = dbo_DataRows;
                T::query_type::record_range range;
                if (1) {
                    range = tab->SELECT
                        | TOP{ 10 }
                        | BETWEEN<T::col::ID>{1, 200}
                    //&& NOT<T::col::ID>{2}
                    && ORDER_BY<T::col::ID>{}
                    ;
                }

                std::cout << "SELECT => " << range.size() << "\n";

                if (1) {
                    size_t i = 0;
                    for (auto p : range) {
                        std::cout << "SELECT[" << i++ << "] => Id=" << p.ID()
                            << " Col1=" << p.type_col<T::col::Col1>()
                            << " Col2=" << p.type_col<T::col::Col2>()
                            << " Col3=" << p.type_col<T::col::Col3>()
                            << "\n";
                    }
                }

            }//if table
        }//SECTION
    }
} // TEST_CASE
*/

TEST_CASE("Check which T-SQL data types are supported", "[SupportedTypes][supported]") {
    INFO("Open dstest.mdf");
    db::database db("dstest.mdf");
    REQUIRE(db.is_open());

    SECTION("create table") {
        using namespace sdl::db::make::where_;
        using namespace sdl::db::make::dstest;
        using T = sdl::db::make::dstest::dbo_SupportedTypes;

        auto ptab = db.make_table<T>();
        INFO("Create object for table SupportedTypes");
        REQUIRE(ptab);
        auto & tab = *ptab;

        T::query_type::record_range range;

        SECTION("check table is empty") {
            // select * from dbo.SupportedTypes
            range = tab->SELECT
                | IF([](const auto &) { return true; })
                ;
            INFO("SupportedTypes table must be empty");
            CHECK(range.empty());
        }
    }
}

TEST_CASE("Table1: simple queries", "[Table1][SimpleQueries]") {
    INFO("Open dstest.mdf");
    db::database db("dstest.mdf");
    REQUIRE(db.is_open());

    SECTION("SELECT * from dbo.Table1") {
        using namespace sdl::db::make::where_;
        using namespace sdl::db::make::dstest;
        using T = sdl::db::make::dstest::dbo_Table1;

        auto ptab = db.make_table<T>();
        INFO("Create object for table: dbo.Table1");
        REQUIRE(ptab);
        auto & tab = *ptab;

        T::query_type::record_range range;

        /*
        enum class condition {
        WHERE,          // 0
        IN,             // 1
        NOT,            // 2
        LESS,           // 3
        GREATER,        // 4
        LESS_EQ,        // 5
        GREATER_EQ,     // 6
        BETWEEN,        // 7
        lambda,         // 8
        order,          // 9
        top,            // 10
        _end
        };

        TOP
        BETWEEN
        NOT
        ORDER_BY
        */

        SECTION("using GREATER") {
            range = tab->SELECT
                | GREATER<T::col::Id>(0)
                ;
            INFO("dbo.Table1 size must be 10");
            CHECK(range.size() == 10);
        }

        SECTION("using LAMBDA") {
            range = tab->SELECT
                | IF([](const auto &) { return true; })
                ;
            INFO("dbo.Table1 size must be 10");
            CHECK(range.size() == 10);
        }
    }
#if 0
            SECTION("SELECT using LAMBDA") {
                range = tab->SELECT
                    | IF([](const auto &r) { return r.Id() == 5; })
                    ;
                REQUIRE(range.size() == 1);

                //CAPTURE(it->type_col<T::col::str255>());
                //cout << it->type_col<T::col::str255>() << endl;
                //cout << it->str255() << endl;
                //CHECK(it->type_col<T::col::str255>() == "five");
            }
#endif
} // TEST_CASE

TEST_CASE("Table2: working with NULL data and empty strings", "[Table2][Null]") {
    INFO("Open dstest.mdf");
    db::database db("dstest.mdf");
    REQUIRE(db.is_open());

    SECTION("Table2") {
        using namespace sdl::db::make::where_;
        using namespace sdl::db::make::dstest;
        using T = sdl::db::make::dstest::dbo_Table2;

        auto ptab = db.make_table<T>();
        INFO("Create object for table: dbo.Table2");
        REQUIRE(ptab);
        auto & tab = *ptab;

        T::query_type::record_range range;

        SECTION("queries") {
            SECTION("SELECT using LAMBDA") {
                range = tab->SELECT
                    | IF([](const auto &r) { return true; })
                    ;
                CHECK(range.size() == 3);

                for (const auto &record : range) {
                    cout
                        << "Id=" << record.Id()

                        << " c_char=" << record.c_char() << " (" << record.type_col<T::col::c_char>() << ')'
                        << " c_vchar=" << record.type_col<T::col::c_vchar>()
                        //<< " c_vchar=" << record.c_vchar()
                        << " c_text=" << record.type_col<T::col::c_text>()
                        //<< " c_text=" << record.c_text()
                        << '\n';
                }

                //CAPTURE(it->type_col<T::col::str255>());
                //cout << it->type_col<T::col::str255>() << endl;
                //cout << it->str255() << endl;
                //CHECK(it->type_col<T::col::str255>() == "five");
            }
        }
    } // SECTION 

#if 0
    SECTION("Table3") {
        using namespace sdl::db::make::where_;
        using namespace sdl::db::make::dstest;
        using T = sdl::db::make::dstest::dbo_Table3;

        auto ptab = db.make_table<T>();
        INFO("Create object for table: dbo.Table3");
        REQUIRE(ptab);
        auto & tab = *ptab;

        T::query_type::record_range range;

        SECTION("queries") {
            SECTION("SELECT using LAMBDA") {
                range = tab->SELECT
                    | IF([](const auto &r) { return r.Id() == 1; })
                    ;
                REQUIRE(range.size() == 1);

                auto record = range.begin();

                cout << "c_char=" << record->c_char() << " (" << record->type_col<T::col::c_char>() << ")\n";
            }
        }
    } // SECTION
#endif
} // TEST_CASE

} //namespace
