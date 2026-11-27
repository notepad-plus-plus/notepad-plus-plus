//Copyright (c) 2006-2010 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_EXCEPTION_C3E1741C754311DDB2834CCA55D89593
#define BOOST_EXCEPTION_C3E1741C754311DDB2834CCA55D89593

#include <boost/config.hpp>
#include <boost/core/typeinfo.hpp>
#include <boost/core/demangle.hpp>
#include <boost/current_function.hpp>
#include <string>
#include <ostream>
#include <cstring>
#include <cstddef>

#ifndef BOOST_EXCEPTION_PRETTY_FUNCTION
#   if defined(_MSC_VER) && !defined(__clang__) && !defined(__GNUC__)
#       define BOOST_EXCEPTION_PRETTY_FUNCTION __FUNCSIG__
#   else
#       define BOOST_EXCEPTION_PRETTY_FUNCTION __PRETTY_FUNCTION__
#   endif
#endif

#ifndef BOOST_EXCEPTION_ENABLE_WARNINGS
#if defined(__GNUC__) && __GNUC__*100+__GNUC_MINOR__>301
#pragma GCC system_header
#endif
#ifdef __clang__
#pragma clang system_header
#endif
#ifdef _MSC_VER
#pragma warning(push,1)
#endif
#endif

namespace
boost
    {
    template <class T>
    inline
    std::string
    tag_type_name()
        {
#ifdef BOOST_NO_TYPEID
        return BOOST_CURRENT_FUNCTION;
#else
        return core::demangle(typeid(T*).name());
#endif
        }

    template <class T>
    inline
    std::string
    type_name()
        {
#ifdef BOOST_NO_TYPEID
        return BOOST_CURRENT_FUNCTION;
#else
        return core::demangle(typeid(T).name());
#endif
        }

    namespace
    exception_detail
        {
        struct
        type_info_
            {
            core::typeinfo const * type_;

            explicit
            type_info_( core::typeinfo const & type ):
                type_(&type)
                {
                }

            friend
            bool
            operator<( type_info_ const & a, type_info_ const & b )
                {
                return a.type_!=b.type_ && strcmp(a.type_->name(), b.type_->name()) < 0;
                }
            };

        template <int S1, int S2, int I, bool = (S1 >= S2)>
        struct
        cpp11_prefix
            {
            BOOST_FORCEINLINE static constexpr
            bool
            check(char const (&)[S1], char const (&)[S2]) noexcept
                {
                return false;
                }
            };

        template <int S1, int S2, int I>
        struct
        cpp11_prefix<S1, S2, I, true>
            {
            BOOST_FORCEINLINE static constexpr
            bool
            check(char const (&str)[S1], char const (&prefix)[S2]) noexcept
                {
                return str[I] == prefix[I] && cpp11_prefix<S1, S2, I - 1>::check(str, prefix);
                }
            };

        template <int S1, int S2>
        struct
        cpp11_prefix<S1, S2, 0, true>
            {
            BOOST_FORCEINLINE static constexpr
            bool
            check(char const (&str)[S1], char const (&prefix)[S2]) noexcept
                {
                return str[0] == prefix[0];
                }
            };

        template <int S1, int S2>
        BOOST_FORCEINLINE constexpr
        int
        check_prefix(char const (&str)[S1], char const (&prefix)[S2]) noexcept
            {
            return cpp11_prefix<S1, S2, S2 - 2>::check(str, prefix) ? S2 - 1 : 0;
            }

        template <int S1, int S2, int I1, int I2, bool = (S1 >= S2)>
        struct
        cpp11_suffix
            {
            BOOST_FORCEINLINE static constexpr
            bool
            check(char const (&)[S1], char const (&)[S2]) noexcept
                {
                return false;
                }
            };

        template <int S1, int S2, int I1, int I2>
        struct
        cpp11_suffix<S1, S2, I1, I2, true>
            {
            BOOST_FORCEINLINE static constexpr
            bool
            check(char const (&str)[S1], char const (&suffix)[S2]) noexcept
                {
                return str[I1] == suffix[I2] && cpp11_suffix<S1, S2, I1 - 1, I2 - 1>::check(str, suffix);
                }
            };

        template <int S1, int S2, int I1>
        struct
        cpp11_suffix<S1, S2, I1, 0, true>
            {
            BOOST_FORCEINLINE static constexpr
            bool
            check(char const (&str)[S1], char const (&suffix)[S2]) noexcept
                {
                return str[I1] == suffix[0];
                }
            };

        template <int S1, int S2>
        BOOST_FORCEINLINE constexpr
        int
        check_suffix(char const (&str)[S1], char const (&suffix)[S2]) noexcept
            {
            return cpp11_suffix<S1, S2, S1 - 2, S2 - 2>::check(str, suffix) ? S1 - S2 : 0;
            }

        }

    namespace
    n
        {
        struct
        r
            {
            char const * name_not_zero_terminated_at_length;
            std::size_t length;
            };

#ifdef _MSC_VER
#   define BOOST_EXCEPTION_CDECL __cdecl
#else
#   define BOOST_EXCEPTION_CDECL
#endif

        template <class T>
        BOOST_FORCEINLINE
        r
        BOOST_EXCEPTION_CDECL
        p()
            {
#define BOOST_EXCEPTION_P(P) (sizeof(char[1 + exception_detail::check_prefix(BOOST_EXCEPTION_PRETTY_FUNCTION, P)]) - 1)
            // clang style:
            std::size_t const p01 = BOOST_EXCEPTION_P("r boost::n::p() [T = ");
            std::size_t const p02 = BOOST_EXCEPTION_P("r __cdecl boost::n::p(void) [T = ");
            // old clang style:
            std::size_t const p03 = BOOST_EXCEPTION_P("boost::n::r boost::n::p() [T = ");
            std::size_t const p04 = BOOST_EXCEPTION_P("boost::n::r __cdecl boost::n::p(void) [T = ");
            // gcc style:
            std::size_t const p05 = BOOST_EXCEPTION_P("boost::n::r boost::n::p() [with T = ");
            std::size_t const p06 = BOOST_EXCEPTION_P("boost::n::r __cdecl boost::n::p() [with T = ");
            // msvc style, struct:
            std::size_t const p07 = BOOST_EXCEPTION_P("struct boost::n::r __cdecl boost::n::p<struct ");
            // msvc style, class:
            std::size_t const p08 = BOOST_EXCEPTION_P("struct boost::n::r __cdecl boost::n::p<class ");
            // msvc style, enum:
            std::size_t const p09 = BOOST_EXCEPTION_P("struct boost::n::r __cdecl boost::n::p<enum ");
            // msvc style, built-in type:
            std::size_t const p10 = BOOST_EXCEPTION_P("struct boost::n::r __cdecl boost::n::p<");
#undef BOOST_EXCEPTION_P

#define BOOST_EXCEPTION_S(S) (sizeof(char[1 + exception_detail::check_suffix(BOOST_EXCEPTION_PRETTY_FUNCTION, S)]) - 1)
            // clang/gcc style:
            std::size_t const s01 = BOOST_EXCEPTION_S("]");
            // msvc style:
            std::size_t const s02 = BOOST_EXCEPTION_S(">(void)");
#undef BOOST_EXCEPTION_S

            char static_assert_unrecognized_pretty_function_format_please_file_github_issue[sizeof(
                char[
                    (s01 && (1 == (!!p01 + !!p02 + !!p03 + !!p04 + !!p05 + !!p06)))
                    ||
                    (s02 && (1 == (!!p07 + !!p08 + !!p09)))
                    ||
                    (s02 && !!p10)
                ]
            ) * 2 - 1];
            (void) static_assert_unrecognized_pretty_function_format_please_file_github_issue;

            if( std::size_t const p = sizeof(char[1 + !!s01 * (p01 + p02 + p03 + p04 + p05 + p06)]) - 1 )
                return { &BOOST_EXCEPTION_PRETTY_FUNCTION[p], s01 - p };

            if( std::size_t const p = sizeof(char[1 + !!s02 * (p07 + p08 + p09)]) - 1 )
                return { &BOOST_EXCEPTION_PRETTY_FUNCTION[p], s02 - p };

            std::size_t const p = sizeof(char[1 + !!s02 * p10]) - 1;
            return { &BOOST_EXCEPTION_PRETTY_FUNCTION[p], s02 - p };
            }

#undef BOOST_EXCEPTION_CDECL
        }

    namespace
    exception_detail
        {
        struct
        pretty_type_name
            {
            char const * name_not_zero_terminated_at_length;
            std::size_t length;

            template <class CharT, class Traits>
            friend
            std::basic_ostream<CharT, Traits> &
            operator<<(std::basic_ostream<CharT, Traits> & os, pretty_type_name const & x)
                {
                return os.write(x.name_not_zero_terminated_at_length, x.length);
                }

            template <std::size_t S>
            friend
            char *
            to_zstr(char (&zstr)[S], pretty_type_name const & x) noexcept
                {
                std::size_t n = x.length < S - 1 ? x.length : S - 1;
                std::memcpy(zstr, x.name_not_zero_terminated_at_length, n);
                zstr[n] = 0;
                return zstr;
                }
            };

        template <class T>
        pretty_type_name
        get_pretty_tag_type_name()
            {
            n::r parsed = n::p<T>();
            return { parsed.name_not_zero_terminated_at_length, parsed.length };
            }
        }
    }

#define BOOST_EXCEPTION_STATIC_TYPEID(T) ::boost::exception_detail::type_info_(BOOST_CORE_TYPEID(T))

#ifndef BOOST_NO_RTTI
#define BOOST_EXCEPTION_DYNAMIC_TYPEID(x) ::boost::exception_detail::type_info_(typeid(x))
#endif

#if defined(_MSC_VER) && !defined(BOOST_EXCEPTION_ENABLE_WARNINGS)
#pragma warning(pop)
#endif
#endif
