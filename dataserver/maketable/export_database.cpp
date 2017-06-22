// export_database.cpp
//
#include "dataserver/maketable/export_database.h"
#include "dataserver/maketable/generator_util.h"
#include "dataserver/common/algorithm.h"
#include <fstream>
#include <map>

#if defined(SDL_OS_WIN32)
#pragma warning(disable: 4503) //decorated name length exceeded, name was truncated
#endif

namespace sdl { namespace db { namespace make { namespace {

const char INSERT_TEMPLATE[] = R"(
print 'INSERT INTO %s{TABLE_DEST}...'
GO
SET IDENTITY_INSERT %s{TABLE_DEST} ON;
GO
INSERT INTO %s{TABLE_DEST} (%s{COL_TEMPLATE_DEST})
       SELECT %s{COL_TEMPLATE_SRC}
       FROM %s{TABLE_SRC};
SET IDENTITY_INSERT %s{TABLE_DEST} OFF;
GO
)";

const char TABLE_TEMPLATE[] = R"(%s{database}.%s{dbo}.%s{table})";

const char CREATE_SPATIAL_INDEX[] = R"(
CREATE SPATIAL INDEX %s{index} ON %s{table}
(
	%s{geography}
)USING  GEOGRAPHY_GRID 
WITH (GRIDS =(LEVEL_1 = HIGH,LEVEL_2 = HIGH,LEVEL_3 = HIGH,LEVEL_4 = HIGH), 
CELLS_PER_OBJECT = 8192, PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, SORT_IN_TEMPDB = OFF, DROP_EXISTING = OFF, ONLINE = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
GO
)";

const char SPATIAL_TEMPLATE[] = R"(SPATIAL_%s{dbo}_%s{table})";

const char SELECT_NOT_VALID_GEOGRAPHY[] = R"(
print 'CHECK %s{database}.%s{dbo}.%s{table}...'
GO
SELECT * FROM %s{database}.%s{dbo}.%s{table}
WHERE %s{geography}.STIsValid() = 0
GO
)";

//-----------------------------------------------------------

struct export_types: is_static {
    using map_column = std::map<int, std::string>;
    using map_table = std::map<std::string, map_column>;
    using map_schema = std::map<std::string, map_table>;
};

using export_database_error = sdl_exception_t<export_database>;

bool export_read_input(export_types::map_schema & result, std::string const & in_file)
{
    SDL_ASSERT(result.empty());    
    if (!in_file.empty()) {

        std::ifstream infile(in_file);
        if (infile.rdstate() & std::ifstream::failbit) {
            throw_error<export_database_error>("export_database: error opening in_file");
            return false;
        }
        {
            const std::string dbo("dbo");
            std::string s; // s = TABLE_NAME, COLUMN_NAME, ORDINAL_POSITION [,SCHEMA_NAME]
            do {
                std::getline(infile, s);
                if (s.empty())
                    break;
                SDL_ASSERT(s.find(' ') == std::string::npos);
                const auto col = util::split(s, ',');
                if (col.size() > 2) {
                    const auto & table_name = col[0];
                    const auto & col_name = col[1];
                    const auto & sch_name = (col.size() > 3) ? col[3] : dbo;
                    const auto pos = atoi(col[2].c_str());
                    result[sch_name][table_name][pos] = col_name;
                }
                else {
                    SDL_ASSERT(0);
                    break;
                }
            } while (!(infile.rdstate() & std::ios_base::eofbit));
        }
        infile.close();        
        return !result.empty();
    }
    return false;
}

bool export_write_output(std::string const & out_file, std::string const & script)
{
    SDL_ASSERT(!script.empty());
    if (!out_file.empty()) {
        std::ofstream outfile(out_file, std::ofstream::out|std::ofstream::trunc);
        if (outfile.rdstate() & std::ifstream::failbit) {
            throw_error<export_database_error>("export_database: error opening out_file");
            return false;
        }
        outfile << script;
        outfile.close();
        SDL_TRACE("File created : ", out_file);
        return true;
    }
    return false;
}

bool find_col(export_types::map_column const & tab, std::string const & col_name) {
    for (auto const & col : tab) {
        if (col.second == col_name) {
            return true;
        }
    }
    return false;
}

std::string export_check_geography(
    export_types::map_schema const & input,
    export_database::param_type const & param)
{
    std::string result;
    if (!param.geography.empty()) {
        for (auto const & schema : input) {
        for (auto const & tab : schema.second) {
            if (find_col(tab.second, param.geography)) {
                std::string s(SELECT_NOT_VALID_GEOGRAPHY);
                replace(s, "%s{database}", param.source);
                replace(s, "%s{dbo}", schema.first);
                replace(s, "%s{table}", tab.first);
                replace(s, "%s{geography}", param.geography);
                result += s;
            }
        }}
    }
    SDL_ASSERT(result.find("%s{") == std::string::npos);
    return result;
}

std::string export_make_spatial_index(
    export_types::map_schema const & input,
    export_database::param_type const & param)
{
    std::string result;
    if (!param.geography.empty()) {
        for (auto const & schema : input) {
        for (auto const & tab : schema.second) {
            if (find_col(tab.second, param.geography)) {
                std::string s(CREATE_SPATIAL_INDEX);
                {
                    std::string s_index(SPATIAL_TEMPLATE);
                    replace(s_index, "%s{dbo}", algo::to_upper(schema.first));
                    replace(s_index, "%s{table}", algo::to_upper(tab.first));
                    replace(s, "%s{index}", s_index);
                }
                {
                    std::string s_table(TABLE_TEMPLATE);
                    replace(s_table, "%s{database}", param.dest);
                    replace(s_table, "%s{dbo}", schema.first);
                    replace(s_table, "%s{table}", tab.first);
                    replace(s, "%s{table}", s_table);
                }
                replace(s, "%s{geography}", param.geography);
                result += s;
            }
        }}
    }
    SDL_ASSERT(result.find("%s{") == std::string::npos);
    return result;
}

std::string export_make_script(
    export_types::map_schema const & input,
    export_database::param_type const & param)
{
    std::string result;
    result += export_check_geography(input, param);
    if (param.create_spatial_index) {
        result += export_make_spatial_index(input, param);
    }
    for (auto const & schema : input) {
        for (auto const & tab : schema.second) {
            std::string s(INSERT_TEMPLATE);
            {
                std::string tab_source(TABLE_TEMPLATE);
                replace(tab_source, "%s{database}", param.source);
                replace(tab_source, "%s{table}", tab.first);
                replace(tab_source, "%s{dbo}", schema.first);
                replace(s, "%s{TABLE_SRC}", tab_source);
            }
            {
                std::string tab_dest(TABLE_TEMPLATE);
                replace(tab_dest, "%s{database}", param.dest);
                replace(tab_dest, "%s{table}", tab.first);
                replace(tab_dest, "%s{dbo}", schema.first);
                replace(s, "%s{TABLE_DEST}", tab_dest);
            }
            {
                const bool is_geo = find_col(tab.second, param.geography);
                size_t i = 0;
                std::string col_names_dest, col_names_src;
                for (auto const & col : tab.second) {
                    if (i++) {
                        col_names_dest += ", ";
                        col_names_src += ", ";
                    }
                    if (is_geo) {
                        if (col.second == "ST_AREA") {
                            col_names_src += param.geography;
                            col_names_src += ".STArea() as ST_AREA";
                        }
                        else if (col.second == "ST_LENGTH") {
                            col_names_src += param.geography;
                            col_names_src += ".STLength() as ST_LENGTH";
                        }
                        else {
                            col_names_src += col.second;
                        }
                    }
                    else {
                        col_names_src += col.second;
                    }
                    col_names_dest += col.second;
                    SDL_ASSERT(static_cast<size_t>(col.first) == i);
                }
                replace(s, "%s{COL_TEMPLATE_DEST}", col_names_dest);
                replace(s, "%s{COL_TEMPLATE_SRC}", col_names_src);
            }
            result += s;
        }
    }
    SDL_ASSERT(result.find("%s{") == std::string::npos);
    return result;
}

} // namespace

bool export_database::make_file(param_type const & p)
{
    if (p.empty()) {
        return false;
    }
    export_types::map_schema input;
    if (export_read_input(input, p.in_file)) {
        std::string const s = export_make_script(input, p);
        if (!s.empty()) {
            return export_write_output(p.out_file, s);
        }
    }
    return false;
}

} // make
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
                    if (0) {
                        using T = make::export_database;
                        T::param_type p;
                        p.in_file = "D:/UNIT_TEST/input.csv";
                        p.out_file = "D:/UNIT_TEST/output.sql"; 
                        p.source = "source";
                        p.dest = "dest";
                        SDL_ASSERT(T::make_file(p));
                    }
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SDL_DEBUG

#if 0
------------------------------------------------------------------
//all_table_names.sql

USE [database]
GO 

SET NOCOUNT ON;

SELECT TABLE_NAME, COLUMN_NAME, ORDINAL_POSITION
FROM INFORMATION_SCHEMA.COLUMNS
INNER JOIN sys.Tables
ON INFORMATION_SCHEMA.COLUMNS.TABLE_NAME=sys.Tables.name
WHERE sys.Tables.schema_id = 1

------------------------------------------------------------------
//all_table_names.bat

sqlcmd -i all_table_names.sql -o output.csv -h -1 -s "," -W -m 1
------------------------------------------------------------------
#endif