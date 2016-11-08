// main.cpp : Defines the entry point for the console application.
// main.cpp is used for tests and research only and is not part of the dataserver library.
#include "dataserver/system/database.h"
#include "dataserver/system/version.h"
#include "dataserver/third_party/cmdLine/cmdLine.h"
#include "dataserver/common/outstream.h"
#include "dataserver/common/locale.h"
#include "dataserver/common/time_util.h"
#include "dataserver/common/outstream.h"
#include "dataserver/utils/conv.h"
#include "dataserver/maketable/generator_util.h"
#include <fstream>
#include <iomanip> // for std::setprecision

#if 0 // defined(SDL_OS_WIN32)
#include <windows.h>
#endif

namespace {

using namespace sdl;

struct cmd_option : noncopyable {
    using vector_string = std::vector<std::string>;
    static const char * default_locale() { return "Russian"; }
    std::string mdf_file;
    bool silence = false;
    int record_num = 10;
    std::string col_name;
    std::string tab_name;
    std::string include;
    std::string exclude;
    vector_string includes;
    vector_string excludes;
    int precision = 0;
    bool trim = false;
    bool utf8 = false;
    bool stop = false;
    std::string locale{default_locale()};
};

void type_col_utf8(std::string && s, cmd_option const & opt)
{
    SDL_WARNING(!s.empty());
    if (opt.trim) {
        std::cout << db::to_string::trim(std::move(s));
    }
    else {
        std::cout << s;
    }
}

void type_col_wide(std::wstring && s, cmd_option const & opt)
{
    SDL_WARNING(!s.empty());
    if (opt.trim) {
        std::wcout << db::to_string::trim(std::move(s));
    }
    else {
        std::wcout << s;
    }
}

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
            if (record.is_null(i)) {
                std::cout << "[NULL]";
                continue;
            }
            if (opt.utf8) {
                type_col_utf8(record.type_col_utf8(i), opt);
            }
            else {
                type_col_wide(record.type_col_wide(i), opt);
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
        trace_table_record(db, table, record, opt, ++row_index);
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
        << "\n[-q|--silence] 0|1 : allow output std::cout|wcout"
        << "\n[-r|--record] int : number of records to select (-1 to select all)"
        << "\n[-c|--col] name of column to select"
        << "\n[-t|--tab] name of table to select"
        << "\n[-u|--utf8] 0|1 print col as utf8"
        << "\n[--include] include tables"
        << "\n[--exclude] exclude tables"
        << "\n[--precision] int : float precision"
        << "\n[--trim] 0|1 : trim col spaces"
        << "\n[--stop] 0|1 : stop or skip if conversion error"
        << "\n[--locale] locale name (default = " << cmd_option::default_locale() << ")"
        << "\n[--warning] 0|1|2 : warning level"
        << std::endl;
}

int run_main(cmd_option & opt)
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
            << "\nsilence = " << opt.silence
            << "\nrecord_num = " << opt.record_num
            << "\ncol = " << opt.col_name
            << "\ntab = " << opt.tab_name
            << "\nutf8 = " << opt.utf8
            << "\ninclude = " << opt.include            
            << "\nexclude = " << opt.exclude  
            << "\nprecision = " << opt.precision
            << "\ntrim = " << opt.trim
            << "\nstop = " << opt.stop
            << "\nlocale = " << opt.locale       
            << "\nwarning level = " << debug::warning_level
            << std::endl;
    }
    if (opt.precision) {
        db::to_string::precision(opt.precision);
    }
    setlocale_t::set(opt.locale);
    SDL_ASSERT(!db::conv::method_stop());
    db::conv::method_stop(opt.stop);
    db::database m_db(opt.mdf_file);
    db::database const & db = m_db;
    if (db.is_open()) {
        std::cout << "\ndatabase opened: " << db.filename() << std::endl;
    }
    else {
        std::cerr << "\ndatabase failed: " << db.filename() << std::endl;
        return EXIT_FAILURE;
    }
    if (opt.record_num) {
        opt.includes = db::make::util::split(opt.include);
        opt.excludes = db::make::util::split(opt.exclude);
        trace_datatables(db, opt);
    }
    return EXIT_SUCCESS;
}

int run_main(int argc, char* argv[])
{
    cmd_option opt{};

    CmdLine cmd;
    cmd.add(make_option('i', opt.mdf_file, "mdf_file"));
    cmd.add(make_option('q', opt.silence, "silence"));
    cmd.add(make_option('r', opt.record_num, "record_num"));
    cmd.add(make_option('c', opt.col_name, "col"));
    cmd.add(make_option('t', opt.tab_name, "tab"));
    cmd.add(make_option('u', opt.utf8 , "utf8"));    
    cmd.add(make_option(0, opt.include, "include"));
    cmd.add(make_option(0, opt.exclude, "exclude"));
    cmd.add(make_option(0, opt.precision, "precision"));    
    cmd.add(make_option(0, opt.trim, "trim"));    
    cmd.add(make_option(0, opt.stop, "stop"));    
    cmd.add(make_option(0, opt.locale , "locale"));    
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
