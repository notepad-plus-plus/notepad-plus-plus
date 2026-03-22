#ifndef BOOST_ASSERT_SOURCE_LOCATION_HPP_INCLUDED
#define BOOST_ASSERT_SOURCE_LOCATION_HPP_INCLUDED

// http://www.boost.org/libs/assert
//
// Copyright 2019, 2021 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt

#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <string>
#include <cstdio>
#include <cstring>

#if !defined(BOOST_NO_IOSTREAM)
#include <iosfwd>
#endif

#if defined(__cpp_lib_source_location) && __cpp_lib_source_location >= 201907L
# include <source_location>
#endif

namespace boost
{

struct source_location
{
private:

    char const * file_;
    char const * function_;
    boost::uint_least32_t line_;
    boost::uint_least32_t column_;

public:

    BOOST_CONSTEXPR source_location() BOOST_NOEXCEPT: file_( "" ), function_( "" ), line_( 0 ), column_( 0 )
    {
    }

    BOOST_CONSTEXPR source_location( char const * file, boost::uint_least32_t ln, char const * function, boost::uint_least32_t col = 0 ) BOOST_NOEXCEPT: file_( file ), function_( function ), line_( ln ), column_( col )
    {
    }

#if defined(__cpp_lib_source_location) && __cpp_lib_source_location >= 201907L

    BOOST_CONSTEXPR source_location( std::source_location const& loc ) BOOST_NOEXCEPT: file_( loc.file_name() ), function_( loc.function_name() ), line_( loc.line() ), column_( loc.column() )
    {
    }

#endif

    BOOST_CONSTEXPR char const * file_name() const BOOST_NOEXCEPT
    {
        return file_;
    }

    BOOST_CONSTEXPR char const * function_name() const BOOST_NOEXCEPT
    {
        return function_;
    }

    BOOST_CONSTEXPR boost::uint_least32_t line() const BOOST_NOEXCEPT
    {
        return line_;
    }

    BOOST_CONSTEXPR boost::uint_least32_t column() const BOOST_NOEXCEPT
    {
        return column_;
    }

#if defined(BOOST_MSVC)
# pragma warning( push )
# pragma warning( disable: 4996 )
#endif

#if ( defined(_MSC_VER) && _MSC_VER < 1900 ) || ( defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR) )
# define BOOST_ASSERT_SNPRINTF(buffer, format, arg) std::sprintf(buffer, format, arg)
#else
# define BOOST_ASSERT_SNPRINTF(buffer, format, arg) std::snprintf(buffer, sizeof(buffer)/sizeof(buffer[0]), format, arg)
#endif

    std::string to_string() const
    {
        unsigned long ln = line();

        if( ln == 0 )
        {
            return "(unknown source location)";
        }

        std::string r = file_name();

        char buffer[ 16 ];

        BOOST_ASSERT_SNPRINTF( buffer, ":%lu", ln );
        r += buffer;

        unsigned long co = column();

        if( co )
        {
            BOOST_ASSERT_SNPRINTF( buffer, ":%lu", co );
            r += buffer;
        }

        char const* fn = function_name();

        if( *fn != 0 )
        {
            r += " in function '";
            r += fn;
            r += '\'';
        }

        return r;
    }

#undef BOOST_ASSERT_SNPRINTF

#if defined(BOOST_MSVC)
# pragma warning( pop )
#endif

    inline friend bool operator==( source_location const& s1, source_location const& s2 ) BOOST_NOEXCEPT
    {
        return std::strcmp( s1.file_, s2.file_ ) == 0 && std::strcmp( s1.function_, s2.function_ ) == 0 && s1.line_ == s2.line_ && s1.column_ == s2.column_;
    }

    inline friend bool operator!=( source_location const& s1, source_location const& s2 ) BOOST_NOEXCEPT
    {
        return !( s1 == s2 );
    }
};

#if !defined(BOOST_NO_IOSTREAM)

template<class E, class T> std::basic_ostream<E, T> & operator<<( std::basic_ostream<E, T> & os, source_location const & loc )
{
    os << loc.to_string();
    return os;
}

#endif

} // namespace boost

#if defined(BOOST_DISABLE_CURRENT_LOCATION)

# define BOOST_CURRENT_LOCATION ::boost::source_location()

#elif defined(BOOST_MSVC) && BOOST_MSVC >= 1935

# define BOOST_CURRENT_LOCATION ::boost::source_location(__builtin_FILE(), __builtin_LINE(), __builtin_FUNCSIG(), __builtin_COLUMN())

#elif defined(BOOST_MSVC) && BOOST_MSVC >= 1926

// std::source_location::current() is available in -std:c++20, but fails with consteval errors before 19.31, and doesn't produce
// the correct result under 19.31, so prefer the built-ins
# define BOOST_CURRENT_LOCATION ::boost::source_location(__builtin_FILE(), __builtin_LINE(), __builtin_FUNCTION(), __builtin_COLUMN())

#elif defined(BOOST_MSVC)

// __LINE__ is not a constant expression under /ZI (edit and continue) for 1925 and before

# define BOOST_CURRENT_LOCATION_IMPL_1(x) BOOST_CURRENT_LOCATION_IMPL_2(x)
# define BOOST_CURRENT_LOCATION_IMPL_2(x) (x##0 / 10)

# define BOOST_CURRENT_LOCATION ::boost::source_location(__FILE__, BOOST_CURRENT_LOCATION_IMPL_1(__LINE__), "")

#elif defined(__cpp_lib_source_location) && __cpp_lib_source_location >= 201907L && !defined(__NVCC__)

// Under nvcc, __builtin_source_location is not constexpr
// https://github.com/boostorg/assert/issues/32

# define BOOST_CURRENT_LOCATION ::boost::source_location(::std::source_location::current())

#elif defined(BOOST_CLANG) && BOOST_CLANG_VERSION >= 90000

# define BOOST_CURRENT_LOCATION ::boost::source_location(__builtin_FILE(), __builtin_LINE(), __builtin_FUNCTION(), __builtin_COLUMN())

#elif defined(BOOST_GCC) && BOOST_GCC >= 80000

// The built-ins are available in 4.8+, but are not constant expressions until 7
// In addition, reproducible builds require -ffile-prefix-map, which is GCC 8
// https://github.com/boostorg/assert/issues/38

# define BOOST_CURRENT_LOCATION ::boost::source_location(__builtin_FILE(), __builtin_LINE(), __builtin_FUNCTION())

#elif defined(BOOST_GCC) && BOOST_GCC >= 50000

// __PRETTY_FUNCTION__ is allowed outside functions under GCC, but 4.x suffers from codegen bugs
# define BOOST_CURRENT_LOCATION ::boost::source_location(__FILE__, __LINE__, __PRETTY_FUNCTION__)

#else

// __func__ macros aren't allowed outside functions, but BOOST_CURRENT_LOCATION is
# define BOOST_CURRENT_LOCATION ::boost::source_location(__FILE__, __LINE__, "")

#endif

#endif // #ifndef BOOST_ASSERT_SOURCE_LOCATION_HPP_INCLUDED
