//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___TYPE_TRAITS_IS_BOUNDED_ARRAY_H
#define _LIBCPP___TYPE_TRAITS_IS_BOUNDED_ARRAY_H

#include <__config>
#include <__type_traits/integral_constant.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

template <class _Tp>
inline const bool __is_bounded_array_v = __is_bounded_array(_Tp);

#if _LIBCPP_STD_VER >= 20

template <class _Tp>
struct _LIBCPP_NO_SPECIALIZATIONS is_bounded_array : bool_constant<__is_bounded_array(_Tp)> {};

template <class _Tp>
_LIBCPP_NO_SPECIALIZATIONS inline constexpr bool is_bounded_array_v = __is_bounded_array(_Tp);

#endif

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___TYPE_TRAITS_IS_BOUNDED_ARRAY_H
