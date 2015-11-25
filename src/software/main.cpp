// OpenMDF.cpp : Defines the entry point for the console application.
//

#include "common/common.h"
#include "system/page_head.h"
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
    if (0) {
        if (max_page > 0)
            max_page = a_min(max_page, page_count);
        else
            max_page = page_count;
        for (size_t i = 0; i < max_page; ++i) {
            auto p = db.load_page(i);
            if (p && !p->is_null()) {
                std::cout
                    << db::page_info::type(*p, i)
                    << std::endl;
            }
        }
    }
    if (1) { // print boot
        auto const boot = db.bootpage();
        if (boot.first && boot.second) {
            auto & b = *boot.second;
            std::cout
                << db::page_info::type(*boot.first, 9)
                << "\nMemory Dump:\n"
                //<< db::boot_info::type_raw(b)
                << "\nDBINFO:\n"
                << db::boot_info::type(b)
                << "\ntype_meta:\n"
                << db::boot_info::type_meta(b)
                << std::endl;
        }
    }
    return EXIT_SUCCESS;
}

