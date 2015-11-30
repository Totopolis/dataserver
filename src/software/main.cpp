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
            << "\nUsage: " << argv[0]
            << "\n[-i|--input_file] path to a MDF file"
            << "\n[-v|--verbosity]"
            << "\n[-m|--max_page]"
            << std::endl;
    }
}

int main(int argc, char* argv[])
{
    using namespace sdl;

#if SDL_DEBUG
    std::cout << "SDL_DEBUG=1\n";
#else
    std::cout << "SDL_DEBUG=0\n";
#endif

    CmdLine cmd;
    std::string mdf_file;
    size_t verbosity = 0;
    size_t max_page = 10;

    cmd.add(make_option('i', mdf_file, "input_file"));
    cmd.add(make_option('v', verbosity, "verbosity"));
    cmd.add(make_option('m', max_page, "max_page"));

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

    std::cout
        << "\ncalled with:"
        << "\nmdf_file = " << mdf_file
        << "\nverbosity = " << verbosity
        << "\nmax_page = " << max_page
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
    if (1) {
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
                    << "\nslot_array(" << slot.size() << ") = ";
                for (auto i : slot.copy()) {
                    std::cout << i << " ";
                }
                std::cout 
                    << std::endl;
            }
        }
    }
    if (1) { // print boot page
        auto const boot = db.load_bootpage();
        if (boot.head && boot.row) {
            auto & h = *boot.head;
            auto & b = *boot.row;
            std::cout
                << db::page_info::type(h)
                << "\npage_info::type_meta:\n" << db::page_info::type_meta(h)
                << "\npage_info::type_raw:\n"  << db::page_info::type_raw(h)
                << "\nMemory Dump:\n"        //<< db::boot_info::type_raw(b)
                << "\nDBINFO:\n"               << db::boot_info::type(b)
                << "\nboot_info::type_meta:\n" << db::boot_info::type_meta(b)
                << std::endl;
        }
    }
    return EXIT_SUCCESS;
}

