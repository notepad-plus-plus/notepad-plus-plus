/*
 *
 * Copyright (c) 1998-2002
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         regex_fwd.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Forward declares boost::basic_regex<> and
  *                associated typedefs.
  */

#ifndef BOOST_REGEX_FWD_HPP_INCLUDED
#define BOOST_REGEX_FWD_HPP_INCLUDED

#ifndef BOOST_REGEX_CONFIG_HPP
#include <boost/regex/config.hpp>
#endif

//
// define BOOST_REGEX_NO_FWD if this
// header doesn't work!
//
#ifdef BOOST_REGEX_NO_FWD
#  ifndef BOOST_RE_REGEX_HPP
#     include <boost/regex.hpp>
#  endif
#else

namespace boost{

BOOST_REGEX_MODULE_EXPORT template <class charT>
class cpp_regex_traits;
BOOST_REGEX_MODULE_EXPORT template <class charT>
struct c_regex_traits;
BOOST_REGEX_MODULE_EXPORT template <class charT>
class w32_regex_traits;

#ifdef BOOST_REGEX_USE_WIN32_LOCALE
BOOST_REGEX_MODULE_EXPORT template <class charT, class implementationT = w32_regex_traits<charT> >
struct regex_traits;
#elif defined(BOOST_REGEX_USE_CPP_LOCALE)
BOOST_REGEX_MODULE_EXPORT template <class charT, class implementationT = cpp_regex_traits<charT> >
struct regex_traits;
#else
BOOST_REGEX_MODULE_EXPORT template <class charT, class implementationT = c_regex_traits<charT> >
struct regex_traits;
#endif

BOOST_REGEX_MODULE_EXPORT template <class charT, class traits = regex_traits<charT> >
class basic_regex;

BOOST_REGEX_MODULE_EXPORT typedef basic_regex<char, regex_traits<char> > regex;
#ifndef BOOST_NO_WREGEX
BOOST_REGEX_MODULE_EXPORT typedef basic_regex<wchar_t, regex_traits<wchar_t> > wregex;
#endif

} // namespace boost

#endif  // BOOST_REGEX_NO_FWD

#endif




