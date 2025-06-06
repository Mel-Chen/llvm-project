//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___CXX03___TYPE_TRAITS_EXTENT_H
#define _LIBCPP___CXX03___TYPE_TRAITS_EXTENT_H

#include <__cxx03/__config>
#include <__cxx03/__type_traits/integral_constant.h>
#include <__cxx03/cstddef>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if __has_builtin(__array_extent)

template <class _Tp, size_t _Dim = 0>
struct _LIBCPP_TEMPLATE_VIS extent : integral_constant<size_t, __array_extent(_Tp, _Dim)> {};

#else // __has_builtin(__array_extent)

template <class _Tp, unsigned _Ip = 0>
struct _LIBCPP_TEMPLATE_VIS extent : public integral_constant<size_t, 0> {};
template <class _Tp>
struct _LIBCPP_TEMPLATE_VIS extent<_Tp[], 0> : public integral_constant<size_t, 0> {};
template <class _Tp, unsigned _Ip>
struct _LIBCPP_TEMPLATE_VIS extent<_Tp[], _Ip> : public integral_constant<size_t, extent<_Tp, _Ip - 1>::value> {};
template <class _Tp, size_t _Np>
struct _LIBCPP_TEMPLATE_VIS extent<_Tp[_Np], 0> : public integral_constant<size_t, _Np> {};
template <class _Tp, size_t _Np, unsigned _Ip>
struct _LIBCPP_TEMPLATE_VIS extent<_Tp[_Np], _Ip> : public integral_constant<size_t, extent<_Tp, _Ip - 1>::value> {};

#endif // __has_builtin(__array_extent)

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___CXX03___TYPE_TRAITS_EXTENT_H
