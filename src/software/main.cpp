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

} // namespace 

int main(int argc, char* argv[])
{
    using namespace sdl;

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
    } opt;

    cmd.add(make_option('i', opt.mdf_file, "input_file"));
    cmd.add(make_option('v', opt.verbosity, "verbosity"));
    cmd.add(make_option('d', opt.dump_mem, "dump_mem"));
    cmd.add(make_option('m', opt.max_page, "max_page"));
    cmd.add(make_option('p', opt.page_num, "page_num"));
    cmd.add(make_option('s', opt.print_sys, "print_sys"));

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
        print_file_header = 0,
        print_sysallocunits = 1,
    };
    std::cout
        << "\n--- called with: ---"
        << "\nmdf_file = " << opt.mdf_file
        << "\nverbosity = " << opt.verbosity
        << "\ndump_mem = " << opt.dump_mem
        << "\nmax_page = " << opt.max_page
        << "\npage_num = " << opt.page_num
        << "\nprint_sys = " << opt.print_sys
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
    if (print_file_header) {
        auto p = db.get_datapage(0);
        if (p) {
            std::cout
                << "\n\ndatapage(0):\n\n"
                << db::page_info::type_meta(*p->head)
                << db::to_string::type(p->slot)
                << std::endl;
            auto row = db::cast::page_body<db::file_header_row>(p->head);
            if (row) {
                if (opt.dump_mem) {
                    std::cout
                    << "\nDump file_header_row:\n"
                    << db::to_string::type_raw(row->raw)
                    << std::endl;
                }
                std::cout
                    << "\nfile_header_row_meta:\n"
                    << db::file_header_row_info::type(*row)
                    << std::endl;
            }
        }
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
    if (print_sysallocunits) {
        if (auto p = db.get_sysallocunits()) {
            auto print_sysallocunits_row = [&db](db::sysallocunits_row const * row, size_t const i) {
                if (row) {
                    std::cout
                        << "\n\nsysallocunits_row(" << i << ") @"
                        << db.memory_offset(row)
                        << ":\n\n"
                        << db::sysallocunits_row_info::type_meta(*row)
                        << db::sysallocunits_row_info::type_raw(*row);
                }
                else {
                    SDL_WARNING(!"row not found");
                }
            };
            db::sysallocunits & sa = *p.get();
            if (opt.print_sys) {
                std::cout
                    << "\n\nsysallocunits @" 
                    << db.memory_offset(sa.head)
                    << ":\n\n"
                    << db::page_info::type_meta(*sa.head)
                    << "slotCnt = " << sa.slot.size()
                    << std::endl;
                for (size_t i = 0; i < sa.slot.size(); ++i) {
                    print_sysallocunits_row(sa[i], i);
                }
                std::cout << std::endl;
            }
#if 0
            enum { syschobjs_obj = (int)(db::database::sysObj::syschobjs_obj) };
            auto slot_34 = sa.find_auid(syschobjs_obj);
            if (slot_34.first) {
                db::sysallocunits_row const & row = *slot_34.first;
                std::cout
                    << "\nsyschobjs_obj at slot "
                    << slot_34.second
                    << "\npgfirst = "
                    << db::to_string::type(row.data.pgfirst)
                    << std::endl;
            }
#endif
        }
        if (auto p = db.get_syschobjs()) {
            auto print_syschobjs_row = [&db](db::syschobjs_row const * row, size_t const i) {
                if (row) {
                    std::cout
                        << "\n\nsyschobjs_row(" << i << ") @"
                        << db.memory_offset(row)
                        << ":\n\n"
                        << db::syschobjs_row_info::type_meta(*row)
                        << db::syschobjs_row_info::type_raw(*row);
                }
                else {
                    SDL_WARNING(!"row not found");
                }
            };
            db::syschobjs & obj = *p.get();
            if (opt.print_sys) {
                std::cout
                    << "\n\nsyschobjs @" 
                    << db.memory_offset(obj.head)
                    << ":\n\n"
                    << db::page_info::type_meta(*obj.head)
                    << "slotCnt = " << obj.slot.size()
                    << std::endl;
                for (size_t i = 0; i < obj.slot.size(); ++i) {
                    print_syschobjs_row(obj[i], i);
                }
                std::cout << std::endl;
            }
        }
        if (auto p = db.get_syscolpars()) {
            auto print_syscolpars_row = [&db](db::syscolpars_row const * row, size_t const i) {
                if (row) {
                    std::cout
                        << "\n\nsyscolpars_row(" << i << ") @"
                        << db.memory_offset(row)
                        << ":\n\n"
                        << db::syscolpars_row_info::type_meta(*row)
                        << db::syscolpars_row_info::type_raw(*row);
                }
                else {
                    SDL_WARNING(!"row not found");
                }

            };
            db::syscolpars & obj = *p.get();
            if (opt.print_sys) {
                std::cout
                    << "\n\nsyscolpars @" 
                    << db.memory_offset(obj.head)
                    << ":\n\n"
                    << db::page_info::type_meta(*obj.head)
                    << "slotCnt = " << obj.slot.size()
                    << std::endl;
                for (size_t i = 0; i < obj.slot.size(); ++i) {
                    print_syscolpars_row(obj[i], i);
                }
                std::cout << std::endl;
            }
        }
    }

    return EXIT_SUCCESS;
}

