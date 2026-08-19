//Copyright (c) 2006-2026 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_EXCEPTION_SERIALIZATION_BOOST_JSON_ENCODER_HPP_INCLUDED
#define BOOST_EXCEPTION_SERIALIZATION_BOOST_JSON_ENCODER_HPP_INCLUDED

#include <type_traits>
#include <utility>

namespace boost { namespace json {

class value;

template <class T>
void value_from(T &&, value &);

} }

namespace
boost
    {
    namespace
    exception_serialization
        {
        template <class Value = boost::json::value>
        struct
        boost_json_encoder_
            {
            Value & v_;

            template <class Encoder, class T, class... Deprioritize>
            friend
            typename std::enable_if<std::is_same<Encoder, boost_json_encoder_>::value>::type
            output(Encoder & e, T const & x, Deprioritize...)
                {
                boost::json::value_from(x, e.v_);
                }

            template <class T>
            friend
            void
            output_at(boost_json_encoder_ & e, T const & x, char const * name)
                {
                if( e.v_.is_null() )
                    e.v_.emplace_object();
                boost_json_encoder_ nested{e.v_.as_object()[name]};
                output(nested, x);
                }
            };

        using boost_json_encoder = boost_json_encoder_<>;
        }
    }

#endif
