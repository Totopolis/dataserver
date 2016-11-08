// usertable.cpp
//
#include "dataserver/common/common.h"
#include "dataserver/system/primary_key.h" // includes "usertable.h"
#include "dataserver/system/page_info.h"

namespace sdl { namespace db {

usertable::column::column(syscolpars_row const * const p, sysscalartypes_row const * const s)
    : colpar(p)
    , scalar(s)
    , name(col_name_t(p))
    , type(s->data.id)
    , length(p->data.length)
{
    SDL_ASSERT(colpar && scalar);    
    SDL_ASSERT(colpar->data.utype == scalar->data.id);
    SDL_ASSERT(this->type != scalartype::t_none);
    SDL_ASSERT(!this->name.empty());
}

//----------------------------------------------------------------------------

usertable::usertable(sysschobjs_row const * const p, columns && c, primary_key const * const PK)
    : schobj(p)
    , m_name(col_name_t(p))
    , m_schema(std::move(c))
{
    SDL_ASSERT(schobj);
    SDL_ASSERT(schobj->is_USER_TABLE() || schobj->is_INTERNAL_TABLE());

    SDL_ASSERT(!m_name.empty());
    SDL_ASSERT(!m_schema.empty());
    SDL_ASSERT(get_id()._32);

    init_offset(PK);
}

namespace {
    template<typename T>
    bool is_unique(std::vector<T> const & vec) {
        std::vector<char> flag(vec.size());
        for (auto x : vec) {
            SDL_ASSERT(x < vec.size());
            if (flag[x]++) {
                return false;
            }
        }
        return true;
    }
}

void usertable::init_offset(primary_key const * const PK)
{
    const size_t schema_size = m_schema.size();
    m_offset.resize(schema_size);
    m_place.resize(schema_size);
    if (PK) {
        std::vector<size_t> keyord(schema_size);
        std::vector<uint8> is_key(schema_size);
        size_t key_size = 0;
        for (size_t i = 0; i < schema_size; ++i) {
            auto found = PK->find_colpar(m_schema[i]->colpar);
            if (found != PK->colpar.end()) {
                SDL_ASSERT(m_schema[i]->is_fixed()); //FIXME: support only fixed columns as key
                const size_t j = found - PK->colpar.begin();
                SDL_ASSERT(j < m_schema.size());
                throw_error_if<usertable_error>(j >= schema_size, "bad primary_key");
                keyord[j] = i + 1;
                is_key[i] = 1;
                ++key_size;
            }
        }
        SDL_ASSERT(keyord[0] && key_size); // expect non-empty primary_key
        // 1) process keys in order
        size_t offset = 0;
        size_t var_index = 0;
        for (size_t place = 0; place < schema_size; ++place) {
            if (size_t const j = keyord[place]) {
                const size_t i = j - 1;
                if (m_schema[i]->is_fixed()) {
                    m_offset[i] = offset;
                    offset += m_schema[i]->fixed_size();
                }
                else {
                    SDL_ASSERT(!"primary key is variable");
                    m_offset[i] = var_index++;
                }
                m_place[i] = place;
                SDL_ASSERT(is_key[i]);
            }
        }
        // 1) process non-keys
        size_t place = key_size;
        for (size_t i = 0; i < schema_size; ++i) {
            if (!is_key[i]) {
                if (m_schema[i]->is_fixed()) {
                    m_offset[i] = offset;
                    offset += m_schema[i]->fixed_size();
                }
                else {
                    m_offset[i] = var_index++;
                }
                SDL_ASSERT(!m_place[i]);
                m_place[i] = place++;
            }
        }
    }
    else {
        size_t offset = 0;
        size_t var_index = 0;
        for (size_t i = 0; i < schema_size; ++i) {
            if (m_schema[i]->is_fixed()) {
                m_offset[i] = offset;
                offset += m_schema[i]->fixed_size();
            }
            else {
                m_offset[i] = var_index++;
            }
            m_place[i] = i;
        }
    }
    SDL_ASSERT(is_unique(m_place));
}

size_t usertable::count_var() const
{
    return count_if([](column_ref c){
        return !c.is_fixed();
    });
}

size_t usertable::count_fixed() const
{
    return count_if([](column_ref c){
        return c.is_fixed();
    });
}

size_t usertable::fixed_size() const
{
    size_t ret = 0;
    for_col([&ret](column_ref c){
        if (c.is_fixed()) {
            ret += c.fixed_size();
        }
    });
    return ret;
}

std::string usertable::column::type_schema(primary_key const * const PK) const
{
    column_ref d = *this;
    std::stringstream ss;
    ss << "[" << d.colpar->data.colid._32 << "] ";
    ss << d.name << " : " << scalartype::get_name(d.type) << " (";
    if (d.length.is_var())
        ss << "var";
    else 
        ss << d.length._16;
    ss << ")";
    if (d.is_fixed()) {
        ss << " fixed";
    }
    if (PK) {
        auto found = PK->find_colpar(d.colpar);
        if (found != PK->colpar.end()) {
            if (found == PK->colpar.begin()) {
                ss << " PRIMARY_KEY";
            }
            else {
                ss << " INDEX_KEY";
            }
            ss << " [idxstat =";
            if (PK->idxstat->is_clustered()) { ss << " is_clustered"; }
            if (PK->idxstat->IsPrimaryKey()) { ss << " IsPrimaryKey"; }
            if (PK->idxstat->IsUnique()) { ss << " IsUnique"; }
            ss << "]";
            if (1) {
                const size_t i = found - PK->colpar.begin();
                ss << " [key_pos = " << i;
                ss << " order = " << to_string::type_name(PK->order[i]) << "]";
            }
        }
    }
    return ss.str();
}

std::string usertable::type_schema(primary_key const * const PK) const
{
    usertable const & ut = *this;
    std::stringstream ss;
    ss  << "name = " << ut.m_name
        << "\nid = " << ut.get_id()
        << std::uppercase << std::hex 
        << " (" << ut.get_id() << ")"
        << std::dec
        << "\nColumns(" << ut.m_schema.size() << ")"
        << "\n";
    size_t i = 0;
    for (auto & p : ut.m_schema) {
        column_ref col = (*this)[i];
        ss << p->type_schema(PK);
        ss << (col.is_fixed() ? " [off = " : " [var = ") << offset(i) << "]";
        ss << " [place = " << place(i) << "]\n";
        ++i;
    }
    return ss.str();
}

usertable::col_index
usertable::find_col(syscolpars_row const * const p) const
{
    SDL_ASSERT(p);
    const size_t i = find_if([p](column_ref c) {
        return c.colpar == p;
    });
    if (i < size()) {
        return { m_schema[i].get(), i};
    }
    SDL_ASSERT(0); // to be tested
    return {};
}

size_t usertable::find_geography() const 
{
    return find_if([](usertable::column const & c){
        return c.type == scalartype::t_geography;
    });
}

} // db
} // sdl

