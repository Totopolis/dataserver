// main.cpp : Defines the entry point for the console application.
//

#include "common/common.h"
#include "system/page_head.h"
#include "system/page_info.h"
#include "system/database.h"
#include "third_party/cmdLine/cmdLine.h"

#if !defined(SDL_DEBUG)
#error !defined(SDL_DEBUG)
#endif

namespace {

    using namespace sdl;

    void print_help(int argc, char* argv[])
    {
        std::cerr
            << "\nBuild date: " << __DATE__
            << "\nBuild time: " << __TIME__
            << "\nUsage: " << argv[0]
            << "\n[-i|--input_file] path to a .mdf file"
            << "\n[-v|--verbosity]"
            << "\n[-d|--dump_mem]"
            << "\n[-m|--max_page]"
            << "\n[-p|--page_num]"
            << "\n[-s|--print_sys]"
            << "\n[-f|--print_file]"
            << std::endl;
    }

    template<class sys_info, class sys_obj>
    static void trace_sys(db::database & db, 
        std::unique_ptr<sys_obj> p, 
        const char * const sys_obj_name) 
    {
        typedef typename sys_obj::value_type sys_obj_row;
        if (p) {
            auto print_row = [&db, sys_obj_name](sys_obj_row const * row, size_t const i) {
                if (row) {
                    std::cout
                        << "\n\n" << sys_obj_name << "_row(" << i << ") @"
                        << db.memory_offset(row)
                        << ":\n\n"
                        << sys_info::type_meta(*row)
                        << sys_info::type_raw(*row);
                }
                else {
                    SDL_WARNING(!"row not found");
                }

            };
            auto & obj = *p.get();
            std::cout
                << "\n\n" << sys_obj_name << " @" 
                << db.memory_offset(obj.head)
                << ":\n\n"
                << db::page_info::type_meta(*obj.head)
                << "slotCnt = " << obj.slot.size()
                << std::endl;
            for (size_t i = 0; i < obj.slot.size(); ++i) {
                print_row(obj[i], i);
            }
            std::cout << std::endl;
        }
        else
        {
            SDL_WARNING(0);
        }
    }

} // namespace 

int main(int argc, char* argv[])
{
#if SDL_DEBUG
    std::cout << "\nSDL_DEBUG=1\n";
#else
    std::cout << "\nSDL_DEBUG=0\n";
#endif

    CmdLine cmd;

    struct cmd_option {
        std::string mdf_file;
        size_t verbosity = 0;
        bool dump_mem = false;
        size_t max_page = 1;
        int page_num = -1;
        bool print_sys = false;
        bool print_file = false;
    } opt;

    cmd.add(make_option('i', opt.mdf_file, "input_file"));
    cmd.add(make_option('v', opt.verbosity, "verbosity"));
    cmd.add(make_option('d', opt.dump_mem, "dump_mem"));
    cmd.add(make_option('m', opt.max_page, "max_page"));
    cmd.add(make_option('p', opt.page_num, "page_num"));
    cmd.add(make_option('s', opt.print_sys, "print_sys"));
    cmd.add(make_option('f', opt.print_file, "print_file"));

    try {
        if (argc == 1)
            throw std::string("Invalid parameter.");
        cmd.process(argc, argv);
        if (opt.mdf_file.empty())
            throw std::string("Missing input file");
    }
    catch (const std::string& s) {
        print_help(argc, argv);
        std::cerr << "\n" << s << std::endl;
        return EXIT_FAILURE;
    }

    enum {
        print_max_page = 1,
        print_boot_page = 1,
    };
    std::cout
        << "\n--- called with: ---"
        << "\nmdf_file = " << opt.mdf_file
        << "\nverbosity = " << opt.verbosity
        << "\ndump_mem = " << opt.dump_mem
        << "\nmax_page = " << opt.max_page
        << "\npage_num = " << opt.page_num
        << "\nprint_sys = " << opt.print_sys
        << "\nprint_file = " << opt.print_file
        << std::endl;

    db::database db(opt.mdf_file);
    if (db.is_open()) {
        std::cout << "\ndatabase opened: " << db.filename() << std::endl;
    }
    else {
        std::cerr << "\ndatabase failed: " << db.filename() << std::endl;
        return EXIT_FAILURE;
    }
    const size_t page_count = db.page_count();
    std::cout << "page_count = " << page_count << std::endl;

    if (print_boot_page) {
        auto const boot = db.get_bootpage();
        if (boot) {
            auto & h = *(boot->head);
            auto & b = *(boot->row);
            std::cout
                << "\n\nbootpage:\n\n"
                << db::page_info::type_meta(h)
                << db::to_string::type(boot->slot)
                ;
            if (opt.dump_mem) {
                std::cout 
                    << "\nMemory Dump bootpage header:\n"
                    << db::page_info::type_raw(h)
                    << "\nMemory Dump bootpage row:\n"
                    << db::boot_info::type_raw(b);
            }
            std::cout
                << "\n\nDBINFO:\n\n" << db::boot_info::type_meta(b)
                << std::endl;
        }
    }
    if (opt.print_file) {
        trace_sys<db::fileheader_row_info>(db, db.get_fileheader(), "fileheader");
    }
    auto print_page = [&db](db::page_head const * p, int i) {
        if (p && !p->is_null()) {
            db::slot_array slot(p);
            std::cout 
                << "\n\npage(" << i << ") @"
                << db.memory_offset(p)
                << ":\n\n"
                << db::page_info::type_meta(*p) << "\n"
                << db::to_string::type(slot)
                << std::endl;
        }
    };
    if (opt.page_num >= 0) {
        print_page(db.load_page(opt.page_num), opt.page_num);
    }
    else if (print_max_page) {
        const size_t max_page = (opt.max_page > 0) ?
            a_min(opt.max_page, page_count) : page_count;
        for (size_t i = 0; i < max_page; ++i) {
            print_page(db.load_page(i), i);
        }
    }
    if (opt.print_sys) {
        trace_sys<db::sysallocunits_row_info>(db, db.get_sysallocunits(), "sysallocunits");
        trace_sys<db::syschobjs_row_info>(db, db.get_syschobjs(), "syschobjs");
        trace_sys<db::syscolpars_row_info>(db, db.get_syscolpars(), "syscolpars");
        trace_sys<db::sysidxstats_row_info>(db, db.get_sysidxstats(), "sysidxstats");
        trace_sys<db::sysscalartypes_row_info>(db, db.get_sysscalartypes(), "sysscalartypes");
    }
    return EXIT_SUCCESS;
}

