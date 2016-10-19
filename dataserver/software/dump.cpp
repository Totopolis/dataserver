// main.cpp : Defines the entry point for the console application.
// main.cpp is used for tests and research only and is not part of the dataserver library.
#include "common/common.h"
#include "system/database.h"
#include "system/version.h"
#include "third_party/cmdLine/cmdLine.h"
#include "common/outstream.h"
#include "common/locale.h"
#include "common/time_util.h"
#include "utils/conv.h"
#include "maketable/generator_util.h"
#include <fstream>
#include <cstdlib> // atof
#include <iomanip> // for std::setprecision

namespace {

using namespace sdl;

struct cmd_option : noncopyable {
    using vector_string = std::vector<std::string>;
    std::string mdf_file;
    std::string out_file;
    bool silence = false;
    int record_num = 10;
    int verbosity = 0;
    std::string col_name;
    std::string tab_name;
    std::string include;
    std::string exclude;
    vector_string includes;
    vector_string excludes;
    int precision = 0;
    bool trim_space = false;
};

void trace_table_record(db::database const &,
                        db::datatable const &,
                        db::datatable::record_type const & record,
                        cmd_option const & opt,
                        size_t const row_index)
{
    std::cout << row_index;
    for (size_t i = 0; i < record.size(); ++i) {
        auto const & col = record.usercol(i);
        if (opt.col_name.empty() || (opt.col_name == col.name)) {
            std::cout << ",";
            if (opt.trim_space) {
                std::wcout << db::to_string::trim(record.type_col_wide(i));
            }
            else {
                std::wcout << record.type_col_wide(i);
            }
        }
    }
    std::cout << std::endl;
}

void trace_schema(db::database const & db, db::datatable const & table, cmd_option const & opt)
{
    std::cout << "\n[" << db.dbi_dbname() << "].[" << table.name() << "]" << std::endl;
    std::cout << "#";
    for (auto const & col : table.ut()) {
        A_STATIC_CHECK_TYPE(db::usertable::column_ref, col);
        if (opt.col_name.empty() || (opt.col_name == col.name)) {
            std::cout << "," << col.name;
        }
    }
    std::cout << std::endl;
}

void trace_datatable(db::database const & db, db::datatable const & table, cmd_option const & opt, bool const is_internal)
{
    trace_schema(db, table, opt);
    size_t row_index = 0;
    for (auto const record : table._record) {
        if ((opt.record_num != -1) && ((int)row_index >= opt.record_num))
            break;
        trace_table_record(db, table, record, opt, row_index++);
    }
}

void trace_exclude(db::datatable const & table)
{
    std::cout << "exclude: " << table.name() << std::endl;
}

void trace_datatables(db::database const & db, cmd_option const & opt)
{
    for (auto const & tt : db._datatables) {
        db::datatable const & table = *tt.get();
        if (!opt.tab_name.empty() && (table.name() != opt.tab_name)) {
            trace_exclude(table);
            continue;
        }
        if (!opt.excludes.empty() && db::make::util::is_find(opt.excludes, table.name())) {
            trace_exclude(table);
            continue;
        }
        if (!opt.includes.empty() && !db::make::util::is_find(opt.includes, table.name())) {
            trace_exclude(table);
            continue;
        }
        trace_datatable(db, table, opt, false);
    }
}

void print_version()
{
#if SDL_DEBUG
    std::cout << "\nSDL_DEBUG=1\n";
#else
    std::cout << "\nSDL_DEBUG=0\n";
#endif
    std::cout << __DATE__ << " " << __TIME__ << std::endl;
    std::cout << SDL_DATASERVER_VERSION << std::endl;
}

void print_help(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    std::cout
        << "\nBuild date: " << __DATE__
        << "\nBuild time: " << __TIME__
        << "\nUsage: " << argv[0]
        << "\n[-i|--mdf_file] path to mdf file"
        << "\n[-o|--out_file] path to output files"
        << "\n[-q|--silence] 0|1 : allow output std::cout|wcout"
        << "\n[-r|--record] int : number of table records to select"
        << "\n[-v|--verbosity] 0|1 : show more details"
        << "\n[-c|--col] name of column to select"
        << "\n[-t|--tab] name of table to select"
        << "\n[--include] include tables"
        << "\n[--exclude] exclude tables"
        << "\n[--precision] int : float precision"
        << "\n[--trim_space] 0|1 : trim col spaces"
        << "\n[--warning] 0|1|2 : warning level"
        << std::endl;
}

int run_main(cmd_option const & opt)
{
    std::unique_ptr<scoped_null_cout> scoped_silence;
    std::unique_ptr<scoped_null_wcout> scoped_silence_w;
    if (opt.silence) {
        reset_new(scoped_silence);
        reset_new(scoped_silence_w);
    }
    else {
        print_version();
        std::cout
            << "\n--- called with: ---"
            << "\nmdf_file = " << opt.mdf_file
            << "\nout_file = " << opt.out_file
            << "\nsilence = " << opt.silence
            << "\nrecord_num = " << opt.record_num
            << "\nverbosity = " << opt.verbosity
            << "\ncol = " << opt.col_name
            << "\ntab = " << opt.tab_name
            << "\ninclude = " << opt.include            
            << "\nexclude = " << opt.exclude  
            << "\nprecision = " << opt.precision
            << "\ntrim_space = " << opt.trim_space
            << "\nwarning level = " << debug::warning_level
            << std::endl;
    }
    if (opt.precision) {
        db::to_string::precision(opt.precision);
    }
    db::database m_db(opt.mdf_file);
    db::database const & db = m_db;
    if (db.is_open()) {
        std::cout << "\ndatabase opened: " << db.filename() << std::endl;
    }
    else {
        std::cerr << "\ndatabase failed: " << db.filename() << std::endl;
        return EXIT_FAILURE;
    }
    SDL_TRACE("page_count = ", db.page_count());
    if (opt.record_num) {
        trace_datatables(db, opt);
    }
    return EXIT_SUCCESS;
}

int run_main(int argc, char* argv[])
{
    setlocale_t::set("Russian");

    cmd_option opt{};

    CmdLine cmd;
    cmd.add(make_option('i', opt.mdf_file, "mdf_file"));
    cmd.add(make_option('o', opt.out_file, "out_file"));
    cmd.add(make_option('q', opt.silence, "silence"));
    cmd.add(make_option('r', opt.record_num, "record_num"));
    cmd.add(make_option('v', opt.verbosity, "verbosity"));
    cmd.add(make_option('c', opt.col_name, "col"));
    cmd.add(make_option('t', opt.tab_name, "tab"));
    cmd.add(make_option(0, opt.include, "include"));
    cmd.add(make_option(0, opt.exclude, "exclude"));
    cmd.add(make_option(0, opt.precision, "precision"));    
    cmd.add(make_option(0, opt.trim_space, "trim_space"));    
    cmd.add(make_option(0, debug::warning_level, "warning"));

    try {
        if (argc == 1) {
            throw std::string("Missing parameters");
        }
        cmd.process(argc, argv);
        if (opt.mdf_file.empty()) {
            throw std::string("Missing input file");
        }
    }
    catch (const std::string& s) {
        print_help(argc, argv);
        std::cerr << "\n" << s << std::endl;
        return EXIT_FAILURE;
    }
    opt.includes = db::make::util::split(opt.include);
    opt.excludes = db::make::util::split(opt.exclude);
    return run_main(opt);
}

} // namespace 

int main(int argc, char* argv[])
{
    try {
        time_span timer;
        const int ret = run_main(argc, argv);
        const time_t sec = timer.now();
        std::cout << "\nTOTAL_TIME = " << sec << " seconds" << std::endl;
        return ret;
    }
    catch (sdl_exception & e) {
        (void)e;
        std::cout << "\ncatch exception [" << typeid(e).name() << "] = " << e.what();
        SDL_ASSERT(0);
    }
    catch (std::exception & e) {
        (void)e;
        std::cout << "\ncatch std::exception = " << e.what();
        SDL_ASSERT(0);
    }
}
