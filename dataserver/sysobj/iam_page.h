// iam_page.h
//
#pragma once
#ifndef __SDL_SYSOBJ_IAM_PAGE_H__
#define __SDL_SYSOBJ_IAM_PAGE_H__

#include "system/datapage.h"
#include "iam_page_row.h"

namespace sdl { namespace db {

class database;
class iam_page : noncopyable {

    using extent_type = datapage_t<iam_extent_row>;
    const extent_type extent;
private:
    class extent_access : noncopyable {
        iam_page const * const page;
    public:
        using iterator = extent_type::iterator;
        using value_type = extent_type::value_type;

        explicit extent_access(iam_page const * p): page(p) {
            SDL_ASSERT(page);
            A_STATIC_ASSERT_TYPE(value_type, iam_extent_row const *);
        }
        size_t size() const {
            auto const sz = page->extent.size();
            return (sz <= 1) ? 0 : (sz - 1);
        }
        bool empty() const {
            return 0 == this->size();
        }
        iterator begin() const {
            if (empty()) {
                return page->extent.end();
            }
            auto it = page->extent.begin();
            return ++it; // must skip first row 
        }
        iterator end() const {
            return page->extent.end();
        }
        iam_extent_row const & operator[](size_t i) const {
            SDL_ASSERT(i < size());
            return *(page->extent[i + 1]);
        }
        iam_extent_row const & first() const {
            return (*this)[0];
        }
    };
    //using allocated_fun = std::function<void(pageFileID const &)>;
    //void _allocated_extents(allocated_fun) const;
    //void _allocated_pages(database *, allocated_fun) const;
public:
    static const char * name() { return "iam_page"; }
    page_head const * const head;
    slot_array const slot;
    explicit iam_page(page_head const * h)
        : extent(h), head(h), slot(h) 
    {
        SDL_ASSERT(head);
    }
    bool empty() const {
        return slot.empty();
    }
    size_t size() const {
        return slot.size();
    }
    iam_page_row const * first() const;
    const extent_access _extent{ this };

    using fun_param = pageFileID const &;
    
    template<class allocated_fun>
    void allocated_extents(allocated_fun) const; // uniform extent

    template<class allocated_fun>
    void allocated_pages(database * db, allocated_fun) const;
};

using shared_iam_page = std::shared_ptr<iam_page>;

} // db
} // sdl

#include "iam_page.inl"

#endif // __SDL_SYSOBJ_IAM_PAGE_H__