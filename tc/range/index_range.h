
// think-cell public library
//
// Copyright (C) 2016-2022 think-cell Software GmbH
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../base/assert_defs.h"
#include "../base/type_traits_fwd.h"
#include "../base/as_lvalue.h"
#include "../base/reference_or_value.h"
#include "../base/static_polymorphism.h"
#include "../algorithm/break_or_continue.h"
#include "../algorithm/size.h"
#include "meta.h"


MODIFY_WARNINGS_BEGIN(((disable)(4018)))
#include <boost/range/difference_type.hpp>
#include <boost/range/category.hpp>
MODIFY_WARNINGS_END

#include <boost/range/iterator_range.hpp>

#include <boost/mpl/has_xxx.hpp>

#include <type_traits>

namespace tc {
	namespace iterator {
		template<typename It> constexpr It middle_point(It const&, It const&) noexcept;
	}

	namespace no_adl {
		// By standard, the lifetime of a reference may be limited to the lifetime of the iterator ("stashing iterators").
		// Stashing is forbidden for C++17 iterators that fulfill the ForwardIterator/BidirectionalIterator concept.
		// This essentially follows from the multipass guarantee: a==b <--> *a points to the same object as *b.
		// The C++17 STL still comes with some stashing iterators (LegacyForwardIterator/LegacyBidirectionalIterator concept).
		// A notable example is std::filesystem::path::iterator. Unfortunately, neither std:: nor boost::iterator_traits allows
		// us to tell apart legacy from non-stashing iterators.
		// tc::counting_iterator are stashing, as well as the iterators of all ranges adapted from a counting range.

		template <typename Element>
		struct is_stashing_element : tc::constant<false> {};
	}
	using no_adl::is_stashing_element;

	namespace no_adl {
		template< typename It >
		struct TC_EMPTY_BASES iterator_base {
			using iterator = It;
			using const_iterator = It;
			using tc_index = It;

			static constexpr bool c_bHasStashingIndex=tc::is_stashing_element<It>::value;

			static constexpr typename std::iterator_traits<iterator>::reference dereference_index(tc_index const& idx) noexcept {
				return *idx;
			}

			static constexpr void increment_index(tc_index& idx) noexcept {
				++idx;
			}

			static constexpr void decrement_index(tc_index& idx) noexcept {
				--idx;
			}

			static constexpr void advance_index(tc_index& idx, typename std::iterator_traits<iterator>::difference_type d) noexcept {
				idx+=d;
			}

			template<ENABLE_SFINAE>
			static constexpr auto distance_to_index(SFINAE_TYPE(tc_index) const& idxLhs, tc_index const& idxRhs) return_decltype_NOEXCEPT(
				idxRhs - idxLhs
			)

			static constexpr void middle_point( tc_index & idxBegin, tc_index const& idxEnd ) noexcept {
				idxBegin=tc::iterator::middle_point( idxBegin, idxEnd );
			}

			static constexpr iterator make_iterator( tc_index idx ) noexcept {
				return idx;
			}
		};
	}
	using no_adl::iterator_base;

	TC_HAS_EXPR(index, (Rng), std::declval<typename Rng::tc_index>())

	namespace no_adl {
		template<typename Rng>
		struct index final {
			static_assert( !std::is_reference<Rng>::value );
			using type=tc::iterator_t<Rng>;
		};

		template<typename Rng> requires has_index<Rng>::value
		struct index<Rng> final  {
			static_assert( !std::is_reference<Rng>::value );
			using type=typename Rng::tc_index;
		};

		template<typename Rng>
		using index_t=typename index<Rng>::type;

		template<typename Rng>
		struct has_stashing_index : tc::is_stashing_element<index_t<Rng>> {
			static_assert( !std::is_reference<Rng>::value );
		};

		template<typename Rng> requires tc::has_index<Rng>::value
		struct has_stashing_index<Rng> : tc::constant<Rng::c_bHasStashingIndex>  {
			static_assert( !std::is_reference<Rng>::value );
		};		
		
	}
	using no_adl::index_t;
	using no_adl::has_stashing_index;

	template<typename Rng, std::enable_if_t< !has_index< std::remove_reference_t<Rng> >::value >* =nullptr >
	constexpr auto begin_index(Rng&& rng) return_decltype_MAYTHROW(
		tc::begin(std::forward<Rng>(rng))
	)

	template<typename Rng, std::enable_if_t< has_index< std::remove_reference_t<Rng> >::value >* =nullptr >
	constexpr auto begin_index(Rng&& rng) return_decltype_MAYTHROW(
		std::forward<Rng>(rng).begin_index()
	)

	template<tc::common_range Rng, std::enable_if_t< !has_index< std::remove_reference_t<Rng> >::value >* =nullptr >
	constexpr auto end_index(Rng&& rng) return_decltype_MAYTHROW(
		tc::end(std::forward<Rng>(rng))
	)

	template<typename Rng, std::enable_if_t< has_index< std::remove_reference_t<Rng> >::value >* =nullptr >
	constexpr auto end_index(Rng&& rng) return_decltype_MAYTHROW(
		std::forward<Rng>(rng).end_index()
	)

	template<typename Rng, typename It, std::enable_if_t< !has_index<Rng>::value >* =nullptr >
	constexpr bool at_end_index(Rng const& rng, It const& it) return_MAYTHROW(
		it==tc::end(rng)
	)

	template<typename Rng, typename Index, std::enable_if_t< has_index<Rng>::value >* =nullptr >
	constexpr bool at_end_index(Rng const& rng, Index const& idx) return_MAYTHROW(
		rng.at_end_index(idx)
	)

	template<typename Rng, typename It, std::enable_if_t<!has_index< std::remove_reference_t<Rng> >::value>* =nullptr >
	constexpr auto dereference_index(Rng&& rng, It&& it) noexcept(noexcept(*std::forward<It>(it)))
		-> typename std::conditional_t<
			tc::is_safely_convertible<decltype(*std::declval<It>()), tc::iter_reference_t<tc::iterator_t<Rng>>>::value,
			tc::type::identity<tc::iter_reference_t<tc::iterator_t<Rng>>>,
			tc::decay<tc::iter_reference_t<tc::iterator_t<Rng>>>
		>::type
	{
		return *std::forward<It>(it);
	}

	template<typename Rng, typename Index, std::enable_if_t< has_index< std::remove_reference_t<Rng> >::value >* =nullptr >
	constexpr auto dereference_index(Rng&& rng, Index&& idx) return_decltype_xvalue_by_ref_MAYTHROW(
		std::forward<Rng>(rng).dereference_index(std::forward<Index>(idx))
	)

	template<typename Rng, typename It, std::enable_if_t< !has_index<Rng>::value >* =nullptr >
	constexpr void increment_index(Rng const&, It& it) noexcept(noexcept(++it)) {
		++it;
	}

	template<typename Rng, typename Index, std::enable_if_t< has_index<Rng>::value >* =nullptr >
	constexpr void increment_index(Rng const& rng, Index& idx) return_MAYTHROW(
		rng.increment_index(idx)
	)

	template<typename Rng, typename It, std::enable_if_t< !has_index<Rng>::value >* =nullptr >
	constexpr auto decrement_index(Rng const&, It& it) return_decltype_MAYTHROW(
		void(--it)
	)

	TC_HAS_MEM_FN_XXX_TRAIT_DEF(decrement_index, const&, std::declval<typename T::tc_index &>())
	TC_HAS_MEM_FN_XXX_TRAIT_DEF(distance_to_index, const&, std::declval<typename T::tc_index const&>(), std::declval<typename T::tc_index const&>())
	TC_HAS_MEM_FN_XXX_TRAIT_DEF(middle_point, const&, std::declval<typename T::tc_index &>(), std::declval<typename T::tc_index const&>());
	TC_HAS_MEM_FN_XXX_TRAIT_DEF(advance_index, const&, std::declval<typename T::tc_index &>(), std::declval<T const&>().distance_to_index(std::declval<typename T::tc_index const&>(), std::declval<typename T::tc_index const&>()));

	TC_HAS_MEM_FN_XXX_TRAIT_DEF(base_range, &)
	TC_HAS_MEM_FN_XXX_TRAIT_DEF(border_base_index, const&, std::declval<typename T::tc_index &>())
	TC_HAS_MEM_FN_XXX_TRAIT_DEF(element_base_index, const&, std::declval<typename T::tc_index &>())

	template<typename Rng, typename Index, std::enable_if_t< has_index<Rng>::value >* =nullptr>
	constexpr auto decrement_index(Rng const& rng, Index& idx) return_decltype_MAYTHROW(
		rng.decrement_index(idx)
	)

	template<typename Rng, typename It, typename Difference, std::enable_if_t< !has_index<Rng>::value >* =nullptr >
	constexpr void advance_index(Rng const&, It& it, Difference&& d) MAYTHROW {
		it+=std::forward<Difference>(d);
	}

	template<typename Rng, typename Index, typename Difference, std::enable_if_t< has_index<Rng>::value >* =nullptr >
	constexpr void advance_index(Rng const& rng, Index& idx, Difference&& d) MAYTHROW {
		rng.advance_index(idx, std::forward<Difference>(d));
	}

	template<typename Rng, typename IndexLhs, typename IndexRhs, std::enable_if_t< has_index<Rng>::value >* =nullptr >
	constexpr auto distance_to_index(Rng const& rng, IndexLhs const& idxLhs, IndexRhs const& idxRhs) return_decltype_MAYTHROW(
		rng.distance_to_index(idxLhs,idxRhs)
	)

	template<typename Rng, typename ItLhs, typename ItRhs, std::enable_if_t< !has_index<Rng>::value >* =nullptr >
	constexpr auto distance_to_index(Rng const&, ItLhs const& itLhs, ItRhs const& itRhs) return_decltype_MAYTHROW(
		itRhs-itLhs
	)

	template<typename Rng, typename ItLhs, typename ItRhs, std::enable_if_t< !has_index<Rng>::value >* =nullptr >
	constexpr void middle_point(Rng const&, ItLhs& itLhs, ItRhs const& itRhs) MAYTHROW {
		itLhs=tc::iterator::middle_point( tc::as_const(itLhs), itRhs );
	}

	template<typename Rng, typename IndexLhs, typename IndexRhs, std::enable_if_t< has_index<Rng>::value >* =nullptr >
	constexpr void middle_point(Rng const& rng, IndexLhs& idxLhs, IndexRhs const& idxRhs) MAYTHROW {
		rng.middle_point(idxLhs,idxRhs);
	}

	TC_HAS_EXPR(decrement_index, (Rng), tc::decrement_index(std::declval<Rng const&>(), std::declval<index_t<Rng>&>()))
	TC_HAS_EXPR(end_index, (Rng), tc::end_index(std::declval<Rng const&>()));
	TC_HAS_EXPR(distance_to_index, (Rng), tc::distance_to_index(std::declval<Rng const&>(), std::declval<index_t<Rng> const&>(), std::declval<index_t<Rng> const&>()));
	TC_HAS_EXPR(advance_index, (Rng), tc::advance_index(std::declval<Rng const&>(), std::declval<index_t<Rng> &>(), tc::distance_to_index(std::declval<Rng const&>(), std::declval<index_t<Rng> const&>(), std::declval<index_t<Rng> const&>())));
	TC_HAS_EXPR(middle_point, (Rng), tc::middle_point(std::declval<Rng const&>(), std::declval<index_t<Rng> &>(), std::declval<index_t<Rng> const&>()));

	template<typename Rng, typename It, std::enable_if_t< !has_index< std::remove_reference_t<Rng> >::value >* =nullptr >
	constexpr decltype(auto) make_iterator(Rng&&, It&& it) noexcept {
		return std::forward<It>(it);
	}

	template<typename Rng, typename Index, std::enable_if_t< has_index< std::remove_reference_t<Rng> >::value >* =nullptr >
	constexpr decltype(auto) make_iterator(Rng&& rng, Index&& idx) noexcept {
		return std::forward<Rng>(rng).make_iterator(std::forward<Index>(idx));
	}

	namespace no_adl {
		template<typename Rng>
		struct is_index_valid_for_move_constructed_range : std::is_lvalue_reference<Rng> {};

		template<typename It>
		struct is_index_valid_for_move_constructed_range<tc::iterator_base<It>> : tc::constant<true> {};

		template<typename Char> requires tc::is_char<Char>::value
		struct is_index_valid_for_move_constructed_range<Char*> : tc::constant<true> {};

		template<typename T, typename Alloc>
		struct is_index_valid_for_move_constructed_range<std::vector<T, Alloc>> : tc::constant<true> {}; // end iterator is not guaranteed by the standard but we assume it's still valid
	}
	using no_adl::is_index_valid_for_move_constructed_range;
}
