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
            const char * const sys_obj_name,
            bool const dump_mem) 
{
    typedef typename sys_obj::value_type sys_row;
    if (p) {
        auto print_row = [&db, sys_obj_name, dump_mem](sys_row const * row, size_t const i) {
            if (row) {
                std::cout
                    << "\n\n" << sys_obj_name << "_row(" << i << ") @"
                    << db.memory_offset(row)
                    << ":\n\n"
                    << sys_info::type_meta(*row);
                if (dump_mem) {
                    std::cout
                        << "\nDump " << sys_obj_name << "_row(" << i << ")\n"
                        << sys_info::type_raw(*row);
                }
                std::cout
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

#if 0
void dump_whole_page(db::page_head const * p)
{
    std::cout << "\ndump_whole_page\n";
    std::cout << db::to_string::type_raw(
        db::page_head::begin(p),
        db::page_head::page_size); 
}
#endif

void trace_page_data(db::datapage const * data, db::slot_array const & slot)
{
    SDL_ASSERT(data->head->data.type == db::pageType::type::data);
    auto const & page_id = data->head->data.pageId;
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
                << "\nDump slot(" << slot_id << ")"
                << " page(" << page_id.pageId << ")"
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

void trace_page_textmix(db::datapage const * data, db::slot_array const & slot)
{
    SDL_ASSERT(data->head->data.type == db::pageType::type::textmix);
    auto const & page_id = data->head->data.pageId;
    const size_t slot_size = slot.size();
    for (size_t slot_id = 0; slot_id < slot_size; ++slot_id) {
        db::row_head const * const h = data->get_row_head(slot_id);
        auto const mem = h->fixed_data(); // fixed length column data
        size_t const bytes = (mem.second - mem.first);
        std::cout
            << "\nDump slot(" << slot_id << ")"
            << " page(" << page_id.pageId << ")"
            << " Fixed length (bytes) = " << bytes
            << "\nhas_null = " << h->has_null()
            << "\nhas_variable = " << h->has_variable()
            << "\n\nrow_head:\n"
            << db::page_info::type_meta(*h)
            << db::to_string::type_raw(mem)
            ;
        //00009968 00000000 0300 (10 bytes)
        //Blob Id:1754857472 = 0x68990000
        //LOB Storage
    }
}

void trace_page(db::database & db, db::datapage const * data, bool const dump_mem)
{
    if (data) {
        db::page_head const * const p = data->head;
        if (p && !p->is_null()) {
            auto const & page_id = p->data.pageId;
            db::slot_array slot(p);
            std::cout 
                << "\n\npage(" << page_id.pageId << ") @"
                << db.memory_offset(p)
                << ":\n\n"
                << db::page_info::type_meta(*p) << "\n"
                << db::to_string::type(slot)
                << std::endl;
            if (dump_mem) {
                switch (p->data.type) {
                case db::pageType::type::data: // 1
                    trace_page_data(data, slot);
                    break;
                case db::pageType::type::textmix: // 3
                    trace_page_textmix(data, slot);
                    break;
                default:
                    break;
                }
            }
        }
    }
    else {
        SDL_WARNING(0);
    }
}

template<class sys_info, class vec_type>
void trace_sys_list(db::database & db, 
                    vec_type const & vec,
                    const char * const sys_obj_name,
                    bool const dump_mem)
{
    size_t i = 0;
    std::cout << "\n" << sys_obj_name << " pages = " << vec.size() << "\n\n";
    for (auto & p : vec) {
        std::cout
            << sys_obj_name << " page[" << (i++) << "] at "
            << db::to_string::type(p->head->data.pageId);
        trace_sys<sys_info>(db, p, sys_obj_name, dump_mem);
    }
}

void trace_sysallocunits(db::database & db, bool const dump_mem)
{
    trace_sys_list<db::sysallocunits_row_info>(db, db.get_sysallocunits_list(), "sysallocunits", dump_mem);
}

void trace_sysschobjs(db::database & db, bool const dump_mem)
{
    trace_sys_list<db::sysschobjs_row_info>(db, db.get_sysschobjs_list(), "sysschobjs", dump_mem);
}

void trace_syscolpars(db::database & db, bool const dump_mem)
{
    trace_sys_list<db::syscolpars_row_info>(db, db.get_syscolpars_list(), "syscolpars", dump_mem);
}

void trace_sysscalartypes(db::database & db, bool const dump_mem)
{
    trace_sys_list<db::sysscalartypes_row_info>(db, db.get_sysscalartypes_list(), "sysscalartypes", dump_mem);
}

void trace_sysidxstats(db::database & db, bool const dump_mem)
{
    trace_sys_list<db::sysidxstats_row_info>(db, db.get_sysidxstats_list(), "sysidxstats", dump_mem);
}

void trace_sysobjvalues(db::database & db, bool const dump_mem)
{
    trace_sys_list<db::sysobjvalues_row_info>(db, db.get_sysobjvalues_list(), "sysobjvalues", dump_mem);
}

void trace_sysiscols(db::database & db, bool const dump_mem)
{
    trace_sys_list<db::sysiscols_row_info>(db, db.get_sysiscols_list(), "sysiscols", dump_mem);
}

void trace_user_tables(db::database & db)
{
    size_t index = 0;
    for (auto & p : db.get_sysschobjs_list()) {
        for (size_t i = 0; i < p->slot.size(); ++i) {
            auto row = (*p)[i];
            A_STATIC_CHECK_TYPE(db::sysschobjs_row const *, row);
            if (row->is_USER_TABLE() && (row->data.id > 0)) {
                std::cout << "\nUSER_TABLE[" << (index++) << "]:\n";
                std::cout << db::sysschobjs_row_info::type_meta(*row);
                //std::cout << "\nname = " << db::sysschobjs_row_info::type_name(*row);
            }
        }
    }
}

/*void trace_user_tables_scheme(db::database & db)
{
}*/

} // namespace 

int main(int argc, char* argv[])
{
#if SDL_DEBUG
    std::cout << "\nSDL_DEBUG=1\n";
#else
    std::cout << "\nSDL_DEBUG=0\n";
#endif

    auto print_help = [](int argc, char* argv[])
    {
        std::cout
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
            << "\n[-u|--user_table]"
            << std::endl;
    };
    CmdLine cmd;

    struct cmd_option {
        std::string mdf_file;
        bool dump_mem = 0;
        int max_page = 0;
        int page_num = -1;
        bool print_sys = false;
        bool print_file = false;
        bool boot_page = true;
        bool user_table = false;
    } opt;

    cmd.add(make_option('i', opt.mdf_file, "input_file"));
    cmd.add(make_option('d', opt.dump_mem, "dump_mem"));
    cmd.add(make_option('m', opt.max_page, "max_page"));
    cmd.add(make_option('p', opt.page_num, "page_num"));
    cmd.add(make_option('s', opt.print_sys, "print_sys"));
    cmd.add(make_option('f', opt.print_file, "print_file"));
    cmd.add(make_option('b', opt.boot_page, "boot_page"));
    cmd.add(make_option('u', opt.user_table, "user_table"));

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
        << "\nuser_table = " << opt.user_table
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
        trace_sys<db::fileheader_row_info>(db, p, "fileheader", opt.dump_mem);
        std::cout << db::to_string::type(p->slot);
    }
    if (opt.page_num >= 0) {
        trace_page(db, db.get_datapage(opt.page_num).get(), opt.dump_mem);
    }
    const int max_page = a_min(opt.max_page, int(page_count));
    for (int i = 0; i < max_page; ++i) {
        trace_page(db, db.get_datapage(db::make_page(i)).get(), opt.dump_mem);
    }
    if (opt.print_sys) {
        trace_sysallocunits(db, opt.dump_mem);
        trace_sysschobjs(db, opt.dump_mem);
        trace_syscolpars(db, opt.dump_mem);
        trace_sysscalartypes(db, opt.dump_mem);
        trace_sysidxstats(db, opt.dump_mem);
        trace_sysobjvalues(db, opt.dump_mem);
        trace_sysiscols(db, opt.dump_mem);
    }
    if (opt.user_table) {
        trace_user_tables(db);
    }
    return EXIT_SUCCESS;
}

