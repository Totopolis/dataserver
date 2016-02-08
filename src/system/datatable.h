// datatable.h
//
#ifndef __SDL_SYSTEM_DATATABLE_H__
#define __SDL_SYSTEM_DATATABLE_H__

#pragma once

#include "usertable.h"
#include "page_iterator.h"

namespace sdl { namespace db {

class datatable;
class database;

class database_base 
{
protected:
    database_base() = default;
    ~database_base() = default;
public:
    using shared_usertable = std::shared_ptr<usertable>;
    using vector_shared_usertable = std::vector<shared_usertable>;
    
    using shared_datatable = std::shared_ptr<datatable>; 
    using vector_shared_datatable = std::vector<shared_datatable>; 

    using unique_datatable = std::unique_ptr<datatable>;
    using shared_datapage = std::shared_ptr<datapage>; 
    using shared_iam_page = std::shared_ptr<iam_page>;

    using vector_sysallocunits_row = std::vector<sysallocunits_row const *>;
    using vector_page_head = std::vector<page_head const *>;
};

class datatable : noncopyable
{
    using shared_usertable = database_base::shared_usertable;
    using shared_datapage = database_base::shared_datapage;
    using shared_iam_page = database_base::shared_iam_page;
    using vector_sysallocunits_row = database_base::vector_sysallocunits_row;
    using vector_page_head = database_base::vector_page_head;
private:
    database * const db;
    shared_usertable const schema;
private:
    class sysalloc_access {
        using vector_data = vector_sysallocunits_row;
        datatable * const table;
        dataType::type const data_type;
    public:
        using iterator = vector_data::const_iterator;
        sysalloc_access(datatable * p, dataType::type t)
            : table(p), data_type(t)
        {
            SDL_ASSERT(table);
            SDL_ASSERT(data_type != dataType::type::null);
        }
        iterator begin() {
            return find_sysalloc().begin();
        }
        iterator end() {
            return find_sysalloc().end();
        }
    private:
        vector_data const & find_sysalloc() const;
    };
//------------------------------------------------------------------
    class datapage_access {
        using vector_data = vector_page_head;
        datatable * const table;
        dataType::type const data_type;
        pageType::type const page_type;
    public:
        using iterator = vector_data::const_iterator;
        datapage_access(datatable * p, dataType::type t1, pageType::type t2)
            : table(p), data_type(t1), page_type(t2)
        {
            SDL_ASSERT(table);
            SDL_ASSERT(data_type != dataType::type::null);
            SDL_ASSERT(page_type != pageType::type::null);
        }
        iterator begin() {
            return find_datapage().begin();
        }
        iterator end() {
            return find_datapage().end();
        }
    private:
        vector_data const & find_datapage() const;
    };
//------------------------------------------------------------------
    class datarow_access {
        datatable * const table;
        datapage_access _datapage;
        using page_slot = std::pair<datapage_access::iterator, size_t>;        
    public:
        using iterator = page_iterator<datarow_access, page_slot>;
        datarow_access(datatable * p, dataType::type t1, pageType::type t2)
            : table(p), _datapage(p, t1, t2)
        {
            SDL_ASSERT(table);
        }
        iterator begin();
        iterator end();
    private:
        friend iterator;
        void load_next(page_slot &);
        void load_prev(page_slot &);
        bool is_empty(page_slot const &);
        bool is_begin(page_slot const &);
        bool is_end(page_slot const &);
        static bool is_same(page_slot const &, page_slot const &);
        row_head const * dereference(page_slot const &);
    };
//------------------------------------------------------------------
    class record_type {
        datatable * const table;
        row_head const * const record;
    public:
        using column = usertable::column;
        using columns = usertable::columns;
        const columns & schema;
        explicit record_type(datatable *, row_head const *);
        size_t size() const {
            return schema.size();
        }
    };
//------------------------------------------------------------------
    class record_access: noncopyable {
        datatable * const table;
        datarow_access _datarow;
        using datarow_iterator = datarow_access::iterator;
    public:
        using iterator = forward_iterator<record_access, datarow_iterator>;
        explicit record_access(datatable *);
        iterator begin();
        iterator end();
    private:
        friend iterator;
        void load_next(datarow_iterator &);
        bool is_end(datarow_iterator const &);
        static bool is_same(datarow_iterator const &, datarow_iterator const &);
        record_type dereference(datarow_iterator const &);
        bool use_record(datarow_iterator const &);
    };
public:
    datatable(database * p, shared_usertable const & t): db(p), schema(t) {
        SDL_ASSERT(db && schema);
    }
    ~datatable(){}

    const std::string & name() const {
        return schema->name;
    }
    schobj_id get_id() const {
        return schema->get_id();
    }
    const usertable & ut() const {
        return *schema.get();
    }
    sysalloc_access _sysalloc(dataType::type t1) {
        return sysalloc_access(this, t1);
    }
    datapage_access _datapage(dataType::type t1, pageType::type t2) {
        return datapage_access(this, t1, t2);
    }
    datarow_access _datarow(dataType::type t1, pageType::type t2) {
        return datarow_access(this, t1, t2);
    }
    template<class T, class fun_type> static
    void for_datarow(T && data, fun_type fun) {
        A_STATIC_ASSERT_TYPE(datarow_access, remove_reference_t<T>);
        for (row_head const * row : data) {
            if (row) { 
                fun(*row);
            }
        }
    }
    record_access _record{ this };
};

} // db
} // sdl

#endif // __SDL_SYSTEM_DATATABLE_H__
