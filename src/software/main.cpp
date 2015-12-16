// main.cpp : Defines the entry point for the console application.
//

#include "common/common.h"
#include "system/page_head.h"
#include "system/page_info.h"
#include "system/database.h"
#include "third_party/cmdLine/cmdLine.h"
#include "common/static_type.h"

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
        << "\n[-d|--dump_mem]"
        << "\n[-m|--max_page]"
        << "\n[-p|--page_num]"
        << "\n[-s|--print_sys]"
        << "\n[-f|--print_file]"
        << "\n[-b|--boot_page]"
        << std::endl;
}

template<class sys_row>
void trace_var(sys_row const * row, Int2Type<0>){}

template<class sys_row>
void trace_var(sys_row const * row, Int2Type<1>)
{
    if (db::variable_array::has_variable(row)) {
        std::cout << db::to_string::type(db::variable_array(row)) << std::endl;
    }
    else {
        std::cout << "\nvariable_array = none\n";
    }
}

template<class sys_row>
void trace_null(sys_row const * row, Int2Type<0>){}

template<class sys_row>
void trace_null(sys_row const * row, Int2Type<1>)
{
    if (db::null_bitmap::has_null(row)) {
        std::cout << db::to_string::type(db::null_bitmap(row)) << std::endl;
    }
    else {
        std::cout << "\nnull_bitmap = none\n";
    }
}

template<class sys_row>
void trace_sys_row(sys_row const * row)
{
    trace_null(row, Int2Type<db::null_bitmap_traits<sys_row>::value>());
    trace_var(row, Int2Type<db::variable_array_traits<sys_row>::value>());
}

template<class sys_info, class sys_obj>
void trace_sys(
            db::database & db, 
            std::unique_ptr<sys_obj> const & p, 
            const char * const sys_obj_name) 
{
    typedef typename sys_obj::value_type sys_row;
    if (p) {
        auto print_row = [&db, sys_obj_name](sys_row const * row, size_t const i) {
            if (row) {
                std::cout
                    << "\n\n" << sys_obj_name << "_row(" << i << ") @"
                    << db.memory_offset(row)
                    << ":\n\n"
                    << sys_info::type_meta(*row)
                    << "\nDump " << sys_obj_name << "_row(" << i << ")\n"
                    << sys_info::type_raw(*row)
                    << std::endl;
                trace_sys_row(row);
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

void dump_whole_page(db::page_head const * p)
{
    std::cout << db::to_string::type_raw(
        db::page_head::begin(p),
        db::page_head::page_size); 
}

void trace_page(db::database & db, db::datapage const * data,
    db::pageIndex const page_id, int const dump_mem)
{
    enum { dump_slots = 1 };
    if (data) {
        db::page_head const * const p = data->head;
        if (p && !p->is_null()) {
            db::slot_array slot(p);
            std::cout 
                << "\n\npage(" << page_id.value() << ") @"
                << db.memory_offset(p)
                << ":\n\n"
                << db::page_info::type_meta(*p) << "\n"
                << db::to_string::type(slot)
                << std::endl;
            if (dump_mem && p->is_data()) {
                if (dump_slots) {
                    const size_t slot_size = slot.size();
                    for (size_t slot_id = 0; slot_id < slot_size; ++slot_id) {
                        db::row_head const * const h = data->get_row_head(slot_id);
                        if (h->has_null() && h->has_variable()) {
                            db::row_data const row(h);
                            auto const mem = row.data();
                            size_t const bytes = (mem.second - mem.first);
                            size_t const row_size = row.size(); // # of columns
                            SDL_ASSERT(bytes < db::page_head::body_size); // ROW_OVERFLOW data ?
                            std::cout
                                << "Dump slot(" << slot_id << ")"
                                << " page(" << page_id.value() << ")"
                                << " Length (bytes) = " << bytes
                                << " Columns = " << row_size
                                << " Variable = " << row.variable.size()
                                ;
                            std::cout << " NULL = ";
                            for (size_t j = 0; j < row_size; ++j) {
                                std::cout << (row.is_null(j) ? "1" : "0");
                            }
                            std::cout << " Fixed = ";
                            for (size_t j = 0; j < row_size; ++j) {
                                std::cout << (row.is_fixed(j) ? "f" : "-");
                            }
                            std::cout
                                << "\n\nrow_head:\n"
                                << db::page_info::type_meta(*h)
                                << db::to_string::type(row.null) << std::endl
                                << db::to_string::type(row.variable) << std::endl
                                << db::to_string::type_raw(mem);
                        }
                    }
                }
                else {
                    dump_whole_page(p);
                }
            }
        }
    }
    else {
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
        bool dump_mem = 0;
        int max_page = 0;
        int page_num = -1;
        bool print_sys = false;
        bool print_file = false;
        bool boot_page = true;
    } opt;

    cmd.add(make_option('i', opt.mdf_file, "input_file"));
    cmd.add(make_option('d', opt.dump_mem, "dump_mem"));
    cmd.add(make_option('m', opt.max_page, "max_page"));
    cmd.add(make_option('p', opt.page_num, "page_num"));
    cmd.add(make_option('s', opt.print_sys, "print_sys"));
    cmd.add(make_option('f', opt.print_file, "print_file"));
    cmd.add(make_option('b', opt.boot_page, "boot_page"));

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

    std::cout
        << "\n--- called with: ---"
        << "\nmdf_file = " << opt.mdf_file
        << "\ndump_mem = " << opt.dump_mem
        << "\nmax_page = " << opt.max_page
        << "\npage_num = " << opt.page_num
        << "\nprint_sys = " << opt.print_sys
        << "\nprint_file = " << opt.print_file
        << "\nboot_page = " << opt.boot_page
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

    if (opt.boot_page) {
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
        auto p = db.get_fileheader();
        trace_sys<db::fileheader_row_info>(db, p, "fileheader");
        std::cout << db::to_string::type(p->slot);
    }
    if (opt.page_num >= 0) {
        trace_page(db, db.get_datapage(opt.page_num).get(), 
            db::make_page(opt.page_num), opt.dump_mem);
    }
    const int max_page = a_min(opt.max_page, int(page_count));
    for (int i = 0; i < max_page; ++i) {
        auto const j = db::make_page(i);
        trace_page(db, db.get_datapage(j).get(), j, opt.dump_mem);
    }
    if (opt.print_sys) {
        trace_sys<db::sysallocunits_row_info>(db, db.get_sysallocunits(), "sysallocunits");
        trace_sys<db::syschobjs_row_info>(db, db.get_syschobjs(), "syschobjs");
        trace_sys<db::syscolpars_row_info>(db, db.get_syscolpars(), "syscolpars");
        trace_sys<db::sysidxstats_row_info>(db, db.get_sysidxstats(), "sysidxstats");
        trace_sys<db::sysscalartypes_row_info>(db, db.get_sysscalartypes(), "sysscalartypes");
        trace_sys<db::sysobjvalues_row_info>(db, db.get_sysobjvalues(), "sysobjvalues");
        trace_sys<db::sysiscols_row_info>(db, db.get_sysiscols(), "sysiscols");
    }
    return EXIT_SUCCESS;
}

