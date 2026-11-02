//Copyright (c) 2006-2026 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_EXCEPTION_DETAIL_ENCODER_HPP_INCLUDED
#define BOOST_EXCEPTION_DETAIL_ENCODER_HPP_INCLUDED

#include <boost/exception/detail/type_info.hpp>
#include <type_traits>
#include <utility>

namespace
boost
    {
    template <class Tag, class T> class error_info;

    namespace
    exception_serialization
        {
        struct encoder_adl {};
        }

    namespace
    exception_detail
        {
        template <class T>
        struct first_arg;

        template <class C, class R, class A1, class... A>
        struct
        first_arg<R(C::*)(A1, A...)>
            {
            using type = A1;
            };

        template <class C, class R, class A1, class... A>
        struct
        first_arg<R(C::*)(A1, A...) const>
            {
            using type = A1;
            };

        class
        encoder:
            exception_serialization::encoder_adl
            {
            encoder(encoder const &) = delete;
            encoder & operator=(encoder const &) = delete;

            core::typeinfo const * type_;
            void * e_;

            bool
            dispatch_()
                {
                return false;
                }

            template <class F1, class... Fn>
            bool
            dispatch_(F1 && f1, Fn && ... fn)
                {
                using encoder_type = typename std::decay<typename first_arg<decltype(&std::decay<F1>::type::operator())>::type>::type;
                if (encoder_type * e = get<encoder_type>())
                    {
                    std::forward<F1>(f1)(*e);
                    return true;
                    }
                return dispatch_(std::forward<Fn>(fn)...);
                }

            protected:

            template <class Encoder>
            explicit
            encoder(Encoder * e) noexcept:
                type_(&BOOST_CORE_TYPEID(Encoder)),
                e_(e)
                {
                }

            public:

            template <class Encoder>
            Encoder *
            get() noexcept
                {
                return *type_ == BOOST_CORE_TYPEID(Encoder) ? static_cast<Encoder *>(e_) : nullptr;
                }

            template <class... Fn>
            bool
            dispatch(Fn && ... fn)
                {
                return dispatch_(std::forward<Fn>(fn)...);
                }
            };

        template <class Encoder>
        struct
        encoder_adaptor:
            encoder
            {
            explicit
            encoder_adaptor(Encoder & e) noexcept:
                encoder(&e)
                {
                }
            };
        }
    }

#endif
