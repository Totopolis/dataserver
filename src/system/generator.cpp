// generator.cpp
//
#include "common/common.h"
#include "generator.h"
#include "database.h"
#include "page_info.h"
#include <fstream>
#include <functional>

namespace sdl { namespace db { namespace make { namespace {

const char FILE_BEGIN_TEMPLATE[] = R"(//%s{out_file}
//%s{database}
//)" __DATE__ " " __TIME__
R"(
#ifndef __SDL_GENERATOR_%s{unique}_H__
#define __SDL_GENERATOR_%s{unique}_H__

#pragma once

#include "system/maketable.h"

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

const char MAKE_TEMPLATE[] = R"(
struct dbo_%s{name}_META {
    struct col {%s{COL_TEMPLATE}
    };
    typedef TL::Seq<%s{TYPE_LIST}
    >::Type type_list;%s{CLUSTER_INDEX}
    static const char * name() { return "%s{name}"; }
    static const int32 id = %s{schobj_id};
};

class dbo_%s{name} : public dbo_%s{name}_META, public make_base_table<dbo_%s{name}_META> {
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
        record() = default;
    public:%s{REC_TEMPLATE}
    };
public:
    using iterator = record::access::iterator;
    using query_type = record::query;
    explicit dbo_%s{name}(database * p, shared_usertable const & s)
        : base_table(p, s), _record(this, p, s) {}
    iterator begin() { return _record.begin(); }
    iterator end() { return _record.end(); }
    query_type query{ this };
    query_type * operator ->() { return &query; }
private:
    record::access _record;
};
)";

const char TYPE_LIST[] = R"(
        col::%s{col_name} /*[%d]*/)";

const char KEY_TEMPLATE[] = R"(, meta::key<%s{PK}, %s{key_subid}, sortorder::%s{key_order}>)";

const char COL_TEMPLATE[] = R"(
        struct %s{col_name} : meta::col<%s{col_off}, scalartype::t_%s{col_type}, %s{col_len}%s{KEY_TEMPLATE}> { static const char * name() { return "%s{col_name}"; } };)";

const char REC_TEMPLATE[] = R"(
        auto %s{col_name}() const -> col::%s{col_name}::ret_type { return val<col::%s{col_name}>(); })";

const char CLUSTER_INDEX[] = R"(
    struct cluster_META {%s{index_col}
        typedef TL::Seq<%s{type_list}>::Type type_list;
    };
    class cluster_index : public base_cluster<cluster_META> {
        using base = base_cluster<cluster_META>;
    public:
#pragma pack(push, 1)
        struct key_type {%s{index_val}
            template<size_t i>
            auto get() const -> decltype(base::get_col<i>(nullptr)) {
                return base::get_col<i>(this);
            }
            template<size_t i>
            auto set() -> decltype(base::set_col<i>(nullptr)) {
                return base::set_col<i>(this);
            }
        };
#pragma pack(pop)
        static const char * name() { return ""; }
        friend key_type;
    };)";

const char VOID_CLUSTER_INDEX[] = R"(
    using cluster_index = void;)";

const char CLUSTER_INDEX_COL[] = R"(
        using T%d = meta::index_col<col::%s{col_name}%s{offset}>;)";

const char CLUSTER_INDEX_VAL[] = R"(
            T%d::type _%d;)";

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

    std::string s_columns;
    std::string s_type_list;
    std::string s_record;

    const auto PK = db.get_PrimaryKey(tab.get_id());
    for (size_t i = 0; i < tab.size(); ++i) {
        usertable::column_ref t = tab[i];
        std::string s_col(COL_TEMPLATE);
        replace(s_col, "%s{col_name}", t.name);
        replace(s_col, "%s{col_off}", tab.offset(i));
        replace(s_col, "%s{col_type}", scalartype::get_name(t.type));
        replace(s_col, "%s{col_len}", t.length._16);
        std::string s_key;
        if (PK) {
            auto found = PK->find_colpar(t.colpar);
            if (found != PK->colpar.end()) {
                const size_t subid = found - PK->colpar.begin();
                s_key = replace_(KEY_TEMPLATE, "%s{PK}", "true");
                replace(s_key, "%s{key_subid}", subid);
                replace(s_key, "%s{key_order}", to_string::type_name(PK->order[subid]));
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

    if (auto key = table.get_cluster_index()) {
        std::string s_cluster(CLUSTER_INDEX);
        std::string s_index_col;
        std::string s_index_type;
        std::string s_index_val;
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
            if (i) s_index_type += ", ";
            s_index_type += replace_("T%d", "%d", i);
            s_index_val += replace_(CLUSTER_INDEX_VAL, "%d", i);
        }
        replace(s_cluster, "%s{index_col}", s_index_col);
        replace(s_cluster, "%s{type_list}", s_index_type);
        replace(s_cluster, "%s{index_val}", s_index_val);
        replace(s, "%s{CLUSTER_INDEX}", s_cluster);
    }
    else {
        replace(s, "%s{CLUSTER_INDEX}", VOID_CLUSTER_INDEX);
    }
    SDL_ASSERT(s.find("%s{") == std::string::npos);
    return s;
}

bool generator::make_file(database & db, std::string const & out_file, const char * const _namespace)
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
            outfile << s_begin;
            for (auto p : db._datatables) {
                outfile << generator::make_table(db, *p);
            }
            std::string s_end(FILE_END_TEMPLATE);
            replace(s_end, "%s{namespace}", 
                _namespace ? replace_(NS_END, "%s", _namespace) : std::string()
            );
            outfile << s_end;
            outfile.close();
            SDL_TRACE_2("File created : ", out_file);
            return true;
        }
    }
    return false;
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

} // make
} // db
} // sdl
