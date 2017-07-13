// database_impl.h
//
#pragma once
#ifndef __SDL_SYSTEM_DATABASE_IMPL_H__
#define __SDL_SYSTEM_DATABASE_IMPL_H__

#include "dataserver/common/map_enum.h"
#include "dataserver/common/compact_map.h"
#include "dataserver/system/page_map.h"
#if SDL_USE_BPOOL
#include "dataserver/bpool/page_bpool.h"
#endif

namespace sdl { namespace db {

class database_PageMapping : noncopyable {
public:
#if 0 //SDL_USE_BPOOL
    bpool::page_bpool pm;
#else
    const PageMapping pm;
#endif
    explicit database_PageMapping(const std::string & fname)
        : pm(fname) {}
};

class database::shared_data final : public database_PageMapping {
    using map_sysalloc = compact_map<schobj_id, shared_sysallocunits>;
    using map_datapage = compact_map<schobj_id, shared_page_head_access>;
    using map_index = compact_map<schobj_id, pgroot_pgfirst>;
    using map_primary = compact_map<schobj_id, shared_primary_key>;
    using map_cluster = compact_map<schobj_id, shared_cluster_index>;
    using map_spatial_tree = compact_map<schobj_id, spatial_tree_idx>;
    struct data_type {
        shared_usertables usertable;
        shared_usertables internal;
        shared_datatables datatable;
        map_enum_1<map_sysalloc, dataType> sysalloc;
        map_enum_2<map_datapage, dataType, pageType> datapage; // not preloaded in init_database()
        map_enum_1<map_index, pageType> index;
        map_primary primary;
        map_cluster cluster;
        map_spatial_tree spatial_tree;
        data_type()
            : usertable(std::make_shared<vector_shared_usertable>())
            , internal(std::make_shared<vector_shared_usertable>())
            , datatable(std::make_shared<vector_shared_datatable>())
        {}
    };
public:
    bool initialized = false;
    const std::string filename;
    explicit shared_data(const std::string & fname)
        : database_PageMapping(fname)
        , filename(fname)
    {}
    shared_usertables & usertable() { // get/set shared_ptr only
        return m_data.usertable;
    } 
    shared_usertables & internal() {
        return m_data.internal;
    }
    shared_datatables & datatable() {
        return m_data.datatable;
    }
    bool empty_usertable() {
        lock_guard lock(m_mutex);
        return m_data.usertable->empty();
    }
    bool empty_internal() {
        lock_guard lock(m_mutex);
        return m_data.internal->empty();
    }
    bool empty_datatable() {
        lock_guard lock(m_mutex);
        return m_data.datatable->empty();
    }
    std::pair<shared_sysallocunits, bool> 
    find_sysalloc(schobj_id const id, dataType::type const data_type) {
        lock_guard lock(m_mutex);
        if (auto found = m_data.sysalloc.find(id, data_type)) {
            return { *found, true };
        }
        return{};
    }
    void set_sysalloc(schobj_id const id, dataType::type const data_type,
                      shared_sysallocunits const & value) {
        lock_guard lock(m_mutex);
        m_data.sysalloc(id, data_type) = value;
    }
    std::pair<shared_page_head_access, bool>
    find_datapage(schobj_id const id, dataType::type const data_type, pageType::type const page_type) {
        lock_guard lock(m_mutex);
        if (auto found = m_data.datapage.find(id, data_type, page_type)) {
            return { *found, true };
        }
        return{};
    }
    void set_datapage(schobj_id const id, 
                      dataType::type const data_type,
                      pageType::type const page_type,
                      shared_page_head_access const & value) {
        lock_guard lock(m_mutex);
        m_data.datapage(id, data_type, page_type) = value;
    }
    std::pair<pgroot_pgfirst, bool> load_pg_index(schobj_id const id, pageType::type const page_type) {
        lock_guard lock(m_mutex);
        if (auto found = m_data.index.find(id, page_type)) {
            return { *found, true };
        }
        return{};
    }
    void set_pg_index(schobj_id const id, pageType::type const page_type, pgroot_pgfirst const & value) {
        lock_guard lock(m_mutex);
        m_data.index(id, page_type) = value;
    }
    std::pair<shared_primary_key, bool> get_primary_key(schobj_id const table_id) {
        lock_guard lock(m_mutex);
        auto const found = m_data.primary.find(table_id);
        if (found != m_data.primary.end()) {
            return { found->second, true };
        }
        return{};
    }
    void set_primary_key(schobj_id const table_id, shared_primary_key const & value) {
        lock_guard lock(m_mutex);
        m_data.primary[table_id] = value;
    }
    std::pair<shared_cluster_index, bool> get_cluster_index(schobj_id const id) {
        lock_guard lock(m_mutex);
        auto const found = m_data.cluster.find(id);
        if (found != m_data.cluster.end()) {
            return { found->second, true };
        }
        return{};
    }
    void set_cluster_index(schobj_id const id, shared_cluster_index const & value) {
        lock_guard lock(m_mutex);
        m_data.cluster[id] = value;
    }
    std::pair<spatial_tree_idx, bool> find_spatial_tree(schobj_id const table_id) {
        lock_guard lock(m_mutex);
        auto const found = m_data.spatial_tree.find(table_id);
        if (found != m_data.spatial_tree.end()) {
            return { found->second, true };
        }
        return{};
    }
    void set_spatial_tree(schobj_id const table_id, spatial_tree_idx const & value) {
        lock_guard lock(m_mutex);
        m_data.spatial_tree[table_id] = value;
    }
private:
    data_type const & const_data() const { return m_data; }
    data_type & data() { return m_data; }
    using lock_guard = std::lock_guard<std::mutex>;
    std::mutex m_mutex;
    data_type m_data; //FIXME: preload all and use read-only ?
};

} // db
} // sdl

#endif // __SDL_SYSTEM_DATABASE_IMPL_H__