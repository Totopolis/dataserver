// main.cpp : Defines the entry point for the console application.
// main.cpp is used for tests and research only and is not part of the dataserver library.
#include "dataserver/system/database.h"
#include "dataserver/system/version.h"
#include "dataserver/maketable/generator.h"
#include "dataserver/maketable/generator_util.h"
#include "dataserver/maketable/export_database.h"
#include "dataserver/spatial/geography_info.h"
#include "dataserver/spatial/interval_cell.h"
#include "dataserver/spatial/interval_set.h"
#include "dataserver/third_party/cmdLine/cmdLine.h"
#include "dataserver/common/outstream.h"
#include "dataserver/common/locale.h"
#include "dataserver/common/time_util.h"
#include "dataserver/common/spinlock.h"
#include "dataserver/utils/conv.h"
#include "dataserver/system/page_info.h"
#include <map>
#include <set>
#include <fstream>
#include <iomanip> // for std::setprecision

#if SDL_DEBUG_maketable
#include "dataserver/usertables/maketable_test.h"
namespace sdl { namespace db { namespace make {
    void test_maketable_$$$(database const &);
}}}
#endif

namespace {

using namespace sdl;

struct cmd_option : noncopyable {
    std::string mdf_file;
    std::string out_file;
    bool dump_mem = 0;
    int page_num = -1;
    int page_sys = 0;
    bool file_header = false;
    bool boot_page = false;
    bool user_table = false;
    bool internal = false;
    int alloc_page = 0; // 0-3
    bool silence = false;
    int record_num = 0; // 10
    int index = 0;
    int max_output = 10;
    int verbosity = 0;
    std::string col_name;
    std::string tab_name;
    std::string index_key;
    bool write_file = false;
    int spatial_page = 0;
    int64 pk0 = 0;
    int64 pk1 = 0;
    int cells_per_object = 0;
    bool index_for_table = false;
    std::string cell_id;
    double latitude = 0;
    double longitude = 0;
    int depth = 0;
    bool export_cells;
    std::string poi_file; //ID, POINT(Lon, Lat)
    size_t test_performance = 0;
    bool test_maketable = false;
    bool trace_poi_csv = false;
    double range_meters = 0;
    bool test_for_range = false;
    bool test_for_rect = false;
    db::spatial_rect test_rect {};
    db::spatial_point test_point { 360, 360 }; // set invalid
    bool full_globe = false;
    std::string include;
    std::string exclude;
    db::make::export_database::param_type export_database;
    int precision = 0;
    bool record_count = false;
    bool trim_space = false;
    std::string _namespace;
    std::string schema_names;
    std::string dump_pages;
    bool checksum = false;
};

template<class sys_row>
void trace_var(sys_row const *, Int2Type<0>){}

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
void trace_null(sys_row const *, Int2Type<0>){}

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
void trace_col_name(sys_row const *, Int2Type<0>){}

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
            db::database const & db, 
            page_ptr const & p,
            size_t * const row_index,
            cmd_option const & opt) 
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
            if (opt.dump_mem) {
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

void trace_page_data(db::database const &, db::page_head const * const head)
{
    SDL_ASSERT(head->is_data());
    db::datapage const data(head);
    auto const & page_id = head->data.pageId;
    for (size_t slot_id = 0; slot_id < data.size(); ++slot_id) {
        db::row_head const * const h = data[slot_id];
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

template<class T>
void trace_index_page_row(db::database const & db, db::index_page_row_t<T> const & row, size_t const i)
{
    std::cout
        << "\nstatusA = " << db::to_string::type(row.data.statusA)
        << "\nkey = " << db::to_string::type(row.data.key);
    if (0 == i) {
        std::cout << " [NULL]";
    }
    std::cout << "\npage = " << db::to_string::type(row.data.page);
    std::cout << " " << db::to_string::type(db.get_pageType(row.data.page));
    std::cout << std::endl;
}

template<typename key_type>
void trace_page_index_t(db::database const & db, db::page_head const * const head)
{
    using index_page_row = db::index_page_row_t<key_type>;
    using index_page = db::datapage_t<index_page_row>;
    SDL_ASSERT(head->data.pminlen == sizeof(index_page_row));
    index_page const data(head);
    for (size_t slot_id = 0; slot_id < data.size(); ++slot_id) {
        auto const & row = *data[slot_id];
        std::cout << "\nindex[" << slot_id << "]";
        trace_index_page_row(db, row, slot_id);
    }
}

void trace_spatial_index(db::database const &, db::page_head const * const head)
{
    SDL_ASSERT(head->data.type == db::pageType::type::index);
    SDL_ASSERT(head->data.pminlen == sizeof(db::bigint::spatial_tree_row));
    using index_page = db::datapage_t<db::bigint::spatial_tree_row>;
    index_page const data(head);
    for (size_t slot_id = 0; slot_id < data.size(); ++slot_id) {
        auto const & row = *data[slot_id];
        std::cout << "\nspatial_tree_row[" << slot_id << "][" << db::to_string::type_less(head->data.pageId) << "]\n";
        std::cout << db::bigint::spatial_tree_row_info::type_meta(row);
        SDL_ASSERT(row.get_type() == db::recordType::index_record);
    }
}

void trace_page_index(db::database const & db, db::page_head const * const head) // experimental
{
    SDL_ASSERT(head->is_index());    
    switch (head->data.pminlen) {
    case sizeof(db::index_page_row_t<uint32>): // 7+4 bytes
        trace_page_index_t<uint32>(db, head); 
        break;
    case sizeof(db::index_page_row_t<uint64>): // 7+8 bytes
        trace_page_index_t<uint64>(db, head);
        break;
#if 0
    case sizeof(db::index_page_row_t<db::guid_t>): // 7+16 bytes 
        trace_page_index_t<db::guid_t>(db, head); //FIXME: guid or pair of 8-byte keys
        break;
#else
    case sizeof(db::index_page_row_t<db::pair_key<uint64>>): // 7+16 bytes 
        trace_page_index_t<db::pair_key<uint64>>(db, head); 
        break;
#endif
    case sizeof(db::bigint::spatial_tree_row): // 20 bytes
        if (head->data.type == db::pageType::type::index) {
            trace_spatial_index(db, head);
        }
        break;
    default:
        SDL_TRACE("trace_page_index = ", head->data.pminlen);
        SDL_ASSERT(0); //not implemented
        break;
    }
}

void trace_page_textmix(db::database const &, db::page_head const * const head)
{
    SDL_ASSERT(head->data.type == db::pageType::type::textmix);
    db::datapage const data(head);
    auto const & page_id = head->data.pageId;
    for (size_t slot_id = 0; slot_id < data.size(); ++slot_id) {
        db::row_head const * const h = data[slot_id];
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

void trace_page_IAM(db::database const &, db::page_head const * const head)
{
    SDL_ASSERT(head->data.type == db::pageType::type::IAM);
    const db::iam_page iampage(head);
    std::cout 
        << db::to_string::type(iampage.head->data.pageId) << " " 
        << db::to_string::type(iampage.head->data.type)
        << std::endl;
}

void trace_page(db::database const & db, db::pageIndex const page_num, cmd_option const & opt)
{
    db::page_head const * const p = db.load_page_head(page_num);
    if (db.is_allocated(p)) {
        {
            auto const & id = p->data.pageId;
            std::cout
                << "\n\npage(" << id.pageId << ") @"
                << db.memory_offset(p);
            std::cout
                << "\n\n"
                << db::page_info::type_meta(*p) << "\n"
                << db::to_string::type(db::slot_array(p))
                << std::endl;
        }
        if (opt.dump_mem) {
            switch (p->data.type) {
            case db::pageType::type::data:      trace_page_data(db, p);     break;
            case db::pageType::type::index:     trace_page_index(db, p);    break;
            case db::pageType::type::textmix:   trace_page_textmix(db, p);  break;  
            case db::pageType::type::IAM:       trace_page_IAM(db, p);      break;
            default:
                break;
            }
        }
    }
    else {
        std::cout << "\npage [" << page_num << "] not allocated" << std::endl;
    }
}

template<class page_access>
void trace_sys_list(db::database const & db, 
                    page_access & vec,
                    cmd_option const & opt)
{
    using datapage = typename page_access::value_type;
    const char * const sys_obj_name = datapage::name();
    size_t row_index = 0; // [in/out]
    size_t index = 0;
    for (auto & p : vec) {
        std::cout
            << sys_obj_name << " page[" << (index++) << "] at "
            << db::to_string::type(p->head->data.pageId);
        trace_sys_page(db, p, &row_index, opt);
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

void trace_datatable_iam(db::database const & db, db::datatable & table, 
    db::dataType::type const data_type, cmd_option const & opt)
{
    enum { print_nextPage = 1 };
    enum { long_pageId = 0 };
    enum { alloc_pageType = 1 };

    auto print_pageFileID = [&db](const char * name, const db::pageFileID & id) {
        if (id) {
            std::cout << name << db::to_string::type_less(id);
            std::cout << " " << db::to_string::type(db.get_pageType(id));
        }
    };
    auto print_pageType = [&db](const db::pageFileID & id) {
        if (id) {
            std::cout << " " << db::to_string::type(db.get_pageType(id));
        }
    };
    for (auto const row : table.get_sysalloc(data_type)) {
        A_STATIC_CHECK_TYPE(db::sysallocunits_row const * const, row);
        std::cout << "\nsysalloc[" << table.name() << "][" 
            << db::to_string::type_name(data_type) << "]";
        std::cout << " pgroot = " << db::to_string::type_less(row->data.pgroot); print_pageType(row->data.pgroot);
        std::cout << " pgfirst = " << db::to_string::type_less(row->data.pgfirst);
        if (row->data.pgroot) {
            print_pageType(row->data.pgfirst);
        }
        else {
            std::cout << " [NULL]";
        }
        std::cout << " pgfirstiam = " << db::to_string::type_less(row->data.pgfirstiam); print_pageType(row->data.pgfirstiam);
        std::cout << " type = " << db::to_string::type(row->data.type);
        if (auto id = db.nextPageID(row->data.pgfirstiam)) {
            std::cout << " nextiam = " << db::to_string::type_less(id);
        }
        std::cout << " ownerid = " << db::to_string::type(row->data.ownerid);
        std::cout << " @" << db.memory_offset(row);
        for (auto & iam : db.pgfirstiam(row)) {
            A_STATIC_CHECK_TYPE(db::iam_page*, iam.get());
            auto const & iam_page = *iam.get();
            auto const & pid = iam_page.head->data.pageId;
            size_t iam_page_cnt = 0;
            if (db::iam_page_row const * const iam_page_row = iam_page.first()) {
                if (opt.dump_mem) {
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
                        print_pageFileID(" nextPage = ", db.nextPageID(id));
                        print_pageFileID(" prevPage = ", db.prevPageID(id));
                    }
                    if (db::page_head const * const head = db.load_page_head(id)) {
                        if (head->data.ghostRecCnt) {
                            std::cout << " ghostRecCnt = " << head->data.ghostRecCnt;
                        }
                    }
                }
                ++iam_page_cnt;
            }
            if (opt.alloc_page > 1) {
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
                SDL_ASSERT(iam_page_cnt == iam_page.size());
                auto & d = iam->head->data.pageId;
                std::cout
                    << "\n["
                    << d.fileId << ":" << d.pageId
                    << "] iam_page_row count = "
                    << iam_page_cnt;                
            }
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
}

void trace_datarow(db::datatable & table,
                   db::dataType::type const t1,
                   db::pageType::type const t2)
{
    size_t row_cnt = 0;
    size_t forwarding_cnt = 0;
    size_t forwarded_cnt = 0;
    size_t null_row_cnt = 0;
    size_t ghost_data_cnt = 0;
    db::datatable::for_datarow(table.get_datarow(t1, t2), [&](db::row_head const & row) {
        if (row.is_forwarding_record()) {
            ++forwarding_cnt;
        }
        else {
            ++row_cnt;
        }
        if (row.is_forwarded_record()) {
            ++forwarded_cnt;
        }
        if (row.get_type() == db::recordType::ghost_data) {
            ++ghost_data_cnt;
        }
    });
    SDL_ASSERT(forwarding_cnt == forwarded_cnt);
    if (row_cnt || forwarding_cnt || null_row_cnt || ghost_data_cnt) {
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
        if (ghost_data_cnt) {
            std::cout << " ghost_data = " << ghost_data_cnt;
        }
    }
}

void trace_datapage(db::datatable & table, 
                    db::dataType::type const t1,
                    db::pageType::type const t2)
{
    const auto datapage = table.get_datapage(t1, t2);
    size_t i = 0;
    for (auto const p : datapage) {
        A_STATIC_CHECK_TYPE(db::page_head const * const, p);
        if (0 == i) {
            std::cout
                << "\nDATAPAGE [" << table.name() << "]["
                << db::to_string::type_name(t1) << "]["
                << db::to_string::type_name(t2) << "]";
        }
        std::cout << "\n[" << (i++) << "] = ";
        std::cout << db::to_string::type(p->data.pageId);
        std::cout << " slotCnt = " << p->data.slotCnt;
        if (p->data.ghostRecCnt) {
            std::cout << " ghostRecCnt = " << p->data.ghostRecCnt;
        }
        size_t forwarding = 0;
        size_t forwarded = 0;
        for (auto s : db::datapage(p)) {
            if (s->is_forwarding_record()) { ++forwarding; }
            if (s->is_forwarded_record()) { ++forwarded; }
        }
        if (forwarding) { std::cout << " forwarding = " << forwarding; }
        if (forwarded) { std::cout << " forwarded = " << forwarded; }
    }
    if (i) {
        std::cout << std::endl;
    }
}

void trace_printable(std::string const & s, db::vector_mem_range_t const & vm, db::scalartype::type const type)
{
    if (type == db::scalartype::t_geography) {
        (void)vm;
#if 0 //defined(SDL_OS_WIN32)
        enum { block_len = sizeof(double)*2 };
        static_assert(block_len == 16, "");

        if (block_len && (s.size() > 4) && (db::mem_size(vm[0]) * 2 == s.size()))
        {
            enum { code_len = 4 }; // WGS84 = 0xE610 = 4326                        
            std::cout << std::endl << s.substr(0, code_len) << std::endl;

            size_t prefix = 0;            
            std::cout.precision(10);

            size_t point_num = 0;
            const size_t count = (s.size() + block_len - code_len - 1) / block_len;
            for (size_t i = 0; i < count; ++i) {
                if (i % 2) {
                    std::cout << "[" << (++point_num) << "] = ";
                }
                size_t const j = code_len + i * block_len;
                std::cout << s.substr(j, block_len);
                if (1) {
                    const double & p = * reinterpret_cast<const double *>(vm[0].first + j / 2);
                    std::cout << " (" << p << ") ";
                }
                if (i % 2)
                    std::cout << " ";
                else
                    std::cout << std::endl;
            }
        }
        else
#endif
        {
            std::cout << s; // memory dump
        }
    }
    else {
        for (unsigned char ch : s) {
            if ((ch > 31) && (ch < 127)) { // is printable ?
                std::cout << ch;
            }
            else {
                std::cout << "\\" << std::hex << int(ch) << std::dec;
            }
        }
    }
}

template<class output, class string>
void trace_trim_string(output & out, string && s, cmd_option const & opt)
{
    if (opt.trim_space) {
        out << db::to_string::trim(std::move(s));
    }
    else {
        out << s;
    }
}

void trace_string_value(std::string const & s, db::vector_mem_range_t const & vm,
                        db::scalartype::type const type,
                        cmd_option const & opt)
{
    switch (type) {
    case db::scalartype::t_text:
    case db::scalartype::t_char:
    case db::scalartype::t_varchar:
        trace_trim_string(std::wcout, db::conv::cp1251_to_wide(s), opt);
        break;
    case db::scalartype::t_ntext:
    case db::scalartype::t_nchar:
    case db::scalartype::t_nvarchar:
        trace_trim_string(std::wcout, db::conv::nchar_to_wide(vm), opt); 
        break;
    default:
        trace_printable(s, vm, type);
        break;
    }
}

void trace_record_value(std::string && s, db::vector_mem_range_t const & vm, db::scalartype::type const type,
                        cmd_option const & opt)
{
    size_t const length = s.size();
    bool concated = false;
    size_t const max_output = (opt.max_output > 0) ? (size_t)(opt.max_output) : (size_t)(-1);
    if (s.size() > max_output) { // limit output size
        s.resize(max_output);
        concated = true;
    }
    trace_string_value(s, vm, type, opt);
    if (concated) {
        std::cout << "(" << db::to_string::type(length) << ")";
    }
}

template<class T>
void test_geography(T const & record, size_t col_index)
{
    auto geo1 = record.geography(col_index);
    auto geo2 = std::move(geo1);
    SDL_ASSERT(geo2 && !geo1);
}

template<class T>
void trace_table_record(db::database const &, T const & record, cmd_option const & opt)
{
    enum { test_API = 0 };
    for (size_t col_index = 0; col_index < record.size(); ++col_index) {
        auto const & col = record.usercol(col_index);
        if (!opt.col_name.empty() && (col.name != opt.col_name)) {
            continue;
        }
        std::cout << " " << col.name << " = ";
        if (record.is_null(col_index)){
            std::cout << "NULL";
            continue;
        }
        if (record.data_col(col_index).empty()) {
            std::cout << "EMPTY";
            continue;
        }
        std::string type_col;
        if (col.type == db::scalartype::t_geography) {
            type_col = record.STAsText(col_index);
            if (test_API) {
                SDL_ASSERT(!record.STContains(col_index, {}));
            }
        } else {
            type_col = record.type_col(col_index);
        }
        if (test_API) {
            SDL_ASSERT(!record[col.name.c_str()].empty());
            SDL_ASSERT(!record[col.name].empty());
            SDL_ASSERT(!record.type_col_utf8(col_index).empty());
        }
        SDL_ASSERT(!type_col.empty());
        trace_record_value(std::move(type_col), record.data_col(col_index), col.type, opt);
    }
    if (opt.verbosity) {
        std::cout << " | fixed_data = " << record.fixed_size();
        std::cout << " var_data = " << record.var_size();
        std::cout << " null = " << record.count_null();                
        std::cout << " var = " << record.count_var();     
        std::cout << " fixed = " << record.count_fixed(); 
#if SDL_DEBUG_RECORD_ID
        std::cout << " [" << db::to_string::type(record.get_id()) << "]";
#endif
        if (auto stub = record.forwarded()) {
            std::cout << " forwarded from ["
                << db::to_string::type(stub->data.row)
                << "]";
        }
    }
}

struct find_index_key_t: noncopyable
{
    db::database const & db;
    db::datatable const & table;
    cmd_option const & opt;

    find_index_key_t(db::database const & _db, db::datatable const & _t, cmd_option const & _opt)
        : db(_db), table(_t), opt(_opt)
    {}

    template<class T> void find_record(T const & key);

    template<class T> // T = index_key_t
    void operator()(T) { 
        typename T::type k{};
        std::stringstream ss(opt.index_key);
        ss >> k;
        find_record(k);
    }
    void unexpected(db::scalartype::type) {}
};

struct wrap_find_index_key_t : noncopyable
{
    using ret_type = void; // optional
    find_index_key_t & fun;
    wrap_find_index_key_t(find_index_key_t & f) : fun(f){}
    template<class T> ret_type operator()(T t) { fun(t); }
    ret_type unexpected(db::scalartype::type) {}
};

template<class T>
void find_index_key_t::find_record(T const & key)
{
    if (auto col = table.get_PrimaryKeyOrder().first) {
        if (auto record = table.find_record_t(key)) {
            std::cout
                << "\nfind_record[" << table.name() << "][" 
                << col->name << " = " << key << "] => ["
#if SDL_DEBUG_RECORD_ID
                << db::to_string::type(record.get_id())
#endif
                << "]\n";
            std::cout << "[" << table.name() << "]";
            trace_table_record(db, record, opt);
        }
        else {
            std::cout
                << "\nfind_record[" << table.name() << "][" 
                << col->name << " = " << key << "] => not found";
        }
    }
    else {
        SDL_ASSERT(0);
    }
}

struct parse_index_key: noncopyable
{
    std::stringstream ss;
    std::vector<char> data;

    parse_index_key(std::string const& s): ss(s){}

    template<class T> // T = index_key_t
    void operator()(T) { 
        typename T::type k{};
        ss >> k;
        data.resize(sizeof(k));
        memcpy(data.data(), &k, sizeof(k));
    }
    void unexpected(db::scalartype::type) {}
};

struct wrap_parse_index_key : noncopyable
{
    parse_index_key & fun;
    wrap_parse_index_key(parse_index_key & f): fun(f){}
    template<class T> void operator()(T t) { fun(t); }
    void unexpected(db::scalartype::type) {}
};

struct find_composite_key_t : noncopyable
{
    db::database const & db;
    db::datatable const & table;
    cmd_option const & opt;

    find_composite_key_t(db::database const & _db, db::datatable const & _t, cmd_option const & _opt)
        : db(_db), table(_t), opt(_opt)
    {}

    void find_index_key(db::cluster_index const &);

    std::vector<char> parse_key(db::cluster_index const &);
};

std::vector<char> find_composite_key_t::parse_key(db::cluster_index const & index)
{
    std::vector<char> key_mem;
    std::string s = opt.index_key;
    s.erase(std::remove_if(s.begin(), s.end(), [](char const c){
        return (c == '(') || (c == ')');
    }), s.end());
    size_t pos = 0;
    for (size_t i = 0; i < index.size(); ++i) {
        if (!(pos < s.size())) {
            SDL_ASSERT(0);
            return {};
        }
        size_t p = s.find(',', pos);
        if (p == std::string::npos) 
            p = s.size();
        if (pos < p) {
            parse_index_key parser(s.substr(pos, p - pos));
            db::case_index_key(index[i].type, wrap_parse_index_key(parser));
            if (parser.data.empty()) {
                SDL_ASSERT(0);
                return {};
            }
            const size_t last = key_mem.size();
            key_mem.resize(key_mem.size() + parser.data.size());
            memcpy(key_mem.data() + last, parser.data.data(), parser.data.size());
        }
        else {
            SDL_ASSERT(0);
            return {};
        }
        pos = p + 1;
    }
    if (key_mem.size() == index.key_length()) {
        return key_mem;
    }
    SDL_ASSERT(0);
    return {};
}

void find_composite_key_t::find_index_key(db::cluster_index const & index)
{
    const std::vector<char> v = parse_key(index);
    if (!v.empty()) {
        const db::datatable::key_mem key(v.data(), v.data() + v.size());
        if (auto record = table.find_record(key)) {
            std::cout
                << "\nfind_record[" << table.name() << "][" << opt.index_key << "] => ["
#if SDL_DEBUG_RECORD_ID
                << db::to_string::type(record.get_id())
#endif
                << "]\n";
            std::cout << "[" << table.name() << "]";
            trace_table_record(db, record, opt);
        }
        else {
            std::cout
                << "\nfind_record[" << table.name() << "][" << opt.index_key << "] => not found";
        }
    }
}

void find_index_key(db::database const & db, cmd_option const & opt)
{
    if (opt.index_key.empty() || opt.tab_name.empty()) {
        return;
    }
    if (auto table = db.find_table(opt.tab_name)) {
        if (auto index = table->get_cluster_index()) {
            if (index->size() == 1) {
                find_index_key_t parser(db, *table, opt);
                db::case_index_key((*index)[0].type, wrap_find_index_key_t(parser));
            }
            else {
                std::cout << "\n[" << table->name() << "] composite index key\n"; 
                find_composite_key_t(db, *table, opt).find_index_key(*index);
            }
        }
        else {
            std::cout << "\n[" << table->name() << "] cluster index not found"; 
        }
    }
}

void trace_table_index(db::database const & db, db::datatable & table, cmd_option const & opt)
{
    enum { dump_key = 0 };
    enum { test_find = 1 };
    enum { test_sorting = 1 };
    enum { test_reverse = 0 };

    if (auto tree = table.get_index_tree()) {
        auto const cluster_index = table.get_cluster_index();
        auto & tree_row = tree->_rows;
        std::cout
            << "\n\n[" << table.name() << "] cluster_index = "
            << db::to_string::type(tree->root()->data.pageId) << " PK =";
        tree->index().for_column([](const db::usertable::column & c){
            std::cout << " " << c.name;
        });
        std::cout << std::endl;
        if (1) {
            auto const min_page = tree->min_page();
            auto const max_page = tree->max_page();
            SDL_ASSERT(min_page && !db.prevPageID(min_page));
            SDL_ASSERT(max_page && !db.nextPageID(max_page));
            std::cout 
                << "\nmin_page[" << table.name() << "] = "
                << db::to_string::type(min_page) << " "
                << db::to_string::type(db.get_pageType(min_page))
                << "\nmax_page[" << table.name() << "] = "
                << db::to_string::type(max_page) << " "
                << db::to_string::type(db.get_pageType(max_page))
                << std::endl;
        }
        std::pair<db::index_tree::key_mem, size_t> prev_key;
        size_t count = 0;
        auto const last = tree_row.end();
        for (auto it = tree_row.begin(); it != last; ++it) { 
            if ((opt.index != -1) && ((int)count >= opt.index))
                break;
            auto const row = *it;
            SDL_ASSERT(db::mem_size(row.first));
            std::cout << "\nindex[" << table.name() << "][" << count << "][" << tree_row.slot(it) << "]";
            std::cout << " key = " << tree->type_key(row.first);
            if (dump_key) {
                std::cout << " (" << db::to_string::dump_mem(row.first) << ")";
            }
            if (tree_row.is_key_NULL(it)) {
                std::cout << " [NULL]";
            }
            else {
                SDL_ASSERT(count);
                if (test_sorting) {
                    if (db::mem_size(prev_key.first)) {
                        if (tree->key_less(row.first, prev_key.first)) {
                            std::cout 
                                << "\nprev_key[" << prev_key.second << "] = "
                                << tree->type_key(prev_key.first);
                            SDL_ASSERT(0);
                        }
                    }
                    prev_key.first = row.first;
                    prev_key.second = count;
                }
            }
            std::cout
                << " page = " 
                << db::to_string::type_less(row.second) << " "
                << db::to_string::type(db.get_pageType(row.second))
                << " | index_page[" << db::to_string::type(tree->get_RID(it)) << "]";
            ++count;
        }
        std::cout << std::endl;
        if (test_reverse) {
            size_t rcount = 0;
            algo::for_reverse(tree->_rows, [&rcount](db::index_tree::row_iterator_value){
                ++rcount;
            });
            if (opt.index == -1) {
                SDL_ASSERT(rcount == count);
            }
        }
        if (test_find) {
            size_t count = 0;
            auto const last = tree_row.end();
            for (auto it = tree_row.begin(); it != last; ++it) { 
                if ((opt.index != -1) && ((int)count >= opt.index))
                    break;
                auto const row = *it;
                if (!tree_row.is_key_NULL(it)) {
                    std::cout
                        << "\n[" << table.name() << "][" << count << "] find_page("
                        << tree->type_key(row.first)
                        << ") = ";
                    auto const id = tree->find_page(row.first);
                    std::cout
                        << db::to_string::type_less(id) << " "
                        << db::to_string::type(db.get_pageType(id));
                    if (opt.verbosity > 1) {
                        if (auto const record = table.find_record(row.first)) {
                            auto const record_key = record.get_cluster_key(*cluster_index);
                            SDL_ASSERT(!(tree->key_less(record_key, row.first) || tree->key_less(row.first, record_key)));
#if SDL_DEBUG_RECORD_ID
                            SDL_ASSERT(record.get_id().id == id);
                            std::cout << " record = " << db::to_string::type(record.get_id());
#endif
                            std::cout << " PK = " << record.type_col(cluster_index->col_ind(0));
                            std::cout << " | index_page[" << db::to_string::type(tree->get_RID(it)) << "]";                        }
                        else {
                            SDL_ASSERT(0);
                            std::cout << " record not found";
                        }
                    }
                }
                ++count;
            }
            if (tree->index().size() == 1) {
                if (tree->index()[0].type == db::scalartype::t_int) {
                    const int32 keys[] = { 0, 1, 100 };
                    for (auto key : keys) {
                        std::cout << "\n[" << table.name() << "] find_page(" << key << ") = ";
                        auto const id = tree->find_page_t(key);
                        std::cout
                            << db::to_string::type_less(id) << " "
                            << db::to_string::type(db.get_pageType(id));
                        if (opt.verbosity > 1) {
                            if (auto const record = table.find_record_t(key)) {
#if SDL_DEBUG_RECORD_ID
                                std::cout << " record = " << db::to_string::type(record.get_id());
#else
                                (void)record;
#endif
                            }
                        }
                    }
                }
            }
            std::cout << std::endl;
        }
        if (opt.verbosity > 1) {
            size_t count = 0;
            for (auto const p : tree->_pages) {
                std::cout << "\nindex_page[" << table.name() << "][" << count << "]";
                std::cout << " size = " << p->size();
                std::cout << " [" << db::to_string::type_less(p->get_head()->data.pageId) << "]";
                ++count;
            }
            if (test_reverse) {
                size_t rcount = 0;
                algo::for_reverse(tree->_pages, [&rcount](db::index_tree::page_iterator_value){
                    ++rcount;
                });
                SDL_ASSERT(count == rcount);
            }
        }
    }
}

void trace_datatable(db::database const & db, db::datatable & table, cmd_option const & opt, bool const is_internal)
{
    enum { trace_iam = 1 };
    enum { print_nextPage = 1 };
    enum { long_pageId = 0 };
    enum { alloc_pageType = 0 };
    enum { find_record_iterator = 1000 };

    auto const & exclude = db::make::util::split(opt.exclude); 
    if (!exclude.empty() && db::make::util::is_find(exclude, table.name())) {
        std::cout << "\ntrace_datatable exclude: " << table.name() << std::endl;
        return;
    }
    if (opt.alloc_page) {
        std::cout << "\nDATATABLE [" << table.name() << "]";
        std::cout << " [" << db::to_string::type(table.get_id()) << "]";
        if (auto const ci = table.get_cluster_index()) {
            std::cout << " [CLUSTER =";
            for (size_t i = 0; i < ci->size(); ++i) {
                std::cout << " " << (*ci)[i].name;
            }
            std::cout << "]";
        }
        {
            auto const pk = table.get_PrimaryKeyOrder();
            if (pk.first) {
                std::cout << " [PK = " << pk.first->name << " " 
                    << db::to_string::type_name(pk.second) << "]";
            }
        }
        if (auto PK = table.get_PrimaryKey()) {
            std::cout << " [PK_root = " 
                << db::to_string::type_less(PK->root->data.pageId) << " " 
                << db::to_string::type(PK->root->data.type) << "]";
        }
        if (trace_iam) {
            db::for_dataType([&db, &table, &opt](db::dataType::type t){
                trace_datatable_iam(db, table, t, opt);
            });
        }
        if (opt.alloc_page > 2) {
            db::for_dataType([&table](db::dataType::type t1){
            db::for_pageType([&table, t1](db::pageType::type t2){
                trace_datapage(table, t1, t2);
            });
            });
            db::for_dataType([&table](db::dataType::type t1){
            db::for_pageType([&table, t1](db::pageType::type t2){
                trace_datarow(table, t1, t2);
            });
            });
        }
    }
    if (!is_internal) {
        if (opt.record_num) {
            std::cout << "\n\nDATARECORD [" << table.name() << "]";
            const size_t found_col = opt.col_name.empty() ?
                table.ut().size() :
                table.ut().find_if([&opt](db::usertable::column_ref c){
                    return c.name == opt.col_name;
                });
            if (opt.col_name.empty() || (found_col < table.ut().size())) {
                size_t row_index = 0;
                for (auto const record : table._record) {
                    if ((opt.record_num != -1) && ((int)row_index >= opt.record_num))
                        break;
                    std::cout << "\n[" << (row_index++) << "]";
                    trace_table_record(db, record, opt);
                }
                if (find_record_iterator) { // test API
                    if (auto primary = table.get_PrimaryKey()) {
                        if (primary->size() == 1) {
                            const auto pk = table.get_PrimaryKeyCol();
                            if (pk.first) {
                                size_t count = 0;
                                for (auto const record : table._record) {
                                    auto const it = table.find_record_iterator(record.data_col(pk.second)); (void)it;
                                    SDL_ASSERT(it != table._record.end());
                                    SDL_ASSERT((*it).head() == record.head());
                                    if (find_record_iterator == ++count)
                                        break;
                                }
                            }
                        }
                        else {
                            SDL_TRACE("\nprimary->size() = ", primary->size());
                        }
                    }
                }
            }
        }
        if (opt.index) {
            trace_table_index(db, table, opt);
        }
    }
    std::cout << std::endl;
}

void trace_datatables(db::database const & db, cmd_option const & opt)
{
    bool found = false;
    for (auto & tt : db._datatables) {
        db::datatable & table = *tt.get();
        if (!(opt.tab_name.empty() || (table.name() == opt.tab_name))) {
            continue;
        }
        found = true;
        trace_datatable(db, table, opt, false);
    }
    if (!found && !opt.tab_name.empty()) {
        if (auto tab = db.find_internal(opt.tab_name)) {
            trace_datatable(db, *tab, opt, true);
        }
    }
}

void trace_user_tables(db::database const & db, cmd_option const & opt)
{
    size_t index = 0;
    for (auto const & ut : db._usertables) {
        if (opt.tab_name.empty() || (ut->name() == opt.tab_name)) {
            std::cout << "\nUSER_TABLE[" << index << "]:\n";
            if (auto pk = db.get_primary_key(ut->get_id())) {
                std::cout << ut->type_schema(pk.get());
            }
            else {
                std::cout << ut->type_schema();
            }
        }
        ++index;
    }
    std::cout << "\nUSER_TABLE COUNT = " << index << std::endl;
}

void trace_internal_tables(db::database const & db, cmd_option const & opt)
{
    size_t index = 0;
    for (auto const & ut : db._internals) {
        if (opt.tab_name.empty() || (ut->name() == opt.tab_name)) {
            std::cout << "\nINTERNAL_TABLE[" << index << "]:\n";
            if (auto pk = db.get_primary_key(ut->get_id())) {
                std::cout << ut->type_schema(pk.get());
            }
            else {
                std::cout << ut->type_schema();
            }
        }
        ++index;
    }
    std::cout << "\nINTERNAL_TABLE COUNT = " << index << std::endl;
}

template<class T>
void trace_access(db::database const & db)
{
    auto & access = db::get_access<T>(db);
    SDL_ASSERT(access.begin() == access.begin());
    SDL_ASSERT(access.end() == access.end());
    int i = 0;
    for (auto & p : access) {
        ++i;
        SDL_ASSERT(p.get()); (void)p;
    }
    std::cout << db::page_name<T>() << " = " << i << std::endl;
}

template<class bootpage>
void trace_boot_page(db::database const & db, bootpage const & boot, cmd_option const & opt)
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
    else {
        SDL_ASSERT(0);
    }
}

void trace_pfs_page(db::database const & db, cmd_option const & opt)
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
        if (opt.dump_mem) {
            dump_whole_page(pfs->head);
        }
        std::cout << std::endl;
    }
}

template<typename T>
void trace_hex(T value) { 
    std::cout
        << "(0x" 
        << std::uppercase << std::hex
        << value 
        << std::dec << std::nouppercase << ")";
}

template<class table_type>
bool get_geo_point(db::geo_point & point, db::database const &, table_type const & table,
    db::bigint::spatial_page_row::pk0_type const pk0)
{
    if (auto obj = table->find_record_t(pk0)) {
        for (size_t i = 0; i < obj.size(); ++i) {
            auto const & col = obj.usercol(i);
            if (col.type == db::scalartype::t_geography) {
                auto const data_col = obj.data_col(i);
                const size_t data_col_size = db::mem_size(data_col);
                if (data_col_size == sizeof(db::geo_point)) {
                    db::geo_point const * pt = nullptr;
                    std::vector<char> buf;
                    if (data_col.size() == 1) {
                        pt = reinterpret_cast<db::geo_point const *>(data_col[0].first);
                    }
                    else {
                        buf = db::mem_utils::make_vector(data_col);
                        SDL_ASSERT(buf.size() == sizeof(db::geo_point));
                        pt = reinterpret_cast<db::geo_point const *>(buf.data());
                    }
                    if (pt->data.head.tag == db::spatial_tag::t_point) {
                        point = *pt;
                        return true;
                    }
                    else {
                        SDL_ASSERT(0);
                    }
                }
            }
        }
    }
    memset_zero(point);
    return false;
}

template<class table_type, class row_type>
void trace_spatial_object(db::database const &, cmd_option const & opt, 
                            table_type const & table,
                            row_type const & row,
                            std::string const & pk0_name)
{
    if (auto obj = table->find_record_t(row->data.pk0)) {        
        std::vector<char> buf;
        auto print_record_id = [&pk0_name, &obj, &row]() {
            std::cout
            << "record[" << pk0_name << " = " << row->data.pk0 << "][" 
#if SDL_DEBUG_RECORD_ID
            << db::to_string::type(obj.get_id()) 
#endif
            << "]";
        };
        std::cout << "\n";
        print_record_id();
        for (size_t i = 0; i < obj.size(); ++i) {
            auto const & col = obj.usercol(i);
            if (col.type == db::scalartype::t_geography) {
                std::cout
                    << "\ncol[" << i << "] = "
                    << col.name << " [" 
                    << db::scalartype::get_name(col.type)
                    << "]\n";
                if (1) {
                    SDL_ASSERT(!obj.STAsText(i).empty()); // test API
                }
                if (opt.verbosity) {
                    auto const data_col = obj.data_col(i);
                    const size_t data_col_size = db::mem_size(data_col);
                    static_assert(sizeof(db::geo_data) < sizeof(db::geo_point), "");
                    if (data_col_size == sizeof(db::geo_point)) {
                        db::geo_point const * pt = nullptr;
                        if (data_col.size() == 1) {
                            pt = reinterpret_cast<db::geo_point const *>(data_col[0].first);
                        }
                        else {
                            buf = db::mem_utils::make_vector(data_col);
                            SDL_ASSERT(buf.size() == sizeof(db::geo_point));
                            pt = reinterpret_cast<db::geo_point const *>(buf.data());
                        }
                        if (pt->data.head.tag == db::spatial_tag::t_point) {
                            SDL_ASSERT(obj.geo_type(i) == db::spatial_type::point);
                            std::cout << "geo_point:\n" << db::geo_point_info::type_meta(*pt);
                            std::cout << obj.type_col(i);
                        }
                        else {
                            SDL_ASSERT(!"unknown geo_point");
                        }
                    }
                    else {
                        static_assert(sizeof(db::geo_point) < sizeof(db::geo_multipolygon), "");
                        char const * pbuf = nullptr;
                        if (data_col.size() == 1) {
                            pbuf = data_col[0].first;
                        }
                        else {
                            buf = db::mem_utils::make_vector(data_col);
                            pbuf = buf.data();
                        }
                        if (data_col_size >= sizeof(db::geo_multipolygon)) {
                            auto const pg = reinterpret_cast<db::geo_multipolygon const *>(pbuf);
                            if (pg->data.head.tag == db::spatial_tag::t_multipolygon) {
                                SDL_ASSERT(obj.geo_type(i) != db::spatial_type::null);
                                std::cout << "geo_multipolygon:\n" << db::geo_multipolygon_info::type_meta(*pg);
#if 0 //SDL_DEBUG
                                const size_t ring_num = pg->ring_num();
                                std::cout << "\nring_num = " << ring_num << " ";
                                trace_hex(ring_num);
                                if (ring_num)
                                {
                                    size_t ring_i = 0;
                                    size_t total = 0;
                                    pg->for_ring([&ring_i, &total](db::spatial_point const * const b, db::spatial_point const * const e) {
                                        size_t const length = (e - b);
                                        std::cout << "\nring[" << (ring_i++) << "] = " << length << " "; trace_hex(length);
                                        std::cout << " offset = " << total << " "; trace_hex(total);
                                        total += length;
                                    });
                                    SDL_ASSERT(ring_i == ring_num);
                                    SDL_ASSERT(total == pg->data.num_point);
                                }
#endif
                                if (opt.verbosity > 1) {
                                    const db::spatial_grid grid_high;
                                    for (size_t i = 0; i < pg->size(); ++i) {
                                        const auto & pt = (*pg)[i];
                                        const auto cell = db::transform::make_cell(pt, grid_high);
                                        std::cout
                                            << "\n[" << i << "]"
                                            << " latitude = " << pt.latitude
                                            << " longitude = " << pt.longitude
                                            << " cell_id = " << db::to_string::type(cell);
                                    }
                                }
                                if (pg->data_mem_size() <= data_col_size) {
                                    if (const size_t tail_size = data_col_size - pg->data_mem_size()) {
                                        db::mem_range_t const tail {
                                            pbuf + data_col_size - tail_size,
                                            pbuf + data_col_size 
                                        };
                                        std::cout << "\n\nDump tail bytes[" << tail_size << "] ";
                                        print_record_id();
                                        std::cout << db::to_string::type_raw(tail);
                                    }
                                }
                                else {
                                    SDL_ASSERT(false);
                                }
                            }
                            else {
                                SDL_ASSERT(pg->data.head.tag == db::spatial_tag::t_linesegment);
                                static_assert(sizeof(db::geo_multipolygon) < sizeof(db::geo_linesegment), "");
                                if (data_col_size >= sizeof(db::geo_linesegment)) {
                                    auto const line = reinterpret_cast<db::geo_linesegment const *>(pbuf);
                                    if (line->data.head.tag == db::spatial_tag::t_linesegment) {
                                        SDL_ASSERT(obj.geo_type(i) == db::spatial_type::linesegment);
                                        std::cout << "geo_linesegment:\n" << db::geo_linesegment_info::type_meta(*line);
                                        std::cout << db::geo_linesegment_info::type_raw(*line);
                                    }
                                }
                            }
                        }

                    } // if (op.verbosity)
                } // if (col.type == db::scalartype::t_geography)
            } // for
        }
        std::cout << std::endl;
    }
}

template<class poi_id>
bool read_poi_file(std::vector<std::pair<poi_id, db::spatial_point>> & poi_vec, cmd_option const & opt)
{
    if (!opt.poi_file.empty()) { // parse poi coordinates
        poi_vec.clear();
        {
            std::cout << "\nread poi_file " << opt.poi_file << std::endl;
            std::ifstream read(opt.poi_file);
            {
                setlocale_t::auto_locale loc("en-US"); // decimal point character for atof depends on locale
                std::string s;
                do {
                    std::getline(read, s);
                    if (s.empty()) 
                        break;
                    if (const poi_id id = atoi(s.c_str())) {
                        const size_t i = s.find(',');
                        if (i != std::string::npos) {
                            poi_vec.push_back({id, db::spatial_point::STPointFromText(s.substr(i + 1))});
                        }
                        else {
                            break;
                        }
                    }
                    else {
                        break;
                    }
                } while (!(read.rdstate() & std::ios_base::eofbit));
                std::cout << "\n" << opt.poi_file << "\nlines  = " << poi_vec.size() << "\n";
            }
            read.close();
        }
        return !poi_vec.empty();
    }
    return false;
}

void trace_spatial_pages(db::database const & db, cmd_option const & opt)
{
    enum { test_spatial_index = 1 };
    const double dump_geo_point = opt.verbosity > 1;
    if (!opt.tab_name.empty() && opt.spatial_page && opt.pk0) {
        if (auto table = db.find_table(opt.tab_name)) {
            using pk0_type = db::bigint::spatial_page_row::pk0_type;
            std::string pk0_name;
            if (auto cl = table->get_cluster_index()) {
                SDL_ASSERT(cl->size());
                pk0_name = cl->get_column(0).name;
            }
            size_t count_page = 0;
            db::page_head const * p = db.load_page_head(opt.spatial_page);
            std::map<pk0_type, size_t> obj_processed;
            std::map<db::spatial_cell, pk0_type> cell_map;
            while (p) {
                if (db.is_allocated(p)) {
                    auto const & id = p->data.pageId;
                    if (0) {
                        std::cout << "\n\nspatial_page(" << id.pageId << ") @" << db.memory_offset(p);
                    }
                    using spatial_page = db::datapage_t<db::bigint::spatial_page_row>;
                    spatial_page const data(p);
                    for (size_t slot_id = 0; slot_id < data.size(); ++slot_id) {
                        auto const row = data[slot_id];
                        if ((row->data.pk0 >= opt.pk0) && (!opt.pk1 || (row->data.pk0 <= opt.pk1))) {                           
                            if (dump_geo_point) {
                                cell_map[row->data.cell_id] = row->data.pk0;
                            }
                            if (1) {
                                bool print_cell_info = true;
                                bool print_obj_info = true;
                                size_t cells_count = 1;

                                auto const it = obj_processed.find(row->data.pk0);
                                if (it == obj_processed.end()) {
                                    obj_processed[row->data.pk0] = cells_count;
                                }
                                else {
                                    cells_count = ++(it->second);
                                    if (opt.cells_per_object && (opt.cells_per_object < (int)cells_count)) {
                                        print_cell_info = false;
                                    }
                                    print_obj_info = false;
                                }
                                if (print_cell_info) {
                                    std::cout 
                                        << "\nspatial_page_row[" << slot_id << "] [1:" << id.pageId << "]"
                                        << " [cell " << cells_count << "]\n\n";
                                    std::cout << db::bigint::spatial_page_row_info::type_meta(*row);
                                    std::cout << db::bigint::spatial_page_row_info::type_raw(*row);
                                }
                                if (print_obj_info) {
                                    trace_spatial_object(db, opt, table, row, pk0_name);
                                }
                            }
                        }                    
                    }
                }
                else {
                    std::cout << "\npage [" << opt.spatial_page << "] not allocated" << std::endl;
                }
                p = db.load_next_head(p);
                ++count_page;
            } // while
            if (1) {
                size_t i = 0;
                for (auto & p : obj_processed) {
                    std::cout << "\n[" << i << "][pk0 = " << p.first << "] cells_count = " << p.second;
                }
            }
            if (dump_geo_point) {
                db::geo_point point{};
                size_t i = 0;
                std::cout << "\n#,cell_id,X,Y,point.X,point.Y,pk0,latitude,longitude";
                for (auto & p : cell_map) {
                    db::spatial_cell const & cell_id = p.first;
                    auto const xy = db::transform::d2xy(cell_id[0]);
                    auto const pos = db::transform::cell2point(cell_id);
                    std::cout
                        << "\n" << (i++)
                        << "," << db::to_string::type(cell_id, db::to_string::type_format::less)
                        << "," << xy.X
                        << "," << xy.Y
                        << "," << pos.X
                        << "," << pos.Y
                        << "," << p.second;
                    if (get_geo_point(point, db, table, p.second)) {
                        std::cout
                            << "," << point.data.point.latitude
                            << "," << point.data.point.longitude;
                    }
                }
            }
            std::cout << "\nspatial_pages = " << count_page << std::endl;
            if (test_spatial_index) {
                if (auto tree_ = table->get_spatial_tree()) {
                    SDL_ASSERT(tree_.pk0_scalartype() == db::scalartype::t_bigint);
                    auto const tree = tree_.cast<int64>();                    
                    auto const min_page = tree->min_page();
                    auto const max_page = tree->max_page();
                    SDL_ASSERT(!min_page->data.prevPage);
                    SDL_ASSERT(!max_page->data.nextPage);

                    auto const r1 = tree->min_page_row();
                    auto const r2 = tree->max_page_row();
                    SDL_ASSERT(r1 && r2);

                    auto const & k1 = r1->key();
                    auto const & k2 = r2->key();
                    SDL_ASSERT(k1 < k2);

                    auto const c1 = r1->data.cell_id;
                    auto const c2 = r2->data.cell_id;
                    SDL_ASSERT(c1 && c2);
                    SDL_ASSERT(c1 == k1.cell_id);
                    SDL_ASSERT(c2 == k2.cell_id);

                    auto const p1 = tree->find_cell(c1);
                    auto const p2 = tree->find_cell(c2);
                    SDL_ASSERT(p1 && p2);

                    auto const c_min = db::spatial_cell::min();
                    auto const c_max = db::spatial_cell::max();

                    auto const p_min = tree->find_cell(c_min);
                    auto const p_max = tree->find_cell(c_max);
                    SDL_ASSERT(p_min && !p_max);

                    if (!opt.cell_id.empty()) {
                        auto const cell = db::spatial_cell::parse_hex(opt.cell_id.c_str());
                        auto const id = tree->find_cell(cell);
                        std::cout 
                            << "\ncell_id = " << db::to_string::type(cell)
                            << " find => " << db::to_string::type(id)
                            << std::endl;
                    }
                    std::cout
                        << "\nmin_key = [" << db::to_string::type(c1) << " pk0 = " << k1.pk0 << "]" << " find => [" << db::to_string::type(p1) << "]"
                        << "\nmax_key = [" << db::to_string::type(c2) << " pk0 = " << k2.pk0 << "]" << " find => [" << db::to_string::type(p2) << "]"
                        << "\nfind min [" << db::to_string::type_less(c_min) << "] => [" << db::to_string::type(p_min) << "]"
                        << "\nfind max [" << db::to_string::type_less(c_max) << "] => [" << db::to_string::type(p_max) << "]"
                        << "\nmin_page = " << db::to_string::type_less(min_page->data.pageId)
                        << "\nmax_page = " << db::to_string::type_less(max_page->data.pageId)
                        << std::endl;
                    if (1) {
                        size_t page_count = 0;
                        size_t cell_count = 0;
                        size_t cell_match = 0;
                        db::bigint::spatial_key prev_key{};
                        //auto & tt = *tree;
                        for (auto p : tree->_datapage) {
                            SDL_ASSERT(p != nullptr);
                            if (page_count < 10) {
                                auto const & pageId = p->head->data.pageId;
                                std::cout << "\n[" << page_count << "] spatial page = " << db::to_string::type(pageId);
                            }
                            for (auto s : (*p)) {
                                SDL_ASSERT(s != nullptr);
                                A_STATIC_CHECK_TYPE(db::bigint::spatial_page_row const *, s);
                                SDL_ASSERT(prev_key < s->key());
                                if (prev_key.cell_id == s->data.cell_id) {
                                    SDL_ASSERT(prev_key.pk0 < s->data.pk0);
                                    ++cell_match;
                                }
                                prev_key = s->key();
                            }
                            cell_count += p->size();
                            ++page_count;
                        }
                        std::cout
                            << "\npage_count = " << page_count
                            << "\ncell_count = " << cell_count
                            << "\ncell_match = " << cell_match
                            << std::endl;
                        for (auto it = tree->_datapage.end(); it != tree->_datapage.begin();) {
                             --page_count;
                            --it;
                             SDL_ASSERT(*it != nullptr);
                        }
                        SDL_ASSERT(0 == page_count);
                    }
                }
            }
        }
        else {
            std::cout << "\ntable not found: " << opt.tab_name << std::endl;
        }
    }
}

#if SDL_USE_INTERVAL_CELL
void trace_cells(db::interval_cell const & cells) {
    size_t i = 0;
    cells.for_each([&i](db::spatial_cell const & cell){
        auto const p2 = db::transform::cell2point(cell);
        auto const sp = db::transform::spatial(cell);
        std::cout << (i++)
            << "," << p2.X
            << "," << p2.Y
            << "," << sp.longitude
            << "," << sp.latitude
            << "\n";
        return break_or_continue::continue_;
    });
}
#endif // #if SDL_USE_INTERVAL_CELL

template<class T>
void test_for_rect(T & tree, 
                    db::Latitude min_lat, 
                    db::Longitude min_lon, 
                    db::Latitude max_lat, 
                    db::Longitude max_lon)
{
    if (0) {
        tree->for_rect(db::spatial_rect::init(min_lat, min_lon, max_lat, max_lon), 
            [](db::bigint::spatial_page_row const *){
            return false;
        });
    }
    else {
        tree->for_rect(db::spatial_rect::init(min_lat, min_lon, max_lat, max_lon), 
            [](db::bigint::spatial_page_row const *){
            return bc::continue_;
        });
    }
}

template<class T>
void test_full_globe(T & tree)
{
    size_t count = 0;
    tree->full_globe([&count](db::bigint::spatial_page_row const * const p){
        SDL_ASSERT(p->data.cell_id.zero_tail());
        ++count;
        return bc::continue_;
    });
    std::cout << "\nfull_globe = " << count << " cells" << std::endl;
}

template<class table_type, class tree_type>
void test_spatial_performance(table_type & table, tree_type & tree, db::database const & db, cmd_option const & opt)
{
    SDL_ASSERT(tree);
    enum { test_special_cases = 0 };
    enum { dump_type_col = 0 };
    using pk0_type = db::bigint::spatial_page_row::pk0_type;
    if (!opt.poi_file.empty()) { // parse poi coordinates
        using poi_id = int;
        using poi_pair = std::pair<poi_id, db::spatial_point>;                    
        std::vector<poi_pair> poi_vec;
        if (read_poi_file(poi_vec, opt)) {
            if (opt.test_performance) {
                std::cout << "\ntest_performance started...\n";
                size_t count = 0;
                size_t const col_pos = opt.col_name.empty() ? table->ut().size() : table->ut().find(opt.col_name);
                db::vector_mem_range_t last_data; // simulate usage
                size_t STContains = 0;
                time_span timer;
                for (size_t test = 0; test < opt.test_performance; ++test) {
                    for (auto const & poi : poi_vec) {
                        tree->for_point(poi.second, 
                            [&count, &table, &col_pos, &last_data, &poi, &STContains]
                            (db::bigint::spatial_page_row const * const p) {
                            if (auto record = table->find_record_t(p->data.pk0)) {
                                if (col_pos < record.size()) {
                                    bool contains;
                                    if (!(contains = p->cell_cover())) {
                                        if ((contains = record.STContains(col_pos, poi.second))) {
                                            ++STContains;
                                        }
                                    }
                                    if (contains) {
                                        last_data = record.data_col(col_pos); // simulate usage
                                    }
                                }
                                ++count;
                            }
                            else {
                                SDL_ASSERT(0);
                            }
                            return bc::continue_;
                        });
                    }
                }
                const time_t result = timer.now(); // time in seconds elapsed since reset()
                std::cout
                    << "\ntest_performance = " << opt.test_performance
                    << " find count = " << count
                    << " seconds = " << result
                    << " last_data = " << db::mem_size(last_data)
                    << " STContains = " << STContains;
                if (col_pos < table->ut().size()) {
                    std::cout << " [" << table->ut()[col_pos].name << "]";
                }
                std::cout << std::endl;
            }
            else {
                using poi_idx = std::pair<size_t, db::bigint::spatial_page_row const *>;
                std::map<pk0_type, std::vector<poi_idx>> found;
                size_t cell_attr[3]{};
                size_t STContains = 0;
                auto & tt = *table;
                const size_t col_geography = tt.ut().find_geography();
                for (size_t i = 0; i < poi_vec.size(); ++i) {
                    auto const & poi = poi_vec[i];
                    tree->for_point(poi.second, [i, &poi, &found, &cell_attr, &STContains, &tt, col_geography]
                    (db::bigint::spatial_page_row const * const p) {
                        if (p->data.cell_attr >= A_ARRAY_SIZE(cell_attr)) {
                            SDL_ASSERT(0);
                            return bc::break_;
                        }
                        ++cell_attr[p->data.cell_attr];
                        bool contains;
                        if (!(contains = p->cell_cover())) {
                            SDL_ASSERT(col_geography < tt.ut().size());
                            if (auto const record = tt.find_record_t(p->data.pk0)) {
                                if ((contains = record.STContains(col_geography, poi.second))) {
                                    ++STContains; //FIXME: -104815,40994
                                }
                                test_geography(record, col_geography);
                            }
                            else {
                                SDL_ASSERT(0);
                            }
                        }
                        if (contains) {
                            found[p->data.pk0].push_back({i, p});
                        }
                        return bc::continue_;
                    });
                    if (opt.test_for_range) {
                        bool found = false;
                        break_or_continue ret = 
                        tree->for_range(poi.second, db::Meters(0), [&found](db::bigint::spatial_page_row const *){
                            found = true;
                            return false;
                        });
                        (void)ret;
                        SDL_ASSERT(found == (ret == break_or_continue::break_));
                        if (opt.range_meters > 0) {
                            const db::Meters range_meters = opt.range_meters;
                            ret = tree->for_range(poi.second, range_meters, [](db::bigint::spatial_page_row const *){
                                return bc::continue_;
                            });
                            SDL_ASSERT(ret == break_or_continue::continue_);
#if SDL_USE_INTERVAL_CELL
                            if (0) {
                                static size_t trace = 0;
                                if (trace++ < 1) {
                                    std::cout
                                        << "\ncell_range for(latitude = "
                                        << poi.second.latitude << ", longitude = "
                                        << poi.second.longitude
                                        << ")\n";
                                    db::interval_cell cells;
                                    db::transform::old_cell_range(cells, poi.second, range_meters);
                                    trace_cells(cells);
                                }
                            }
#endif
                            if (0) {
                                db::spatial_rect rc = db::spatial_rect::init(poi.second, poi.second);
                                rc.min_lat = db::SP::norm_latitude(rc.min_lat - 1);
                                rc.min_lon = db::SP::norm_longitude(rc.min_lon - 1);
                                rc.max_lat = db::SP::norm_latitude(rc.min_lat + 1);
                                rc.max_lon = db::SP::norm_longitude(rc.max_lon + 1);
                                tree->for_rect(rc, [](db::bigint::spatial_page_row const *){
                                    return bc::continue_;
                                });
                            }
                        }
                    }
                }
                if (opt.test_for_range) {
                    if (opt.range_meters > 0) {
                        const db::Meters range_meters = opt.range_meters;
                        auto const north_pole = db::spatial_point::init(db::Latitude(90), db::Longitude(0));
                        auto const south_pole = db::spatial_point::init(db::Latitude(-90), db::Longitude(0));
                        tree->for_range(north_pole, range_meters, [](db::bigint::spatial_page_row const *){
                            return bc::continue_;
                        });
                        tree->for_range(south_pole, range_meters, [](db::bigint::spatial_page_row const *){
                            return bc::continue_;
                        });
                    }
                    if (test_special_cases) { // test special cases #330
                        test_for_rect(tree, 50, 30, 60, 40);
                        test_for_rect(tree, 30, 50, 40, 60);
                    }
                    if (test_special_cases) { // test special cases #330
                        test_for_rect(tree, 0, -179, 89, 179);
                        test_for_rect(tree, 0, -179, 89, -45);
                    }
                    if (test_special_cases) { // test special cases #330
                        test_full_globe(tree);
                    }
                    if (test_special_cases) { // test special cases
                        db::spatial_rect rc;
                        rc.min_lat = 0;
                        rc.min_lon = 90 - 0.1;
                        rc.max_lat = 0.1;
                        rc.max_lon = 90;
                        tree->for_rect(rc, [](db::bigint::spatial_page_row const *){
                            return bc::continue_;
                        });
                        //trace_cells(db::transform::cell_rect(rc));
                        rc.min_lat = 0;
                        rc.max_lat = 1;
                        rc.min_lon = 179;
                        rc.max_lon = 180;
                        tree->for_rect(rc, [](db::bigint::spatial_page_row const *){
                            return bc::continue_;
                        });
                        tree->for_range(
                            db::SP::init(db::Latitude(0), db::Longitude(45)),
                            opt.range_meters, [](db::bigint::spatial_page_row const *){
                            return bc::continue_;
                        });
                        tree->for_range(
                            db::SP::init(db::Latitude(0.1), db::Longitude(45)),
                            1000*1000, [](db::bigint::spatial_page_row const *){
                            return bc::continue_;
                        });
                        tree->for_range(
                            db::SP::init(db::Latitude(0), db::Longitude(45)),
                            1000*1000, [](db::bigint::spatial_page_row const *){
                            return bc::continue_;
                        });
                    }
                }
                if (!found.empty()) {
                    size_t i = 0;
                    std::cout.precision(8);
                    const size_t col_index = opt.col_name.empty() ?
                        table->ut().size() : table->ut().find(opt.col_name);
                    const bool find_col = (col_index != table->ut().size());
                    if (opt.trace_poi_csv) {
                        std::cout << "\nID,POI_ID";
                    }
                    for (auto const & f : found) {
                        for (auto const & idx : f.second) {
                            auto const & poi = poi_vec[idx.first];
                            db::spatial_point const & v = poi.second;
                            if (opt.trace_poi_csv) {
                                std::cout << "\n" << f.first << "," << poi.first;
                            }
                            else {
                                std::cout
                                    << "[" << (i++) << "]"
                                    << " pk0 = " << f.first
                                    << " lon = " << v.longitude
                                    << " lat = " << v.latitude
                                    << " POI_ID = " << poi.first
                                    << " cell_id = " << db::to_string::type_less(idx.second->data.cell_id)
                                    << " cell_attr = " << idx.second->data.cell_attr;
                                if (find_col) {
                                    A_STATIC_CHECK_TYPE(pk0_type const, f.first);
                                    if (auto re = table->find_record_t(f.first)) {
                                        if (dump_type_col) {
                                            std::cout
                                                << " " << opt.col_name
                                                << " = " << re.type_col(col_index);
                                        }
                                    }
                                    else {
                                        SDL_ASSERT(0);
                                    }
                                }
                                std::cout << "\n";
                            }
                        }
                    } // for
                    std::cout 
                        << "\ncell_touch = " << cell_attr[0]
                        << "\ncell_part = " << cell_attr[1]
                        << "\ncell_cover = " << cell_attr[2]
                        << "\nSTContains = " << STContains
                        << std::endl;
                } // if (!found.empty())
            } //  if (test_performance)
        }
    }

}

void trace_spatial_search(db::database const & db, cmd_option const & opt)
{
    if (!opt.tab_name.empty()) {
        if (auto table = db.find_table(opt.tab_name)) {
            if (auto tree_ = table->get_spatial_tree()) {
                SDL_ASSERT(tree_.pk0_scalartype() == db::scalartype::t_bigint);
                auto tree = tree_.cast<int64>();
                using pk0_type = db::bigint::spatial_page_row::pk0_type;
                if (opt.latitude && opt.longitude) {
                    const db::spatial_point pos = db::spatial_point::init(db::Latitude(opt.latitude), db::Longitude(opt.longitude));
                    db::spatial_cell cell = db::transform::make_cell(pos);
                    if (opt.depth && ((size_t)opt.depth <= db::spatial_cell::size)) {
                        cell = db::spatial_cell::set_depth(cell, static_cast<uint8>(opt.depth));
                    }
                    std::set<db::bigint::spatial_tree::pk0_type> found;
                    tree->for_cell(cell, [&found](db::bigint::spatial_page_row const * const p) {
                        found.insert(p->data.pk0);
                        return bc::continue_;
                    });
                    std::cout << "\nfor_point(lat = " << opt.latitude << ",lon = " << opt.longitude << ") =>";
                    if (!found.empty()) {
                        for (auto & v : found) {
                            std::cout << "\npk0 = " << v;
                        }
                        std::cout << std::endl;
                    }
                    else {
                        std::cout << " not found\n";
                    }
                }
                if (opt.export_cells && opt.pk0) {
                    std::cout << "\nexport_cells for pk0 = " << opt.pk0 << "\n";
                    using vec_cell = std::vector<db::spatial_cell>;
                    using map_cell = std::map<pk0_type, vec_cell>;
                    map_cell map;
                    for (auto data : tree->_datapage) {
                        for (auto p : *data) {
                            if (p->data.pk0 == opt.pk0) {
                                map[p->data.pk0].push_back(p->data.cell_id);
                            }
                        }
                    }
                    if (!map.empty()) {
                        size_t i = 0;
                        for (auto const & m : map) {
                            vec_cell const & v = m.second;
                            for (auto const & c : v) {
                                auto const pt = db::transform::cell2point(c);
                                std::cout
                                    << i
                                    << "," << db::to_string::type_less(c)
                                    << "," << pt.X
                                    << "," << pt.Y
                                    << "\n";
                                ++i;
                            }
                        }
                        std::cout << "cell_id count = " << i << "\n";
                    }
                    else {
                        std::cout << "cell_id " << opt.pk0 << " not found\n";
                    }
                }
                test_spatial_performance(table, tree, db, opt);
                if (opt.test_for_rect) {
                    if (opt.full_globe || opt.test_rect) {
                        std::cout << "\ntest_for_rect:\n";
                        const db::spatial_rect rc = opt.test_rect;
                        const size_t geography = table->ut().find_geography();
                        if (geography < table->ut().size()) {
                            std::set<int64> processed;
                            if (opt.full_globe) {
                                tree->full_globe([&table, &processed, geography, &opt]
                                    (db::bigint::spatial_page_row const * row){
                                    if (opt.pk0) {
                                        if (row->data.pk0 != opt.pk0) {
                                            return bc::continue_;
                                        }
                                    }
                                    processed.insert(row->data.pk0);
                                    return bc::continue_;
                                });
                            }
                            else {
                                SDL_ASSERT(opt.test_rect);
                                tree->for_rect(rc, [&table, &processed, geography, &opt]
                                    (db::bigint::spatial_page_row const * row){
                                    if (opt.pk0) {
                                        if (row->data.pk0 != opt.pk0) {
                                            return bc::continue_;
                                        }
                                    }
                                    processed.insert(row->data.pk0);
                                    return bc::continue_;
                                });
                                if (1) {
                                    auto test = tree->for_rect_pk0(rc);
                                    test = tree->for_range_pk0(rc.center(), 100);
                                }
                            }
                            size_t count = 0;
                            for (int64 const pk0 : processed) {
                                if (auto p = table->find_record_t(pk0)) {
                                    if (opt.verbosity) {
                                        std::cout << "[" << count << "] pk0 = " << pk0;
                                        std::cout << " geo_type = " << db::to_string::type_name(p.geo_type(geography));
                                        if (1) {
                                            if (auto _geo = p.geography(geography)) {
                                                const db::geo_mem geo = std::move(_geo); // test API
                                                size_t const numobj = geo.numobj();
                                                if (numobj) {
                                                    std::cout << " numobj = " << numobj << " [";
                                                    auto const & orient = geo.ring_orient();
                                                    auto const wind = geo.ring_winding();
                                                    for (size_t i = 0; i < numobj; ++i) {
                                                        if (i) std::cout << " ";
                                                        std::cout << geo.get_subobj(i).size();
                                                        if (geo.type() == db::spatial_type::multipolygon) {
                                                            std::cout << (db::is_exterior(orient[i]) ? "_E" : "_I");
                                                            std::cout << (db::is_clockwise(wind[i]) ? "_CW" : "_CCW");
                                                        }
                                                    }
                                                    std::cout << "]";
                                                }
                                            }
                                            else {
                                                SDL_ASSERT(0);
                                            }
                                        }
                                        if (opt.verbosity > 1) {
                                            std::cout << " STAsText = " << p.STAsText(geography);
                                        }
                                    }
                                    else {
                                        if (opt.verbosity > 1) {
                                            std::cout << pk0;
                                        }
                                        SDL_ASSERT(p.geo_type(geography) != db::spatial_type::null);
                                    }
                                    if (opt.test_point.is_valid() || opt.test_rect) { 
                                        //db::spatial_rect const * const bbox = opt.test_rect ? &opt.test_rect : nullptr;
                                        db::spatial_point where = opt.test_point.is_valid() ? opt.test_point : opt.test_rect.center();                                        
                                        db::Meters const dist = p.STDistance(geography, where);
                                        if (opt.verbosity) {
                                            std::cout << " [STDistance(lat = "
                                                << where.latitude << ", lon = "
                                                << where.longitude << ", range = "
                                                << opt.range_meters << ") = ";
                                            if (dist.value() != sdl::db::transform::infinity) {
                                                std::cout << dist.value() << "]";
                                            }
                                            else {
                                                std::cout << "not found]";
                                            }
                                        }
                                    }
                                    if (opt.verbosity > 1) {
                                        std::cout << std::endl;
                                    }
                                    ++count;
                                }
                                else {
                                    std::cout << "[" << table->name() << "] pk0 = " << pk0 << " not found !\n";
                                    SDL_ASSERT(!"bad index");
                                }
                            }
                            if (opt.verbosity) {
                                std::cout << "count = " << count << std::endl;
                            }
                        }
                    }
                }
            }
        }
    }
}

void trace_spatial(db::database const & db, cmd_option const & opt)
{
    trace_spatial_pages(db, opt);
    for (size_t test = 0; test <= opt.test_performance; ++test) {
        trace_spatial_search(db, opt);
    }
    if (opt.test_rect && (opt.verbosity > 1)) { // test STDistance
        auto const p1 = opt.test_rect.min_pt();
        auto const p2 = opt.test_rect.max_pt();
        auto const dist = db::transform::STDistance(p1, p2);
        std::cout << "\nSTDistance("
            << std::setprecision(9)
            << p1.longitude << " " << p1.latitude << " "
            << p2.longitude << " " << p2.latitude << ") = "
            << dist.value() << std::endl;
    }
}

void trace_index_for_table(db::database const & db, cmd_option const &)
{
    for (auto const & table : db._datatables) {
        std::cout << "\nindex_for_table[" << table->name() << "]"
            << "[id = " << db::to_string::type(table->get_id()) << "]"
            << std::endl;
        size_t i = 0;
        for (auto const idx : db.index_for_table(table->get_id())) {
            std::cout
                << "[" << (i++) << "] "
                << " name = " << idx->name()
                << " id = " << db::to_string::type(idx->data.id)
                << " indid = " << db::to_string::type(idx->data.indid)
                << " type = " << db::to_string::type(idx->data.type)
                << std::endl;
            if (idx->is_spatial()) {
                auto const root = db.find_spatial_root(table->get_id());
                SDL_ASSERT(root.first && root.second);
                if (root.first) {
                    //SDL_TRACE(idx->name());
                    SDL_ASSERT(root.second == idx);
                    auto const row = root.first;
                    std::cout
                        << "\nfind_spatial_root[" << db::to_string::type(table->get_id()) << "][" << idx->name() << "]"
                        << " pgfirst = " << db::to_string::type(row->data.pgfirst)
                        << " pgroot = " << db::to_string::type(row->data.pgroot)
                        << " pgfirstiam = " << db::to_string::type(row->data.pgfirstiam)
                        << std::endl;
                }
            }
        }
    }
}

void trace_interval_set(db::interval_set<uint32> const & pages)
{
    size_t i = 0;
    pages.for_each2([&i](uint32 const x1, uint32 const x2){
        SDL_ASSERT(x1 <= x2);
        if (x1 == x2) {
            std::cout << (i++) << ":" << x1 << std::endl;
        }
        else {
            std::cout << (i++) << ":" << x1 << "-" << x2
                << " (" << (x2 - x1 + 1) << ")" << std::endl;
        }
        return true;
    });
}

void table_dump_pages(db::database const & db, cmd_option const & opt, db::datatable const * const tab)
{
    if (tab) {
        SDL_TRACE("\ndump_pages [", tab->name(), "]");
        using page32 = db::pageFileID::page32;
        A_STATIC_ASSERT_TYPE(uint32, page32);
        {
            using datarow_access = db::datatable::datarow_access;
            page32 min_pageId = page32(-1), max_pageId = 0;
            page32 pageId = 0;
            db::interval_set<page32> pages;
            auto it = tab->_datarow.begin();
            const auto end = tab->_datarow.end();
            for (; it != end; ++it) {
                const db::recordID r = datarow_access::get_id(it);
                if (pageId != r.id.pageId) {
                    pageId = r.id.pageId;
                    pages.insert(pageId);
                    if (pageId < min_pageId) min_pageId = pageId;
                    if (pageId > max_pageId) max_pageId = pageId;
                    const auto head = datarow_access::get_page(it);
                    const auto checksum = head->checksum(head);
                    const auto tornBits = head->data.tornBits;
                    if (checksum != tornBits) {
                        SDL_ASSERT(0);
                    }
                }
            }
            std::cout << "min_pageId = " << min_pageId << std::endl;
            std::cout << "max_pageId = " << max_pageId << std::endl;
            trace_interval_set(pages);
        }
        if (const auto sp = tab->get_index_tree()) {
            const auto & tree = *sp;
            SDL_ASSERT(tree.root());
            db::interval_set<page32> pages;
            if (0) {
                for (const auto & p : tree._rows) {
                    A_STATIC_CHECK_TYPE(db::pageFileID, p.second);
                    pages.insert(p.second.pageId);
                }
                std::cout << "\nindex_tree[" << tab->name() << "]:\n";
                trace_interval_set(pages);
            }
            if (0) {
                for (const auto & p : tree._pages) {
                    const auto h = p->get_head();
                    A_STATIC_CHECK_TYPE(db::page_head const * const, h);
                    pages.insert(h->data.pageId.pageId);
                }
                std::cout << "\nindex_tree[" << tab->name() << "]:\n";
                trace_interval_set(pages);
            }
            else {
                page32 root = 0;
                tree.for_each_index_page([&root, &pages](db::page_head const * const head){
                    SDL_ASSERT(head->is_index());
                    pages.insert(head->data.pageId.pageId);
                    if (!root)
                        root = head->data.pageId.pageId;
                    return true;
                });
                std::cout << "\nindex_tree[" << tab->name() << "]:\n";
                std::cout << "root_page = " << root << std::endl;
                trace_interval_set(pages);
                std::cout << "index_tree[" << tab->name() << "] pages = "
                        << pages.size() << std::endl;
            }
        }
        if (const auto sp = tab->get_spatial_tree()) {
            if (sp.pk0_scalartype() == db::scalartype::t_bigint) {
                const auto tree = sp.cast<int64>();
                db::interval_set<page32> indexpages, datapages;
                page32 root = 0;
                tree->for_each_index_page([&root, &indexpages, &datapages](db::page_head const * const head) {
                    if (head->is_index()) {
                        indexpages.insert(head->data.pageId.pageId);
                    }
                    else {
                        datapages.insert(head->data.pageId.pageId);
                    }
                    if (!root)
                        root = head->data.pageId.pageId;
                    return true;
                });
                std::cout << "\nspatial_tree[" << tab->name() << "] root_page = " << root << std::endl;
                std::cout << "\nspatial_tree[" << tab->name() << "] indexpages:\n";
                trace_interval_set(indexpages);
                std::cout << "spatial_tree[" << tab->name() << "] indexpages = " << indexpages.size() << std::endl;
                std::cout << "\nspatial_tree[" << tab->name() << "] datapages:\n";
                trace_interval_set(datapages);
                std::cout << "spatial_tree[" << tab->name() << "] datapages = " << datapages.size() << std::endl;
            }
        }
    }
}

void table_dump_pages_all(db::database const & db, cmd_option const & opt)
{
    SDL_ASSERT(!opt.dump_pages.empty());
    if (opt.dump_pages == "*") {
        SDL_TRACE("\ndump_pages *");
        for (const auto & p : db._datatables) {
            table_dump_pages(db, opt, p.get());
        }
    }
    else {
        table_dump_pages(db, opt, db.find_table(opt.dump_pages).get());
    }
}

void maketables(db::database const & db, cmd_option const & opt)
{
    if (!opt.out_file.empty()) {
        if (opt.write_file) {
            std::cout << "\ngenerate headers for database, please wait...\n";
            db::make::generator::param_type param;
            param.out_file = opt.out_file;
            param.schema_names = opt.schema_names;
            param.include = db::make::util::split(opt.include);
            param.exclude = db::make::util::split(opt.exclude);
            param.make_namespace = opt._namespace.empty() ? db::make::util::extract_filename(db.filename(), true) : opt._namespace;
            param.is_record_count = opt.record_count;
            db::make::generator::make_file(db, param);
        }
#if 0
        else {
            std::cout << db::make::generator::make_tables(db,
                db::make::util::split(opt.include),
                db::make::util::split(opt.exclude),
                opt.record_count, nullptr);
        }
#endif
    }
}

bool export_database(cmd_option const & opt)
{
    return db::make::export_database::make_file(opt.export_database);
}

void print_version()
{
#if defined(NDEBUG)
    std::cout << "\nNDEBUG version";
#else
    std::cout << "\nDEBUG version";
#endif
#if defined(SDL_DEBUG)
    std::cout << "\nSDL_DEBUG = " << SDL_DEBUG;
#else
    std::cout << "\nSDL_DEBUG not defined";
#endif
    std::cout << "\nSDL_USE_BPOOL = " << db::database::use_page_bpool();
    std::cout
        << "\nBuild date: " << __DATE__
        << "\nBuild time: " << __TIME__
        << "\nSDL_DATASERVER_VERSION: " << SDL_DATASERVER_VERSION
        << std::endl;
}

void print_help(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    print_version();
    std::cout
        << "\nUsage: " << argv[0]
        << "\n[-i|--mdf_file] path to mdf file"
        << "\n[-o|--out_file] path to output files"
        << "\n[-d|--dump_mem] 0|1 : allow to dump memory"
        << "\n[-p|--page_num] int : index of the page to trace"
        << "\n[-s|--page_sys] 0|1 : trace system tables"
        << "\n[-f|--file_header] 0|1 : trace fileheader page(0)"
        << "\n[-b|--boot_page] 0|1 : trace boot page(9)"
        << "\n[-u|--user_table] 0|1 : trace user tables"
        << "\n[--internal] 0|1 : trace internal tables"
        << "\n[-a|--alloc_page] 0-3 : trace allocation level"
        << "\n[-q|--silence] 0|1 : allow output std::cout|wcout"
        << "\n[-r|--record] int : number of table records to select"
        << "\n[-j|--index] int : number of index records to trace"
        << "\n[-x|--max_output] int : limit column value length in chars"
        << "\n[-v|--verbosity] 0|1 : show more details for records and indexes"
        << "\n[-c|--col] name of column to select"
        << "\n[-t|--tab] name of table to select"
        << "\n[-k|--index_key] value of index key to find"
        << "\n[-w]--write_file] 0|1 : enable to write file"
        << "\n[--warning] 0|1|2 : warning level"
        << "\n[--spatial] int : spatial data page to trace"
        << "\n[--pk0] int64 : primary key to trace object(s) with spatial data"
        << "\n[--pk1] int64 : primary key to trace object(s) with spatial data"
        << "\n[--cells_per_object] int : limit traced cells per object"
        << "\n[--index_for_table] 0|1 : trace index for tables"
        << "\n[--cell_id] hex string to convert into spatial_cell"
        << "\n[--lat] float : geography latitude"
        << "\n[--lon] float : geography longitude"
        << "\n[--depth] 1..4 : geography depth"
        << "\n[--export_cells] 0|1"
        << "\n[--poi_file] path to csv file"
        << "\n[--include] include tables for generator"
        << "\n[--exclude] exclude tables for generator"
        << "\n[--export_in] input csv file"
        << "\n[--export_out] output sql file"
        << "\n[--export_source] source database name"
        << "\n[--export_dest] dest database name"
        << "\n[--trim_space]"
        << "\n[--namespace] maketable generator namespace"
        << "\n[--schema_names] maketable schema names"
        << "\n[--geography] geography column name"
        << "\n[--create_spatial_index] export database parameter"
        << "\n[--dump_pages]"
        << "\n[--checksum]"
        << std::endl;
}

int run_main(cmd_option const & opt)
{
    std::unique_ptr<scoped_null_cout> scoped_silence;
    std::unique_ptr<scoped_null_wcout> scoped_silence_w;
    if (opt.silence) {
        reset_new(scoped_silence);
        reset_new(scoped_silence_w);
    }
    else {
        print_version();
        std::cout
            << "\n--- called with: ---"
            << "\nmdf_file = " << opt.mdf_file
            << "\nout_file = " << opt.out_file
            << "\ndump_mem = " << opt.dump_mem
            << "\npage_num = " << opt.page_num
            << "\npage_sys = " << opt.page_sys
            << "\nfile_header = " << opt.file_header
            << "\nboot_page = " << opt.boot_page
            << "\nuser_table = " << opt.user_table
            << "\ninternal = " << opt.internal            
            << "\nalloc_page = " << opt.alloc_page
            << "\nsilence = " << opt.silence
            << "\nrecord_num = " << opt.record_num
            << "\nindex = " << opt.index
            << "\nmax_output = " << opt.max_output
            << "\nverbosity = " << opt.verbosity
            << "\ncol = " << opt.col_name
            << "\ntab = " << opt.tab_name
            << "\nindex_key = " << opt.index_key
            << "\nwrite_file = " << opt.write_file
#if SDL_DEBUG
            << "\nwarning level = " << debug::warning_level()
#endif
            << "\nspatial_page = " << opt.spatial_page
            << "\npk0 = " << opt.pk0
            << "\npk1 = " << opt.pk1
            << "\ncells_per_object = " << opt.cells_per_object
            << "\nindex_for_table = " << opt.index_for_table
            << "\ncell_id = " << opt.cell_id
            << "\nlatitude = " << opt.latitude
            << "\nlongitude = " << opt.longitude
            << "\ndepth = " << opt.depth
            << "\nexport_cells = " << opt.export_cells
            << "\npoi_file = " << opt.poi_file
            << "\ntest_performance = " << opt.test_performance
            << "\ntest_maketable = " << opt.test_maketable
            << "\ntrace_poi_csv = " << opt.trace_poi_csv
            << "\nrange_meters = " << opt.range_meters
            << "\ntest_for_range = " << opt.test_for_range
            << "\ntest_for_rect = " << opt.test_for_rect
            << "\nmin_lat = " << opt.test_rect.min_lat
            << "\nmin_lon = " << opt.test_rect.min_lon
            << "\nmax_lat = " << opt.test_rect.max_lat
            << "\nmax_lon = " << opt.test_rect.max_lon
            << "\nfull_globe = " << opt.full_globe
            << "\ninclude = " << opt.include            
            << "\nexclude = " << opt.exclude   
            << "\nexport_in = " << opt.export_database.in_file   
            << "\nexport_out = " << opt.export_database.out_file   
            << "\nexport_source = " << opt.export_database.source   
            << "\nexport_dest = " << opt.export_database.dest   
            << "\ngeography = " << opt.export_database.geography
            << "\ncreate_spatial_index = " << opt.export_database.create_spatial_index
            << "\nprecision = " << opt.precision
            << "\nrecord_count = " << opt.record_count
            << "\ntrim_space = " << opt.trim_space
            << "\nnamespace = " << opt._namespace  
            << "\nschema_names = " << opt.schema_names
            << "\ndump_pages = " << opt.dump_pages            
            << "\nchecksum = " << opt.checksum           
            << std::endl;
    }
    if (opt.precision) {
        db::to_string::precision(opt.precision);
    }
    if (!opt.export_database.empty()) {
        if (export_database(opt)) {
            return EXIT_SUCCESS;
        }
        std::cerr << "\nexport database failed" << std::endl;
        return EXIT_FAILURE;
    }
    db::database m_db(opt.mdf_file);
    db::database const & db = m_db;
    if (db.is_open()) {
        std::cout << "\ndatabase opened: " << db.filename() << std::endl;
    }
    else {
        std::cerr << "\ndatabase failed: " << db.filename() << std::endl;
        return EXIT_FAILURE;
    }
    const size_t page_count = db.page_count();
    {
        enum { page_size = db::page_head::page_size };
        const size_t page_allocated = db.page_allocated();
        const size_t page_free = page_count - page_allocated;
        SDL_ASSERT(page_allocated <= page_count);
        std::cout
            << "page_count = " << page_count << " (" << (page_count * page_size) << " byte)"
            << "\npage_allocated = " << page_allocated << " (" << (page_allocated * page_size) << " byte)"
            << "\npage_free = " << page_free << " (" << (page_free * page_size) << " byte)"
            << std::endl;
    }
#if SDL_USE_BPOOL && defined(SDL_OS_WIN32) && SDL_DEBUG 
    if (1) {
        joinable_thread test([page_count, &db](){
            for (auto & it : db._datatables) {
                db::datatable const & table = *it;
                for (db::row_head const * const row : table._datarow) {
                    SDL_ASSERT(row != nullptr);
                }
                if (auto count = db.unlock_thread(true)) {
                    SDL_TRACE("[", table.name(), "] unlock_thread = ", count);
                }
            }
            for (uint32 i = 0; i < 8; ++i) {
                SDL_ASSERT(!db.unlock_page(i));
            }
            /*db::pageFileID id = db::pageFileID::init(0);
            size_t progress = 0;
            for (size_t i = 0; i < page_count; ++i) {
                id.pageId = (uint32)i;
                if (db.is_allocated(id)) {
                    auto test = db.auto_lock_page(id); // must be called in new thread !
                    SDL_ASSERT(test);
                    test.reset();
                    SDL_ASSERT(!test);
                }
                const size_t p = 100 * i / page_count;
                if (p != progress) {
                    progress = p;
                    SDL_TRACE(p, " %");
                }
                if (p > 4) {
                    break; // too slow
                }
            }*/
        });
    }
#endif
    if (opt.checksum) {
        SDL_UTILITY_SCOPE_TIMER_SEC(timer, "checksum seconds = ");
        std::cout << "checksum started" << std::endl;
        db.scan_checksum([](db::page_head const * const p) {
            std::cout << "checksum failed at page: "
                << sdl::db::to_string::type(p->data.pageId)
                << " tornBits = " << p->data.tornBits
                << std::endl;
            return true;
        });
        std::cout << "checksum ended" << std::endl;
    }
    if (opt.boot_page) {
        trace_boot_page(db, db.get_bootpage(), opt);
        if (opt.alloc_page) {
            trace_pfs_page(db, opt);
        }
    }
    if (opt.file_header) {
        trace_sys_page(db, db.get_fileheader(), nullptr, opt);
    }
    if ((opt.page_num >= 0) && (opt.page_num < (int)page_count)) {
        trace_page(db, opt.page_num, opt);
    }
    if (opt.page_sys) {
        std::cout << std::endl;
        trace_sys_list(db, db._sysallocunits, opt);
        trace_sys_list(db, db._sysschobjs, opt);
        trace_sys_list(db, db._syscolpars, opt);
        trace_sys_list(db, db._sysscalartypes, opt);
        trace_sys_list(db, db._sysidxstats, opt);
        trace_sys_list(db, db._sysobjvalues, opt);
        trace_sys_list(db, db._sysiscols, opt);
        trace_sys_list(db, db._sysrowsets, opt);
    }
    if (opt.user_table) {
        trace_user_tables(db, opt);
    }
    if (opt.internal) {
        trace_internal_tables(db, opt);
    }
    if (opt.alloc_page) {
        std::cout << "\nTEST PAGE ACCESS:\n";
        trace_access<db::sysallocunits>(db);
        trace_access<db::sysschobjs>(db);
        trace_access<db::syscolpars>(db);
        trace_access<db::sysidxstats>(db);
        trace_access<db::sysscalartypes>(db);
        trace_access<db::sysobjvalues>(db);
        trace_access<db::sysiscols>(db);
        trace_access<db::sysrowsets>(db);
        trace_access<db::sysrowsets>(db);
        trace_access<db::pfs_page>(db);
        trace_access<db::usertable>(db);
        trace_access<db::datatable>(db);
    }
    if (opt.alloc_page || opt.record_num) {
        trace_datatables(db, opt);
    }
    if (!opt.index_key.empty()) {
        find_index_key(db, opt);
    }
    if (!opt.out_file.empty()) {
        maketables(db, opt);
    }
    if (!opt.write_file && opt.test_maketable) {
#if SDL_DEBUG_maketable
        db::make::test_maketable_$$$(db);
#endif
    }
    trace_spatial(db, opt);
    if (opt.index_for_table) {
        trace_index_for_table(db, opt);
    }
    if (!opt.dump_pages.empty()) {
        table_dump_pages_all(db, opt);
    }
    return EXIT_SUCCESS;
}

// C++ option parser library, supporting the standard GNU style syntax for options.
//FIXME: https://github.com/jarro2783/cxxopts

int run_main(int argc, char* argv[])
{
    setlocale_t::set("Russian");

    cmd_option opt{};

    CmdLine cmd;
    SDL_DEBUG_CPP(cmd.add(make_option(0, debug::warning_level(), "warning")));
    cmd.add(make_option('i', opt.mdf_file, "mdf_file"));
    cmd.add(make_option('o', opt.out_file, "out_file"));
    cmd.add(make_option('d', opt.dump_mem, "dump_mem"));
    cmd.add(make_option('p', opt.page_num, "page_num"));
    cmd.add(make_option('s', opt.page_sys, "page_sys"));
    cmd.add(make_option('f', opt.file_header, "file_header"));
    cmd.add(make_option('b', opt.boot_page, "boot_page"));
    cmd.add(make_option('u', opt.user_table, "user_table"));
    cmd.add(make_option(0, opt.internal, "internal"));
    cmd.add(make_option('a', opt.alloc_page, "alloc_page"));
    cmd.add(make_option('q', opt.silence, "silence"));
    cmd.add(make_option('r', opt.record_num, "record_num"));
    cmd.add(make_option('j', opt.index, "index"));
    cmd.add(make_option('x', opt.max_output, "max_output"));
    cmd.add(make_option('v', opt.verbosity, "verbosity"));
    cmd.add(make_option('c', opt.col_name, "col"));
    cmd.add(make_option('t', opt.tab_name, "tab"));
    cmd.add(make_option('k', opt.index_key, "index_key"));
    cmd.add(make_option('w', opt.write_file, "write_file"));
    cmd.add(make_option(0, opt.spatial_page, "spatial"));
    cmd.add(make_option(0, opt.pk0, "pk0"));
    cmd.add(make_option(0, opt.pk1, "pk1"));
    cmd.add(make_option(0, opt.cells_per_object, "cells_per_object"));
    cmd.add(make_option(0, opt.index_for_table, "index_for_table"));
    cmd.add(make_option(0, opt.cell_id, "cell_id"));
    cmd.add(make_option(0, opt.latitude, "lat"));
    cmd.add(make_option(0, opt.longitude, "lon"));
    cmd.add(make_option(0, opt.depth, "depth"));
    cmd.add(make_option(0, opt.export_cells, "export_cells"));
    cmd.add(make_option(0, opt.poi_file, "poi_file"));
    cmd.add(make_option(0, opt.test_performance, "test_performance"));  
    cmd.add(make_option(0, opt.test_maketable, "test_maketable"));  
    cmd.add(make_option(0, opt.trace_poi_csv, "trace_poi_csv"));      
    cmd.add(make_option(0, opt.range_meters, "range_meters"));
    cmd.add(make_option(0, opt.test_for_range, "test_for_range"));   
    cmd.add(make_option(0, opt.test_for_rect, "test_for_rect"));   
    cmd.add(make_option(0, opt.test_rect.min_lat, "min_lat"));
    cmd.add(make_option(0, opt.test_rect.min_lon, "min_lon"));
    cmd.add(make_option(0, opt.test_rect.max_lat, "max_lat"));
    cmd.add(make_option(0, opt.test_rect.max_lon, "max_lon"));
    cmd.add(make_option(0, opt.test_point.latitude, "latitude"));
    cmd.add(make_option(0, opt.test_point.longitude, "longitude"));
    cmd.add(make_option(0, opt.full_globe, "full_globe"));
    cmd.add(make_option(0, opt.include, "include"));
    cmd.add(make_option(0, opt.exclude, "exclude"));
    cmd.add(make_option(0, opt.export_database.in_file, "export_in"));
    cmd.add(make_option(0, opt.export_database.out_file, "export_out"));
    cmd.add(make_option(0, opt.export_database.source, "export_source"));
    cmd.add(make_option(0, opt.export_database.dest, "export_dest"));
    cmd.add(make_option(0, opt.export_database.geography, "geography"));
    cmd.add(make_option(0, opt.export_database.create_spatial_index, "create_spatial_index"));
    cmd.add(make_option(0, opt.precision, "precision"));    
    cmd.add(make_option(0, opt.record_count, "record_count"));
    cmd.add(make_option(0, opt.trim_space, "trim_space"));    
    cmd.add(make_option(0, opt._namespace, "namespace"));
    cmd.add(make_option(0, opt.schema_names, "schema_names"));
    cmd.add(make_option(0, opt.dump_pages, "dump_pages"));
    cmd.add(make_option(0, opt.checksum, "checksum"));    
    try {
        if (argc == 1) {
            print_help(argc, argv);
            std::cout << "\nMissing parameters" << std::endl;
            return EXIT_SUCCESS;
        }
        cmd.process(argc, argv);
        if (opt.mdf_file.empty() && opt.export_database.empty()) {
            throw std::string("Missing input file");
        }
    }
    catch (const std::string& s) {
        print_help(argc, argv);
        std::cerr << "\n" << s << std::endl;
        return EXIT_FAILURE;
    }
    return run_main(opt);
}

} // namespace 

int main(int argc, char* argv[])
{
    try {
        time_span timer;
        const int ret = run_main(argc, argv);
        const time_t sec = timer.now();
        std::cout << "\nTOTAL_TIME = " << sec << " seconds" << std::endl;
        return ret;
    }
    catch (sdl_exception & e) {
        (void)e;
        std::cout << "\ncatch exception [" << typeid(e).name() << "] = " << e.what();
        SDL_ASSERT(0);
    }
    catch (std::exception & e) {
        (void)e;
        std::cout << "\ncatch std::exception = " << e.what();
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

