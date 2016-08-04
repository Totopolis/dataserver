// generator.cpp
//
#include "common/common.h"
#include "generator.h"
#include "system/database.h"
#include "system/page_info.h"
#include <fstream>
#include <functional>

namespace sdl { namespace db { namespace make { namespace {

const char FILE_BEGIN_TEMPLATE[] = R"(//%s{out_file}
//%s{database}
//)" __DATE__ " " __TIME__
R"(
#pragma once
#ifndef __SDL_GENERATOR_%s{unique}_H__
#define __SDL_GENERATOR_%s{unique}_H__

#include "maketable/maketable.h"
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
struct dbo_%s{name}_META {
    struct col {%s{COL_TEMPLATE}
    };
    typedef TL::Seq<%s{TYPE_LIST}
    >::Type type_list;
    struct index {%s{INDEX_TEMPLATE}
    };
    typedef TL::Seq<%s{INDEX_LIST}
    >::Type index_list;%s{CLUSTER_INDEX}
    static const char * name() { return "%s{name}"; }
    static const int32 id = %s{schobj_id};
    using spatial_index = %s{spatial_index};
};

class dbo_%s{name} final : public dbo_%s{name}_META, public make_base_table<dbo_%s{name}_META> {
    using base_table = make_base_table<dbo_%s{name}_META>;
    using this_table = dbo_%s{name};
public:
    class record : public base_record<this_table> {
        using base = base_record<this_table>;
        using access = base_access<this_table, record>;
        using query = make_query<this_table, record>;
        friend access;
        friend query;
        friend this_table;
        record(this_table const * p, row_head const * h): base(p, h) {}
    public:
        record() = default;%s{REC_TEMPLATE}
    };
private:
    record::access _record;
public:
    using iterator = record::access::iterator;
    using query_type = record::query;
    explicit dbo_%s{name}(database * p, shared_usertable const & s)
        : base_table(p, s), _record(this), query(this, p, s) {}
    iterator begin() { return _record.begin(); }
    iterator end() { return _record.end(); }
    query_type * operator ->() { return &query; }
    query_type query;
};
)";

const char TYPE_LIST[] = R"(
        col::%s{col_name} /*[%d]*/)";

const char KEY_TEMPLATE[] = R"(, meta::key<%s{PK}, %s{key_pos}, sortorder::%s{key_order}>)";

const char COL_TEMPLATE[] = R"(
        struct %s{col_name} : meta::col<%s{col_place}, %s{col_off}, scalartype::t_%s{col_type}, %s{col_len}%s{KEY_TEMPLATE}> { static const char * name() { return "%s{col_name}"; } };)";

const char REC_TEMPLATE[] = R"(
        auto %s{col_name}() const -> col::%s{col_name}::ret_type { return val<col::%s{col_name}>(); })";

const char INDEX_TEMPLATE[] = R"(
        struct %s{index_name} : meta::idxstat<%s{schobj_id}, %s{index_id}, idxtype::%s{idxtype}> { static const char * name() { return "%s{index_name}"; } };)";

const char INDEX_LIST[] = R"(
        index::%s{index_name} /*[%d]*/)";

const char VOID_CLUSTER_INDEX[] = R"(
    using clustered = void;)";

const char CLUSTER_INDEX[] = R"(
    struct clustered_META {%s{index_col}
        typedef TL::Seq<%s{type_list}>::Type type_list;
    };
    struct clustered : make_clustered<clustered_META> {
#pragma pack(push, 1)
        struct key_type {%s{index_val}%s{key_get}%s{key_set}
            template<size_t i> auto get() const -> decltype(get(Int2Type<i>())) { return get(Int2Type<i>()); }
            template<size_t i> auto set() -> decltype(set(Int2Type<i>())) { return set(Int2Type<i>()); }
            using this_clustered = clustered;
        };
#pragma pack(pop)
        static const char * name() { return "%s{index_name}"; }
        static bool is_less(key_type const & x, key_type const & y) {%s{key_less}
            return false;
        }
        static bool less_first(decltype(key_type()._0) const & x, decltype(key_type()._0) const & y) {
            if (meta::is_less<T0>::less(x, y)) return true;
            return false;
        }
    };)";

const char CLUSTER_INDEX_COL[] = R"(
        using T%d = meta::index_col<col::%s{col_name}%s{offset}>;)";

const char CLUSTER_INDEX_VAL[] = R"(
            T%d::type _%d; /*%s{comment}*/)";

const char CLUSTER_KEY_GET[] = R"(
            T%d::type const & get(Int2Type<%d>) const { return _%d; })";

const char CLUSTER_KEY_SET[] = R"(
            T%d::type & set(Int2Type<%d>) { return _%d; })";

const char CLUSTER_KEY_LESS_TRUE[] = R"(
            if (meta::is_less<T%d>::less(x._%d, y._%d)) return true;)";

const char CLUSTER_KEY_LESS_FALSE[] = R"(
            if (meta::is_less<T%d>::less(y._%d, x._%d)) return false;)";

const char SPATIAL_INDEX_NAME[] = R"(index::%s)";

//-------------------------------------------------------------------------------------------

std::string & replace(std::string & s, const char * const token, const std::string & value) {
    size_t const n = strlen(token);
    size_t pos = 0;
    while (1) {
        const size_t i = s.find(token, pos, n);
        if (i != std::string::npos) {
            s.replace(i, n, value);
            pos = i + n;
        }
        else
            break;
    }
    SDL_ASSERT(pos);
    return s;
}

template<typename T>
std::string & replace(std::string & s, const char * const token, const T & value) {
    std::stringstream ss;
    ss << value;
    return replace(s, token, ss.str());
};

template<typename T> inline
std::string replace_(const char buf[], const char * const token, const T & value) {
    std::string s(buf);
    replace(s, token, value);
    return s;
};

template<typename T> inline
std::string replace_(std::string && s, const char * const token, const T & value) {
    replace(s, token, value);
    return std::move(s);
};

} // namespace 

std::string generator::make_table(database & db, datatable const & table)
{
    std::string s(MAKE_TEMPLATE);

    const usertable & tab = table.ut();
    replace(s, "%s{name}", tab.name());
    replace(s, "%s{schobj_id}", tab.get_id()._32);
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
            std::string s_key;
            if (PK) {
                auto found = PK->find_colpar(t.colpar);
                if (found != PK->colpar.end()) {
                    const size_t key_pos = found - PK->colpar.begin();
                    s_key = replace_(KEY_TEMPLATE, "%s{PK}", "true");
                    replace(s_key, "%s{key_pos}", key_pos);
                    replace(s_key, "%s{key_order}", to_string::type_name(PK->order[key_pos]));
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
    if (auto key = table.get_cluster_index()) {
        std::string s_cluster(CLUSTER_INDEX);
        std::string s_index_col;
        std::string s_index_type;
        std::string s_index_val;
        std::string s_key_get;
        std::string s_key_set;
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
            s_key_get += replace_(CLUSTER_KEY_GET, "%d", i);
            s_key_set += replace_(CLUSTER_KEY_SET, "%d", i);
            s_key_less += replace_(CLUSTER_KEY_LESS_TRUE, "%d", i);
            if ((i + 1) < key->size()) {
                s_key_less += replace_(CLUSTER_KEY_LESS_FALSE, "%d", i);
            }
        }
        replace(s_cluster, "%s{index_col}", s_index_col);
        replace(s_cluster, "%s{type_list}", s_index_type);
        replace(s_cluster, "%s{index_val}", s_index_val);
        replace(s_cluster, "%s{key_get}", s_key_get);
        replace(s_cluster, "%s{key_set}", s_key_set);
        replace(s_cluster, "%s{key_less}", s_key_less);
        replace(s_cluster, "%s{index_name}", key->name());
        replace(s, "%s{CLUSTER_INDEX}", s_cluster);
    }
    else {
        replace(s, "%s{CLUSTER_INDEX}", VOID_CLUSTER_INDEX);
    }
    if (auto tree = table.get_spatial_tree()) {
        replace(s, "%s{spatial_index}", replace_(SPATIAL_INDEX_NAME, "%s", tree->name()));
    }
    else {
        //index
        replace(s, "%s{spatial_index}", "void");
    }
    SDL_ASSERT(s.find("%s{") == std::string::npos);
    return s;
}

bool generator::make_file_ex(database & db, std::string const & out_file,
                             vector_string const & include,
                             vector_string const & exclude,
                             const char * const _namespace)
{
    if (!out_file.empty()) {
        std::ofstream outfile(out_file, std::ofstream::out|std::ofstream::trunc);
        if (outfile.rdstate() & std::ifstream::failbit) {
            throw_error<generator_error>("generator: error opening file");
        }
        else {
            std::string s_begin(FILE_BEGIN_TEMPLATE);
            replace(s_begin, "%s{out_file}", out_file);
            replace(s_begin, "%s{database}", db.filename());
            replace(s_begin, "%s{unique}", std::hash<std::string>()(out_file));
            replace(s_begin, "%s{namespace}", 
                _namespace ? replace_(NS_BEGIN, "%s", _namespace) : std::string()
            );
            replace(s_begin, "%s{make_namespace}",
                 _namespace ? replace_(SDL_MAKE_NAMESPACE, "%s", _namespace) : std::string()
            );
            outfile << s_begin;
            for (auto const & p : db._datatables) {
                A_STATIC_CHECK_TYPE(shared_datatable const &, p);
                if (!util::is_find(exclude, p->name())) {
                    if (include.empty() || util::is_find(include, p->name())) {
                        SDL_TRACE("make: ", p->name());
                        outfile << generator::make_table(db, *p);
                        continue;
                    }
                }
                SDL_TRACE("exclude: ", p->name());
            }
            std::string s_end(FILE_END_TEMPLATE);
            replace(s_end, "%s{namespace}", 
                _namespace ? replace_(NS_END, "%s", _namespace) : std::string()
            );
            outfile << s_end;
            outfile.close();
            SDL_TRACE("File created : ", out_file);
            return true;
        }
    }
    return false;
}

bool generator::make_file(database & db, std::string const & out_file, const char * const _namespace)
{
    return make_file_ex(db, out_file, {}, {}, _namespace);
}

std::string util::remove_extension( std::string const& filename ) {
    auto pivot = std::find(filename.rbegin(), filename.rend(), '.');
    if (pivot == filename.rend()) {
        return filename;
    }
    return std::string(filename.begin(), pivot.base() - 1);
}

std::string util::extract_filename(const std::string & path, const bool remove_ext) {
    if (!path.empty()) {
        std::string basename(std::find_if( path.rbegin(), path.rend(), [](char ch) {
            return ch == '\\' || ch == '/'; 
        }).base(), path.end());
        return remove_ext ? remove_extension(basename) : basename;
    }
    return{};
}

vector_string util::split(const std::string & s, char const token) {
    SDL_ASSERT(token);
    if (!s.empty()) {
        vector_string result;
        size_t p1 = 0;
        size_t p2 = s.find(token);
        while (p2 != std::string::npos) {
            if (p2 > p1) {
                result.push_back(s.substr(p1, p2 - p1));
            }
            p2 = s.find(token, p1 = p2 + 1);
        }
        if (p1 < s.size()) {
            result.push_back(s.substr(p1));
        }
        return result;
    }
    return {};
}

bool util::is_find(vector_string const & v, const std::string & s) {
    for (auto const & name : v) {
        SDL_ASSERT(!name.empty());
        if (name == s) 
            return true;
    }
    return false;
}

} // make
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
                    SDL_ASSERT(make::util::split("").empty());
                    auto t1 = make::util::split("1 2 3");
                    auto t2 = make::util::split(" 1 2 3 ");
                    SDL_ASSERT(make::util::is_find(t2, "2"));
                    SDL_ASSERT(t1 == t2);
                    SDL_ASSERT(t1.size() == 3);
                    SDL_ASSERT(make::util::split("123").size() == 1);
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG