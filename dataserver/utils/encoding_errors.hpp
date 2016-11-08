// encoding_errors.hpp
//
#pragma once
#ifndef __SDL_UTILS_ENCODING_ERRORS_HPP__
#define __SDL_UTILS_ENCODING_ERRORS_HPP__

#include "dataserver/common/static.h"

namespace sdl {
    namespace locale {
        namespace conv {
            ///
            /// \addtogroup codepage 
            ///
            /// @{

            ///
            /// \brief The excepton that is thrown in case of conversion error
            ///
            class conversion_error : public sdl_exception {
            public:
                conversion_error() : sdl_exception("Conversion failed") {}
            };
            
            ///
            /// \brief This exception is thrown in case of use of unsupported
            /// or invalid character set
            ///
            class invalid_charset_error : public sdl_exception {
            public:

                /// Create an error for charset \a charset
                invalid_charset_error(std::string const & charset) : 
                    sdl_exception("Invalid or unsupported charset:" + charset)
                {
                }
            };
            

            ///
            /// enum that defines conversion policy
            ///
            typedef enum {
                skip            = 0,    ///< Skip illegal/unconvertable characters
                stop            = 1,    ///< Stop conversion and throw conversion_error
                default_method  = skip  ///< Default method - skip
            } method_type;


            /// @}

        } // conv

    } // locale
} // sdl

#endif // #ifndef __SDL_UTILS_ENCODING_ERRORS_HPP__

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4

