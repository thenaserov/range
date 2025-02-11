
// think-cell public library
//
// Copyright (C) 2016-2022 think-cell Software GmbH
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "assert_defs.h"

#include <type_traits>
#include <utility>

#define TC_HAS_MEM_FN_XXX_TRAIT_DEF( name, decoration, ... ) \
	template<typename U> \
	struct BOOST_PP_CAT(has_mem_fn_, name) { \
	private: \
		template<typename T> static auto test(int) -> decltype(std::declval<T decoration >().name ( __VA_ARGS__ ), tc::constant<true>()); \
		template<typename> static tc::constant<false> test(...); \
	public: \
		static constexpr bool value = std::is_same<decltype(test<U>(0)), tc::constant<true>>::value; \
	};

#define TC_HAS_STATIC_FN_XXX_TRAIT_DEF( name, ... ) \
	template<typename U> \
	struct has_static_fn_ ## name { \
	private: \
		template<typename T> static auto test(int) -> decltype(T::name(__VA_ARGS__), tc::constant<true>()); \
		template<typename> static tc::constant<false> test(...); \
	public: \
		static constexpr bool value = std::is_same<decltype(test<U>(0)), tc::constant<true>>::value; \
	};
