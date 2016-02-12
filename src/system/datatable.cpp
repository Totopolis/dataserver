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
    if (row_head const * p = *it) {
        if (!p->is_forwarding_record()) // skip forwarding records 
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
    /*if (auto pv = scalartype_cast<uint32, scalartype::t_smalldatetime>(m, col)) {
        return to_string::type(*pv);
    }*/
    if (col.type == scalartype::t_nchar) {
        return to_string::type(make_nchar_checked(m));
    }
    else if (col.type == scalartype::t_char) {
        return std::string(m.first, m.second); // can be Windows-1251
    }
    return "?"; // FIXME: not implemented
}

//----------------------------------------------------------------------

datatable::varchar_overflow::varchar_overflow(datatable * p, overflow_page const * overflow)
    : table(p), page_over(overflow)
{
    SDL_ASSERT(table && page_over && page_over->row);
    SDL_ASSERT(page_over->length);
}

std::string datatable::varchar_overflow::c_str() const
{
    if (page_head const * const h = table->db->load_page_head(page_over->row.id)) {
        SDL_ASSERT(h->data.type == pageType::type::textmix);
        const datapage data(h);
        if (page_over->row.slot < data.size()) {
            if (row_head const * const row = data[page_over->row.slot]) {
                mem_range_t const m = row->fixed_data();
                if (mem_size(m) == page_over->length + sizeof(lob_struct)) {
                    lob_struct const * const lob = reinterpret_cast<lob_struct const *>(m.first);
                    if (lob->type == lobtype::DATA) {
                        return std::string(m.first + sizeof(lob_struct), m.second);
                    }
                }
            }
        }
    }
    SDL_ASSERT(0);
    return {};
}

//----------------------------------------------------------------------

// varchar, ntext, text, geography
std::string datatable::record_type::type_var_col(column const & col, size_t const col_index) const
{
    const size_t i = table->ut().var_offset(col_index);
    if (i < count_var()) {
        const variable_array data(record);
        const mem_range_t m = data.var_data(i);
        if (data.is_complex(i)) {
            // If length == 16 then we're dealing with a LOB pointer, otherwise it's a regular complex column
            if (auto const text_pointer = data.get_text_pointer(i)) {
                if (col.type == scalartype::t_text) {
                    return "text pointer?";
                }
                if (col.type == scalartype::t_ntext) {
                    return "ntext pointer?";
                }
                SDL_ASSERT(0);
            }
            else {
                const auto comtype = data.get_complextype(i);
                if (comtype != complextype::none) {
                    if (auto const overflow = data.get_overflow_page(i)) {
                        SDL_ASSERT(overflow->type == complextype::row_overflow);
                        if (col.type == scalartype::t_varchar) {
                            return varchar_overflow(table, overflow).c_str();
                        }
                    }
                    if (col.type == scalartype::t_geography) {
                        SDL_ASSERT(comtype == complextype::blob_inline_root);
                        return "geography::blob_inline_root?";
                    }
                }
                SDL_ASSERT(0);
            }
            return std::string(scalartype::get_name(col.type)) + " COMPLEX";
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
            return "?"; // FIXME: not implemented
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

} // db
} // sdl

//----------------------------------------------------------------------------------------------------------
// page iterator -> type, row[]
// row iterator -> column[] -> column type, name, length, value 
// iam_chain(IN_ROW_DATA|LOB_DATA|ROW_OVERFLOW_DATA) -> iam_page[] -> datapage, index_page => row[] => col[]
//----------------------------------------------------------------------------------------------------------
// forwarded row = 16 bytes or 9 bytes ?
// forwarding_stub = 9 bytes
// forwarded_stub = 10 bytes
// overflow_page = 24 bytes
// text_pointer = 16 bytes
// ROW_OVERFLOW_DATA = 24 bytes
// LOB_DATA (text, ntext, image columns) = 16 bytes
//----------------------------------------------------------------------------------------------------------


#if 0
                // Complex columns store special values and may need to be read elsewhere. In this case I'm using somewhat of a hack to detect
                // row-overflow pointers the same way as normal complex columns. See http://improve.dk/archive/2011/07/15/identifying-complex-columns-in-records.aspx
                // for a better description of the issue. Currently there are three cases:
                // - Back pointers (two-byte value of 1024)
                // - Sparse vectors (two-byte value of 5)
                // - BLOB Inline Root (one-byte value of 4)
                // - Row-overflow pointer (one-byte value of 2)
                // First we'll try to read just the very first pointer - hitting case values like 5 and 2. 1024 will result in a value of 0. In that specific
                // case we then try to read a two-byte value.
                // Finally complex columns also store 16 byte LOB pointers. Since these do not store a complex column type ID but are the only 16-byte length
                // complex columns (except for the rare 16-byte sparse vector) we'll use that fact to detect them and retrieve the referenced data. This *is*
                // a bug, I'm just postponing the necessary refactoring for now.
                if (complexColumn)
                {
                    // If length == 16 then we're dealing with a LOB pointer, otherwise it's a regular complex column
                    if (RawVariableLengthColumnData[i].Length == 16)
                        VariableLengthColumnData[i] = new TextPointerProxy(Page, RawVariableLengthColumnData[i]);
                    else
                    {
                        short complexColumnID = RawVariableLengthColumnData[i][0];

                        if (complexColumnID == 0)
                            complexColumnID = BitConverter.ToInt16(RawVariableLengthColumnData[i], 0);

                        switch (complexColumnID)
                        {
                            // Row-overflow pointer, get referenced data
                            case 2:
                                VariableLengthColumnData[i] = new BlobInlineRootProxy(Page, RawVariableLengthColumnData[i]);
                                break;

                            // BLOB Inline Root
                            case 4:
                                VariableLengthColumnData[i] = new BlobInlineRootProxy(Page, RawVariableLengthColumnData[i]);
                                break;

                            // Sparse vectors will be processed at a later stage - no public option for accessing raw bytes
                            case 5:
                                SparseVector = new SparseVectorParser(RawVariableLengthColumnData[i]);
                                break;

                            // Forwarded record back pointer (http://improve.dk/archive/2011/06/09/anatomy-of-a-forwarded-record-ndash-the-back-pointer.aspx)
                            // Ensure we expect a back pointer at this location. For forwarding stubs, the data stems from the referenced forwarded record. For the forwarded record,
                            // the last varlength column is a backpointer. No public option for accessing raw bytes.
                            case 1024:
                                if ((Type == RecordType.ForwardingStub || Type == RecordType.BlobFragment) && i != NumberOfVariableLengthColumns - 1)
                                    throw new ArgumentException("Unexpected back pointer found at column index " + i);
                                break;

                            default:
                                throw new ArgumentException("Invalid complex column ID encountered: 0x" + BitConverter.ToInt16(RawVariableLengthColumnData[i], 0).ToString("X"));
                        }
                    }
                }
                else
                    VariableLengthColumnData[i] = new RawByteProxy(RawVariableLengthColumnData[i]);
#endif