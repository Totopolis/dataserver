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
            << "\n[-i|--input_file] path to a .mdf file"
            << "\n[-v|--verbosity]"
            << "\n[-d|--dump_mem]"
            << "\n[-m|--max_page]"
            << "\n[-p|--page_num]"
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
    bool dump_mem = false;
    size_t max_page = 1;
    int page_num = -1;

    cmd.add(make_option('i', mdf_file, "input_file"));
    cmd.add(make_option('v', verbosity, "verbosity"));
    cmd.add(make_option('d', dump_mem, "dump_mem"));
    cmd.add(make_option('m', max_page, "max_page"));
    cmd.add(make_option('p', page_num, "page_num"));

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
        print_file_header = 0,
        print_sysallocunits = 0,
    };
    std::cout
        << "\n--- called with: ---"
        << "\nmdf_file = " << mdf_file
        << "\nverbosity = " << verbosity
        << "\ndump_mem = " << dump_mem
        << "\nmax_page = " << max_page
        << "\npage_num = " << page_num
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

    auto print_page = [](db::page_head const * p, int i) {
        if (p && !p->is_null()) {
            db::slot_array slot(p);
            std::cout
                << "\n\npage(" << i << "):\n\n"
                << db::page_info::type_meta(*p) << "\n"
                << db::to_string::type(slot)
                << std::endl;
        }
    };
    if (page_num >= 0) {
        print_page(db.load_page(page_num), page_num);
    }
    else if (print_max_page) {
        if (max_page > 0)
            max_page = a_min(max_page, page_count);
        else
            max_page = page_count;
        for (size_t i = 0; i < max_page; ++i) {
            print_page(db.load_page(i), i);
        }
    }
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
            if (dump_mem) {
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
    if (print_file_header) {
        auto p = db.get_datapage(0);
        if (p) {
            std::cout
                << "\n\ndatapage(0):\n\n"
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
                << "\n\nsysallocunits:\n\n"
                << db::page_info::type_meta(*p->head)
                << "slotCnt = " << p->slot.size()
                << std::endl;
        }
    }
    return EXIT_SUCCESS;
}

