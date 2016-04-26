// maketable.hpp
//
#pragma once
#ifndef __SDL_SYSTEM_MAKETABLE_HPP__
#define __SDL_SYSTEM_MAKETABLE_HPP__

namespace sdl { namespace db { namespace make {

template<class this_table, class record>
record make_query<this_table, record>::find_with_index(key_type const & key) {
    if (m_cluster) {
        auto const db = m_table.get_db();
        if (auto const id = make::index_tree<key_type>(db, m_cluster).find_page(key)) {
            if (page_head const * const h = db->load_page_head(id)) {
                SDL_ASSERT(h->is_data());
                const datapage data(h);
                if (!data.empty()) {
                    size_t const slot = data.lower_bound(
                        [this, &key](row_head const * const row, size_t) {
                        return (this->read_key(row) < key);
                    });
                    if (slot < data.size()) {
                        row_head const * const head = data[slot];
                        if (!(key < read_key(head))) {
                            return record(&m_table, head);
                        }
                    }
                }
            }
        }
    }
    return {};
}

#if maketable_select_old_code
template<class this_table, class record>
typename make_query<this_table, record>::record_range
make_query<this_table, record>::select(select_key_list in, ignore_index, unique_false) {
    record_range result;
    if (in.size()) {
        result.reserve(in.size());
        for (auto p : m_table) { // scan all table
            A_STATIC_CHECK_TYPE(record, p);
            auto const key = make_query::read_key(p);
            for (auto const & k : in) {
                if (key == k) {
                    result.push_back(p);
                }
            }
        }
    }
    return result;              
}

template<class this_table, class record>
typename make_query<this_table, record>::record_range
make_query<this_table, record>::select(select_key_list in, ignore_index, unique_true) {
    record_range result;
    if (in.size()) {
        result.reserve(in.size());
        std::vector<const key_type *> look(in.size());
        for (size_t i = 0; i < in.size(); ++i) {
            look[i] = in.begin() + i;
        }
        size_t size = in.size();
        for (auto p : m_table) { // scan table and filter found keys
            A_STATIC_CHECK_TYPE(record, p);
            auto const key = make_query::read_key(p);
            for (size_t i = 0; i < size; ++i) {
                if (key == *look[i]) {
                    result.push_back(p);
                    if (!(--size))
                        return result;
                    look[i] = look[size];
                }
            }
        }
    }
    return result; // not all keys found
}

template<class this_table, class record>
typename make_query<this_table, record>::record_range
make_query<this_table, record>::select(select_key_list in, use_index, unique_true) {
    record_range result;
    if (in.size()) {
        result.reserve(in.size());
        for (auto const & key : in) {
            if (auto p = find_with_index(key)) {
                result.push_back(p);
            }
        }
    }
    return result;
}

/*template<class this_table, class record>
typename make_query<this_table, record>::record_range
make_query<this_table, record>::select(select_key_list in, enum_index const v1, enum_unique const v2) {
    if (enum_index::ignore_index == v1) {
        if (enum_unique::unique_false == v2) {
            return select(in, enum_index_t<ignore_index>(), enum_unique_t<unique_false>());
        }
        SDL_ASSERT(enum_unique::unique_true == v2);
        return select(in, enum_index_t<ignore_index>(), enum_unique_t<unique_true>());
    }
    SDL_ASSERT(enum_index::use_index == v1);
    SDL_ASSERT(enum_unique::unique_true == v2);
    return select(in, enum_index_t<use_index>(), enum_unique_t<unique_true>());
}*/

/*template<typename T, typename... Ts> 
void select_n(where<T> col, Ts const & ... params) {
    enum { col_found = TL::IndexOf<typename this_table::type_list, T>::value };
    enum { key_found = meta::cluster_col_find<KEY_TYPE_LIST, T>::value };
    static_assert(col_found != -1, "");
    using type_list = typename TL::Seq<T, Ts...>::Type; // test
    static_assert(TL::Length<type_list>::value == sizeof...(params) + 1, "");
    //SDL_TRACE(typeid(type_list).name());
    SDL_ASSERT(where<T>::name() == T::name()); // same memory
    SDL_TRACE(
        "col:", col_found, 
        " key:", key_found, 
        " name:", T::name(),
        " value:", col.value);
    select_n(params...);
}*/
#endif

#if 0
template<class this_table, class record>
struct make_query<this_table, record>::sub_expr_fun
{
    template<class T> // T = where_::SEARCH
    void operator()(identity<T>) {
        if (0) {
            SDL_TRACE(
                T::col::name(), " INDEX::", where_::index_name<T::hint>(), " ",
                T::col::PK ? "PK" : "");
        }
        static_assert((T::hint != where_::INDEX::USE) || T::col::PK, "INDEX::USE");
    }
    template<class T, sortorder ord>
    void operator()(identity<where_::ORDER_BY<T, ord>>) {
    }
    template<class F>
    void operator()(identity<where_::SELECT_IF<F>>) {
    }
};
#endif

namespace make_query_ {

//--------------------------------------------------------------
#if maketable_reverse_order
template <class TList, size_t Count> struct reserse_order;

template <size_t Count>
struct reserse_order<NullType, Count>
{
    using Result = NullType;
};

template <size_t i, class Tail, size_t Count>
struct reserse_order<Typelist<Int2Type<i>, Tail>, Count> {
private:
    using reverse_i = Typelist<Int2Type<Count-1-i>, NullType>;
    static_assert(i < Count, "reserse_order");
public:
    using Result = typename TL::Append<reverse_i, typename reserse_order<Tail, Count>::Result>::Result;
};
#endif
//--------------------------------------------------------------

template<class T, where_::condition cond = T::cond>
struct use_index {
    static_assert((T::hint != where_::INDEX::USE) || T::col::PK, "INDEX::USE");
    enum { value = T::col::PK && (T::hint != where_::INDEX::IGNORE) };
};

template<class T>
struct use_index<T, where_::condition::_lambda> {
    enum { value = false };
};

template<class T>
struct use_index<T, where_::condition::_order> {
    enum { value = false };
};

//--------------------------------------------------------------

template <class TList, class OList, size_t> struct search_use_index;
template <size_t i> struct search_use_index<NullType, NullType, i>
{
    using Index = NullType;
    using Types = NullType;
    using OList = NullType;
};

template <class Head, class Tail, where_::operator_ OP, class NextOP, size_t i>
struct search_use_index<Typelist<Head, Tail>, where_::operator_list<OP, NextOP>, i> {
private:
    enum { flag = use_index<Head>::value };
    using indx_i = typename Select<flag, Typelist<Int2Type<i>, NullType>, NullType>::Result;
    using type_i = typename Select<flag, Typelist<Head, NullType>, NullType>::Result;
    using oper_i = typename Select<flag, Typelist<where_::operator_t<OP>, NullType>, NullType>::Result;
public:
    using Types = typename TL::Append<type_i, typename search_use_index<Tail, NextOP, i + 1>::Types>::Result; 
    using Index = typename TL::Append<indx_i, typename search_use_index<Tail, NextOP, i + 1>::Index>::Result;
    using OList = typename TL::Append<oper_i, typename search_use_index<Tail, NextOP, i + 1>::OList>::Result;
};

//--------------------------------------------------------------

template<class TList> struct process_push_back;
template<> struct process_push_back<NullType>
{
    template<class T>
    static void push_back(T &){}
};

template<size_t i, class Tail> 
struct process_push_back<Typelist<Int2Type<i>, Tail>>
{
    template<class T>    
    static void push_back(T & dest){
        A_STATIC_ASSERT_TYPE(size_t, typename T::value_type);
        dest.push_back(i);
        process_push_back<Tail>::push_back(dest);
    }
};

template<class TList, class T> 
inline void push_back(T & dest) {
    dest.reserve(TL::Length<TList>::value);
    process_push_back<TList>::push_back(dest);
}

//--------------------------------------------------------------

template<size_t i, class T, where_::operator_ op>
struct search_key
{
    enum { pos = i };
    using type = T;
    static const where_::operator_ OP = op;
};

template<class Index, class TList, class OList> struct make_search_list;
template<> struct make_search_list<NullType, NullType, NullType>
{
    using Result = NullType;
};

template<
    size_t i, class NextIndex, 
    class T, class NextType,
    where_::operator_ OP,  class NextOP>
struct make_search_list<
    Typelist<Int2Type<i>, NextIndex>,
    Typelist<T, NextType>,
    Typelist<where_::operator_t<OP>, NextOP>>
{
private:
    using Item = Typelist<search_key<i, T, OP>,  NullType>;
    using Next = typename make_search_list<NextIndex, NextType, NextOP>::Result;
public:
    using Result = typename TL::Append<Item, Next>::Result;
};

template<class sub_expr_type>
struct SEARCH_USE_INDEX {
private:
    using use_index = make_query_::search_use_index<
                                        typename sub_expr_type::type_list, 
                                        typename sub_expr_type::oper_list,
                                        0>;
    using Index = typename use_index::Index;
    using Types = typename use_index::Types;
    using OList = typename use_index::OList;     
public:
    using Result = typename make_query_::make_search_list<Index, Types, OList>::Result;

    static_assert(TL::Length<Index>::value == TL::Length<Types>::value, "");
    static_assert(TL::Length<Index>::value == TL::Length<OList>::value, "");
    static_assert(TL::Length<Result>::value == TL::Length<OList>::value, "");
};

//--------------------------------------------------------------

using where_::condition;
using where_::condition_t;

template <class T> // T = search_key
struct SELECT_RECORD_WITH_INDEX : is_static
{
private:
    template<class value_type>
    static void select_cond(value_type const * expr, condition_t<condition::WHERE>) {
        SDL_ASSERT(!expr->value.values.empty());
    }
    template<class value_type>
    static void select_cond(value_type const * expr, condition_t<condition::IN>) {
        SDL_ASSERT(!expr->value.values.empty());
    }
    template<class value_type>
    static void select_cond(value_type const * expr, condition_t<condition::NOT>) {
        SDL_ASSERT(!expr->value.values.empty());
    }
    template<class value_type>
    static void select_cond(value_type const * expr, condition_t<condition::LESS>) {
        SDL_ASSERT(!expr->value.values.empty());
    }
    template<class value_type>
    static void select_cond(value_type const * expr, condition_t<condition::GREATER>) {
        SDL_ASSERT(!expr->value.values.empty());
    }
    template<class value_type>
    static void select_cond(value_type const * expr, condition_t<condition::LESS_EQ>) {
        SDL_ASSERT(!expr->value.values.empty());
    }
    template<class value_type>
    static void select_cond(value_type const * expr, condition_t<condition::GREATER_EQ>) {
        SDL_ASSERT(!expr->value.values.empty());
    }
    template<class value_type>
    static void select_cond(value_type const * expr, condition_t<condition::BETWEEN>) {
        SDL_ASSERT(!expr->value.values.empty());
    }
public:
    template<class sub_expr_type>
    static void select(sub_expr_type const & expr) {
        if (0) {
            SDL_TRACE("[", T::pos, "] = ",
                where_::operator_name<T::OP>(), " ",
                where_::condition_name<T::type::cond>()
                );
        }
        auto value = expr.get(Size2Type<T::pos>());
        SELECT_RECORD_WITH_INDEX::select_cond(value, condition_t<T::type::cond>());
    }
};

template <class search_list> struct SELECT_WITH_INDEX;
template <> struct SELECT_WITH_INDEX<NullType>
{
    template<class sub_expr_type> static void select(sub_expr_type const & ) {}
};

template<class T, class NextType> // T = search_key
struct SELECT_WITH_INDEX<Typelist<T, NextType>> {
public:
    template<class sub_expr_type>
    static void select(sub_expr_type const & expr) {
        SELECT_RECORD_WITH_INDEX<T>::select(expr);
        SELECT_WITH_INDEX<NextType>::select(expr);
    }
};

//--------------------------------------------------------------

} // make_query_

template<class this_table, class record>
template<class sub_expr_type>
typename make_query<this_table, record>::record_range
make_query<this_table, record>::VALUES(sub_expr_type const & expr)
{
    SDL_TRACE("\nVALUES:");
    if (1) {
        where_::trace_::trace_sub_expr(expr);
    }
#if maketable_reverse_order
    using use_index = make_query_::search_use_index<
                                        typename sub_expr_type::reverse_type_list, 
                                        typename sub_expr_type::reverse_oper_list,
                                        0>;
    using Index = typename make_query_::reserse_order<typename use_index::Index, sub_expr_type::type_size>::Result;
    using Types = typename use_index::Types;
    using OList = typename use_index::OList;   
    using SL = typename make_query_::make_search_list<Index, Types, OList>::Result;
#else
    /*using use_index = make_query_::search_use_index<
                                        typename sub_expr_type::type_list, 
                                        typename sub_expr_type::oper_list,
                                        0>;
    using Index = typename use_index::Index;
    using Types = typename use_index::Types;
    using OList = typename use_index::OList;   
    using SL = typename make_query_::make_search_list<Index, Types, OList>::Result;*/
#endif
    using SL = typename make_query_::SEARCH_USE_INDEX<sub_expr_type>::Result;
    meta::trace_typelist<SL>();
    make_query_::SELECT_WITH_INDEX<SL>::select(expr);
    return {};
}

//std::vector<size_t> test;
//make_query_::push_back<Index>(test);

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_MAKETABLE_HPP__
