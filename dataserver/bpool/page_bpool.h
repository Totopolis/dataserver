// page_bpool.h
//
#pragma once
#ifndef __SDL_BPOOL_PAGE_BPOOL_H__
#define __SDL_BPOOL_PAGE_BPOOL_H__

#include "dataserver/bpool/block_head.h"
#include "dataserver/bpool/file.h"

namespace sdl { namespace db { namespace bpool {

class base_page_bpool {
protected:
    explicit base_page_bpool(const std::string & fname);
    ~base_page_bpool();
public:
    static bool valid_filesize(size_t);
protected:
    PagePoolFile m_file;
};

class page_bpool final : public base_page_bpool {
    sdl_noncopyable(page_bpool)
    using block_head_vector = std::vector<block_head>;
public:
    const size_t min_pool_size;
    const size_t max_pool_size;
    page_bpool(const std::string & fname, size_t, size_t);
    ~page_bpool();
private:
    std::mutex m_mutex;
    block_head_vector m_block;
};

}}} // sdl

#endif // __SDL_BPOOL_PAGE_BPOOL_H__
