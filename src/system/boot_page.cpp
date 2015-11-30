// boot_page.cpp
//
#include "common/common.h"
#include "boot_page.h"
#include "page_info.h"
#include <sstream>

namespace sdl { namespace db {

std::string boot_info::type_raw(bootpage_row const & b)
{
    return to_string::type_raw(b.raw);
}

//FIXME: to be tested
std::string boot_info::type(bootpage_row const & b)
{
    typedef to_string S;
    const auto & d = b.data;
    std::stringstream ss;
    ss
        << "\ndbi_version = " << d.dbi_version
        << "\ndbi_createVersion = " << d.dbi_createVersion
        << "\ndbi_status = " << d.dbi_status
        << "\ndbi_nextid = " << d.dbi_nextid
        << "\ndbi_crdate = " << S::type(d.dbi_crdate) // (" << d.dbi_crdate.d1 << "," << d.dbi_crdate.d2 << ")"
        << "\ndbi_dbname = " << S::type(d.dbi_dbname)
        << "\ndbi_dbid = " << d.dbi_dbid
        << "\ndbi_maxDbTimestamp = " << d.dbi_maxDbTimestamp
        << "\ndbi_checkptLSN = " << S::type(d.dbi_checkptLSN)
        << "\ndbi_differentialBaseLSN = " << S::type(d.dbi_differentialBaseLSN)
        << "\ndbi_dbccFlags = " << d.dbi_dbccFlags
        << "\ndbi_collation = " << d.dbi_collation
        << "\ndbi_familyGuid = " << S::type(d.dbi_familyGuid)
        << "\ndbi_maxLogSpaceUsed = " << d.dbi_maxLogSpaceUsed
        << "\ndbi_recoveryForkNameStack = ?"
        << "\ndbi_differentialBaseGuid = " << S::type(d.dbi_differentialBaseGuid)
        << "\ndbi_firstSysIndexes = " << S::type(d.dbi_firstSysIndexes)
        << "\ndbi_createVersion2 = " << d.dbi_createVersion2
        << "\ndbi_versionChangeLSN = " << S::type(d.dbi_versionChangeLSN)
        << "\ndbi_LogBackupChainOrigin = " << S::type(d.dbi_LogBackupChainOrigin)
        << "\ndbi_modDate = ?"
        << "\ndbi_verPriv = ?"
        << "\ndbi_svcBrokerGUID = " << S::type(d.dbi_svcBrokerGUID)
        << "\ndbi_AuIdNext = ?"
    << std::endl;
    auto s = ss.str();
    return s;
}

std::string boot_info::type_meta(bootpage_row const & b)
{
    std::stringstream ss;
    impl::processor<bootpage_row_meta::type_list>::print(ss, &b);
    auto s = ss.str();
    return s;
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
                    
                    A_STATIC_ASSERT_IS_POD(bootpage_row);
                    
                    static_assert(sizeof(bootpage_row().data.dbi_dbname) == 256, "");
                    static_assert(sizeof(recovery_t) == 28, "");
                    static_assert(sizeof(bootpage_row().raw) > sizeof(bootpage_row().data), "");

                    //--------------------------------------------------------------
                    // http://stackoverflow.com/questions/21201888/how-to-make-wchar-t-16-bit-with-clang-for-linux-x64
                    // static_assert(sizeof(wchar_t) == 2, "wchar_t"); Note. differs on 64-bit Clang 
                    static_assert(offsetof(bootpage_row, data._0x0) == 0x0, "");
                    static_assert(offsetof(bootpage_row, data._0x8) == 0x8, "");
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
                    //--------------------------------------------------------------
                    SDL_TRACE_2("sizeof(bootpage_row::data_type) = ", sizeof(bootpage_row::data_type));
                    SDL_TRACE_2("sizeof(bootpage_row) = ", sizeof(bootpage_row));
                    {
                        //typedef bootpage_row_meta T;
                        //static_assert(TL::Length<T::type_list>::value == 2);
                    }
                    SDL_TRACE_2(__FILE__, " end");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG



