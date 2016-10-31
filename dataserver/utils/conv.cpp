// conv.cpp
//
#include "common/common.h"
#include "utils/conv.h"
#include "utils/encoding_utf.hpp"

#if defined(SDL_OS_UNIX)
#include <iconv.h>

namespace sdl { namespace db {

class h_iconv_t : noncopyable {
    iconv_t m_cd;
public:
    h_iconv_t(const char * tocode, const char * fromcode) {
        SDL_ASSERT(tocode && fromcode);
        m_cd = iconv_open(tocode, fromcode);
    }
    ~h_iconv_t(){
        if (is_open()) {
            iconv_close(m_cd);
        }
    }
    iconv_t handle() const {
        return m_cd;
    }
    bool is_open() const {
        return m_cd != (iconv_t)(-1);
    }
};

class make_iconv_t : noncopyable {
    using shared_iconv_t = std::shared_ptr<h_iconv_t>;
    shared_iconv_t m_current;
private:
    make_iconv_t() = default;
    shared_iconv_t make() {
        shared_iconv_t p = m_current;
        m_current.reset();
        if (!p.unique()) {
            reset_new(p, "UTF-8", "WINDOWS-1251");
        }
        return p;
    }
    void push(shared_iconv_t && p) {
        m_current = std::move(p);
    }
public:
    static make_iconv_t & instance() {
        static make_iconv_t obj;
        return obj;
    }
    class lock_iconv_t : noncopyable {
        make_iconv_t & parent;
        shared_iconv_t value;
    public:
        explicit lock_iconv_t(make_iconv_t & p): parent(p){
            value = parent.make();
            SDL_ASSERT(value.get());
        }
        ~lock_iconv_t(){
            parent.push(std::move(value));
        }
        bool is_open() const {
            return value->is_open();
        }
        iconv_t handle() const {
            return value->handle();
        }
    };
};

// http://man7.org/linux/man-pages/man3/iconv.3.html
std::string iconv_cp1251_to_utf8(const std::string & s)
{
    if (s.empty())
        return {};
 
    make_iconv_t::lock_iconv_t lock(make_iconv_t::instance());
    if (!lock.is_open()) {
        SDL_ASSERT(0);
        return {};
    }
    const char * inbuf = const_cast<char *>(s.data());
    size_t inbytesleft = s.size();
    SDL_ASSERT(*inbuf);

    const size_t bufsize = (s.size() + 1) << 1;
    std::unique_ptr<char[]> buf (new char[bufsize]);
    char * outbuf = buf.get();    
    size_t outbytesleft = bufsize;
    
    // iconv returns the number of characters converted in a nonreversible way during this call
    const auto ret = iconv(lock.handle(),
        (char **)&inbuf,    // the address of a variable that points to the first character of the input sequence
        &inbytesleft,       // indicates the number of bytes in that buffer
        &outbuf,            // the address of a variable that points to the first byte available in the output buffer
        &outbytesleft);     // indicates the number of bytes available in the output buffer
    A_STATIC_CHECK_TYPE(const size_t, ret);

    if (ret != (size_t)(-1)) {
        SDL_ASSERT(ret <= s.size());
        SDL_ASSERT(outbytesleft <= bufsize);
        if (outbytesleft < bufsize) {
            std::string result(buf.get(), bufsize - outbytesleft);
            return result;
        }
    }
    SDL_ASSERT(0);
    return{};
}

} // db
} // sdl
#endif // #if defined(SDL_OS_UNIX)

namespace sdl { namespace db { namespace {

static std::atomic<bool> default_method_stop{false};

inline locale::conv::method_type locale_method() {
    static_assert(locale::conv::method_type::skip == locale::conv::method_type::default_method, "");
    return default_method_stop ? 
        locale::conv::method_type::stop : 
        locale::conv::method_type::skip;
}

template <class string_type>
string_type nchar_to_string(vector_mem_range_t const & data)
{
    static_assert(sizeof(nchar_t) == 2, "");
    const size_t len = mem_size(data);
    if (len && !is_odd(len)) {
        const std::vector<char> src = make_vector(data);
        SDL_ASSERT(src.size() == len);
        using CharIn = uint16;
        static_assert(sizeof(nchar_t) == sizeof(CharIn), "");
        const CharIn * const begin = reinterpret_cast<const CharIn *>(src.data());
        const CharIn * const end = begin + (src.size() / sizeof(CharIn));
        return sdl::locale::conv::utf_to_utf<typename string_type::value_type, CharIn>(begin, end, locale_method());
    }
    SDL_ASSERT(!len);
    return{};
}

} // namespace

bool conv::method_stop()
{
    return default_method_stop;
}

void conv::method_stop(bool b)
{
    default_method_stop = b;
}

std::wstring conv::cp1251_to_wide(std::string const & s)
{
    std::wstring w(s.size(), L'\0');
    if (std::mbstowcs(&w[0], s.c_str(), w.size()) == s.size()) {
        return w;
    }
    SDL_ASSERT(!"cp1251_to_wide");
    return{};
}

#if defined(SDL_OS_UNIX)
std::string conv::cp1251_to_utf8(std::string const & s)
{
    return iconv_cp1251_to_utf8(s); // to be tested
}
#else
std::string conv::cp1251_to_utf8(std::string const & s)
{
    A_STATIC_ASSERT_TYPE(char, std::string::value_type);
    std::wstring w(s.size(), L'\0');
    if (std::mbstowcs(&w[0], s.c_str(), w.size()) == s.size()) {
        return sdl::locale::conv::utf_to_utf<std::string::value_type>(w, locale_method());
    }
    SDL_ASSERT(!"cp1251_to_utf8");
    return {};
}
#endif

std::wstring conv::utf8_to_wide(std::string const & s)
{
    A_STATIC_ASSERT_TYPE(wchar_t, std::wstring::value_type); // sizeof(wchar_t) can be 2 or 4 bytes
    return sdl::locale::conv::utf_to_utf<std::wstring::value_type>(s, locale_method());
}

std::string conv::wide_to_utf8(std::wstring const & s)
{
    return sdl::locale::conv::utf_to_utf<std::string::value_type>(s, locale_method());
}

std::string conv::nchar_to_utf8(vector_mem_range_t const & data)
{
    return nchar_to_string<std::string>(data);
}

std::wstring conv::nchar_to_wide(vector_mem_range_t const & data)
{
    return nchar_to_string<std::wstring>(data);
}

} // db
} // sdl

#if SDL_DEBUG
namespace sdl { namespace db { namespace {
    class unit_test {
    public:
        unit_test() {
            const auto old = conv::method_stop();
            conv::method_stop(true);
            SDL_ASSERT(!conv::cp1251_to_utf8("cp1251_to_utf8").empty());
            SDL_ASSERT(conv::cp1251_to_utf8("").empty());
            conv::method_stop(old);
        }
    };
    static unit_test s_test;
}}} // sdl::db
#endif //#if SV_DEBUG
