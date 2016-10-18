// main.cpp : Defines the entry point for the console application.
// main.cpp is used for tests and research only and is not part of the dataserver library.
#include "common/common.h"
#include "system/database.h"
#include "system/version.h"
#include "spatial/geography_info.h"
#include "third_party/cmdLine/cmdLine.h"
#include "common/outstream.h"
#include "common/locale.h"
#include "common/time_util.h"
#include "utils/conv.h"
#include <map>
#include <set>
#include <fstream>
#include <cstdlib> // atof
#include <iomanip> // for std::setprecision

namespace {

using namespace sdl;

struct cmd_option : noncopyable {
    std::string mdf_file;
    std::string out_file;
    bool silence = false;
    int record_num = 10;
    int max_output = 10;
    int verbosity = 0;
    std::string col_name;
    std::string tab_name;
    int precision = 0;
};

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
        << "\n[-x|--max_output] int : limit column length in chars"
        << "\n[-v|--verbosity] 0|1 : show more details"
        << "\n[-c|--col] name of column to select"
        << "\n[-t|--tab] name of table to select"
        << "\n[--precision] int : float precision"
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
            << "\nmax_output = " << opt.max_output
            << "\nverbosity = " << opt.verbosity
            << "\ncol = " << opt.col_name
            << "\ntab = " << opt.tab_name
            << "\nprecision = " << opt.precision
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
    cmd.add(make_option('x', opt.max_output, "max_output"));
    cmd.add(make_option('v', opt.verbosity, "verbosity"));
    cmd.add(make_option('c', opt.col_name, "col"));
    cmd.add(make_option('t', opt.tab_name, "tab"));
    cmd.add(make_option(0, opt.precision, "precision"));    
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

#if 0
SQL server tracks which pages and extents belong to which database objects / allocation units through GAM (Global Allocation Map) intervals. 
A GAM interval is a range of up to 63904 extents in a database file (3994MB, just short of 4GB, since an extent is 64KB in size).  
The first GAM covers extents 0-63903.  The 2nd GAM covers extents 63904-127807, etc...  
Each GAM interval in a database file will have a single GAM, SGAM, DCM, and BCM page allocated for it to track space usage and status in that GAM interval. 
There will also be up to one IAM page per allocation unit in the GAM interval.  
The GAM, SGAM, IAM, DCM, and BCM pages are all 'synoptic' in the sense that they all have the same definition of a GAM interval in terms of the starting and ending extents.
#endif

