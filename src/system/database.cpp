// database.cpp
//
#include "common/common.h"
#include "database.h"
#include "file_map.h"

namespace sdl { namespace db {

size_t sysallocunits::size() const
{
    return slot.size();
}

sysallocunits_row const *
sysallocunits::operator[](size_t i) const
{
    auto offset = slot[i];
    A_STATIC_CHECK_TYPE(uint16, offset);
    // FIXME: find row for offset
    return nullptr;
}

class database::data_t : noncopyable
{
    enum { page_size = page_head::page_size };
public:
    const std::string filename;
    explicit data_t(const std::string & fname);

    bool is_open() const
    {
        return m_fmap.IsFileMapped();
    }
    size_t page_count() const
    {   
        return m_pageCount;
    }
    page_head const * load_page(pageIndex) const;
    page_head const * load_page(pageFileID const &) const;
private:
    size_t m_pageCount;
    FileMapping m_fmap;
};

database::data_t::data_t(const std::string & fname)
    : filename(fname)
    , m_pageCount(0)
{
    static_assert(page_size == 8 * 1024, "");
    if (m_fmap.CreateMapView(fname.c_str())) {
        const size_t sz = m_fmap.GetFileSize();
        SDL_ASSERT(!(sz % page_size));
        m_pageCount = sz / page_size;
    }
    else {
        SDL_WARNING(false);
    }
}

page_head const *
database::data_t::load_page(pageIndex const i) const
{
    const size_t pageIndex = i.value();
    if (pageIndex < m_pageCount) {
        const char * const data = static_cast<const char *>(m_fmap.GetFileView());
        const char * p = data + pageIndex * page_size;
        return reinterpret_cast<page_head const *>(p);
    }
    SDL_ASSERT(0);
    return nullptr;
}

page_head const *
database::data_t::load_page(pageFileID const & id) const
{
    return load_page(pageIndex(id.pageId));
}

database::database(const std::string & fname)
    : m_data(new data_t(fname))
{
}

database::~database()
{
}

const std::string & database::filename() const
{
    return m_data->filename;
}

bool database::is_open() const
{
    return m_data->is_open();
}

size_t database::page_count() const
{
    return m_data->page_count();
}

page_head const *
database::load_page(pageIndex i)
{
    return m_data->load_page(i);
}

page_head const * 
database::load_page(sysPage i)
{
    return load_page(static_cast<pageIndex::value_type>(i));
}

std::unique_ptr<bootpage> 
database::get_bootpage()
{
    page_head const * const h = load_page(sysPage::boot_page);
    if (h) {
        return make_unique<bootpage>(h, page_body<bootpage_row>(h));
    }
    return std::unique_ptr<bootpage>();
}

std::unique_ptr<datapage>
database::get_datapage(pageIndex i)
{
    page_head const * const h = load_page(i);
    if (h) {
        return make_unique<datapage>(h);
    }
    return std::unique_ptr<datapage>();
}

std::unique_ptr<sysallocunits>
database::get_sysallocunits()
{
    auto boot = get_bootpage();
    if (boot) {
        auto & id = boot->row->data.dbi_firstSysIndexes;
        page_head const * const h = m_data->load_page(id);
        if (h) {
            return make_unique<sysallocunits>(h);
        }
    }
    return std::unique_ptr<sysallocunits>();
}

page_head const * database::load_next(page_head const * p)
{
    if (p) {
        return m_data->load_page(p->data.nextPage);
    }
    return nullptr;
}

page_head const * database::load_prev(page_head const * p)
{
    if (p) {
        return m_data->load_page(p->data.prevPage);
    }
    return nullptr;
}

} // db
} // sdl

#if SDL_DEBUG
namespace sdl {
    namespace db {
        namespace {
            class unit_test {
            public:
                unit_test()
                {
                    SDL_TRACE(__FILE__);
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG

#if 0
------------------------------------------------------
PFS_page
The first PFS page is at *:1 in each database file, 
and it stores the statistics for pages 0-8087 in that database file. 
There will be one PFS page for just about every 64MB of file size (8088 bytes * 8KB per page).
A large database file will use a long chain of PFS pages, linked together using the LastPage and NextPage pointers in the 96 byte header. 
------------------------------------------------------
linked list of pages using the PrevPage, ThisPage, and NextPage page locators, when one page is not enough to hold all the data.
------------------------------------------------------
------------------------------------------------------
------------------------------------------------------
#endif

