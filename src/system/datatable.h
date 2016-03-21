// datatable.h
//
#ifndef __SDL_SYSTEM_DATATABLE_H__
#define __SDL_SYSTEM_DATATABLE_H__

#pragma once

#include "sysobj/iam_page.h"
#include "index_tree.h"

namespace sdl { namespace db {

class database;

class datatable : noncopyable {
    using vector_sysallocunits_row = std::vector<sysallocunits_row const *>;
    using vector_page_head = std::vector<page_head const *>;
public:
    using column_order = std::pair<usertable::column const *, sortorder>;
    using key_mem = index_tree::key_mem;
public:
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
            : table(p), data_type(t1), page_type(t2) {
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
        using iterator = page_iterator<datarow_access, page_slot>;
        datarow_access(datatable * p, dataType::type t1, pageType::type t2)
            : table(p), _datapage(p, t1, t2) {
            SDL_ASSERT(table);
        }
        iterator begin();
        iterator end();
    public:
        recordID get_id(iterator const &);
        page_head const * get_page(iterator const &);
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
        datatable const * const table;
        row_head const * const record;
        const recordID this_id;
    public:
        record_type(datatable const *, row_head const *, const recordID & id = {});
        row_head const * head() const { return this->record; }
        const recordID & get_id() const { return this_id; }
        size_t size() const; // # of columns
        column const & usercol(size_t) const;
        std::string type_col(size_t) const;
        vector_mem_range_t data_col(size_t) const;
        vector_mem_range_t get_cluster_key(cluster_index const &) const;
    public:
        size_t fixed_size() const;
        size_t var_size() const;
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
        record_type dereference(datarow_iterator const &);
        bool use_record(datarow_iterator const &);
    };
public:
    using unique_record = std::unique_ptr<record_type>;
    using record_iterator = record_access::iterator;
    using datarow_iterator = datarow_access::iterator;
public:
    datatable(database *, shared_usertable const &);
    ~datatable();

    const std::string & name() const    { return schema->name(); }
    schobj_id get_id() const            { return schema->get_id(); }
    const usertable & ut() const        { return *schema.get(); }

    sysalloc_access get_sysalloc(dataType::type t1)                            { return sysalloc_access(this, t1); }
    datapage_access get_datapage(dataType::type t1, pageType::type t2)         { return datapage_access(this, t1, t2); }
    datapage_order get_datapage_order(dataType::type t1, pageType::type t2)    { return datapage_order(this, t1, t2); }
    datarow_access get_datarow(dataType::type t1, pageType::type t2)           { return datarow_access(this, t1, t2); }
    datarow_access _datarow{ this, dataType::type::IN_ROW_DATA, pageType::type::data };
    record_access _record{ this };

    shared_primary_key get_PrimaryKey() const; 
    column_order get_PrimaryKeyOrder() const;

    shared_cluster_index get_cluster_index() const;  
    unique_index_tree get_index_tree() const;

    unique_record find_record(key_mem const &) const;
    row_head const * find_row_head(key_mem const &) const;

    template<class T> 
    unique_record find_record_t(T const & key) const {
        const char * const p = reinterpret_cast<const char *>(&key);
        return find_record({p, p + sizeof(T)});
    }
    template<class T> 
    row_head const * find_row_head_t(T const & key) const {
        const char * const p = reinterpret_cast<const char *>(&key);
        return find_row_head({p, p + sizeof(T)});
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
private:
    template<class ret_type, class fun_type>
    ret_type find_row_head_impl(key_mem const &, fun_type) const;
private:
    database * const db;
    shared_usertable const schema;
};

class detached_datarow {
    using datarow_access = datatable::datarow_access;
    datatable table;
public:
    using iterator = datarow_access::iterator;
    detached_datarow(database * p, shared_usertable const & s)
        : table(p, s) 
    {}
    iterator begin() {
        return table._datarow.begin();
    }
    iterator end()  {
        return table._datarow.end();
    }
};

using shared_datatable = std::shared_ptr<datatable>; 
using vector_shared_datatable = std::vector<shared_datatable>; 
using unique_datatable = std::unique_ptr<datatable>;

} // db
} // sdl

#include "datatable.inl"

#endif // __SDL_SYSTEM_DATATABLE_H__
