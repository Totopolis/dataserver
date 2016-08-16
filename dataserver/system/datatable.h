// datatable.h
//
#pragma once
#ifndef __SDL_SYSTEM_DATATABLE_H__
#define __SDL_SYSTEM_DATATABLE_H__

#include "sysobj/iam_page.h"
#include "index_tree.h"
#include "spatial/spatial_tree_t.h"
#include "spatial/geography.h"

namespace sdl { namespace db {

class database;

using spatial_tree = spatial_tree_t<int64>;
using shared_spatial_tree = std::shared_ptr<spatial_tree>;

template<typename pk0_type>
using shared_spatial_tree_t = std::shared_ptr<spatial_tree_t<pk0_type> >;

struct spatial_tree_idx {
    page_head const * pgroot = nullptr;
    sysidxstats_row const * idx = nullptr;
};

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
    class page_head_access: noncopyable {
    protected:
        using page_pos = std::pair<page_head const *, size_t>;
        virtual page_pos begin_page() = 0;
    public:
        using iterator = forward_iterator<page_head_access, page_pos>;
        virtual ~page_head_access() {}
        iterator begin() {
            return iterator(this, begin_page());
        }
        iterator end() {
            return iterator(this);
        }
    private:
        friend iterator;
        static page_head const * dereference(page_pos const & p) {
            return p.first;
        }
        virtual void load_next(page_pos &) = 0;
        static bool is_end(page_pos const & p) {
            SDL_ASSERT(p.first || !p.second);
            return (nullptr == p.first);
        }
    };
//------------------------------------------------------------------
    class datapage_access {
        datatable * const table;
        dataType::type const data_type;
        pageType::type const page_type;
        page_head_access * page_access = nullptr;
        page_head_access & find_datapage();
        page_head_access * get() {
            return page_access ? page_access : (page_access = &find_datapage());
        }
    public:
        using iterator = page_head_access::iterator;
        datapage_access(datatable * p, dataType::type t1, pageType::type t2)
            : table(p), data_type(t1), page_type(t2) {
            SDL_ASSERT(table);
            SDL_ASSERT(data_type != dataType::type::null);
            SDL_ASSERT(page_type != pageType::type::null);
        }
        iterator begin() {
            return get()->begin();
        }
        iterator end() {
            return get()->end();
        }
    };
//------------------------------------------------------------------
    class datarow_access {
        datatable * const table;
        datapage_access _datapage;
        using page_slot = std::pair<datapage_access::iterator, size_t>;        
    public:
        using iterator = forward_iterator<datarow_access, page_slot>;
        explicit datarow_access(datatable * p, 
            dataType::type t1 = dataType::type::IN_ROW_DATA, 
            pageType::type t2 = pageType::type::data)
            : table(p), _datapage(p, t1, t2) {
            SDL_ASSERT(table);
        }
        iterator begin();
        iterator end();
    public:
        static recordID get_id(iterator const &);
        static page_head const * get_page(iterator const &);
    private:
        friend iterator;
        void load_next(page_slot &);
        //void load_prev(page_slot &);
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
        using col_size_t = size_t; // column index or size
    public:
        record_type(datatable const *, row_head const *, const recordID & id = {});
        row_head const * head() const { return this->record; }
        const recordID & get_id() const { return this_id; }
        col_size_t size() const; // # of columns
        column const & usercol(col_size_t) const;
        std::string type_col(col_size_t) const;
        std::string STAsText(col_size_t) const;
        bool STContains(col_size_t, spatial_point const &) const;
        bool is_geography(col_size_t) const;
        spatial_type geo_type(col_size_t) const;
        vector_mem_range_t data_col(col_size_t) const;
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
    class record_access;
    class head_access: noncopyable {
        datatable * const table;
        datarow_access _datarow;
        using datarow_iterator = datarow_access::iterator;
    public:
        using iterator = forward_iterator<head_access, datarow_iterator>;
        explicit head_access(datatable *);
        iterator begin();
        iterator end();
    private:
        friend iterator;
        friend record_access;
        void load_next(datarow_iterator &);
        bool _is_end(datarow_iterator const &); // disabled
        static bool use_record(datarow_iterator const &);
        static row_head const * dereference(datarow_iterator const & p) {
            A_STATIC_CHECK_TYPE(row_head const *, *p);
            return *p;
        }
        static recordID get_id(iterator const & p) {
            return datarow_access::get_id(p.current);
        }
    };
//------------------------------------------------------------------
    class record_access: noncopyable {
        head_access _head;
        using head_iterator = head_access::iterator;
    public:
        using iterator = forward_iterator<record_access, head_iterator>;
        explicit record_access(datatable * p): _head(p){}
        iterator begin() {
            return iterator(this, _head.begin());
        }
        iterator end() {
            return iterator(this, _head.end());
        }
    private:
        friend iterator;
        void load_next(head_iterator & it) {
            ++it;
        }
        record_type dereference(head_iterator const & it) {
            A_STATIC_CHECK_TYPE(row_head const *, *it);
            SDL_ASSERT(*it);
            return record_type(_head.table, *it, head_access::get_id(it));
        }
    };
public:
    using unique_record = std::unique_ptr<record_type>;
    using datarow_iterator = datarow_access::iterator;
    using record_iterator = record_access::iterator;
    using head_iterator = head_access::iterator;
public:
    datatable(database *, shared_usertable const &);
    ~datatable();

    const std::string & name() const    { return schema->name(); }
    schobj_id get_id() const            { return schema->get_id(); }
    const usertable & ut() const        { return *schema.get(); }

    sysalloc_access get_sysalloc(dataType::type t1)                            { return sysalloc_access(this, t1); }
    datapage_access get_datapage(dataType::type t1, pageType::type t2)         { return datapage_access(this, t1, t2); }
    datarow_access get_datarow(dataType::type t1, pageType::type t2)           { return datarow_access(this, t1, t2); }
    datarow_access _datarow{ this };
    record_access _record{ this };
    head_access _head{ this };

    shared_primary_key get_PrimaryKey() const; 
    column_order get_PrimaryKeyOrder() const;

    shared_cluster_index get_cluster_index() const;  
    shared_index_tree get_index_tree() const;
    shared_spatial_tree get_spatial_tree() const;

    template<typename pk0_type>
    shared_spatial_tree_t<pk0_type> get_spatial_tree(identity<pk0_type>) const;

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
    spatial_tree_idx find_spatial_tree() const;
private:
    database * const db;
    shared_usertable const schema;
};

using shared_datatable = std::shared_ptr<datatable>; 
using vector_shared_datatable = std::vector<shared_datatable>; 
using unique_datatable = std::unique_ptr<datatable>;

} // db
} // sdl

#include "datatable.inl"

#endif // __SDL_SYSTEM_DATATABLE_H__
