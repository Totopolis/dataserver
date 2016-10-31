// conv_unix.cpp
//
#if defined(SDL_OS_UNIX)

#include "common/common.h"
#include "conv_unix.h"
#include <iconv.h>
#include <errno.h>

namespace sdl { namespace db { namespace unix {

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
        shared_iconv_t p = m_current; m_current.reset();
        if (!p.unique()) { // empty pointers are never unique 
            reset_new(p, "UTF-8", "WINDOWS-1251");
        }
        SDL_ASSERT(p.unique());
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

    for (size_t i = 1; i < 3; ++i) {

        const char * inbuf = s.data();
        size_t inbytesleft = s.size();
        SDL_ASSERT(*inbuf);

        const size_t bufsize = (s.size() + 1) << i; // * 2, * 4
        std::unique_ptr<char[]> buf (new char[bufsize]);
        char * outbuf = buf.get();    
        size_t outbytesleft = bufsize;
    
        // iconv returns the number of characters converted in a nonreversible way during this call
        const auto ret = iconv(lock.handle(),
            (char **)(&inbuf),  // the address of a variable that points to the first character of the input sequence
            &inbytesleft,       // indicates the number of bytes in that buffer
            &outbuf,            // the address of a variable that points to the first byte available in the output buffer
            &outbytesleft);     // indicates the number of bytes available in the output buffer
        A_STATIC_CHECK_TYPE(const size_t, ret);

        if (ret != (size_t)(-1)) {
            SDL_ASSERT(ret <= s.size());
            SDL_ASSERT(outbytesleft <= bufsize);
            if (outbytesleft < bufsize) {
                return std::string(buf.get(), bufsize - outbytesleft);
            }
            return{};
        }
        if (errno != E2BIG) {
            SDL_ASSERT(0);
            break;
        }
    }
    SDL_ASSERT(0);
    return{};
}

} // unix
} // db
} // sdl

#endif // #if defined(SDL_OS_UNIX)