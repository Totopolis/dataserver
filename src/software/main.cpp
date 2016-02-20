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

struct cmd_option : noncopyable {
    std::string mdf_file;
    bool dump_mem = 0;
    int page_num = -1;
    int page_sys = 0;
    bool file_header = false;
    bool boot_page = true;
    bool user_table = false;
    int alloc_page = 0; // 0-3
    bool silence = false;
    int record = 10;
    int max_output = 10;
    int verbosity = 0;
    std::string col_name;
    std::string tab_name;
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
    SDL_ASSERT(head->data.type == db::pageType::type::data);
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

void trace_page_index(db::database & db, db::page_head const * const head)
{
    SDL_ASSERT(head->data.type == db::pageType::type::index);
    
    if (0) { 
        // FIXME: index_page_row depends on primary key type
        using index_page = db::datapage_t<db::index_page_row>;
        index_page const data(head);
        for (size_t slot_id = 0; slot_id < data.size(); ++slot_id) {
            db::index_page_row const * row = data[slot_id];
            auto const pminlen = head->data.pminlen;
            std::cout << "\nindex_row[" << slot_id << "]\n"
                << db::index_page_row_info::type_meta(*row)
                << "pminlen = " << pminlen
                << std::endl;
            auto const s = db::to_string::dump_mem(row, pminlen);
            std::cout << s << std::endl;
        }
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

void trace_page(db::database & db, db::page_head const * p, cmd_option const & opt)
{
    if (p && !p->is_null()) {
        {
            auto const & id = p->data.pageId;
            std::cout 
                << "\n\npage(" << id.pageId << ") @"
                << db.memory_offset(p)
                << ":\n\n"
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
        SDL_WARNING(!"page not found");
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
                        printPage(" nextPage = ", db.nextPageID(id));
                        printPage(" prevPage = ", db.prevPageID(id));
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
    auto datapage = table._datapage_order(t1, t2);
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

// https://en.wikipedia.org/wiki/Windows-1251
std::wstring cp1251_to_wide(std::string const & s)
{
    std::wstring w(s.size(), L'\0');
    if (std::mbstowcs(&w[0], s.c_str(), w.size()) == s.size()) {
        return w;
    }
    SDL_ASSERT(!"cp1251_to_wide");
    return{};
}

void trace_printable(std::string const & s, db::scalartype::type const type)
{
    if (type == db::scalartype::t_geography) {
        std::cout << s; // memory dump
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

void trace_string_value(std::string const & s, db::scalartype::type const type)
{
    if (db::scalartype::t_char == type) { // show binary representation for non-digits
        std::wcout << cp1251_to_wide(s);
    }
    else {
        trace_printable(s, type);
    }
}

void trace_record_value(std::string && s, db::scalartype::type const type, cmd_option const & opt)
{
    size_t const length = s.size();
    bool concated = false;
    size_t const max_output = (opt.max_output > 0) ? (size_t)(opt.max_output) : (size_t)(-1);
    if (s.size() > max_output) { // limit output size
        s.resize(max_output);
        concated = true;
    }
    trace_string_value(s, type);
    if (concated) {
        std::cout << "(" << db::to_string::type(length) << ")";
    }
}

void trace_datatable(db::database & db, cmd_option const & opt)
{
    enum { trace_iam = 1 };
    enum { print_nextPage = 1 };
    enum { long_pageId = 0 };
    enum { alloc_pageType = 0 };

    for (auto & tt : db._datatables) {
        db::datatable & table = *tt.get();
        if (!(opt.tab_name.empty() || (table.name() == opt.tab_name))) {
            continue;
        }
        if (opt.alloc_page) {
            std::cout << "\nDATATABLE [" << table.name() << "]";
            std::cout << " [" << db::to_string::type(table.get_id()) << "]";
            if (auto root = table.data_index()) {
                SDL_ASSERT(root->data.type == db::pageType::type::index);
                std::cout << " data_index = " << db::to_string::type(root->data.pageId);
                auto const pk = table.get_PrimaryKey();
                if (pk.first) {
                    std::cout << " [PK = " << pk.first->name << "]"; //pk.first->type_schema();
                }
                else {
                    SDL_ASSERT(0);
                }
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
        if (opt.record) {
            std::cout << "\n\nDATARECORD [" << table.name() << "]";
            const size_t found_col = opt.col_name.empty() ?
                table.ut().size() :
                table.ut().find_if([&opt](db::usertable::column_ref c){
                    return c.name == opt.col_name;
                });
            if (opt.col_name.empty() || (found_col < table.ut().size())) {
                size_t row_index = 0;
                for (auto const record : table._record) {
                    if ((opt.record != -1) && (row_index >= opt.record))
                        break;
                    std::cout << "\n[" << (row_index++) << "]";
                    for (size_t col_index = 0; col_index < record.size(); ++col_index) {
                        auto const & col = record.usercol(col_index);
                        if (!opt.col_name.empty() && (col_index != found_col)) {
                            continue;
                        }
                        std::cout << " " << col.name << " = ";
                        if (record.is_null(col_index)){
                            std::cout << "NULL";
                            continue;
                        }
                        SDL_ASSERT(!record.data_col(col_index).empty()); // test api
                        trace_record_value(record.type_col(col_index), col.type, opt);
                    }
                    if (opt.verbosity) {
                        std::cout << " | fixed_data = " << record.fixed_data_size();
                        std::cout << " var_data = " << record.var_data_size();
                        std::cout << " null = " << record.count_null();                
                        std::cout << " var = " << record.count_var();     
                        std::cout << " fixed = " << record.count_fixed(); 
                        std::cout << " [" 
                            << db::to_string::type(record.get_id())
                            << "]";
                        if (auto stub = record.forwarded()) {
                            std::cout << " forwarded from ["
                                << db::to_string::type(stub->data.row)
                                << "]";
                        }
                    }
                }
            }
        }
        std::cout << std::endl;
    }
}

void trace_user_tables(db::database & db, cmd_option const & opt)
{
    size_t index = 0;
    for (auto & ut : db._usertables) {
        if (opt.tab_name.empty() || (ut->name() == opt.tab_name)) {
            std::cout << "\nUSER_TABLE[" << index << "]:\n";
            std::cout << ut->type_schema(db.get_PrimaryKey(ut->get_id()));
        }
        ++index;
    }
    std::cout << "\nUSER_TABLE COUNT = " << index << std::endl;
}

template<class T>
void trace_access(db::database & db)
{
    int i = 0;
    for (auto & p : db::get_access<T>(db)) {
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
        << "\n[-d|--dump_mem] 0|1 : allow to dump memory"
        << "\n[-p|--page_num] int : index of the page to trace"
        << "\n[-s|--page_sys] 0|1 : trace system tables"
        << "\n[-f|--file_header] 0|1 : trace fileheader page(0)"
        << "\n[-b|--boot_page] 0|1 : trace boot page(9)"
        << "\n[-u|--user_table] 0|1 : trace user tables"
        << "\n[-a|--alloc_page] 0-3 : trace allocation level"
        << "\n[-q|--silence] 0|1 : allow output std::cout|wcout"
        << "\n[-r|--record] int : number of table records to select"
        << "\n[-x|--max_output] int : limit column value length in chars"
        << "\n[-v|--verbosity] 0|1 : show more details for table records"
        << "\n[-c|--col] name of column to select"
        << "\n[-t|--tab] name of table to select"
        << std::endl;
}

int run_main(int argc, char* argv[])
{
    setlocale(LC_ALL, "Russian");

    cmd_option opt;

    CmdLine cmd;
    cmd.add(make_option('i', opt.mdf_file, "input_file"));
    cmd.add(make_option('d', opt.dump_mem, "dump_mem"));
    cmd.add(make_option('p', opt.page_num, "page_num"));
    cmd.add(make_option('s', opt.page_sys, "page_sys"));
    cmd.add(make_option('f', opt.file_header, "file_header"));
    cmd.add(make_option('b', opt.boot_page, "boot_page"));
    cmd.add(make_option('u', opt.user_table, "user_table"));
    cmd.add(make_option('a', opt.alloc_page, "alloc_page"));
    cmd.add(make_option('q', opt.silence, "silence"));
    cmd.add(make_option('r', opt.record, "record"));
    cmd.add(make_option('x', opt.max_output, "max_output"));
    cmd.add(make_option('v', opt.verbosity, "verbosity"));
    cmd.add(make_option('c', opt.col_name, "col"));
    cmd.add(make_option('t', opt.tab_name, "tab"));

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
    std::unique_ptr<scoped_null_wcout> scoped_silence_w;
    if (opt.silence) {
        reset_new(scoped_silence);
        reset_new(scoped_silence_w);
    }
    trace_version();

    std::cout
        << "\n--- called with: ---"
        << "\nmdf_file = " << opt.mdf_file
        << "\ndump_mem = " << opt.dump_mem
        << "\npage_num = " << opt.page_num
        << "\npage_sys = " << opt.page_sys
        << "\nfile_header = " << opt.file_header
        << "\nboot_page = " << opt.boot_page
        << "\nuser_table = " << opt.user_table
        << "\nalloc_page = " << opt.alloc_page
        << "\nsilence = " << opt.silence
        << "\nrecord = " << opt.record
        << "\nmax_output = " << opt.max_output
        << "\nverbosity = " << opt.verbosity
        << "\ncol = " << opt.col_name
        << "\ntab = " << opt.tab_name
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
        trace_boot_page(db, db.get_bootpage(), opt);
        if (opt.alloc_page) {
            trace_pfs_page(db, opt);
        }
    }
    if (opt.file_header) {
        trace_sys_page(db, db.get_fileheader(), nullptr, opt);
    }
    if (opt.page_num >= 0) {
        trace_page(db, db.load_page_head(opt.page_num), opt);
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
    if (opt.alloc_page || opt.record) {
        trace_datatable(db, opt);
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

