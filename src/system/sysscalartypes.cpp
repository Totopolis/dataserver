// sysscalartypes.cpp
//
#include "common/common.h"
#include "sysscalartypes.h"
#include "page_info.h"

namespace sdl { namespace db { namespace {

const scalartype SCALARTYPE[] = {
    scalartype::t_image             
    ,scalartype::t_text              
    ,scalartype::t_uniqueidentifier  
    ,scalartype::t_date              
    ,scalartype::t_time              
    ,scalartype::t_datetime2         
    ,scalartype::t_datetimeoffset    
    ,scalartype::t_tinyint           
    ,scalartype::t_smallint          
    ,scalartype::t_int               
    ,scalartype::t_smalldatetime      
    ,scalartype::t_real               
    ,scalartype::t_money              
    ,scalartype::t_datetime           
    ,scalartype::t_float              
    ,scalartype::t_sql_variant        
    ,scalartype::t_ntext             
    ,scalartype::t_bit                
    ,scalartype::t_decimal            
    ,scalartype::t_numeric            
    ,scalartype::t_smallmoney         
    ,scalartype::t_bigint            
    ,scalartype::t_hierarchyid       
    ,scalartype::t_geometry          
    ,scalartype::t_geography         
    ,scalartype::t_varbinary         
    ,scalartype::t_varchar           
    ,scalartype::t_binary            
    ,scalartype::t_char              
    ,scalartype::t_timestamp         
    ,scalartype::t_nvarchar          
    ,scalartype::t_nchar             
    ,scalartype::t_xml               
    ,scalartype::t_sysname            
};

} // namespace

static_col_name(sysscalartypes_row_meta, head);
static_col_name(sysscalartypes_row_meta, id);
static_col_name(sysscalartypes_row_meta, schid);
static_col_name(sysscalartypes_row_meta, xtype);
static_col_name(sysscalartypes_row_meta, length);
static_col_name(sysscalartypes_row_meta, prec);
static_col_name(sysscalartypes_row_meta, scale);
static_col_name(sysscalartypes_row_meta, collationid);
static_col_name(sysscalartypes_row_meta, status);
static_col_name(sysscalartypes_row_meta, created);
static_col_name(sysscalartypes_row_meta, modified);
static_col_name(sysscalartypes_row_meta, dflt);
static_col_name(sysscalartypes_row_meta, chk);
static_col_name(sysscalartypes_row_meta, name);

std::string sysscalartypes_row_info::type_meta(sysscalartypes_row const & row)
{
    std::stringstream ss;
    impl::processor<sysscalartypes_row_meta::type_list>::print(ss, &row, 
        impl::identity<to_string_with_head>());
    return ss.str();
}

std::string sysscalartypes_row_info::type_raw(sysscalartypes_row const & row)
{
    return to_string::type_raw(row.raw);
}

std::string sysscalartypes_row_info::col_name(sysscalartypes_row const & row)
{
    return to_string::type_nchar(row.data.head, sysscalartypes_row_meta::name::offset);
}

std::string sysscalartypes_row::col_name() const
{
    return sysscalartypes_row_info::col_name(*this);
}

scalartype sysscalartypes_row::to_scalartype(uint32 const t)
{
    static_assert(A_ARRAY_SIZE(SCALARTYPE) == 34, "");
    for (auto s : SCALARTYPE) {
        if (s == static_cast<scalartype>(t)) {
            return s;
        }
    }
    SDL_ASSERT(0);
    return scalartype::t_none;
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
                    A_STATIC_ASSERT_IS_POD(sysscalartypes_row);
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG