// export_database.cpp
//
#include "common/common.h"
#include "export_database.h"
#include "generator_util.h"
#include <fstream>
#include <map>
#include <set>

namespace sdl { namespace db { namespace make { namespace {

const char INSERT_TEMPLATE[] = R"(
SET IDENTITY_INSERT %s{TABLE_DEST} ON;
GO
INSERT INTO %s{TABLE_DEST} (%s{COL_TEMPLATE})
       SELECT %s{COL_TEMPLATE}
       FROM %s{TABLE_SRC};
SET IDENTITY_INSERT %s{TABLE_DEST} OFF;
GO
)";

const char TABLE_TEMPLATE[] = R"(%s{database}.dbo.%s{table})";

} // namespace 

template<class T>
bool export_database::read_input(T & result, std::string const & in_file)
{
    SDL_ASSERT(result.empty());    
    if (!in_file.empty()) {

        std::ifstream infile(in_file);
        if (infile.rdstate() & std::ifstream::failbit) {
            throw_error<export_database_error>("export_database: error opening in_file");
            return false;
        }
        {
            std::string s;
            do {
                std::getline(infile, s);
                if (s.empty())
                    break;
                const size_t i = s.find(',');
                if (i != std::string::npos) {
                    std::string const table_name = s.substr(0, i);
                    std::string const col_name = s.substr(i + 1);
                    if (!table_name.empty() && !col_name.empty()) {
                        if (!result[table_name].insert(col_name).second) {
                            SDL_ASSERT(0);
                        }
                    }
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

bool export_database::write_output(std::string const & out_file, std::string const & script)
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


template<class T>
std::string export_database::make_script(T const & tables, table_source_dest const & param)
{
    SDL_ASSERT(!param.first.empty());
    SDL_ASSERT(!param.second.empty());
    return "TODO";
}

bool export_database::make_file(std::string const & in_file, std::string const & out_file, table_source_dest const & param)
{
    using set_column = std::set<std::string>;
    using map_table = std::map<std::string, set_column>;
    map_table tables;
    if (read_input(tables, in_file)) {
        std::string const s = make_script(tables, param);
        if (!s.empty()) {
            return write_output(out_file, s);
        }
    }
    return false;
}

} // make
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
                    if (1) {
                        std::string const in_file = "D:/UNIT_TEST/input.csv";
                        std::string const out_file = "D:/UNIT_TEST/output.sql"; 
                        SDL_ASSERT(make::export_database::make_file(in_file, out_file, {"source", "dest"}));
                    }
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG

#if 0
------------------------------------------------------------------
//all_table_names.sql

USE [database]
GO 

SET NOCOUNT ON;

SELECT TABLE_NAME, COLUMN_NAME
FROM INFORMATION_SCHEMA.COLUMNS
INNER JOIN sys.Tables
ON INFORMATION_SCHEMA.COLUMNS.TABLE_NAME=sys.Tables.name
WHERE sys.Tables.schema_id = 1

------------------------------------------------------------------
//all_table_names.bat

sqlcmd -i all_table_names.sql -o output.csv -h -1 -s "," -W -m 1
------------------------------------------------------------------
#endif