// main.cpp : Defines the entry point for the console application.
//
#include "common/common.h"
#include "common/outstream.h"
#include "system/page_head.h"
#include "system/page_info.h"
#include "system/database.h"
#include "system/index_tree.h"
#include "system/version.h"
#include "system/generator.h"
#include "third_party/cmdLine/cmdLine.h"
#include <map>

#if SDL_DEBUG_maketable_$$$
#include "usertables/maketable_$$$.h"
namespace sdl { namespace db { namespace make {
    void test_maketable_$$$(database &);
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
    bool boot_page = true;
    bool user_table = false;
    int alloc_page = 0; // 0-3
    bool silence = false;
    int record_num = 10;
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
};

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

void trace_page_data(db::database & db, db::page_head const * const head)
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
void trace_index_page_row(db::database & db, db::index_page_row_t<T> const & row, size_t const i)
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
void trace_page_index_t(db::database & db, db::page_head const * const head)
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

void trace_page_index(db::database & db, db::page_head const * const head)
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
    default:
        SDL_ASSERT(0); //not implemented
        break;
    }
}

void trace_page_textmix(db::database & db, db::page_head const * const head)
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

void trace_page_IAM(db::database & db, db::page_head const * const head)
{
    SDL_ASSERT(head->data.type == db::pageType::type::IAM);
    const db::iam_page iampage(head);
    std::cout 
        << db::to_string::type(iampage.head->data.pageId) << " " 
        << db::to_string::type(iampage.head->data.type)
        << std::endl;
}

void trace_page(db::database & db, db::pageIndex const page_num, cmd_option const & opt)
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
void trace_sys_list(db::database & db, 
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

void trace_datatable_iam(db::database & db, db::datatable & table, 
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
    auto datapage = table.get_datapage(t1, t2);
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

void trace_string_value(std::string const & s, db::vector_mem_range_t const & vm, db::scalartype::type const type)
{
    if (db::scalartype::t_char == type) { // show binary representation for non-digits
        std::wcout << cp1251_to_wide(s);
    }
    else {
        trace_printable(s, vm, type);
    }
}

void trace_record_value(std::string && s, db::vector_mem_range_t const & vm, db::scalartype::type const type, cmd_option const & opt)
{
    size_t const length = s.size();
    bool concated = false;
    size_t const max_output = (opt.max_output > 0) ? (size_t)(opt.max_output) : (size_t)(-1);
    if (s.size() > max_output) { // limit output size
        s.resize(max_output);
        concated = true;
    }
    trace_string_value(s, vm, type);
    if (concated) {
        std::cout << "(" << db::to_string::type(length) << ")";
    }
}

template<class T>
void trace_table_record(db::database & db, T const & record, cmd_option const & opt)
{
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
            SDL_WARNING(!"test api"); //FIXME: maybe null ? (SQLServerInternals, dbcc page 92275)
            std::cout << "NULL";
            continue;
        }
        trace_record_value(record.type_col(col_index), record.data_col(col_index), col.type, opt);
    }
    if (opt.verbosity) {
        std::cout << " | fixed_data = " << record.fixed_size();
        std::cout << " var_data = " << record.var_size();
        std::cout << " null = " << record.count_null();                
        std::cout << " var = " << record.count_var();     
        std::cout << " fixed = " << record.count_fixed(); 
        std::cout << " [" << db::to_string::type(record.get_id()) << "]";
        if (auto stub = record.forwarded()) {
            std::cout << " forwarded from ["
                << db::to_string::type(stub->data.row)
                << "]";
        }
    }
}

struct find_index_key_t: noncopyable
{
    db::database & db;
    db::datatable const & table;
    cmd_option const & opt;

    find_index_key_t(db::database & _db, db::datatable const & _t, cmd_option const & _opt)
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

struct wrap_find_index_key_t
{
    find_index_key_t & fun;
    wrap_find_index_key_t(find_index_key_t & f) : fun(f){}
    template<class T> void operator()(T t) { fun(t); }
    void unexpected(db::scalartype::type) {}
};

template<class T>
void find_index_key_t::find_record(T const & key)
{
    if (auto col = table.get_PrimaryKeyOrder().first) {
        if (auto record = table.find_record_t(key)) {
            std::cout
                << "\nfind_record[" << table.name() << "][" 
                << col->name << " = " << key << "] => ["
                << db::to_string::type(record->get_id())
                << "]\n";
            std::cout << "[" << table.name() << "]";
            trace_table_record(db, *record, opt);
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

struct wrap_parse_index_key
{
    parse_index_key & fun;
    wrap_parse_index_key(parse_index_key & f): fun(f){}
    template<class T> void operator()(T t) { fun(t); }
    void unexpected(db::scalartype::type) {}
};

struct find_composite_key_t: noncopyable
{
    db::database & db;
    db::datatable const & table;
    cmd_option const & opt;

    find_composite_key_t(db::database & _db, db::datatable const & _t, cmd_option const & _opt)
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
                << db::to_string::type(record->get_id())
                << "]\n";
            std::cout << "[" << table.name() << "]";
            trace_table_record(db, *record, opt);
        }
        else {
            std::cout
                << "\nfind_record[" << table.name() << "][" << opt.index_key << "] => not found";
        }
    }
}

void find_index_key(db::database & db, cmd_option const & opt)
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

void trace_table_index(db::database & db, db::datatable & table, cmd_option const & opt)
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
            if ((opt.index != -1) && (count >= opt.index))
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
            for_reverse(tree->_rows, [&rcount](db::index_tree::row_iterator_value p){
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
                if ((opt.index != -1) && (count >= opt.index))
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
                            SDL_ASSERT(record->get_id().id == id);
                            auto const record_key = record->get_cluster_key(*cluster_index);
                            SDL_ASSERT(!(tree->key_less(record_key, row.first) || tree->key_less(row.first, record_key)));
                            std::cout << " record = " << db::to_string::type(record->get_id());
                            std::cout << " PK = " << record->type_col(cluster_index->col_ind(0));
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
                                std::cout << " record = " << db::to_string::type(record->get_id());
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
                for_reverse(tree->_pages, [&rcount](db::index_tree::page_iterator_value p){
                    ++rcount;
                });
                SDL_ASSERT(count == rcount);
            }
        }
    }
}

void trace_datatable(db::database & db, db::datatable & table, cmd_option const & opt)
{
    enum { trace_iam = 1 };
    enum { print_nextPage = 1 };
    enum { long_pageId = 0 };
    enum { alloc_pageType = 0 };

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
                if ((opt.record_num != -1) && (row_index >= opt.record_num))
                    break;
                std::cout << "\n[" << (row_index++) << "]";
                trace_table_record(db, record, opt);
            }
        }
    }
    if (opt.index) {
        trace_table_index(db, table, opt);
    }
    std::cout << std::endl;
}

void trace_datatables(db::database & db, cmd_option const & opt)
{
    for (auto & tt : db._datatables) {
        db::datatable & table = *tt.get();
        if (!(opt.tab_name.empty() || (table.name() == opt.tab_name))) {
            continue;
        }
        trace_datatable(db, table, opt);
    }
}

void trace_user_tables(db::database & db, cmd_option const & opt)
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

template<class T>
void trace_access(db::database & db)
{
    auto & access = db::get_access<T>(db);
    SDL_ASSERT(access.begin() == access.begin());
    SDL_ASSERT(access.end() == access.end());
    int i = 0;
    for (auto & p : access) {
        ++i;
        SDL_ASSERT(p.get());
    }
    std::cout << db::page_name<T>() << " = " << i << std::endl;
}

template<class bootpage>
void trace_boot_page(db::database & db, bootpage const & boot, cmd_option const & opt)
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

void trace_pfs_page(db::database & db, cmd_option const & opt)
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

template<class table_type, class row_type>
void trace_spatial_object(db::database & db, cmd_option const & opt, 
                            table_type const & table,
                            row_type const & row,
                            std::string const & pk0_name)
{
    if (auto obj = table->find_record_t(row->data.pk0)) {        
        std::vector<char> buf;
        auto print_record_id = [&pk0_name, &obj, &row]() {
            std::cout
            << "record[" << pk0_name << " = " << row->data.pk0 << "][" 
            << db::to_string::type(obj->get_id()) << "]";
        };
        std::cout << "\n";
        print_record_id();
        for (size_t i = 0; i < obj->size(); ++i) {
            auto const & col = obj->usercol(i);
            if (col.type == db::scalartype::t_geography) {
                std::cout
                    << "\ncol[" << i << "] = "
                    << col.name << " [" 
                    << db::scalartype::get_name(col.type)
                    << "]\n";
                if (opt.verbosity) {
                    auto const data_col = obj->data_col(i);
                    const size_t data_col_size = db::mem_size(data_col);
                    if (data_col_size == sizeof(db::geo_point)) {
                        db::geo_point const * pt = nullptr;
                        if (data_col.size() == 1) {
                            pt = reinterpret_cast<db::geo_point const *>(data_col[0].first);
                        }
                        else {
                            buf = db::make_vector(data_col);
                            SDL_ASSERT(buf.size() == sizeof(db::geo_point));
                            pt = reinterpret_cast<db::geo_point const *>(buf.data());
                        }
                        if (pt->data.tag == db::geo_point::TYPEID) {
                            std::cout << "geo_point:\n" << db::geo_point_info::type_meta(*pt);
                            std::cout << obj->type_col(i);
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
                            buf = db::make_vector(data_col);
                            pbuf = buf.data();
                        }
                        if (data_col_size >= sizeof(db::geo_multipolygon)) {
                            auto const pg = reinterpret_cast<db::geo_multipolygon const *>(pbuf);
                            if (pg->data.tag == db::geo_multipolygon::TYPEID) {
                                std::cout << "geo_multipolygon:\n" << db::geo_multipolygon_info::type_meta(*pg);
                                const size_t ring_num = pg->ring_num();
                                std::cout << "\nring_num = " << ring_num << " ";
                                trace_hex(ring_num);
                                if (ring_num)
                                {
                                    size_t ring_i = 0;
                                    size_t total = 0;
                                    pg->for_ring([&ring_i, &total](db::spatial_point const * b, db::spatial_point const * e){
                                        size_t const length = (e - b);
                                        std::cout << "\nring[" << (ring_i++) << "] = " << length << " "; trace_hex(length);
                                        std::cout << " offset = " << total << " "; trace_hex(total);
                                        total += length;
                                    });
                                    SDL_ASSERT(ring_i == ring_num);
                                    SDL_ASSERT(total == pg->data.num_point);
                                }
                                if (opt.verbosity > 1) {
                                    for (size_t i = 0; i < pg->size(); ++i) {
                                        const auto & pt = (*pg)[i];
                                        std::cout
                                            << "\n[" << i << "]"
                                            << " latitude = " << pt.latitude
                                            << " longitude = " << pt.longitude;
                                    }
                                }
                                if (pg->mem_size() <= data_col_size) {
                                    if (const size_t tail_size = data_col_size - pg->mem_size()) {
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
                                SDL_ASSERT(pg->data.tag == db::geo_linestring::TYPEID);
                                static_assert(sizeof(db::geo_multipolygon) < sizeof(db::geo_linestring), "");
                                if (data_col_size >= sizeof(db::geo_linestring)) {
                                    auto const line = reinterpret_cast<db::geo_linestring const *>(pbuf);
                                    if (line->data.tag == db::geo_linestring::TYPEID) {
                                        std::cout << "geo_linestring:\n" << db::geo_linestring_info::type_meta(*line);
                                        std::cout << db::geo_linestring_info::type_raw(*line);
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

void trace_spatial(db::database & db, cmd_option const & opt)
{
    if (!opt.tab_name.empty() && opt.spatial_page && opt.pk0) {
        if (auto table = db.find_table(opt.tab_name)) {
            std::string pk0_name;
            if (auto cl = table->get_cluster_index()) {
                SDL_ASSERT(cl->size());
                pk0_name = cl->get_column(0).name;
            }
            size_t count_page = 0;
            db::page_head const * p = db.load_page_head(opt.spatial_page);
            std::map<db::spatial_page_row::pk0_type, size_t> obj_processed;
            while (p) {
                if (db.is_allocated(p)) {
                    auto const & id = p->data.pageId;
                    if (0) {
                        std::cout << "\n\nspatial_page(" << id.pageId << ") @" << db.memory_offset(p);
                    }
                    using spatial_page = db::datapage_t<db::spatial_page_row>;
                    spatial_page const data(p);
                    for (size_t slot_id = 0; slot_id < data.size(); ++slot_id) {
                        auto const row = data[slot_id];
                        if ((row->data.pk0 >= opt.pk0) && (!opt.pk1 || (row->data.pk0 <= opt.pk1))) {
                            
                            bool print_cell_info = true;
                            bool print_obj_info = true;
                            size_t cells_count = 1;

                            auto const it = obj_processed.find(row->data.pk0);
                            if (it == obj_processed.end()) {
                                obj_processed[row->data.pk0] = cells_count;
                            }
                            else {
                                cells_count = ++(it->second);
                                if (opt.cells_per_object && (opt.cells_per_object < cells_count)) {
                                    print_cell_info = false;
                                }
                                print_obj_info = false;
                            }
                            if (print_cell_info) {
                                std::cout 
                                    << "\nspatial_page_row[" << slot_id << "] [1:" << id.pageId << "]"
                                    << " [cell " << cells_count << "]\n\n";
                                std::cout << db::spatial_page_row_info::type_meta(*row);
                                std::cout << db::spatial_page_row_info::type_raw(*row);
                            }
                            if (print_obj_info) {
                                trace_spatial_object(db, opt, table, row, pk0_name);
                            }
                        }                    
                    }
                }
                else {
                    std::cout << "\npage [" << opt.spatial_page << "] not allocated" << std::endl;
                }
                p = db.load_next_head(p);
                ++count_page;
            }
            {
                size_t i = 0;
                for (auto & p : obj_processed) {
                    std::cout << "\n[" << i << "][pk0 = " << p.first << "] cells_count = " << p.second;
                }
            }
            std::cout << "\nspatial_pages = " << count_page << std::endl;
        }
        else {
            std::cout << "\ntable not found: " << opt.tab_name << std::endl;
        }
    }
}

void maketables(db::database & db, cmd_option const & opt)
{
    if (!opt.out_file.empty()) {
        SDL_TRACE(__FUNCTION__);
        if (opt.write_file) {
            db::make::generator::make_file(db, opt.out_file, 
                db::make::util::extract_filename(db.filename(), true).c_str());
        }
        else {
            for (auto p : db._datatables) {
                std::cout << db::make::generator::make_table(db, *p);
            }
        }
    }
}

void print_version()
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
        << "\n[-i|--mdf_file] path to mdf file"
        << "\n[-o|--out_file] path to output files"
        << "\n[-d|--dump_mem] 0|1 : allow to dump memory"
        << "\n[-p|--page_num] int : index of the page to trace"
        << "\n[-s|--page_sys] 0|1 : trace system tables"
        << "\n[-f|--file_header] 0|1 : trace fileheader page(0)"
        << "\n[-b|--boot_page] 0|1 : trace boot page(9)"
        << "\n[-u|--user_table] 0|1 : trace user tables"
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
            << "\nwarning level = " << debug::warning_level
            << "\nspatial_page = " << opt.spatial_page
            << "\npk0 = " << opt.pk0
            << "\npk1 = " << opt.pk1
            << "\ncells_per_object = " << opt.cells_per_object
            << std::endl;
    }
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
        trace_boot_page(db, db.get_bootpage(), opt);
        if (opt.alloc_page) {
            trace_pfs_page(db, opt);
        }
    }
    if (opt.file_header) {
        trace_sys_page(db, db.get_fileheader(), nullptr, opt);
    }
    if ((opt.page_num >= 0) && (opt.page_num < page_count)) {
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
    if (!opt.write_file && false) {
#if SDL_DEBUG_maketable_$$$
        db::make::test_maketable_$$$(db);
#endif
    }
    trace_spatial(db, opt);
    return EXIT_SUCCESS;
}

int run_main(int argc, char* argv[])
{
    setlocale(LC_ALL, "Russian");

    cmd_option opt{};

    CmdLine cmd;
    cmd.add(make_option('i', opt.mdf_file, "mdf_file"));
    cmd.add(make_option('o', opt.out_file, "out_file"));
    cmd.add(make_option('d', opt.dump_mem, "dump_mem"));
    cmd.add(make_option('p', opt.page_num, "page_num"));
    cmd.add(make_option('s', opt.page_sys, "page_sys"));
    cmd.add(make_option('f', opt.file_header, "file_header"));
    cmd.add(make_option('b', opt.boot_page, "boot_page"));
    cmd.add(make_option('u', opt.user_table, "user_table"));
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
    cmd.add(make_option(0, debug::warning_level, "warning"));
    cmd.add(make_option(0, opt.spatial_page, "spatial"));
    cmd.add(make_option(0, opt.pk0, "pk0"));
    cmd.add(make_option(0, opt.pk1, "pk1"));
    cmd.add(make_option(0, opt.cells_per_object, "cells_per_object"));

    try {
        if (argc == 1) {
            throw std::string("Missing parameters");
        }
        cmd.process(argc, argv);
        if (opt.mdf_file.empty())
            throw std::string("Missing input file");
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
        return run_main(argc, argv);
    }
    catch (sdl_exception & e) {
        SDL_TRACE(typeid(e).name(), " = ", e.what());
        SDL_ASSERT(0);
    }
    catch (std::exception & e) {
        SDL_TRACE("exception = ", e.what());
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

