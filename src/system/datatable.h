// datatable.h
//
#ifndef __SDL_SYSTEM_DATATABLE_H__
#define __SDL_SYSTEM_DATATABLE_H__

#pragma once

#include "usertable.h"
#include "iam_page.h"
#include "index_tree.h"

namespace sdl { namespace db {

class datatable;
class database;

class database_base {
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
        size_t size() const {
            return find_sysalloc().size();
        }
    private:
        vector_data const & find_sysalloc() const;
    };
//------------------------------------------------------------------
    class datapage_order;
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
        size_t size() const {
            return find_datapage().size();
        }
    private:
        vector_data const & find_datapage() const;
        friend datapage_order;
    };
//------------------------------------------------------------------
    class datapage_order {
        using vector_data = vector_page_head;
        vector_data ordered;
    public:
        using iterator = vector_data::const_iterator;
        datapage_order(datatable *, dataType::type, pageType::type);
        iterator begin() {
            return ordered.begin();
        }
        iterator end() {
            return ordered.end();
        }
        size_t size() const {
            return ordered.size();
        }
    };
//------------------------------------------------------------------
    class datarow_access {
        datatable * const table;
        datapage_access _datapage;
        using page_slot = std::pair<datapage_access::iterator, size_t>;        
    public:
        using page_RID = std::pair<page_head const *, recordID>;
        using iterator = page_iterator<datarow_access, page_slot>;
        datarow_access(datatable * p, dataType::type t1, pageType::type t2)
            : table(p), _datapage(p, t1, t2)
        {
            SDL_ASSERT(table);
        }
        iterator begin();
        iterator end();
    public:
        recordID get_id(iterator);
        page_head const * get_page(iterator);
        page_RID get_page_RID(iterator);
    private:
        friend iterator;
        void load_next(page_slot &);
        void load_prev(page_slot &);
        bool is_empty(page_slot const &);
        bool is_begin(page_slot const &);
        bool is_end(page_slot const &);
        row_head const * dereference(page_slot const &);
    };
//------------------------------------------------------------------
    class record_type {
        using record_error = sdl_exception_t<record_type>;
    public:
        using column = usertable::column;
        using columns = usertable::columns;
    private:
        using page_RID = datarow_access::page_RID;
        datatable * const table;
        row_head const * const record;
        const recordID m_id;
        mem_range_t const fixed_data;
    public:
        record_type(datatable *, row_head const *, const page_RID &);
        const recordID & get_id() const { return m_id; }
        size_t size() const; // # of columns
        column const & usercol(size_t) const;
        std::string type_col(size_t) const;
        vector_mem_range_t data_col(size_t) const;
    public:
        size_t fixed_data_size() const;
        size_t var_data_size() const;
        size_t count_null() const;
        size_t count_var() const;
        size_t count_fixed() const;
        bool is_null(size_t) const;
        bool is_forwarded() const;
        forwarded_stub const * forwarded() const; // returns nullptr if not forwarded
    private:
        mem_range_t fixed_memory(column const & col, size_t) const;
        static std::string type_fixed_col(mem_range_t const & m, column const & col);
        std::string type_var_col(column const & col, size_t) const;
        vector_mem_range_t data_var_col(column const & col, size_t) const;
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
    datatable(database *, shared_usertable const &);
    ~datatable();

    const std::string & name() const {
        return schema->name();
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
    datapage_order _datapage_order(dataType::type t1, pageType::type t2) {
        return datapage_order(this, t1, t2);
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

    shared_primary_key get_PrimaryKey() const; 

    using column_order = std::pair<usertable::column const *, sortorder>;
    column_order get_PrimaryKeyOrder() const;

    unique_cluster_index get_cluster_index();  
    unique_index_tree get_index_tree();
private:
    database * const db;
    shared_usertable const schema;
};

} // db
} // sdl

#endif // __SDL_SYSTEM_DATATABLE_H__
