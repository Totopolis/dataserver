// generator.cpp
//
#include "dataserver/maketable/generator.h"
#include "dataserver/maketable/generator_util.h"
#include "dataserver/system/database.h"
#include "dataserver/system/page_info.h"
#include <fstream>

namespace sdl { namespace db { namespace make { namespace {

const char FILE_BEGIN_TEMPLATE[] = R"(//%s{out_file}
//%s{database}
//)" __DATE__ " " __TIME__
R"(
#pragma once
#ifndef __SDL_GENERATOR_%s{unique}_H__
#define __SDL_GENERATOR_%s{unique}_H__

#include "dataserver/maketable/maketable.h"

#if defined(SDL_OS_WIN32)
//warning C4503: decorated name length exceeded, name was truncated
#pragma warning(disable:4503)
#endif
%s{make_namespace}
namespace sdl { namespace db { namespace make {%s{namespace}
)";

const char FILE_END_TEMPLATE[] = R"(%s{namespace}
} // make
} // db
} // sdl

#endif
)";

const char NS_BEGIN[] = R"( namespace %s {)";
const char NS_END[] =  R"(
} // %s)";

const char SDL_MAKE_NAMESPACE[] = R"(
#define SDL_MAKE_NAMESPACE_%s)";

const char MAKE_TEMPLATE[] = R"(
struct %s{dbo}_%s{name}_META {
    struct col {%s{COL_TEMPLATE}
    };
    typedef TL::Seq<%s{TYPE_LIST}
    >::Type type_list;
    struct index {%s{INDEX_TEMPLATE}
    };
    typedef TL::Seq<%s{INDEX_LIST}
    >::Type index_list;%s{CLUSTER_INDEX}
    static constexpr const char * name() { return "%s{name}"; }
    static constexpr const char * schema_name() { return "%s{dbo}"; }
    static constexpr int32 id = %s{schobj_id};
    static constexpr int32 nsid = %s{nsid_id};
    using spatial_index = %s{spatial_index};
};

class %s{dbo}_%s{name} final : public %s{dbo}_%s{name}_META, public make_base_table<%s{dbo}_%s{name}_META> {
    using base_table = make_base_table<%s{dbo}_%s{name}_META>;
    using this_table = %s{dbo}_%s{name};
public:
    class record final : public base_record<this_table> {
        using base = base_record<this_table>;
        using access = base_access<this_table, record>;
        using query = make_query<this_table, record>;
        friend access;
        friend query;
        friend this_table;
    public:
        record(this_table const * p, row_head const * h) noexcept : base(p, h) {}
        record() = default;%s{REC_TEMPLATE}
    };%s{static_record_count}
private:
    record::access const _record;
public:
    using iterator = record::access::iterator;
    using query_type = record::query;
    explicit %s{dbo}_%s{name}(database const * p, shared_usertable const & s)
        : base_table(p, s), _record(this), query(this, p) {}
    iterator begin() const { return _record.begin(); }
    iterator end() const { return _record.end(); }
    query_type const * operator ->() const { return &query; }
    query_type const query;
};
)";

const char TYPE_LIST[] = R"(
        col::%s{col_name} /*[%d]*/)";

const char KEY_TEMPLATE[] = R"(, meta::key<%s{PK}, %s{key_pos}, sortorder::%s{key_order}>)";
const char SPATIAL_KEY[] = R"(, meta::spatial_key)";

const char COL_TEMPLATE[] = R"(
        struct %s{col_name} : meta::col<%s{col_place}, %s{col_off}, scalartype::t_%s{col_type}, %s{col_len}%s{KEY_TEMPLATE}> { static constexpr const char * %s{_name}() { return "%s{col_name}"; } };)";

#if 0
const char REC_TEMPLATE[] = R"(
        auto %s{col_name}() const -> col::%s{col_name}::ret_type { return val<col::%s{col_name}>(); })";
#else // C++14
const char REC_TEMPLATE[] = R"(
        decltype(auto) %s{col_name}() const { return val<col::%s{col_name}>(); })";
#endif

const char INDEX_TEMPLATE[] = R"(
        struct %s{index_name} : meta::idxstat<%s{schobj_id}, %s{index_id}, idxtype::%s{idxtype}> { static constexpr const char * name() { return "%s{index_name}"; } };)";

const char INDEX_LIST[] = R"(
        index::%s{index_name} /*[%d]*/)";

const char STATIC_RECORD_COUNT[] = R"(
    static constexpr size_t static_record_count = %d; /*%s{dbo}_%s{table}*/)";

const char VOID_CLUSTER_INDEX[] = R"(
    using clustered = void;)";

#if 0
const char CLUSTER_INDEX[] = R"(
    struct clustered_META {%s{index_col}
        typedef TL::Seq<%s{type_list}>::Type type_list;
    };
    struct clustered final : make_clustered<clustered_META> {
#pragma pack(push, 1)
        struct key_type {%s{index_val}%s{key_get_set}
            template<size_t i> void get() const && = delete;
            template<size_t i> void set() && = delete;
            template<size_t i> auto get() const & -> decltype(get(Int2Type<i>())) { return get(Int2Type<i>()); }
            template<size_t i> auto set() & -> decltype(set(Int2Type<i>())) { return set(Int2Type<i>()); }
            using this_clustered = clustered;
        };
#pragma pack(pop)
        static constexpr const char * name() { return "%s{index_name}"; }
        static bool is_less(key_type const & x, key_type const & y) {%s{key_less}
            return false;
        }
        static bool less_first(decltype(key_type()._0) const & x, decltype(key_type()._0) const & y) {
            if (meta::is_less<T0>::less(x, y)) return true;
            return false;
        }
        static constexpr pageType::type root_page_type = pageType::type::%s{root_pageType};
    };)";
#else // C++14
const char CLUSTER_INDEX[] = R"(
    struct clustered_META {%s{index_col}
        typedef TL::Seq<%s{type_list}>::Type type_list;
    };
    struct clustered final : make_clustered<clustered_META> {
#pragma pack(push, 1)
        struct key_type {%s{index_val}%s{key_get_set}
            template<size_t i> void get() const && = delete;
            template<size_t i> void set() && = delete;
            template<size_t i> decltype(auto) get() const & { return get(Int2Type<i>()); }
            template<size_t i> decltype(auto) set() & { return set(Int2Type<i>()); }
            using this_clustered = clustered;
        };
#pragma pack(pop)
        static constexpr const char * name() { return "%s{index_name}"; }
        static bool is_less(key_type const & x, key_type const & y) {%s{key_less}
            return false;
        }
        static bool less_first(decltype(key_type()._0) const & x, decltype(key_type()._0) const & y) {
            if (meta::is_less<T0>::less(x, y)) return true;
            return false;
        }
        static constexpr pageType::type root_page_type = pageType::type::%s{root_pageType};
    };)";
#endif

const char CLUSTER_INDEX_COL[] = R"(
        using T%d = meta::index_col<col::%s{col_name}%s{offset}>;)";

const char CLUSTER_INDEX_VAL[] = R"(
            T%d::type _%d; /*%s{comment}*/)";

const char CLUSTER_KEY_GET_SET[] = R"(
            void get(Int2Type<%d>) const && = delete;
            void set(Int2Type<%d>) && = delete;
            T%d::type const & get(Int2Type<%d>) const & { return _%d; }
            T%d::type & set(Int2Type<%d>) & { return _%d; })";

const char CLUSTER_KEY_LESS_TRUE[] = R"(
            if (meta::is_less<T%d>::less(x._%d, y._%d)) return true;)";

const char CLUSTER_KEY_LESS_FALSE[] = R"(
            if (meta::is_less<T%d>::less(y._%d, x._%d)) return false;)";

const char SPATIAL_INDEX_NAME[] = R"(index::%s)";

const char DATABASE_TABLE_LIST[] = R"(
struct database_table_list {
    typedef TL::Seq<%s{TYPE_LIST}
    >::Type type_list;
    static constexpr size_t static_table_count = %s{table_count};
    static constexpr const char * dbname() { return "%s{dbi_dbname}"; }
};
)";

const char TABLE_NAME_TEMPLATE[] = R"(
        %s{dbo}_%s{name} /*[%d]*/)";

//-------------------------------------------------------------------------------------------

std::string table_schema(datatable const & table,
    generator::map_schema_names const * const names)
{
    if (names && !names->empty()) {
        const auto nsid = table.ut().get_nsid()._32;
        const auto it = names->find(nsid);
        if (it != names->end()) {
            return it->second;
        }
        SDL_ASSERT(0);
    }
    return "dbo";
}

struct table_info {
    std::string schema;
    shared_datatable table;
    bool is_include;
    table_info(std::string const & s, shared_datatable const & t, const bool b)
        : schema(s), table(t), is_include(b)
    {}
    static bool less(table_info const & x, table_info const & y) {
        if (x.schema == y.schema) {
            return x.table->name() < y.table->name();
        }
        return x.schema < y.schema;
    }
};

using unique_table_info = std::unique_ptr<table_info>;

template<class vector_string, class fun_type>
void for_datatables(database const & db,
                    vector_string const & include,
                    vector_string const & exclude,
                    const generator::map_schema_names * const schema_names,
                    fun_type && fun)
{
    if (schema_names && !schema_names->empty()) {
        std::vector<unique_table_info> tables;
        {
            for (auto const & p : db._datatables) {
                A_STATIC_CHECK_TYPE(shared_datatable const &, p);
                bool is_include = false;
                if (!util::is_find(exclude, p->name())) {
                    if (include.empty() || util::is_find(include, p->name())) {
                        is_include = true;
                    }
                }
                tables.push_back(std::make_unique<table_info>(table_schema(*p, schema_names), p, is_include));
            }
            std::sort(tables.begin(), tables.end(),
                [](const unique_table_info & x, const unique_table_info & y){
                return table_info::less(*x, *y);
            });
        }
        for (auto const & p : tables) {
            fun(*(p->table), p->is_include, schema_names);
        }
    }
    else {
        for (auto const & p : db._datatables) {
            A_STATIC_CHECK_TYPE(shared_datatable const &, p);
            bool is_include = false;
            if (!util::is_find(exclude, p->name())) {
                if (include.empty() || util::is_find(include, p->name())) {
                    is_include = true;
                }
            }
            fun(*p, is_include, schema_names);
        }
    }
}

void replace_namespace(std::string & s, const char * const token,
    const char buf[], std::string const & value)
{
    replace(s, token, value.empty() ? value : replace_(buf, "%s", value));
}

std::string & replace_dbo(std::string & s, datatable const & table, 
    generator::map_schema_names const * const names)
{
    return replace(s, "%s{dbo}", table_schema(table, names));
}

std::string & remove_newline(std::string & s) {
    size_t pos = 0;
    for (;;) {
        SDL_ASSERT(pos <= s.size());
        pos = s.find_first_of("\n\r", pos);
        if (pos == std::string::npos) {
            break;
        }
        s.erase(pos, 1);
    }
    return s;
}

bool read_schema_names(generator::map_schema_names & result, std::string const & file_name)
{
    SDL_ASSERT(result.empty());    
    if (!file_name.empty()) {
        std::ifstream infile(file_name);
        std::string s; // s = nsid, name
        do {
            std::getline(infile, s);
            if (remove_newline(s).empty()) 
                break;
            const auto col = util::split(s, ',');
            if (col.size() > 1) {
                const auto nsid = atoi(col[0].c_str());
                result[nsid] = col[1];
            }
            else {
                SDL_ASSERT(0);
                break;
            }
        } while (!(infile.rdstate() & std::ios_base::eofbit));
        infile.close();        
        return !result.empty();
    }
    return false;
}

} // namespace

//-------------------------------------------------------------------------------------------

std::string generator::make_table(database const & db, datatable const & table, 
    const bool is_record_count,
    map_schema_names const * const schema_names)
{
    std::string s(MAKE_TEMPLATE);
    replace_dbo(s, table, schema_names);

    const usertable & tab = table.ut();
    replace(s, "%s{name}", tab.name());
    replace(s, "%s{schobj_id}", tab.get_id()._32);
    replace(s, "%s{nsid_id}", tab.get_nsid()._32);
    {
        std::string s_columns;
        std::string s_type_list;
        std::string s_record;
        const auto PK = db.get_primary_key(tab.get_id());
        for (size_t i = 0; i < tab.size(); ++i) {
            usertable::column_ref t = tab[i];
            std::string s_col(COL_TEMPLATE);
            replace(s_col, "%s{col_name}", t.name);
            replace(s_col, "%s{col_place}", tab.place(i));
            replace(s_col, "%s{col_off}", tab.offset(i));
            replace(s_col, "%s{col_type}", scalartype::get_name(t.type));
            replace(s_col, "%s{col_len}", t.length._16);
            replace(s_col, "%s{_name}", (t.name == "name") ? "_name" : "name");
            std::string s_key;
            if (PK) {
                auto found = PK->find_colpar(t.colpar);
                if (found != PK->colpar.end()) {
                    const size_t key_pos = found - PK->colpar.begin();
                    s_key = replace_(KEY_TEMPLATE, "%s{PK}", "true");
                    replace(s_key, "%s{key_pos}", key_pos);
                    replace(s_key, "%s{key_order}", to_string::type_name(PK->order[key_pos]));
                }
                else if (t.is_geography()) {
                    if (db.find_spatial_tree(tab.get_id())) {
                        s_key = SPATIAL_KEY;
                    }
                }
            }
            replace(s_col, "%s{KEY_TEMPLATE}", s_key);
            s_columns += s_col;
            s_record += replace_(REC_TEMPLATE, "%s{col_name}", t.name);
            if (i) s_type_list += ",";
            std::string s_type(TYPE_LIST);
            s_type_list += replace(replace(s_type, "%s{col_name}", t.name), "%d", i);
        }
        replace(s, "%s{COL_TEMPLATE}", s_columns);
        replace(s, "%s{TYPE_LIST}", s_type_list);
        replace(s, "%s{REC_TEMPLATE}", s_record);
    }
    {
        std::string s_index_template;
        std::string s_index_list;
        size_t i = 0;
        for (auto const idx : db.index_for_table(tab.get_id())) {
            std::string s_index(INDEX_TEMPLATE);
            auto const index_name = col_name_t(idx);
            replace(s_index, "%s{index_name}", index_name);
            replace(s_index, "%s{schobj_id}", idx->data.id._32);
            replace(s_index, "%s{index_id}", idx->data.indid._32);
            replace(s_index, "%s{idxtype}", idx->data.type.name());
            s_index_template += s_index;
            if (i) s_index_list += ",";
            std::string s_type(INDEX_LIST);
            s_index_list += replace(replace(s_type, "%s{index_name}", index_name), "%d", i++);
        }
        replace(s, "%s{INDEX_TEMPLATE}", s_index_template);
        replace(s, "%s{INDEX_LIST}", s_index_list);
    }
    if (auto const key = table.get_cluster_index()) {
        std::string s_cluster(CLUSTER_INDEX);
        std::string s_index_col;
        std::string s_index_type;
        std::string s_index_val;
        std::string s_key_get_set;
        std::string s_key_less;
        for (size_t i = 0; i < key->size(); ++i) {
            cluster_index::column_ref k = (*key)[i];
            auto s_col = replace_(CLUSTER_INDEX_COL, "%d", i);
            replace(s_col, "%s{col_name}", k.name);
            if (i) {
                replace(s_col, "%s{offset}", 
                    replace_(", T%d::offset + sizeof(T%d::type)", "%d", i - 1));
            }
            else {
                replace(s_col, "%s{offset}", "");
            }
            s_index_col += s_col;
            if (i) {
                s_index_type += ", ";
            }
            s_index_type += replace_("T%d", "%d", i);
            s_index_val += replace_(CLUSTER_INDEX_VAL, "%d", i);
            if (k.is_array()) {
                std::string s = scalartype::get_name(k.type);
                s += replace_("(%d)", "%d", k.length._16);
                replace(s_index_val, "%s{comment}", s);
            }
            else {
                replace(s_index_val, "%s{comment}", scalartype::get_name(k.type));
            }
            s_key_get_set += replace_(CLUSTER_KEY_GET_SET, "%d", i);
            s_key_less += replace_(CLUSTER_KEY_LESS_TRUE, "%d", i);
            if ((i + 1) < key->size()) {
                s_key_less += replace_(CLUSTER_KEY_LESS_FALSE, "%d", i);
            }
        }
        replace(s_cluster, "%s{index_col}", s_index_col);
        replace(s_cluster, "%s{type_list}", s_index_type);
        replace(s_cluster, "%s{index_val}", s_index_val);
        replace(s_cluster, "%s{key_get_set}", s_key_get_set);
        replace(s_cluster, "%s{key_less}", s_key_less);
        replace(s_cluster, "%s{index_name}", key->name());
        SDL_ASSERT(key->is_root_index() || key->is_root_data());
        replace(s_cluster, "%s{root_pageType}", 
            key->is_root_index() ? "index" : (key->is_root_data() ? "data" : "null"));
        replace(s, "%s{CLUSTER_INDEX}", s_cluster);
    }
    else {
        replace(s, "%s{CLUSTER_INDEX}", VOID_CLUSTER_INDEX);
    }
    if (auto tree = table.get_spatial_tree()) {
        replace(s, "%s{spatial_index}", replace_(SPATIAL_INDEX_NAME, "%s", tree->name()));
    }
    else {
        replace(s, "%s{spatial_index}", "void");
    }
    if (is_record_count) {
        std::string s_record_count(STATIC_RECORD_COUNT);
        replace_dbo(s_record_count, table, schema_names);
        replace(s_record_count, "%d", table._record.count());
        replace(s_record_count, "%s{table}", table.name());
        replace(s, "%s{static_record_count}", s_record_count);
    }
    else {
        replace(s, "%s{static_record_count}", "");
    }
    SDL_ASSERT(s.find("%s{") == std::string::npos);
    return s;
}

bool generator::make_file(database const & db, param_type const & par)
{
    if (par.out_file.empty()) {
        return false;
    }
    std::ofstream outfile(par.out_file, std::ofstream::out|std::ofstream::trunc);
    if (outfile.rdstate() & std::ifstream::failbit) {
        throw_error<generator_error>("generator: error opening file");
        return false;
    }
    map_schema_names schema_names;
    if (!par.schema_names.empty()) {
        read_schema_names(schema_names, par.schema_names);
    }
    const bool is_record_count = par.is_record_count;
    std::string s_begin(FILE_BEGIN_TEMPLATE);
    replace(s_begin, "%s{out_file}", par.out_file);
    replace(s_begin, "%s{database}", db.filename());
    replace(s_begin, "%s{unique}", std::hash<std::string>()(par.out_file));
    replace_namespace(s_begin, "%s{namespace}", NS_BEGIN, par.make_namespace);
    replace_namespace(s_begin, "%s{make_namespace}", SDL_MAKE_NAMESPACE, par.make_namespace);
    outfile << s_begin;
    std::string s_table_list;
    size_t table_count = 0;
    for_datatables(db, par.include, par.exclude, &schema_names,
        [&outfile, &db, &table_count, &s_table_list, is_record_count]
        (datatable const & table, bool const is_include, const map_schema_names * schema_names){
            if (is_include) {
                {
                    std::string s("%s{dbo}_");
                    replace_dbo(s, table, schema_names);
                    SDL_TRACE("make: ", s, table.name());
                }
                outfile << generator::make_table(db, table, is_record_count, schema_names);
                {
                    std::string s(TABLE_NAME_TEMPLATE);
                    replace_dbo(s, table, schema_names);
                    if (table_count) s_table_list += ",";
                    replace(s, "%s{name}", table.name());
                    replace(s, "%d", table_count);
                    s_table_list += s;
                    ++table_count;
                }
            }
            else {
                SDL_TRACE("exclude: ", table.name());
            }
        });
    if (table_count) {
        std::string s_tables(DATABASE_TABLE_LIST);
        replace(s_tables, "%s{TYPE_LIST}", s_table_list);
        replace(s_tables, "%s{table_count}", table_count);
        replace(s_tables, "%s{dbi_dbname}", db.dbi_dbname());
        outfile << s_tables;
    }
    std::string s_end(FILE_END_TEMPLATE);
    replace_namespace(s_end, "%s{namespace}", NS_END, par.make_namespace);
    outfile << s_end;
    outfile.close();
    SDL_TRACE("File created : ", par.out_file);
    return true;
}

std::string generator::make_tables(database const & db, 
    vector_string const & include,
    vector_string const & exclude,
    const bool is_record_count,
    const map_schema_names * const schema_names)
{
    std::stringstream ss;
    for_datatables(db, include, exclude, schema_names,
        [&db, &ss, is_record_count](datatable const & table,
            bool const is_include, const map_schema_names * schema_names) {
            if (is_include) {
                SDL_TRACE("make: ", table.name());
                ss << generator::make_table(db, table, is_record_count, schema_names);
            }
            else {
                SDL_TRACE("exclude: ", table.name());
            }
    });
    return ss.str();
}


#if SDL_DEBUG
namespace {
    class unit_test {
    public:
        unit_test()
        {
            std::string s;
            SDL_ASSERT(remove_newline(s).empty());
            s = "1,dbo\r\n";
            SDL_ASSERT(remove_newline(s) == "1,dbo");
            s = "\r\n1,dbo\r\n";
            SDL_ASSERT(remove_newline(s) == "1,dbo");
            s = "\r\n"; 
            SDL_ASSERT(remove_newline(s).empty());
        }
    };
    static unit_test s_test;
}
#endif
} // make
} // db
} // sdl
