// export_database.cpp
//
#include "common/common.h"
#include "export_database.h"
#include "generator_util.h"
#include <fstream>
#include <map>

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

struct export_types: is_static {
    using map_column = std::map<int, std::string>;
    using map_table = std::map<std::string, map_column>;
};

} // namespace 

template<class T>
bool export_database::read_input(T & result, std::string const & in_file)
{
    A_STATIC_ASSERT_TYPE(export_types::map_table, T);
    SDL_ASSERT(result.empty());    
    if (!in_file.empty()) {

        std::ifstream infile(in_file);
        if (infile.rdstate() & std::ifstream::failbit) {
            throw_error<export_database_error>("export_database: error opening in_file");
            return false;
        }
        {
            std::string s; // s = TABLE_NAME, COLUMN_NAME, ORDINAL_POSITION
            do {
                std::getline(infile, s);
                if (s.empty())
                    break;
                SDL_ASSERT(s.find(' ') == std::string::npos);
                bool inserted = false;
                size_t const i = s.find(',');
                if (i != std::string::npos) {
                    size_t const j = s.find(',', i + 1);
                    if (j != std::string::npos) {
                        std::string const table_name = s.substr(0, i);
                        std::string const col_name = s.substr(i + 1, j - i - 1);
                        if (!table_name.empty() && !col_name.empty()) {
                            int const pos = atoi(s.substr(j + 1).c_str());
                            if (pos > 0) {
                                result[table_name][pos] = col_name;
                                inserted = true;
                            }
                        }
                    }
                }
                if (!inserted) {
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
std::string export_database::make_script(T const & tables, param_type const & param)
{
    A_STATIC_ASSERT_TYPE(export_types::map_table, T);
    std::string result;
    for (auto const & tab : tables) {
        std::string s(INSERT_TEMPLATE);
        {
            std::string tab_dest(TABLE_TEMPLATE);
            std::string tab_source(TABLE_TEMPLATE);
            replace(tab_source, "%s{database}", param.source);
            replace(tab_dest, "%s{database}", param.dest);
            replace(tab_source, "%s{table}", tab.first);
            replace(tab_dest, "%s{table}", tab.first);
            replace(s, "%s{TABLE_DEST}", tab_dest);
            replace(s, "%s{TABLE_SRC}", tab_source);
        }
        {
            size_t i = 0;
            std::string col_names;
            for (auto const & col : tab.second) {
                if (i++) { 
                    col_names += ", ";
                }
                col_names += col.second;
                SDL_ASSERT(col.first == i);
            }
            replace(s, "%s{COL_TEMPLATE}", col_names);
        }
        result += s;
    }
    SDL_ASSERT(result.find("%s{") == std::string::npos);
    return result;
}

bool export_database::make_file(param_type const & p)
{
    export_types::map_table tables;
    if (read_input(tables, p.in_file)) {
        std::string const s = make_script(tables, p);
        if (!s.empty()) {
            return write_output(p.out_file, s);
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
#endif //#if SV_DEBUG

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