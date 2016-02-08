// main.cpp : Defines the entry point for the console application.
//
#include "common/common.h"
#include "system/page_head.h"
#include "system/page_info.h"
#include "system/database.h"
#include "system/version.h"
#include "system/output_stream.h"
#include "third_party/cmdLine/cmdLine.h"

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

template<class page_ptr>
void trace_sys_page(
            db::database & db, 
            page_ptr const & p,
            size_t * const row_index,
            bool const dump_mem) 
{
    enum { dump_slot = 0 };
    using sys_obj = typename page_ptr::element_type;
    using sys_row = typename sys_obj::row_type;
    using sys_info = typename sys_row::info;
    if (p) {
        const char * const sys_obj_name = sys_obj::name();
        auto const & obj = *p.get();
        auto const & pageId = obj.head->data.pageId;
        std::cout
            << "\n\n" << sys_obj_name << " @"
            << db.memory_offset(obj.head)
            << ":\n\n"
            << db::page_info::type_meta(*obj.head);
        if (dump_slot) {
            std::cout << db::to_string::type(p->slot) << std::endl;
        }
        else {
            std::cout << "\nslotCnt = " << obj.slot.size() << std::endl;
        }
        for (size_t i = 0; i < obj.slot.size(); ++i) {
            sys_row const * const row = obj[i];
            std::cout
                << "\n\n" << sys_obj_name << "_row(" << i << ") @"
                << db.memory_offset(row)
                << " ";
            if (row_index) {
                std::cout << "[" << (*row_index)++ << "] ";
                std::cout << "[" << pageId.fileId << ":" << pageId.pageId << "] ";
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
        std::cout << std::endl;
    }
    else
    {
        SDL_ASSERT(0);
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
        const bool has_null = h->has_null();
        const bool has_variable = h->has_variable();
        if (has_null && has_variable) {
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
                << "\n\nrow_head[" << slot_id << "] = "
                << db::to_string_with_head::type(*h)
                << db::to_string::type_raw(mem) << std::endl
                << db::to_string::type(row.null) << std::endl;

            std::cout
                << db::to_string::type(row.variable)
                << std::endl;
        }
        else {
            if (h->is_forwarding_record()) {
                const db::forwarding_record forward(h);
                std::cout
                    << "\npage(" << page_id.pageId << ")"
                    << " slot = " << slot_id
                    << "\nstatusA = " << db::to_string::type(h->data.statusA)
                    << "\nforwarding_record = "
                    << db::to_string::type(forward.row())
                    << std::endl; 
            }
            else {
                std::cout
                    << "\nrow_head[" << slot_id << "] = "
                    << db::to_string_with_head::type(*h)
                    << "\nhas_null = " << has_null
                    << "\nhas_variable = " << has_variable
                    << std::endl;            
            }
        }
    }
}

void trace_page_index(db::database & db, db::datapage const * data, db::slot_array const & slot)
{
    SDL_ASSERT(data->head->data.type == db::pageType::type::index);
    const size_t slot_size = slot.size();
    for (size_t slot_id = 0; slot_id < slot_size; ++slot_id) {
        db::row_head const * const h = (*data)[slot_id];
        std::cout
            << "\nrow_head[" << slot_id << "] = "
            << db::to_string_with_head::type(*h);
        if (h->is_index_record()) {
            //FIXME: 11 = 4+6+1 = sizeof(key)+sizeof(pageFileID)+sizeof(status)
            std::cout 
                << "\nMemory Dump @" << db.memory_offset(h)
                << ":\n" << db::to_string::dump_mem(h, 11);
        }
        std::cout << std::endl;
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

void trace_page_IAM(db::datapage const * const data, db::slot_array const & slot)
{
    SDL_ASSERT(data->head->data.type == db::pageType::type::IAM);
    const db::iam_page iampage(data->head);
    std::cout 
        << db::to_string::type(iampage.head->data.pageId) << " " 
        << db::to_string::type(iampage.head->data.type)
        << std::endl;
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
                case db::pageType::type::index:     // = 2
                    trace_page_index(db, data, slot);
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

template<class page_access>
void trace_sys_list(db::database & db, 
                    page_access & vec,
                    bool const dump_mem)
{
    using datapage = typename page_access::value_type;
    const char * const sys_obj_name = datapage::name();
    size_t row_index = 0; // [in/out]
    size_t index = 0;
    for (auto & p : vec) {
        std::cout
            << sys_obj_name << " page[" << (index++) << "] at "
            << db::to_string::type(p->head->data.pageId);
        trace_sys_page(db, p, &row_index, dump_mem);
    }
    std::cout << "\n" << sys_obj_name << " pages = " << index << "\n\n";
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

void trace_datatable_iam(db::database & db, db::datatable & table, 
    db::dataType::type const data_type, bool const dump_mem)
{
    enum { print_nextPage = 1 };
    enum { long_pageId = 0 };
    enum { alloc_pageType = 0 };

    auto printPage = [](const char * name, const db::pageFileID & id) {
        if (!id.is_null()) {
            std::cout << name 
                << db::to_string::type(id, db::to_string::type_format::less);
        }
    };
    for (auto const row : table._sysalloc(data_type)) {
        A_STATIC_CHECK_TYPE(db::sysallocunits_row const * const, row);
        std::cout 
            << "\nsysalloc[" << table.name() << "][" 
            << db::to_string::type_name(data_type) << "]";
        std::cout << " pgfirst = " << db::to_string::type(row->data.pgfirst);
        std::cout << " pgroot = " << db::to_string::type(row->data.pgroot);            
        std::cout << " pgfirstiam = " << db::to_string::type(row->data.pgfirstiam);
        std::cout << " type = " << db::to_string::type(row->data.type);
        if (print_nextPage) {
            printPage(" nextIAM = ", db.nextPageID(row->data.pgfirstiam));
        }
        std::cout << " @" << db.memory_offset(row);
        for (auto & iam : db.pgfirstiam(row)) {
            A_STATIC_CHECK_TYPE(db::iam_page*, iam.get());
            auto const & iam_page = *iam.get();
            auto const & pid = iam_page.head->data.pageId;
            size_t iam_page_cnt = 0;
            if (db::iam_page_row const * const iam_page_row = iam_page.first()) {
                if (dump_mem) {
                    dump_iam_page_row(iam_page_row, iam_page_cnt);
                }
                else {
                    auto const & d = iam_page_row->data;
                    std::cout
                        << "\niam_page[" << pid.fileId << ":" << pid.pageId << "] "
                        << "start_pg = " << db::to_string::type(d.start_pg) << " ["
                        << table.name() << "]";
                }
                for (size_t i = 0; i < iam_page_row->size(); ++i) {
                    auto & id = (*iam_page_row)[i];
                    std::cout
                        << "\niam_slot["
                        << pid.fileId << ":" << pid.pageId
                        << "]["
                        << iam_page_cnt << "]["
                        << i
                        << "] = ";
                    if (long_pageId) {
                        std::cout << db::to_string::type(id);
                    }
                    else {
                        std::cout << id.fileId << ":" << id.pageId;
                    }
                    if (!id.is_null()) {
                        std::cout << " " << db::to_string::type(db.get_pageType(id));
                    }
                    if (print_nextPage) {
                        printPage(" nextPage = ", db.nextPageID(id));
                        printPage(" prevPage = ", db.prevPageID(id));
                    }
                }
                ++iam_page_cnt;
            }
            if (1) {
                size_t alloc_cnt = 0;
                iam_page.allocated_extents([&db, &alloc_cnt, &pid](db::iam_page::fun_param id){
                    std::cout 
                        << "\n[" << (alloc_cnt++) 
                        << "] ALLOCATED Ext = [" 
                        << id.fileId << ":" 
                        << id.pageId << "]";
                    if (alloc_pageType) {
                        std::cout << " " << db::to_string::type(db.get_pageType(id));
                    }
                    std::cout
                        << " IAMPID = ["
                        << pid.fileId << ":" 
                        << pid.pageId << "]";
                });
                std::cout
                    << "\niam_ext["
                    << pid.fileId << ":" << pid.pageId << "]["
                    << iam_page_cnt << "] alloc_cnt = "
                    << alloc_cnt;
                ++iam_page_cnt;
            }
            SDL_ASSERT(iam_page_cnt == iam_page.size());
            auto & d = iam->head->data.pageId;
            std::cout
            << "\n[" 
            << d.fileId << ":" << d.pageId
            << "] iam_page_row count = "
            << iam_page_cnt
            << std::endl;
        }
    }
}

void trace_datarow(db::datatable & table,
                   db::dataType::type const t1,
                   db::pageType::type const t2)
{
    enum { test_reverse_iteration = 0 };
    size_t row_cnt = 0;
    size_t forwarding_cnt = 0;
    size_t forwarded_cnt = 0;
    size_t null_row_cnt = 0;
    db::datatable::for_datarow(table._datarow(t1, t2), [&](db::row_head const & row) {
        if (row.is_forwarding_record()) {
            ++forwarding_cnt;
        }
        else {
            ++row_cnt;
        }
        if (row.is_forwarded_record()) {
            ++forwarded_cnt;
        }
    });
    SDL_ASSERT(forwarding_cnt == forwarded_cnt);
    if (row_cnt || forwarding_cnt || null_row_cnt) {
        std::cout
            << "\nDATAROW [" << table.name() << "]["
            << db::to_string::type_name(t1) << "]["
            << db::to_string::type_name(t2) << "] = "
            << row_cnt;
        if (forwarding_cnt) {
            std::cout << " forwarding = " << forwarding_cnt;
        }
        if (null_row_cnt) {
            std::cout << " null_row = " << null_row_cnt;
        }
    }
    if (test_reverse_iteration) {
        auto _datarow = table._datarow(t1, t2);
        auto p1 = _datarow.begin();
        auto p2 = _datarow.end();
        SDL_ASSERT(p1 == _datarow.begin());
        SDL_ASSERT(p2 == _datarow.end());
        while (p1 != p2) {
            --p2;
            if (db::row_head const * row = *p2) {
                if (!row->is_forwarding_record()) {
                    SDL_ASSERT(row_cnt);
                    --row_cnt;
                }
            }
        }
        SDL_ASSERT(!row_cnt);
    }
    if (1) {
        for (auto record : table._record) {
            for (auto & col : record.cols()) {
                SDL_ASSERT(col.type != db::scalartype::t_none);
            }
        }
    }
}

void trace_datapage(db::datatable & table, 
                    db::dataType::type const t1,
                    db::pageType::type const t2)
{
    auto datapage = table._datapage(t1, t2);
    size_t i = 0;
    for (auto p : datapage) {
        A_STATIC_CHECK_TYPE(db::page_head const *, p);
        if (0 == i) {
            std::cout
                << "\nDATAPAGE [" << table.name() << "]["
                << db::to_string::type_name(t1) << "]["
                << db::to_string::type_name(t2) << "]";
        }
        std::cout << "\n[" << (i++) << "] = ";
        std::cout << db::to_string::type(p->data.pageId);
    }
    if (i) {
        std::cout << std::endl;
    }
}

void trace_datatable(db::database & db, bool const dump_mem)
{
    enum { trace_iam = 1 };
    enum { print_nextPage = 1 };
    enum { long_pageId = 0 };
    enum { alloc_pageType = 0 };

    for (auto & tt : db._datatables) {
        db::datatable & table = *tt.get();
        std::cout << "\nDATATABLE [" << table.name() << "]";
        std::cout << " [" << db::to_string::type(table.get_id()) << "]";
        if (trace_iam) {
            db::for_dataType([&db, &table, dump_mem](db::dataType::type t){
                trace_datatable_iam(db, table, t, dump_mem);
            });
        }
        if (1) {
            db::for_dataType([&table](db::dataType::type t1){
            db::for_pageType([&table, t1](db::pageType::type t2){
                trace_datapage(table, t1, t2);
            });
            });
        }
        if (1) {
            db::for_dataType([&table](db::dataType::type t1){
            db::for_pageType([&table, t1](db::pageType::type t2){
                trace_datarow(table, t1, t2);
            });
            });
        }
        std::cout << std::endl;
    }
}

void trace_user_tables(db::database & db)
{
    size_t index = 0;
    for (auto & ut : db._usertables) {
        std::cout << "\nUSER_TABLE[" << (index++) << "]:\n";
        std::cout << ut->type_schema();
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

void trace_version()
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
        << "\n[--silence]"
        << std::endl;
}

int run_main(int argc, char* argv[])
{
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
        bool silence = false;
    } opt;

    CmdLine cmd;
    cmd.add(make_option('i', opt.mdf_file, "input_file"));
    cmd.add(make_option('d', opt.dump_mem, "dump_mem"));
    cmd.add(make_option('m', opt.max_page, "max_page"));
    cmd.add(make_option('p', opt.page_num, "page_num"));
    cmd.add(make_option('s', opt.page_sys, "page_sys"));
    cmd.add(make_option('f', opt.file_header, "file_header"));
    cmd.add(make_option('b', opt.boot_page, "boot_page"));
    cmd.add(make_option('u', opt.user_table, "user_table"));
    cmd.add(make_option('a', opt.alloc_page, "alloc_page"));
    cmd.add(make_option(0, opt.silence, "silence"));

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

    std::unique_ptr<scoped_null_cout> scoped_silence;
    if (opt.silence) {
        reset_new(scoped_silence);
    }
    trace_version();

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
        << "\nsilence = " << opt.silence
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
        trace_sys_page(db, db.get_fileheader(), nullptr, opt.dump_mem);
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
        trace_sys_list(db, db._sysallocunits, opt.dump_mem);
        trace_sys_list(db, db._sysschobjs, opt.dump_mem);
        trace_sys_list(db, db._syscolpars, opt.dump_mem);
        trace_sys_list(db, db._sysscalartypes, opt.dump_mem);
        trace_sys_list(db, db._sysidxstats, opt.dump_mem);
        trace_sys_list(db, db._sysobjvalues, opt.dump_mem);
        trace_sys_list(db, db._sysiscols, opt.dump_mem);
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
        SDL_ASSERT(0);
    }
    catch (std::exception & e) {
        SDL_TRACE_2("exception = ", e.what());
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

