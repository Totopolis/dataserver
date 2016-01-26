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
        std::cout 
            << db::to_string::type(db::variable_array(row))
            << std::endl;        
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
void trace_col_name(sys_row const * row, Int2Type<0>){}

template<class sys_row>
void trace_col_name(sys_row const * row, Int2Type<1>) {
    std::cout << "[" << db::col_name_t(row) << "]";
}

template<class sys_row>
void trace_col_name(sys_row const * row) {
    trace_col_name(row, Int2Type<db::variable_array_traits<sys_row>::value>());
}

template<class sys_info, class page_ptr>
void trace_sys_page(
            db::database & db, 
            page_ptr const & p,
            const char * const sys_obj_name,
            size_t * const row_index,
            bool const dump_mem) 
{
    using sys_obj = typename page_ptr::element_type;
    using sys_row = typename sys_obj::row_type;
    if (p) {
        auto print_row = [&db, sys_obj_name, dump_mem]
            (sys_row const * row, size_t const i, size_t * const row_index) {
            if (row) {
                std::cout
                    << "\n\n" << sys_obj_name << "_row(" << i << ") @"
                    << db.memory_offset(row)
                    << " ";
                if (row_index) {
                    std::cout << "[" << (*row_index)++ << "] ";
                }
                trace_col_name(row);
                std::cout << "\n\n";
                std::cout << sys_info::type_meta(*row);
                if (dump_mem) {
                    std::cout
                        << "\nDump " << sys_obj_name << "_row(" << i << ")\n"
                        << sys_info::type_raw(*row);
                }
                std::cout << std::endl;
                trace_null(row, Int2Type<db::null_bitmap_traits<sys_row>::value>());
                trace_var(row, Int2Type<db::variable_array_traits<sys_row>::value>());
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
            print_row(obj[i], i, row_index);
        }
        std::cout << std::endl;
    }
    else
    {
        SDL_WARNING(0);
    }
}

void dump_whole_page(db::page_head const * const p)
{
    std::cout << "\nDump page("
        << p->data.pageId.fileId << ":" 
        << p->data.pageId.pageId << ")\n";
    std::cout << db::to_string::type_raw(
        db::page_head::begin(p),
        db::page_head::page_size); 
}

void trace_page_data(db::datapage const * data, db::slot_array const & slot)
{
    SDL_ASSERT(data->head->data.type == db::pageType::type::data);
    auto const & page_id = data->head->data.pageId;
    const size_t slot_size = slot.size();
    for (size_t slot_id = 0; slot_id < slot_size; ++slot_id) {
        db::row_head const * const h = (*data)[slot_id];
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
                std::cout << (row.is_fixed(j) ? "1" : "-");
            }
            std::cout
                << "\n\nrow_head:\n"
                << db::page_info::type_meta(*h)
                << db::to_string::type_raw(mem) << std::endl
                << db::to_string::type(row.null) << std::endl
                << db::to_string::type(row.variable)
                << std::endl;
        }
        if (h->has_version()) {
            std::cout << "\nhas_version = 1";
        }
        if (h->ghost_forwarded()) {
            std::cout << "\nghost_forwarded = 1";
        }
    }
}

void trace_page_textmix(db::datapage const * data, db::slot_array const & slot)
{
    SDL_ASSERT(data->head->data.type == db::pageType::type::textmix);
    auto const & page_id = data->head->data.pageId;
    const size_t slot_size = slot.size();
    for (size_t slot_id = 0; slot_id < slot_size; ++slot_id) {
        db::row_head const * const h = (*data)[slot_id];
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
            << std::endl;
        //00009968 00000000 0300 (10 bytes)
        //Blob Id:1754857472 = 0x68990000
        //LOB Storage
    }
}

size_t trace_iam_page(db::iam_page const & iampage)
{
    std::cout << db::to_string::type(iampage.head->data.pageId);
    std::cout << " " << db::to_string::type(iampage.head->data.type);
    std::cout << std::endl;
    size_t iam_page_cnt = 0;
    for (auto row : iampage) {
        A_STATIC_CHECK_TYPE(db::iam_page_row const *, row);
        std::cout << "\niam_page_row[" << (iam_page_cnt++) << "]:\n";
        std::cout << db::iam_page_row_info::type_meta(*row);
    }
    return iam_page_cnt;
}

void trace_page_IAM(db::datapage const * const data, db::slot_array const & slot)
{
    SDL_ASSERT(data->head->data.type == db::pageType::type::IAM);
    trace_iam_page(db::iam_page(data->head));
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
                case db::pageType::type::data:      // = 1
                    trace_page_data(data, slot);
                    break;
                case db::pageType::type::textmix:   // = 3
                    trace_page_textmix(data, slot);
                    break;  
                case db::pageType::type::IAM:       // = 10
                    trace_page_IAM(data, slot);
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
                    vec_type & vec,
                    const char * const sys_obj_name,
                    bool const dump_mem)
{
    size_t row_index = 0; // [in/out]
    size_t index = 0;
    for (auto & p : vec) {
        std::cout
            << sys_obj_name << " page[" << (index++) << "] at "
            << db::to_string::type(p->head->data.pageId);
        trace_sys_page<sys_info>(db, p, sys_obj_name, &row_index, dump_mem);
    }
    std::cout << "\n" << sys_obj_name << " pages = " << index << "\n\n";
}

void trace_sysallocunits(db::database & db, bool const dump_mem)
{
    trace_sys_list<db::sysallocunits_row_info>(db, db._sysallocunits, "sysallocunits", dump_mem);
}

void trace_sysschobjs(db::database & db, bool const dump_mem)
{
    trace_sys_list<db::sysschobjs_row_info>(db, db._sysschobjs, "sysschobjs", dump_mem);
}

void trace_syscolpars(db::database & db, bool const dump_mem)
{
    trace_sys_list<db::syscolpars_row_info>(db, db._syscolpars, "syscolpars", dump_mem);
}

void trace_sysscalartypes(db::database & db, bool const dump_mem)
{
    trace_sys_list<db::sysscalartypes_row_info>(db, db._sysscalartypes, "sysscalartypes", dump_mem);
}

void trace_sysidxstats(db::database & db, bool const dump_mem)
{
    trace_sys_list<db::sysidxstats_row_info>(db, db._sysidxstats, "sysidxstats", dump_mem);
}

void trace_sysobjvalues(db::database & db, bool const dump_mem)
{
    trace_sys_list<db::sysobjvalues_row_info>(db, db._sysobjvalues, "sysobjvalues", dump_mem);
}

void trace_sysiscols(db::database & db, bool const dump_mem)
{
    trace_sys_list<db::sysiscols_row_info>(db, db._sysiscols, "sysiscols", dump_mem);
}

/*FIXME:
Each IAM page in the chain covers a particular GAM interval and represents the bitmap where each bit indicates
if a corresponding extent stores the data that belongs to a particular allocation unit for a particular object. 
In addition, the first IAM page for the object stores the actual page addresses for the first eight object pages,
which are stored in mixed extents.
*/

void dump_iam_page_row(db::iam_page_row const * const iam_page_row, size_t const iam_page_cnt)
{
    std::cout
        << "\n\n" << db::iam_page_row_info::type_meta(*iam_page_row)
        << "\nDump iam_page_row("
        << iam_page_cnt
        << "):\n"
        << db::iam_page_row_info::type_raw(*iam_page_row);
}

void trace_datatable(db::database & db, bool const dump_mem)
{
    for (auto & tt : db._datatables) {
        db::datatable & table = *tt.get();
        std::cout << "\nDATATABLE [" << table.ut().name() << "]";
        std::cout << " [" << db::to_string::type(table.ut().get_id()) << "]";
        size_t alloc_cnt = 0;
        for (auto it = table._sysalloc.begin(); it != table._sysalloc.end(); ++it) {
            auto row = *it;
            A_STATIC_CHECK_TYPE(db::sysallocunits_row const *, row);
            std::cout << "\nalloc[" << (alloc_cnt++) << "]";
            std::cout << " pgfirst = " << db::to_string::type(row->data.pgfirst);
            std::cout << " pgfirstiam = " << db::to_string::type(row->data.pgfirstiam);
            std::cout << " type = " << db::to_string::type(row->data.type);
            std::cout << " nextiam = " << db::to_string::type(db.nextPage(row->data.pgfirstiam));
            for (auto & iam : table._sysalloc.pgfirstiam(it)) {
                A_STATIC_CHECK_TYPE(db::iam_page*, iam.get());
                auto & iam_page = *iam.get();
                size_t iam_page_cnt = 0;
                for (auto iam_page_row : iam_page) {
                    A_STATIC_CHECK_TYPE(db::iam_page_row const *, iam_page_row);
                    if (dump_mem) {
                        dump_iam_page_row(iam_page_row, iam_page_cnt);
                    }
                    if (!iam_page_cnt) {
                        for (size_t i = 0; i < count_of(iam_page_row->data.slot_pg); ++i) {
                            auto & pid = iam_page.head->data.pageId;
                            auto & id = iam_page_row->data.slot_pg[i];
                            std::cout 
                                << "\niam_slot["
                                << pid.fileId << ":" << pid.pageId << "]["
                                << iam_page_cnt << "]["
                                << i
                                << "] = " 
                                << id.fileId << ":" 
                                << id.pageId;
                            if (!id.is_null()) {
                                std::cout << " " << db::to_string::type(db.get_pageType(id));
                            }
                        }
                    }
                    else {
                        std::cout << "\n\nFIXME: parse uniform extent:\n";
                        //dump_iam_page_row(iam_page_row, iam_page_cnt);
                        break;
                    }
                    ++iam_page_cnt;
                }
                //FIXME: SDL_ASSERT(iam_page_cnt == iam_page.size()); // parse uniform extent
                auto & d = iam->head->data.pageId;
                std::cout
                << "\n[" 
                << d.fileId << ":" << d.pageId
                << "] iam_page_row count = "
                << iam_page_cnt
                << std::endl;
            }
        }
        std::cout << std::endl;
    }
}

void trace_user_tables(db::database & db)
{
    size_t index = 0;
    for (auto & ut : db._usertables) {
        std::cout << "\nUSER_TABLE[" << (index++) << "]:\n";
        std::cout << db::usertable::type_schema(*ut.get());
    }
    std::cout << "\nUSER_TABLE COUNT = " << index << std::endl;
}

template<class T>
void trace_access(T & pa, const char * const name)
{
    int i = 0;
    for (auto & p : pa) {
        ++i;
        SDL_ASSERT(p.get());
    }
    std::cout << name << " = " << i << std::endl;
}

template<class bootpage>
void trace_boot_page(db::database & db, bootpage const & boot, bool const dump_mem)
{
    if (boot) {
        auto & h = *(boot->head);
        auto & b = *(boot->row);
        SDL_ASSERT(db.is_allocated(h.data.pageId)); // test api
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
    else {
        SDL_ASSERT(0);
    }
}

void trace_pfs_page(db::database & db, bool const dump_mem)
{
    for (auto const & pfs : db._pfs_page) {
        auto const & pageId = pfs->head->data.pageId;
        std::cout
            << "\nPage Free Space[1:" << pageId.pageId << "]\n\n"
            << db::page_info::type_meta(*pfs->head)
            << "\nPFS: Page Alloc Status\n";
        auto & body = pfs->row->data.body;
        size_t range = 0;
        std::string s1, s2;
        const size_t max_count = a_min(db.page_count(), A_ARRAY_SIZE(body));
        auto print_range = [&s1](size_t range, size_t i) {
            if (!s1.empty()) {
                SDL_ASSERT(range < i);
                if (range + 1 < i) {
                    std::cout << "\n(1:" << range << ") - (1:" << (i - 1) << ") = ";
                }
                else {
                    std::cout << "\n(1:" << range << ") = ";
                }
                std::cout << s1;
            }
        };
        for (size_t i = 0; i < max_count; ++i) {
            s2 = db::to_string::type(body[i]);
            if (s2 != s1) {
                print_range(range, i);
                s1.swap(s2);
                range = i;
            }
            if (0) { // test api
                SDL_ASSERT(body[i].is_allocated() == db.is_allocated(db::pageFileID{uint32(i), 1}));
            }
        }
        print_range(range, max_count);
        if (dump_mem) {
            dump_whole_page(pfs->head);
        }
        std::cout << std::endl;
    }
}

int run_main(int argc, char* argv[])
{
#if SDL_DEBUG
    std::cout << "\nSDL_DEBUG=1\n";
#else
    std::cout << "\nSDL_DEBUG=0\n";
#endif
    std::cout << __DATE__  << " " << __TIME__ << std::endl;

    auto print_help = [](int argc, char* argv[])
    {
        std::cout
            << "\nBuild date: " << __DATE__
            << "\nBuild time: " << __TIME__
            << "\nUsage: " << argv[0]
            << "\n[-i|--input_file] path to mdf file"
            << "\n[-d|--dump_mem]"
            << "\n[-m|--max_page]"
            << "\n[-p|--page_num]"
            << "\n[-s|--page_sys]"
            << "\n[-f|--print_file]"
            << "\n[-b|--boot_page]"
            << "\n[-u|--user_table]"
            << "\n[-a|--alloc_page]"
            << std::endl;
    };
    CmdLine cmd;

    struct cmd_option {
        std::string mdf_file;
        bool dump_mem = 0;
        int max_page = 0;
        int page_num = -1;
        int page_sys = 0;
        bool file_header = false;
        bool boot_page = true;
        bool user_table = false;
        bool alloc_page = false;
    } opt;

    cmd.add(make_option('i', opt.mdf_file, "input_file"));
    cmd.add(make_option('d', opt.dump_mem, "dump_mem"));
    cmd.add(make_option('m', opt.max_page, "max_page"));
    cmd.add(make_option('p', opt.page_num, "page_num"));
    cmd.add(make_option('s', opt.page_sys, "page_sys"));
    cmd.add(make_option('f', opt.file_header, "file_header"));
    cmd.add(make_option('b', opt.boot_page, "boot_page"));
    cmd.add(make_option('u', opt.user_table, "user_table"));
    cmd.add(make_option('a', opt.alloc_page, "alloc_page"));

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
        << "\npage_sys = " << opt.page_sys
        << "\nfile_header = " << opt.file_header
        << "\nboot_page = " << opt.boot_page
        << "\nuser_table = " << opt.user_table
        << "\nalloc_page = " << opt.alloc_page
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
        trace_boot_page(db, db.get_bootpage(), opt.dump_mem);
        trace_pfs_page(db, opt.dump_mem);
    }
    if (opt.file_header) {
        auto p = db.get_fileheader();
        trace_sys_page<db::fileheader_row_info>(db, p, "fileheader", nullptr, opt.dump_mem);
        std::cout << db::to_string::type(p->slot);
    }
    if (opt.page_num >= 0) {
        trace_page(db, db.get_datapage(opt.page_num).get(), opt.dump_mem);
    }
    const int max_page = a_min(opt.max_page, int(page_count));
    for (int i = 0; i < max_page; ++i) {
        trace_page(db, db.get_datapage(db::make_page(i)).get(), opt.dump_mem);
    }
    if (opt.page_sys) {
        std::cout << std::endl;
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
    if (opt.alloc_page) {
        std::cout << "\nTEST PAGE ACCESS:\n";
        trace_access(db._sysallocunits, "_sysallocunits");
        trace_access(db._sysschobjs, "_sysschobjs");
        trace_access(db._syscolpars, "_syscolpars");
        trace_access(db._sysidxstats, "_sysidxstats");
        trace_access(db._sysscalartypes, "_sysscalartypes");
        trace_access(db._sysobjvalues, "_sysobjvalues");
        trace_access(db._sysiscols, "_sysiscols");
        trace_access(db._usertables, "_usertables");
        trace_access(db._datatables, "_datatables");
        trace_datatable(db, opt.dump_mem);
    }
    return EXIT_SUCCESS;
}

} // namespace 

int main(int argc, char* argv[])
{
    try {
        return run_main(argc, argv);
    }
    catch (sdl_exception & e) {
        SDL_TRACE_3(typeid(e).name(), " = ", e.what());
    }
    catch (std::exception & e) {
        SDL_TRACE_2("exception = ", e.what());
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

