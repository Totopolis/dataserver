// boot_page.cpp
//
#include "common/common.h"
#include "boot_page.h"
#include "page_info.h"

namespace sdl { namespace db {

static_col_name(recovery_meta, l);
static_col_name(recovery_meta, g);
//
static_col_name(bootpage_row_meta, dbi_version);
static_col_name(bootpage_row_meta, dbi_createVersion);
static_col_name(bootpage_row_meta, dbi_status);
static_col_name(bootpage_row_meta, dbi_nextid);
static_col_name(bootpage_row_meta, dbi_crdate);
static_col_name(bootpage_row_meta, dbi_dbname);
static_col_name(bootpage_row_meta, dbi_dbid);
static_col_name(bootpage_row_meta, dbi_maxDbTimestamp);
static_col_name(bootpage_row_meta, dbi_checkptLSN);
static_col_name(bootpage_row_meta, dbi_differentialBaseLSN);
static_col_name(bootpage_row_meta, dbi_dbccFlags);
static_col_name(bootpage_row_meta, dbi_collation);
static_col_name(bootpage_row_meta, dbi_familyGuid);
static_col_name(bootpage_row_meta, dbi_maxLogSpaceUsed);
static_col_name(bootpage_row_meta, dbi_recoveryForkNameStack);
static_col_name(bootpage_row_meta, dbi_differentialBaseGuid);
static_col_name(bootpage_row_meta, dbi_firstSysIndexes);
static_col_name(bootpage_row_meta, dbi_createVersion2);
static_col_name(bootpage_row_meta, dbi_versionChangeLSN);
static_col_name(bootpage_row_meta, dbi_LogBackupChainOrigin);
static_col_name(bootpage_row_meta, dbi_modDate);
static_col_name(bootpage_row_meta, dbi_verPriv);
static_col_name(bootpage_row_meta, dbi_svcBrokerGUID);
static_col_name(bootpage_row_meta, dbi_AuIdNext);

std::string boot_info::type_raw(bootpage_row const & b)
{
    return to_string::type_raw(b.raw);
}

namespace {

struct to_string_ : to_string {
    using to_string::type; // allow type() methods from base class
    template<size_t N>
    static std::string type(recovery_t const(&d)[N]) {
        static_assert(N == 2, "");
        std::stringstream ss;
        ss << "\n";
        for (size_t i = 0; i < N; ++i) {
            impl::processor<recovery_meta::type_list>::print(ss, d+i,
                identity<to_string_>());
        }
        return ss.str();
    }
};

} // namespace

std::string boot_info::type_meta(bootpage_row const & b)
{
    std::stringstream ss;
    impl::processor<bootpage_row_meta::type_list>::print(ss, &b,
        identity<to_string_>());
    return ss.str();
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
                    SDL_TRACE_FILE;
                    
                    A_STATIC_ASSERT_IS_POD(bootpage_row);
                    
                    static_assert(sizeof(bootpage_row().data.dbi_dbname) == 256, "");
                    static_assert(sizeof(recovery_t) == 28, "");
                    static_assert(sizeof(bootpage_row().raw) > sizeof(bootpage_row().data), "");

                    //--------------------------------------------------------------
                    static_assert(offsetof(bootpage_row, data._0x00) == 0x00, "");
                    static_assert(offsetof(bootpage_row, data._0x08) == 0x08, "");
                    static_assert(offsetof(bootpage_row, data.dbi_status) == 0x24, "");
                    static_assert(offsetof(bootpage_row, data.dbi_nextid) == 0x28, "");
                    static_assert(offsetof(bootpage_row, data.dbi_crdate) == 0x2C, "");
                    static_assert(offsetof(bootpage_row, data.dbi_dbname) == 0x34, "");
                    static_assert(offsetof(bootpage_row, data._0x134) == 0x134, "");
                    static_assert(offsetof(bootpage_row, data._0x13A) == 0x13A, "");
                    static_assert(offsetof(bootpage_row, data._0x140) == 0x140, "");
                    static_assert(offsetof(bootpage_row, data._0x15A) == 0x15A, "");
                    static_assert(offsetof(bootpage_row, data._0x168) == 0x168, "");
                    static_assert(offsetof(bootpage_row, data._0x18C) == 0x18C, "");
                    static_assert(offsetof(bootpage_row, data._0x1AC) == 0x1AC, "");
                    static_assert(offsetof(bootpage_row, data._0x20C) == 0x20C, "");
                    static_assert(offsetof(bootpage_row, data._0x222) == 0x222, "");
                    static_assert(offsetof(bootpage_row, data._0x28A) == 0x28A, "");
                    static_assert(offsetof(bootpage_row, data._0x2B0) == 0x2B0, "");
                    static_assert(offsetof(bootpage_row, data._0x2C4) == 0x2C4, "");
                    static_assert(offsetof(bootpage_row, data._0x2E8) == 0x2E8, "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG



