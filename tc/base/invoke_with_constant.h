
// think-cell public library
//
// Copyright (C) 2016-2022 think-cell Software GmbH
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once 

#include "type_traits_fwd.h"
#include "utility.h"

#include "../algorithm/size.h"

namespace tc {
	namespace invoke_with_constant_impl {
		template<typename TResult, typename TIndex, TIndex I, typename Func, typename... Args>
		TResult invoke_impl(Func func, Args&&... args) MAYTHROW {
			return func(tc::constant<I>(), std::forward<Args>(args)...);
		}

		template<typename TIndex, TIndex I, TIndex... Is>
		inline constexpr TIndex IdxFirst = I;

		template<typename T>
		struct invoke_with_constant_impl {};

		template<typename TIndex, TIndex... Is>
		struct invoke_with_constant_impl<std::integer_sequence<TIndex, Is...>> {
			template<typename Func, typename... Args>
			struct inner {
				using result_type = tc::common_reference_prvalue_as_val_t<
					decltype(std::declval<std::decay_t<Func>&>()(tc::constant<Is>(), std::declval<Args>()...))...
				>;

				template<TIndex I, TIndex... IsRemaining>
				static result_type constexpr invoke_constexpr(Func&& func, TIndex nIndex, Args&&... args) MAYTHROW {
					if (I == nIndex) {
						return func(tc::constant<I>(), std::forward<Args>(args)...); // MAYTHROW
					} else if constexpr (0 < sizeof...(IsRemaining)) {
						return invoke_constexpr<IsRemaining...>(std::forward<Func>(func), nIndex, std::forward<Args>(args)...); // MAYTHROW
					} else {
						_ASSERTFALSE;
						return func(tc::constant<I>(), std::forward<Args>(args)...);
					}
				}

				static result_type constexpr invoke_constexpr_outer(Func&& func, TIndex nIndex, Args&&... args) MAYTHROW {
					return invoke_constexpr<Is...>(std::forward<Func>(func), nIndex, std::forward<Args>(args)...); // MAYTHROW (but won't because constexpr)
				}

				static result_type invoke_non_constexpr(Func&& func, TIndex nIndex, Args&&... args) MAYTHROW {
					static constexpr std::add_pointer_t<result_type(std::remove_reference_t<Func>, Args&&...)> apfn[] = {
						invoke_impl<result_type, TIndex, Is, std::decay_t<Func>, Args...>
						...
					};

					auto const nTableIndex = nIndex - IdxFirst<TIndex, Is...>;
					_ASSERTNORETURN(0 <= nTableIndex && nTableIndex < tc::size(apfn));

					return apfn[nTableIndex](std::forward<Func>(func), std::forward<Args>(args)...); // MAYTHROW
				}
			};
		};
	}

	template<typename ContiguousIntegerSequence, typename Func, typename... Args>
	decltype(auto) constexpr invoke_with_constant(Func&& func, typename ContiguousIntegerSequence::value_type nIndex, Args&&... args) MAYTHROW {
		static_assert(tc::is_contiguous_integer_sequence<ContiguousIntegerSequence>::value);

		if constexpr (0 < ContiguousIntegerSequence::size()) {
			using impl_type = typename invoke_with_constant_impl::invoke_with_constant_impl<ContiguousIntegerSequence>::template inner<Func, Args...>;
			if (std::is_constant_evaluated()) {
				return impl_type::invoke_constexpr_outer(std::forward<Func>(func), nIndex, std::forward<Args>(args)...);
			} else {
				return impl_type::invoke_non_constexpr(std::forward<Func>(func), nIndex, std::forward<Args>(args)...);
			}
		}
	}
}
