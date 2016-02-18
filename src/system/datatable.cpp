// datatable.cpp
//
#include "common/common.h"
#include "datatable.h"
#include "database.h"
#include "page_info.h"

namespace sdl { namespace db {
   
void datatable::datarow_access::load_next(page_slot & p)
{
    SDL_ASSERT(!is_end(p));
    if (++p.second >= slot_array(*p.first).size()) {
        p.second = 0;
        ++p.first;
    }
}

void datatable::datarow_access::load_prev(page_slot & p)
{
    if (p.second > 0) {
        SDL_ASSERT(!is_end(p));
        --p.second;
    }
    else {
        SDL_ASSERT(!is_begin(p));
        --p.first;
        const size_t size = slot_array(*p.first).size(); // slot_array can be empty
        p.second = size ? (size - 1) : 0;
    }
    SDL_ASSERT(!is_end(p));
}

bool datatable::datarow_access::is_begin(page_slot const & p)
{
    if (!p.second && (p.first == _datapage.begin())) {
        return true;
    }
    return false;
}

bool datatable::datarow_access::is_end(page_slot const & p)
{
    if (p.first == _datapage.end()) {
        SDL_ASSERT(!p.second);
        return true;
    }
    return false;
}

bool datatable::datarow_access::is_empty(page_slot const & p)
{
    return datapage(*p.first).empty();
}

row_head const * datatable::datarow_access::dereference(page_slot const & p)
{
    const datapage page(*p.first);
    return page.empty() ? nullptr : page[p.second];
}

datatable::datarow_access::iterator
datatable::datarow_access::begin()
{
    return iterator(this, page_slot(_datapage.begin(), 0));
}

datatable::datarow_access::iterator
datatable::datarow_access::end()
{
    return iterator(this, page_slot(_datapage.end(), 0));
}

bool datatable::datarow_access::is_same(page_slot const & p1, page_slot const & p2)
{ 
    return p1 == p2;
}   

page_head const * 
datatable::datarow_access::get_page(iterator it)
{
    SDL_ASSERT(it != this->end());
    page_slot const & current = it.current;
    auto p = *current.first;
    A_STATIC_CHECK_TYPE(page_head const *, p);
    SDL_ASSERT(p);
    return p;
}

recordID datatable::datarow_access::get_id(iterator it)
{
    if (page_head const * page = get_page(it)) {
        A_STATIC_CHECK_TYPE(page_slot, it.current);
        return recordID::init(page->data.pageId, it.current.second );
    }
    return {};
}

datatable::datarow_access::page_RID
datatable::datarow_access::get_page_RID(iterator it)
{
    if (page_head const * page = get_page(it)) {
        A_STATIC_CHECK_TYPE(page_slot, it.current);
        return { page, recordID::init(page->data.pageId, it.current.second) };
    }
    return {};
}

//--------------------------------------------------------------------------

datatable::record_access::record_access(datatable * p)
    : table(p)
    , _datarow(p, dataType::type::IN_ROW_DATA, pageType::type::data)
{
    SDL_ASSERT(table);
}

datatable::record_access::iterator
datatable::record_access::begin()
{
    datarow_iterator it = _datarow.begin();
    while (it != _datarow.end()) {
        if (use_record(it))
            break;
        ++it;
    }
    return iterator(this, std::move(it));
}

void datatable::record_access::load_next(datarow_iterator & it)
{
    SDL_ASSERT(it != _datarow.end());
    for (;;) {
        ++it;
        if (it == _datarow.end())
            break;
        if (use_record(it))
            break;
    }
}

bool datatable::record_access::use_record(datarow_iterator const & it)
{
    if (row_head const * const p = *it) {
        if (p->is_forwarding_record()) // skip forwarding records 
            return false;
        if (p->get_type() == recordType::ghost_data) // skip ghosted records
            return false;
        return true;        
    }
    return false;
}

datatable::record_access::iterator
datatable::record_access::end()
{
    return iterator(this, _datarow.end());
}

bool datatable::record_access::is_end(datarow_iterator const & it)
{
    return (it == _datarow.end());
}

bool datatable::record_access::is_same(datarow_iterator const & p1, datarow_iterator const & p2)
{
    return p1 == p2;
}

datatable::record_type
datatable::record_access::dereference(datarow_iterator const & p)
{
    A_STATIC_CHECK_TYPE(row_head const *, *p);
    return record_type(table, *p, _datarow.get_page_RID(p));
}

//------------------------------------------------------------------

datatable::record_type::record_type(datatable * p, row_head const * row, const page_RID & pid)
    : table(p)
    , record(row)
    , m_id(pid.second)
    , fixed_data(row->fixed_data())
{
    A_STATIC_CHECK_TYPE(page_head const *, pid.first);
    SDL_ASSERT(table && record && m_id && pid.first);
    SDL_ASSERT((fixed_data_size() + sizeof(row_head)) == pid.first->data.pminlen);
    SDL_ASSERT(table->ut().size() == null_bitmap(record).size());
}

size_t datatable::record_type::size() const
{
    return table->ut().size();
}

datatable::record_type::column const & 
datatable::record_type::usercol(size_t i) const
{
    return table->ut()[i];
}

size_t datatable::record_type::fixed_data_size() const
{
    SDL_ASSERT(record->fixed_size() == table->ut().fixed_size());
    SDL_ASSERT(record->fixed_size() == mem_size(this->fixed_data));
    return mem_size(this->fixed_data);
}

size_t datatable::record_type::var_data_size() const
{
    if (record->has_variable()) {
        return variable_array(record).var_data_size();
    }
    return 0;
}

size_t datatable::record_type::count_null() const
{
    if (record->has_null()) {
        return null_bitmap(record).count_null();
    }
    return 0;
}

bool datatable::record_type::is_forwarded() const
{ 
    return record->is_forwarded_record();
}

bool datatable::record_type::is_null(size_t i) const
{
    SDL_ASSERT(i < this->size());
    if (record->has_null()) {
        return null_bitmap(record)[i];
    }
    SDL_ASSERT(0);
    return false;
}

size_t datatable::record_type::count_var() const
{
    if (record->has_variable()) {
        const size_t s = variable_array(record).size();
        // trailing NULL variable-length columns in the row are not stored
        // forwarded records adds forwarded_stub as variable column
        SDL_ASSERT(is_forwarded() || (s <= size()));
        SDL_ASSERT(is_forwarded() || (s <= table->ut().count_var()));
        SDL_ASSERT(!is_forwarded() || forwarded());
        return s;
    }
    return 0;
}

forwarded_stub const *
datatable::record_type::forwarded() const
{
    if (is_forwarded()) {
        if (record->has_variable()) {
            auto const m = variable_array(record).back_var_data();
            if (mem_size(m) == sizeof(forwarded_stub)) {
                forwarded_stub const * p = reinterpret_cast<forwarded_stub const *>(m.first);
                return p;
            }
        }
        SDL_ASSERT(0);
    }
    return nullptr;
}

size_t datatable::record_type::count_fixed() const
{
    const size_t s = table->ut().count_fixed();
    SDL_ASSERT(s <= size());
    return s;
}

mem_range_t datatable::record_type::fixed_memory(column const & col, size_t const i) const
{
    SDL_ASSERT(mem_size(fixed_data));
    const char * const p1 = fixed_data.first + table->ut().fixed_offset(i);
    const char * const p2 = p1 + col.fixed_size();
    if (p2 <= fixed_data.second) {
        return { p1, p2 };
    }
    SDL_ASSERT(!"bad offset");
    return{};
}


namespace {

template<typename T, scalartype::type type>
T const * scalartype_cast(mem_range_t const & m, usertable::column const & col)
{
    if (col.type == type) {
        if ((mem_size(m) == sizeof(T)) && (col.fixed_size() == sizeof(T))) {
            return reinterpret_cast<const T *>(m.first);
        }
        SDL_ASSERT(!"scalartype_cast");
    }
    return nullptr; 
}

} // namespace

std::string datatable::record_type::type_fixed_col(mem_range_t const & m, column const & col)
{
    SDL_ASSERT(mem_size(m) == col.fixed_size());

    if (auto pv = scalartype_cast<int, scalartype::t_int>(m, col)) {
        return to_string::type(*pv);
    }
    if (auto pv = scalartype_cast<int64, scalartype::t_bigint>(m, col)) {
        return to_string::type(*pv);
    }
    if (auto pv = scalartype_cast<int16, scalartype::t_smallint>(m, col)) {
        return to_string::type(*pv);
    }
    if (auto pv = scalartype_cast<float, scalartype::t_real>(m, col)) {
        return to_string::type(*pv);
    }
    if (auto pv = scalartype_cast<double, scalartype::t_float>(m, col)) {
        return to_string::type(*pv);
    }
    if (col.type == scalartype::t_numeric) {
        if ((mem_size(m) == (sizeof(int64) + 1)) && ((*m.first) == 0x01))  {
            auto pv = reinterpret_cast<int64 const *>(m.first + 1);
            return to_string::type(*pv);
        }
        return to_string::dump_mem(m); // FIXME: not implemented
    }
    if (auto pv = scalartype_cast<smalldatetime_t, scalartype::t_smalldatetime>(m, col)) {
        return to_string::type(*pv);
    }
    if (col.type == scalartype::t_nchar) {
        return to_string::type(make_nchar_checked(m));
    }
    else if (col.type == scalartype::t_char) {
        return std::string(m.first, m.second); // can be Windows-1251
    }
    return "?"; // FIXME: not implemented
}

template<class root_type>
mem_range_t datatable::load_slot(root_type const * const root, size_t const slot) const
{
    SDL_ASSERT(slot < root->curlinks);
    auto const & p = root->data[slot];
    if (p.size && p.row) {
        auto const page_row = this->db->load_page_row(p.row);
        if (page_row.first && page_row.second) {
            SDL_ASSERT(page_row.first->data.type == pageType::type::textmix);
            mem_range_t const m = page_row.second->fixed_data();
            size_t const sz = mem_size(m);
            if (sz > sizeof(lob_head)) {
                lob_head const * const lob = reinterpret_cast<lob_head const *>(m.first);
                if (lob->type == lobtype::DATA) {
                    SDL_ASSERT(root->head.blobID == lob->blobID);
                    const char * const p1 = m.first + sizeof(lob_head);
                    if (p1 <= m.second) {
                        return { p1, m.second };
                    }
                }
            }
        }
        SDL_ASSERT(0);
    }
    SDL_ASSERT(0);
    return {};
}

template<class root_type>
vector_mem_range_t
datatable::load_root(root_type const * const root) const
{
    if (root->curlinks > 0) {
        vector_mem_range_t result(root->curlinks);
        size_t offset = 0;
        for (size_t i = 0; i < root->curlinks; ++i) {
            auto & d = result[i];
            d = load_slot(root, i);
            SDL_ASSERT(mem_size(d));
            offset += mem_size(d);
            SDL_ASSERT(offset == root->data[i].size);
        }
        SDL_ASSERT(mem_size_n(result) == offset);
        return result;
    }
    SDL_ASSERT(0);
    return{};
}

//----------------------------------------------------------------------
// SQL Server stores variable-length column data, which does not exceed 8,000 bytes, on special pages called row-overflow pages

class datatable::varchar_overflow_page : noncopyable{
    using data_type = vector_mem_range_t;
    data_type m_data;
public:
    varchar_overflow_page(datatable *, overflow_page const *);
    const data_type & data() const {
        return m_data;
    }
    size_t length() const {
        return mem_size_n(m_data);
    }
    bool empty() const {
        return m_data.empty();
    }
    std::string text() const {
       return to_string::make_text(m_data);
    }
    std::string ntext() const {
       return to_string::make_ntext(m_data);
    }
};

datatable::varchar_overflow_page::varchar_overflow_page(
    datatable * const table, 
    overflow_page const * const page_over)
{
    SDL_ASSERT(table && page_over && page_over->row);
    SDL_ASSERT(page_over->length);

    auto const page_row = table->db->load_page_row(page_over->row);
    if (page_row.first && page_row.second) {
        if (page_row.first->data.type == pageType::type::textmix) {
            mem_range_t const m = page_row.second->fixed_data();
            if (mem_size(m) == page_over->length + sizeof(lob_head)) {
                lob_head const * const lob = reinterpret_cast<lob_head const *>(m.first);
                SDL_ASSERT(lob->blobID == page_over->timestamp);
                if (lob->type == lobtype::DATA) {
                    m_data.emplace_back(m.first + sizeof(lob_head), m.second);
                }
                else {
                    SDL_ASSERT(0);
                }
            }
        }
        else if (page_row.first->data.type == pageType::type::texttree) {
            mem_range_t const m = page_row.second->fixed_data();
            const size_t sz = mem_size(m);
            if (sz > sizeof(lob_head)) {
                lob_head const * const lob = reinterpret_cast<lob_head const *>(m.first);
                SDL_ASSERT(lob->blobID == page_over->timestamp);
                if (lob->type == lobtype::INTERNAL) {
                    // LOB root structure, which contains a set of the pointers to other data pages/rows.
                    // When LOB data is less than 32 KB and can fit into five data pages, 
                    // the LOB root structure contains the pointers to the actual chunks of LOB data
                    if (sz >= sizeof(TextTreeInternal)) {
                        TextTreeInternal const * const root = reinterpret_cast<TextTreeInternal const *>(m.first);
                        SDL_ASSERT(root->head.blobID == lob->blobID);
                        SDL_ASSERT(root->curlinks <= root->maxlinks);
                        if (root->curlinks && (sz >= root->length())) {
                            m_data = table->load_root(root);
                        }
                        else {
                            SDL_ASSERT(0);
                        }
                    }
                }
            }
        }
    }
    SDL_ASSERT(mem_size_n(m_data));
}

//----------------------------------------------------------------------

class datatable::varchar_overflow_link : noncopyable{
    using data_type = vector_mem_range_t;
    data_type m_data;
public:
    varchar_overflow_link(datatable *, overflow_page const *, overflow_link const *);
    const data_type & data() const {
        return m_data;
    }
    size_t length() const {
        return mem_size_n(m_data);
    }
    bool empty() const {
        return m_data.empty();
    }
    std::string text() const {
        return to_string::make_text(m_data);
    }
    std::string ntext() const {
        return to_string::make_ntext(m_data);
    }
};

datatable::varchar_overflow_link::varchar_overflow_link(
    datatable * const table, 
    overflow_page const * const page_over,
    overflow_link const * const page_link)
{
    SDL_ASSERT(table);
    SDL_ASSERT(page_over && page_over->row);
    SDL_ASSERT(page_link && page_link->row);  
    SDL_ASSERT(page_over->length);
    SDL_ASSERT(page_link->size);

    auto const page_row = table->db->load_page_row(page_link->row);
    if (page_row.first && page_row.second) {
        if (page_row.first->data.type == pageType::type::textmix) {
            mem_range_t const m = page_row.second->fixed_data();
            auto const sz = mem_size(m);
            if (sz > sizeof(lob_head)) {
                lob_head const * const lob = reinterpret_cast<lob_head const *>(m.first);
                SDL_ASSERT(lob->blobID == page_over->timestamp);
                if (lob->type == lobtype::DATA) {
                    m_data.emplace_back(m.first + sizeof(lob_head), m.second);
                }
                else {
                    SDL_ASSERT(0);
                }
            }
            else {
                SDL_ASSERT(0);
            }
        }
    }
    SDL_ASSERT(mem_size_n(m_data));
}

//------------------------------------------------------------------
// For the text, ntext, or image columns, SQL Server stores the data off-row by default. It uses another kind of page called LOB data pages.
// Like ROW_OVERFLOW data, there is a pointer to another piece of information called the LOB root structure, which contains a set of the pointers to other data pages/rows.
class datatable::text_pointer_data : noncopyable{
    using data_type = vector_mem_range_t;
    data_type m_data;
public:
    text_pointer_data(datatable *, text_pointer const *);
    const data_type & data() const {
        return m_data;
    }
    size_t length() const {
        return mem_size_n(m_data);
    }
    bool empty() const {
        return m_data.empty();
    }
    std::string text() const {
        return to_string::make_text(m_data);
    }
    std::string ntext() const {
        return to_string::make_ntext(m_data);
    }
};

datatable::text_pointer_data::text_pointer_data(
    datatable * const table, 
    text_pointer const * const text_ptr)
{
    SDL_ASSERT(table && text_ptr && text_ptr->row);

    auto const page_row = table->db->load_page_row(text_ptr->row);
    if (page_row.first && page_row.second) {
        // textmix(3) stores multiple LOB values and indexes for LOB B-trees
        SDL_ASSERT(page_row.first->data.type == pageType::type::textmix);
        mem_range_t const m = page_row.second->fixed_data();
        const size_t sz = mem_size(m);
        if (sz > sizeof(lob_head)) {
            lob_head const * const lob = reinterpret_cast<lob_head const *>(m.first);
            if (lob->type == lobtype::LARGE_ROOT_YUKON) {
                // LOB root structure, which contains a set of the pointers to other data pages/rows.
                // When LOB data is less than 32 KB and can fit into five data pages, 
                // the LOB root structure contains the pointers to the actual chunks of LOB data
                if (sz >= sizeof(LargeRootYukon)) {
                    LargeRootYukon const * const root = reinterpret_cast<LargeRootYukon const *>(m.first);
                    SDL_ASSERT(root->head.blobID == lob->blobID);
                    SDL_ASSERT(root->curlinks <= root->maxlinks);
                    SDL_ASSERT(root->maxlinks == 5);
                    if (root->curlinks && (sz >= root->length())) {
                        m_data = table->load_root(root);
                    }
                    else {
                        SDL_ASSERT(0);
                    }
                }
            }
            else if (lob->type == lobtype::SMALL_ROOT) {
                SDL_ASSERT(sz > sizeof(LobSmallRoot));
                if (sz > sizeof(LobSmallRoot)) {
                    LobSmallRoot const * const root = reinterpret_cast<LobSmallRoot const *>(m.first);      
                    const char * const p1 = m.first + sizeof(LobSmallRoot);
                    const char * const p2 = p1 + root->length;
                    if (p2 <= m.second) {
                        m_data.emplace_back(p1, p2);
                    }
                    else {
                        SDL_ASSERT(0);
                    }
                }
            }
        }
    }
    SDL_ASSERT(mem_size_n(m_data));
}

//----------------------------------------------------------------------

// varchar, ntext, text, geography
std::string datatable::record_type::type_var_col(column const & col, size_t const col_index) const
{
    const size_t i = table->ut().var_offset(col_index);
    if (i < count_var()) {
        const variable_array data(record);
        const mem_range_t m = data.var_data(i);
        const size_t len = mem_size(m);
        if (!len) {
            SDL_ASSERT(0);
            return{};
        }
        if (data.is_complex(i)) {
            // If length == 16 then we're dealing with a LOB pointer, otherwise it's a regular complex column
            if (len == sizeof(text_pointer)) { // 16 bytes
                auto const tp = reinterpret_cast<text_pointer const *>(m.first);
                if (col.type == scalartype::t_text) {
                    return text_pointer_data(table, tp).text();
                }
                if (col.type == scalartype::t_ntext) {
                    return text_pointer_data(table, tp).ntext();
                }
            }
            else {
                const auto type = data.var_complextype(i);
                SDL_ASSERT(type != complextype::none);
                if (type == complextype::row_overflow) {
                    if (len == sizeof(overflow_page)) { // 24 bytes
                        auto const overflow = reinterpret_cast<overflow_page const *>(m.first);
                        SDL_ASSERT(overflow->type == type);
                        if (col.type == scalartype::t_varchar) {
                            return varchar_overflow_page(table, overflow).text();
                        }
                    }
                }
                else if (type == complextype::blob_inline_root) {
                    if (len == sizeof(overflow_page)) { // 24 bytes
                        auto const overflow = reinterpret_cast<overflow_page const *>(m.first);
                        SDL_ASSERT(overflow->type == type);
                        if (col.type == scalartype::t_geography) {
                            const varchar_overflow_page varchar(table, overflow);
                            SDL_ASSERT(varchar.length() == overflow->length);
                            return to_string::dump_mem(varchar.data());
                        }
                    }
                    if (len > sizeof(overflow_page)) { // 24 bytes + 12 bytes * link_count 
                        SDL_ASSERT(!((len - sizeof(overflow_page)) % sizeof(overflow_link)));
                        if (col.type == scalartype::t_geography) {
                            auto const page = reinterpret_cast<overflow_page const *>(m.first);
                            size_t const link_count = (len - sizeof(overflow_page)) / sizeof(overflow_link);
                            auto const link = reinterpret_cast<overflow_link const *>(page + 1);
                            const varchar_overflow_page varchar(table, page);
                            SDL_ASSERT(varchar.length() == page->length);
                            auto memory = varchar.data();
                            for (size_t i = 0; i < link_count; ++i) {
                                const varchar_overflow_link next(table, page, link + i);
                                memory.insert(memory.end(), next.data().begin(), next.data().end());
                                SDL_ASSERT(mem_size_n(memory) == link[i].size);
                            }
                            return to_string::dump_mem(memory);
                        }
                    }
                }
            }
            SDL_ASSERT(0);
            return "?";
        }
        else { // in-row-data
            if (col.type == scalartype::t_varchar) {
                return std::string(m.first, m.second);
            }
            if (col.type == scalartype::t_nvarchar) {
                return to_string::type(make_nchar_checked(m));
            }
            if (col.type == scalartype::t_geography) {
                return to_string::dump_mem(m);
            }
            SDL_ASSERT(0);
            return "?"; // not implemented
        }
    }
    throw_error<record_error>("bad var_offset");
    return {};
}

std::string datatable::record_type::type_col(size_t const i) const
{
    SDL_ASSERT(i < this->size());

    if (is_null(i)) {
        return {};
    }
    column const & col = usercol(i);
    if (col.is_fixed()) {
        return type_fixed_col(fixed_memory(col, i), col);
    }
    return type_var_col(col, i);
}

//--------------------------------------------------------------------------

datatable::sysalloc_access::vector_data const & 
datatable::sysalloc_access::find_sysalloc() const
{
    return table->db->find_sysalloc(table->get_id(), data_type);
}

datatable::datapage_access::vector_data const &
datatable::datapage_access::find_datapage() const
{
    return table->db->find_datapage(table->get_id(), data_type, page_type);
}

//--------------------------------------------------------------------------

datatable::datapage_order::datapage_order(datatable * p, dataType::type t1, pageType::type t2)
    : ordered(datapage_access(p, t1, t2).find_datapage())
{
    std::sort(ordered.begin(), ordered.end(), 
        [](page_head const * x, page_head const * y){
        return (x->data.pageId < y->data.pageId);
    });
}

//--------------------------------------------------------------------------

} // db
} // sdl

