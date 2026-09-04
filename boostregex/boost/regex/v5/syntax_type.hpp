/*
 *
 * Copyright (c) 2003
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */
 
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         syntax_type.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares regular expression synatx type enumerator.
  */

#ifndef BOOST_REGEX_SYNTAX_TYPE_HPP
#define BOOST_REGEX_SYNTAX_TYPE_HPP

#include <boost/regex/config.hpp>

namespace boost{
namespace regex_constants{

BOOST_REGEX_MODULE_EXPORT typedef unsigned char syntax_type;

//
// values chosen are binary compatible with previous version:
//
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_char = 0;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_open_mark = 1;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_close_mark = 2;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_dollar = 3;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_caret = 4;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_dot = 5;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_star = 6;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_plus = 7;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_question = 8;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_open_set = 9;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_close_set = 10;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_or = 11;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_escape = 12;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_dash = 14;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_open_brace = 15;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_close_brace = 16;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_digit = 17;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_comma = 27;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_equal = 37;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_colon = 36;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_not = 53;

// extensions:

BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_hash = 13;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST syntax_type syntax_newline = 26;

// escapes:

BOOST_REGEX_MODULE_EXPORT typedef syntax_type escape_syntax_type;

BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_word_assert = 18;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_not_word_assert = 19;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_control_f = 29;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_control_n = 30;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_control_r = 31;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_control_t = 32;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_control_v = 33;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_ascii_control = 35;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_hex = 34;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_unicode = 0; // not used
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_identity = 0; // not used
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_backref = syntax_digit;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_decimal = syntax_digit; // not used
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_class = 22; 
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_not_class = 23; 

// extensions:

BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_left_word = 20;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_right_word = 21;
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_start_buffer = 24;                 // for \`
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_end_buffer = 25;                   // for \'
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_control_a = 28;                    // for \a
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_e = 38;                            // for \e
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_E = 47;                            // for \Q\E
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_Q = 48;                            // for \Q\E
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_X = 49;                            // for \X
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_C = 50;                            // for \C
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_Z = 51;                            // for \Z
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_G = 52;                            // for \G

BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_property = 54;                     // for \p
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_not_property = 55;                 // for \P
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_named_char = 56;                   // for \N
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_extended_backref = 57;             // for \g
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_reset_start_mark = 58;             // for \K
BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type escape_type_line_ending = 59;                  // for \R

BOOST_REGEX_MODULE_EXPORT BOOST_REGEX_STATIC_CONST escape_syntax_type syntax_max = 60;

}
}


#endif
