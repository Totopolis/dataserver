// main.cpp : Defines the entry point for the console application.
//

#include "common/common.h"
#include "system/page_head.h"
#include "system/page_info.h"
#include "system/database.h"
#include "third_party/cmdLine/cmdLine.h"

namespace {
    void print_help(int argc, char* argv[])
    {
        std::cerr
            << "\nBuild date: " << __DATE__
            << "\nBuild time: " << __TIME__
            << "\nUsage: " << argv[0]
            << "\n[-i|--input_file] path to a MDF file"
            << "\n[-v|--verbosity]"
            << "\n[-m|--max_page]"
            << "\n[-d|--dump_mem]"
            << std::endl;
    }
}

int main(int argc, char* argv[])
{
    using namespace sdl;

#if SDL_DEBUG
    std::cout << "\nSDL_DEBUG=1\n";
#else
    std::cout << "\nSDL_DEBUG=0\n";
#endif

    CmdLine cmd;
    std::string mdf_file;
    size_t verbosity = 0;
    size_t max_page = 1;
    bool dump_mem = false;

    cmd.add(make_option('i', mdf_file, "input_file"));
    cmd.add(make_option('v', verbosity, "verbosity"));
    cmd.add(make_option('m', max_page, "max_page"));
    cmd.add(make_option('d', dump_mem, "dump_mem"));

    try {
        if (argc == 1)
            throw std::string("Invalid parameter.");
        cmd.process(argc, argv);
        if (mdf_file.empty())
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
        print_file_header = 1,
        print_sysallocunits = 1,
    };
    std::cout
        << "\n--- called with: ---"
        << "\nmdf_file = " << mdf_file
        << "\nverbosity = " << verbosity
        << "\nmax_page = " << max_page
        << "\ndump_mem = " << dump_mem
        << std::endl;

    db::database db(mdf_file);
    if (db.is_open()) {
        std::cout << "\ndatabase opened: " << db.filename() << std::endl;
    }
    else {
        std::cerr << "\ndatabase failed: " << db.filename() << std::endl;
        return EXIT_FAILURE;
    }
    const size_t page_count = db.page_count();
    std::cout << "page_count = " << page_count << std::endl;
    
    if (print_max_page) {
        if (max_page > 0)
            max_page = a_min(max_page, page_count);
        else
            max_page = page_count;
        for (size_t i = 0; i < max_page; ++i) {
            auto p = db.load_page(i);
            if (p && !p->is_null()) {
                db::slot_array slot(p);
                std::cout
                    << db::page_info::type(*p)
                    << db::to_string::type(slot)
                    << std::endl;
            }
        }
    }
    if (print_boot_page) {
        auto const boot = db.get_bootpage();
        if (boot) {
            auto & h = *(boot->head);
            auto & b = *(boot->row);
            std::cout
                << db::page_info::type(h)
                << "\npage_info::type_meta:\n" << db::page_info::type_meta(h);
            if (dump_mem) {
                std::cout << "\npage_info::type_raw:\n" << db::page_info::type_raw(h);
                std::cout << "\nMemory Dump:\n" << db::boot_info::type_raw(b);
            }
            std::cout
                << "\nDBINFO:\n" << db::boot_info::type(b)
                << "\nboot_info::type_meta:\n" << db::boot_info::type_meta(b)
                << std::endl;
        }
    }
    if (print_file_header) {
        auto p = db.get_datapage(0);
        if (p) {
            std::cout
                << "\npage_0:\n"
                << db::page_info::type_meta(*p->head)
                << db::to_string::type(p->slot)
                << std::endl;
            auto row = db::page_body<db::file_header_row>(p->head);
            if (row) {
                if (dump_mem) {
                    std::cout
                    << "\nDump file_header_row:\n"
                    << db::to_string::type_raw(row->raw)
                    << std::endl;
                }
                std::cout
                    << "\nfile_header_row_meta:\n"
                    << db::file_header_row_meta::type(*row)
                    << std::endl;
            }
        }
    }
    if (print_sysallocunits) {
        auto p = db.get_sysallocunits();
        if (p) {
            std::cout
                << "\nsysallocunits:\n"
                << db::page_info::type_meta(*p->head)
                << std::endl;
        }
    }
    return EXIT_SUCCESS;
}

