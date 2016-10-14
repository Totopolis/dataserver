// datatable.h
//
#pragma once
#ifndef __SDL_SYSTEM_DATATABLE_H__
#define __SDL_SYSTEM_DATATABLE_H__

#include "sysobj/iam_page.h"
#include "index_tree.h"
#include "spatial/spatial_tree.h"
#include "spatial/geography.h"

#if (SDL_DEBUG > 1) && defined(SDL_OS_WIN32)
#define SDL_DEBUG_RECORD_ID     1
#endif

namespace sdl { namespace db {

class database;

class base_datatable {
protected:
    base_datatable(database const * p, shared_usertable const & t)
        : db(p), schema(t) {
        SDL_ASSERT(db && schema);
    }
    ~base_datatable(){}
public:
    database const * const db;
    const std::string & name() const    { return schema->name(); }
    schobj_id get_id() const            { return schema->get_id(); }
    const usertable & ut() const        { return *schema.get(); }
protected:
    shared_usertable const schema;
};

class datatable final : public base_datatable {
    datatable(const datatable&) = delete;
    datatable& operator=(const datatable&) = delete;
    using vector_sysallocunits_row = std::vector<sysallocunits_row const *>;
    using vector_page_head = std::vector<page_head const *>;
public:
    using column_order = std::pair<usertable::column const *, sortorder>;
    using key_mem = index_tree::key_mem;
public:
    class sysalloc_access { // copyable
        using vector_data = vector_sysallocunits_row;
        using shared_data = std::shared_ptr<vector_data>;
        shared_data sysalloc;
    public:
        using iterator = vector_data::const_iterator;
        sysalloc_access(base_datatable const *, dataType::type);
        iterator begin() const {
            return sysalloc->begin();
        }
        iterator end() const {
            return sysalloc->end();
        }
        size_t size() const {
            return sysalloc->size();
        }
    };
//------------------------------------------------------------------
    class page_head_access: noncopyable {
    protected:
        using page_pos = std::pair<page_head const *, size_t>;
        virtual page_pos begin_page() const = 0;
    public:
        using iterator = forward_iterator<page_head_access const, page_pos>;
        virtual ~page_head_access() {}
        iterator begin() const {
            return iterator(this, begin_page());
        }
        iterator end() const {
            return iterator(this);
        }
        iterator make_iterator(datatable const *, pageFileID const &) const;
    private:
        friend iterator;
        static page_head const * dereference(page_pos const & p) {
            return p.first;
        }
        virtual void load_next(page_pos &) const = 0;
        static bool is_end(page_pos const & p) {
            SDL_ASSERT(p.first || !p.second);
            return (nullptr == p.first);
        }
    };
//------------------------------------------------------------------
    class datapage_access {
        using shared_data = std::shared_ptr<page_head_access>;
        shared_data page_access;
    public:
        datapage_access(base_datatable const *, dataType::type, pageType::type);
        using iterator = page_head_access::iterator;
        iterator begin() const {
            return page_access->begin();
        }
        iterator end() const {
            return page_access->end();
        }
        iterator make_iterator(datatable const *, pageFileID const &) const;
    };
//------------------------------------------------------------------
    class datarow_access {
        const datapage_access _datapage;
        using page_slot = std::pair<datapage_access::iterator, size_t>;        
    public:
        using iterator = forward_iterator<datarow_access const, page_slot>;
        explicit datarow_access(base_datatable const * p, 
            dataType::type t1 = dataType::type::IN_ROW_DATA, 
            pageType::type t2 = pageType::type::data)
            : _datapage(p, t1, t2) {
        }
        iterator begin() const;
        iterator end() const;
        iterator make_iterator(datatable const *, recordID const &) const;
    public:
        static recordID get_id(iterator const &);
        static page_head const * get_page(iterator const &);
    private:
        friend iterator;
        void load_next(page_slot &) const;
        bool is_begin(page_slot const &) const;
        bool is_end(page_slot const &) const;
        bool is_empty(page_slot const &) const;
        static row_head const * dereference(page_slot const &);
    };
//------------------------------------------------------------------
    class record_type {
        using record_error = sdl_exception_t<record_type>;
    public:
        using column = usertable::column;
        using columns = usertable::columns;
    private:
        base_datatable const * table;
        row_head const * record;
#if SDL_DEBUG_RECORD_ID
        const recordID this_id;
#endif
        using col_size_t = size_t; // column index or size
    public:
        record_type(): table(nullptr), record(nullptr)
#if SDL_DEBUG_RECORD_ID
            , this_id() 
#endif
        {}
        record_type(base_datatable const *, row_head const *
#if SDL_DEBUG_RECORD_ID
            , const recordID & id = {}
#endif
        );
        bool is_null() const {
            return (nullptr == record);
        }
        explicit operator bool() const {
            return !is_null();
        }
#if SDL_DEBUG_RECORD_ID
        const recordID & get_id() const { return this_id; }
#endif
        row_head const * head() const { return this->record; }
        col_size_t size() const; // # of columns
        column const & usercol(col_size_t) const;
        std::string type_col(col_size_t) const;
        bool is_geography(col_size_t) const;
        spatial_type geo_type(col_size_t) const;
        geo_mem geography(col_size_t) const;
        vector_mem_range_t data_col(col_size_t) const;
        vector_mem_range_t get_cluster_key(cluster_index const &) const;        
        template<scalartype::type type>
        scalartype_t<type> const * cast_fixed_col(col_size_t) const;
        std::string STAsText(col_size_t) const;
        bool STContains(col_size_t, spatial_point const &) const;   // can use geography().STContains()
        Meters STDistance(col_size_t, spatial_point const &) const; // can use geography().STDistance()
    public:
        size_t fixed_size() const;
        size_t var_size() const;
        size_t count_null() const;
        size_t count_var() const;
        size_t count_fixed() const;
        bool is_null(col_size_t) const;
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
        base_datatable const * const table;
        const datarow_access _datarow;
        using datarow_iterator = datarow_access::iterator;
    public:
        using iterator = forward_iterator<head_access const, datarow_iterator>;
        explicit head_access(base_datatable const *);
        iterator begin() const;
        iterator end() const;
        iterator make_iterator(datatable const *, recordID const &) const;
    private:
        friend iterator;
        friend record_access;
        void load_next(datarow_iterator &) const;
        bool _is_end(datarow_iterator const &) const; // disabled
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
        const head_access _head;
        using head_iterator = head_access::iterator;
    public:
        using iterator = forward_iterator<record_access const, head_iterator>;
        explicit record_access(base_datatable const * p): _head(p){}
        iterator begin() const {
            return iterator(this, _head.begin());
        }
        iterator end() const {
            return iterator(this, _head.end());
        }
        size_t count() const { // checks for forwarded and ghosted records
            return std::distance(begin(), end()); // can be slow
        }
        iterator make_iterator(datatable const * p, recordID const & rec) const {
            return iterator(this, _head.make_iterator(p, rec));
        }
    private:
        friend iterator;
        void load_next(head_iterator & it) const {
            ++it;
        }
        record_type dereference(head_iterator const & it) const {
            A_STATIC_CHECK_TYPE(row_head const *, *it);
            SDL_ASSERT(*it);
            return record_type(_head.table, *it
#if SDL_DEBUG_RECORD_ID
                , head_access::get_id(it)
#endif
            );
        }
    };
public:
    using datarow_iterator = datarow_access::iterator;
    using record_iterator = record_access::iterator;
    using head_iterator = head_access::iterator;
public:
    datatable(database const *, shared_usertable const &);

    sysalloc_access get_sysalloc(dataType::type);
    datapage_access get_datapage(dataType::type, pageType::type);
    datarow_access get_datarow(dataType::type, pageType::type);
    datarow_access const _datarow;
    record_access const _record;
    head_access const _head;

    shared_primary_key const & get_PrimaryKey() const;
    column_order get_PrimaryKeyOrder() const;
    usertable::col_index get_PrimaryKeyCol() const;

    shared_cluster_index const & get_cluster_index() const;  
    shared_index_tree const & get_index_tree() const;
    spatial_tree get_spatial_tree() const;

    template<typename pk0_type> unique_spatial_tree_t<pk0_type>
    get_spatial_tree(identity<pk0_type>) const;

    bool is_index_tree() const {
        return !!m_index_tree;
    }
    row_head const * find_row_head(key_mem const &) const;

    record_type find_record(key_mem const & key) const;
    record_type find_record(vector_mem_range_t const & key) const; 
    record_type find_record(std::vector<char> const & key) const; 

    record_iterator find_record_iterator(key_mem const & key) const;
    record_iterator find_record_iterator(vector_mem_range_t const & key) const;
    record_iterator find_record_iterator(std::vector<char> const & key) const;

    template<typename T>
    record_iterator find(T const & key) const {
        return find_record_iterator(key);
    }
    template<class T> 
    record_type find_record_t(T const & key) const {
        const char * const p = reinterpret_cast<const char *>(&key);
        return find_record(key_mem(p, p + sizeof(T)));
    }
    template<class T> 
    row_head const * find_row_head_t(T const & key) const {
        const char * const p = reinterpret_cast<const char *>(&key);
        return find_row_head({p, p + sizeof(T)});
    }
    template<class T> 
    record_iterator find_record_iterator_t(T const & key) const {
        const char * const p = reinterpret_cast<const char *>(&key);
        return find_record_iterator({p, p + sizeof(T)});
    }
    template<typename T>
    record_iterator find_t(T const & key) const {
        return find_record_iterator_t(key);
    }
    template<class T, class fun_type> static
    void for_datarow(T && data, fun_type && fun);
private:
    template<class ret_type, class fun_type>
    ret_type find_row_head_impl(key_mem const &, fun_type const &) const;
    spatial_tree_idx find_spatial_tree() const;
    record_iterator scan_table_with_record_key(key_mem const &) const;
private:
    shared_primary_key m_primary_key;
    shared_cluster_index m_cluster_index;
    shared_index_tree m_index_tree;
};

using shared_datatable = std::shared_ptr<datatable>; 
using vector_shared_datatable = std::vector<shared_datatable>; 
using unique_datatable = std::unique_ptr<datatable>;

} // db
} // sdl

#include "datatable.inl"

#endif // __SDL_SYSTEM_DATATABLE_H__
