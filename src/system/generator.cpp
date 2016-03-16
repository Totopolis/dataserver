// generator.cpp
//
#include "common/common.h"
#include "generator.h"
#include "database.h"
#include "page_info.h"

namespace sdl { namespace db { namespace make {
namespace {

const char MAKE_TEMPLATE[] = R"(
struct dbo_%s{name}_META {
    struct col {%s{COL_TEMPLATE}
    };
    typedef TL::Seq<%s{TYPE_LIST}
    >::Type type_list;
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
        friend access;
        friend this_table;
        record(this_table const * p, row_head const * h): base(p, h) {}
    public:%s{REC_TEMPLATE}
    };
private:
    using query_type = make_query<this_table, record>;
    record::access _record;
public:
    using iterator = record::access::iterator;
    explicit dbo_%s{name}(database * p, shared_usertable const & s)
        : base_table(p), _record(this, p, s)
    {}
    iterator begin() { return _record.begin(); }
    iterator end() { return _record.end(); }
    query_type query{ this };
};
)";

const char TYPE_LIST[] = R"(
        col::%s{col_name} /*[%d]*/)";

const char KEY_TEMPLATE[] = R"(, meta::key<%s{PK}, %s{key_subid}, sortorder::%s{key_order}>)";

const char COL_TEMPLATE[] = R"(
        using %s{col_name} = meta::col<%s{col_off}, scalartype::t_%s{col_type}, %s{col_len}%s{KEY_TEMPLATE}>;)";

const char REC_TEMPLATE[] = R"(
        auto %s{col_name}() const -> col::%s{col_name}::ret_type { return val<col::%s{col_name}>(); })";

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

template<typename T>
std::string replace_(const char buf[], const char * const token, const T & value) {
    std::string s(buf);
    replace(s, token, value);
    return s;
};

} // namespace 

std::string generator::make(database & db, datatable const & table)
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

    SDL_WARNING(s.find("%s{") == std::string::npos);
    return s;
}

/*
void trace_user_tables(db::database & db, cmd_option const & opt)
{
    size_t index = 0;
    for (auto const & ut : db._usertables) {
        if (opt.tab_name.empty() || (ut->name() == opt.tab_name)) {
            std::cout << "\nUSER_TABLE[" << index << "]:\n";
            if (auto pk = db.get_PrimaryKey(ut->get_id())) {
                std::cout << ut->type_schema(pk.get());
            }
            else {
                std::cout << ut->type_schema();
            }
        }
        ++index;
    }
    std::cout << "\nUSER_TABLE COUNT = " << index << std::endl;
}
*/

} // make
} // db
} // sdl