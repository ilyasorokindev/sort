//----------------------------------------------------------------------------
/// @file timsort.hpp
/// @brief Timsort -- adaptive stable sort for random-access iterator ranges
///
/// Algorithm:   timsort
/// Stability:   stable
/// Complexity:  O(N) best / O(N log N) average and worst
/// Memory:      O(N), merge buffer <= N/2 elements
/// Origin:      Tim Peters, 2002 (Python's sort algorithm)
///
/// Copyright (c) 2026 Ilya Sorokin
/// Distributed under the Boost Software License, Version 1.0.
///     (See accompanying file LICENSE_1_0.txt or copy at
///      http://www.boost.org/LICENSE_1_0.txt)
///
/// @remarks
/// Precondition: comparator must not throw. If comp throws during a merge,
/// the range is left in a valid but unspecified state.
///
/// Refactor to detail/ subdirectory if this file exceeds ~900 lines.
//----------------------------------------------------------------------------

#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <utility>
#include <vector>

namespace boost {
namespace sort {
namespace tim_detail {

//----------------------------------------------------------------------------
// iterator_traits aliases (D3.3 -- mandatory throughout all stories)
// Use iter_value_t<Iter> and iter_diff_t<Iter> everywhere.
// Never use Iter::value_type directly.
//----------------------------------------------------------------------------
template <typename Iter>
using iter_value_t = typename std::iterator_traits<Iter>::value_type;

template <typename Iter>
using iter_diff_t = typename std::iterator_traits<Iter>::difference_type;

//----------------------------------------------------------------------------
// SortState (D1.1)
// Per-call mutable state shared across all merge functions.
// Instantiated once on the stack in the public timsort() call.
// buffer.reserve(n/2) at sort start; buffer.clear() between merges.
// NEVER buffer.resize() -- breaks non-default-constructible types.
//----------------------------------------------------------------------------
template <typename ValueType>
struct SortState {
  std::vector<ValueType> buffer;
  int min_gallop;
  SortState() : min_gallop(7) {}
};

//----------------------------------------------------------------------------
// RunEntry -- one pending sorted run on the run stack
// Field names are exact: base, len -- no synonyms (P1).
//----------------------------------------------------------------------------
template <typename Iter>
struct RunEntry {
  Iter base;
  iter_diff_t<Iter> len;
};

//----------------------------------------------------------------------------
// compute_minrun (P1)
// Canonical Python-derived formula: extract 6 MSBs of n, round up if any
// lower bits are set. Result is always in [32, 64] for n >= 64; returns n
// unchanged for n < 64.
//----------------------------------------------------------------------------
template <typename Iter>
iter_diff_t<Iter> compute_minrun(iter_diff_t<Iter> n) {
  iter_diff_t<Iter> r = 0;
  while (n >= 64) {
    r |= (n & 1);
    n >>= 1;
  }
  return n + r;
}

//----------------------------------------------------------------------------
// binary_insertion_sort (P1, position 3 in tim_detail)
// Sorts [first, last) assuming [first, start) is already sorted.
// Uses upper-bound binary search: equal elements from the left stay to
// the left of the inserted element, preserving stability (AC 6).
// Does NOT receive SortState (D1.1).
//----------------------------------------------------------------------------
template <typename Iter, typename Compare>
void binary_insertion_sort(Iter first, Iter last, Iter start, Compare comp) {
  typedef typename std::iterator_traits<Iter>::value_type value_type;  // P4
  if (first == start)
    ++start;
  for (; start != last; ++start) {
    value_type pivot = std::move_if_noexcept(*start);  // D3.4
    // Upper-bound binary search: find first pos where comp(pivot, *pos).
    // On equal, left = mid + 1 → inserts AFTER existing equals (stability).
    Iter left = first, right = start;
    while (left < right) {
      Iter mid = left + (right - left) / 2;
      if (comp(pivot, *mid))
        right = mid;
      else
        left = mid + 1;
    }
    // Shift [left, start) right by one using backward walk
    for (Iter ptr = start; ptr != left; --ptr)
      *ptr = std::move_if_noexcept(*(ptr - 1));
    *left = std::move_if_noexcept(pivot);
  }
}

//----------------------------------------------------------------------------
// count_run (P1, position 4 in tim_detail)
// Returns the length of the natural run starting at first.
// Strictly descending runs are reversed in-place (stability: equal elements
// are treated as non-descending and are NOT reversed).
// Does NOT receive SortState (D1.1).
//----------------------------------------------------------------------------
template <typename Iter, typename Compare>
iter_diff_t<Iter> count_run(Iter first, Iter last, Compare comp) {
  if (first == last) return 0;
  Iter cur = first;
  ++cur;
  if (cur == last) return 1;

  iter_diff_t<Iter> n = 2;
  if (comp(*cur, *first)) {
    // Strictly descending: scan while each element < previous
    while (++cur != last && comp(*cur, *(cur - 1)))
      ++n;
    std::reverse(first, cur);  // make ascending in-place
  } else {
    // Non-descending (ascending or equal): scan while each element >= previous
    while (++cur != last && !comp(*cur, *(cur - 1)))
      ++n;
  }
  return n;
}

//----------------------------------------------------------------------------
// gallop_left / gallop_right (P1, position 5 in tim_detail)
// Two-phase search: exponential gallop from hint, then binary refinement.
// gallop_left  : comp(*(base+mid), key) → leftmost pos where !(base[p]<key)
// gallop_right : comp(key, *(base+mid)) → rightmost pos where !(key<base[p])
// Asymmetric predicates implement the P3 tie-breaking rule for stability.
//----------------------------------------------------------------------------
template <typename Iter, typename Compare>
iter_diff_t<Iter> gallop_left(
    const iter_value_t<Iter>& key,
    Iter base, iter_diff_t<Iter> len, iter_diff_t<Iter> hint,
    Compare comp)
{
  iter_diff_t<Iter> last_ofs = 0, ofs = 1;
  if (comp(*(base + hint), key)) {
    // base[hint] < key — gallop rightward from hint
    const iter_diff_t<Iter> max_ofs = len - hint;
    while (ofs < max_ofs && comp(*(base + hint + ofs), key)) {
      last_ofs = ofs;
      ofs = (ofs << 1) | 1;
    }
    if (ofs > max_ofs) ofs = max_ofs;
    last_ofs += hint;
    ofs      += hint;
  } else {
    // key <= base[hint] — gallop leftward from hint
    const iter_diff_t<Iter> max_ofs = hint + 1;
    while (ofs < max_ofs && !comp(*(base + hint - ofs), key)) {
      last_ofs = ofs;
      ofs = (ofs << 1) | 1;
    }
    if (ofs > max_ofs) ofs = max_ofs;
    const iter_diff_t<Iter> tmp = last_ofs;
    last_ofs = hint - ofs;
    ofs      = hint - tmp;
  }
  // Binary search in (last_ofs, ofs]: base[last_ofs] < key <= base[ofs]
  ++last_ofs;
  while (last_ofs < ofs) {
    const iter_diff_t<Iter> mid = last_ofs + ((ofs - last_ofs) >> 1);
    if (comp(*(base + mid), key))
      last_ofs = mid + 1;
    else
      ofs = mid;
  }
  return ofs;  // AC1: equals len when key > all elements
}

template <typename Iter, typename Compare>
iter_diff_t<Iter> gallop_right(
    const iter_value_t<Iter>& key,
    Iter base, iter_diff_t<Iter> len, iter_diff_t<Iter> hint,
    Compare comp)
{
  iter_diff_t<Iter> last_ofs = 0, ofs = 1;
  if (comp(key, *(base + hint))) {
    // key < base[hint] — gallop leftward
    const iter_diff_t<Iter> max_ofs = hint + 1;
    while (ofs < max_ofs && comp(key, *(base + hint - ofs))) {
      last_ofs = ofs;
      ofs = (ofs << 1) | 1;
    }
    if (ofs > max_ofs) ofs = max_ofs;
    const iter_diff_t<Iter> tmp = last_ofs;
    last_ofs = hint - ofs;
    ofs      = hint - tmp;
  } else {
    // key >= base[hint] — gallop rightward
    const iter_diff_t<Iter> max_ofs = len - hint;
    while (ofs < max_ofs && !comp(key, *(base + hint + ofs))) {
      last_ofs = ofs;
      ofs = (ofs << 1) | 1;
    }
    if (ofs > max_ofs) ofs = max_ofs;
    last_ofs += hint;
    ofs      += hint;
  }
  // Binary search in (last_ofs, ofs]: base[last_ofs] <= key < base[ofs]
  ++last_ofs;
  while (last_ofs < ofs) {
    const iter_diff_t<Iter> mid = last_ofs + ((ofs - last_ofs) >> 1);
    if (comp(key, *(base + mid)))
      ofs = mid;
    else
      last_ofs = mid + 1;
  }
  return ofs;  // AC2: returns p where base[p]==key (not p+1)
}

//----------------------------------------------------------------------------
// merge_lo (P1, position 7 in tim_detail)
// Precondition: len1 <= len2.  Copies left run into buffer, merges L→R.
// Adaptive galloping: local min_gallop starts from state.min_gallop.
//----------------------------------------------------------------------------
template <typename Iter, typename Compare>
void merge_lo(
    Iter base1, iter_diff_t<Iter> len1,
    Iter base2, iter_diff_t<Iter> len2,
    SortState<iter_value_t<Iter>>& state,
    Compare comp)
{
  typedef iter_value_t<Iter> value_type;  // P4
  state.buffer.clear();
  for (Iter it = base1; it != base1 + len1; ++it)
    state.buffer.push_back(std::move_if_noexcept(*it));  // AC8: move_if_noexcept

  typename std::vector<value_type>::iterator buf   = state.buffer.begin();
  Iter  right = base2;
  Iter  dest  = base1;
  int   min_gallop = state.min_gallop;

  while (true) {
    iter_diff_t<Iter> left_wins = 0, right_wins = 0;
    // Linear phase
    do {
      if (comp(*right, *buf)) {
        *dest++ = std::move_if_noexcept(*right++);
        ++right_wins; left_wins = 0;
        if (--len2 == 0) goto merge_lo_done;
      } else {
        *dest++ = std::move_if_noexcept(*buf++);
        ++left_wins; right_wins = 0;
        if (--len1 == 0) goto merge_lo_done;
      }
    } while ((left_wins | right_wins) < min_gallop);

    // Gallop phase
    do {
      iter_diff_t<Iter> k;
      k = gallop_right<Iter, Compare>(*right, buf, len1, 0, comp);
      for (iter_diff_t<Iter> i = 0; i < k; ++i)
        *dest++ = std::move_if_noexcept(*buf++);
      len1 -= k; left_wins = k;
      if (len1 == 0) goto merge_lo_done;

      *dest++ = std::move_if_noexcept(*right++);
      if (--len2 == 0) goto merge_lo_done;

      k = gallop_left<Iter, Compare>(*buf, right, len2, 0, comp);
      for (iter_diff_t<Iter> i = 0; i < k; ++i)
        *dest++ = std::move_if_noexcept(*right++);
      len2 -= k; right_wins = k;
      if (len2 == 0) goto merge_lo_done;

      *dest++ = std::move_if_noexcept(*buf++);
      if (--len1 == 0) goto merge_lo_done;

      if (min_gallop > 1) --min_gallop;
    } while (left_wins >= min_gallop || right_wins >= min_gallop);

    ++min_gallop;  // penalise leaving gallop mode
  }
merge_lo_done:
  state.min_gallop = (min_gallop < 1) ? 1 : min_gallop;
  // Copy any remaining buffer (left-run) elements
  for (iter_diff_t<Iter> i = 0; i < len1; ++i)
    *dest++ = std::move_if_noexcept(*buf++);
  // Right-run remainder already in place
}

//----------------------------------------------------------------------------
// merge_hi (P1, position 7 in tim_detail)
// Precondition: len1 > len2.  Copies RIGHT run into buffer, merges R→L.
//----------------------------------------------------------------------------
template <typename Iter, typename Compare>
void merge_hi(
    Iter base1, iter_diff_t<Iter> len1,
    Iter base2, iter_diff_t<Iter> len2,
    SortState<iter_value_t<Iter>>& state,
    Compare comp)
{
  typedef iter_value_t<Iter> value_type;  // P4
  state.buffer.clear();
  for (Iter it = base2; it != base2 + len2; ++it)
    state.buffer.push_back(std::move_if_noexcept(*it));  // AC8

  typedef typename std::vector<value_type>::iterator BufIter;
  BufIter buf_end = state.buffer.end();
  Iter    left    = base1 + len1 - 1;      // last element of left run
  Iter    dest    = base2 + len2 - 1;      // last slot in merged range
  BufIter buf     = buf_end - 1;           // last element of buffered right run
  int     min_gallop = state.min_gallop;

  while (true) {
    iter_diff_t<Iter> left_wins = 0, right_wins = 0;
    // Linear phase (right-to-left; left run wins on equal — P3)
    do {
      if (comp(*buf, *left)) {
        *dest-- = std::move_if_noexcept(*left--);
        ++left_wins; right_wins = 0;
        if (--len1 == 0) goto merge_hi_done;
      } else {
        *dest-- = std::move_if_noexcept(*buf--);
        ++right_wins; left_wins = 0;
        if (--len2 == 0) goto merge_hi_done;
      }
    } while ((left_wins | right_wins) < min_gallop);

    // Gallop phase (mirrored)
    do {
      iter_diff_t<Iter> k;
      // How many left-run elements are >= *buf (gallop from right end of left run)
      k = len1 - gallop_left<Iter, Compare>(*buf, base1, len1, len1 - 1, comp);
      for (iter_diff_t<Iter> i = 0; i < k; ++i)
        *dest-- = std::move_if_noexcept(*left--);
      len1 -= k; left_wins = k;
      if (len1 == 0) goto merge_hi_done;

      *dest-- = std::move_if_noexcept(*buf--);
      if (--len2 == 0) goto merge_hi_done;

      // How many right-run (buffer) elements are > *left
      // buf - (len2-1) is the start of remaining buffer elements; use BufIter
      BufIter buf_base = buf - (len2 - 1);
      k = len2 - gallop_right<BufIter, Compare>(*left, buf_base, len2, len2 - 1, comp);
      for (iter_diff_t<Iter> i = 0; i < k; ++i)
        *dest-- = std::move_if_noexcept(*buf--);
      len2 -= k; right_wins = k;
      if (len2 == 0) goto merge_hi_done;

      *dest-- = std::move_if_noexcept(*left--);
      if (--len1 == 0) goto merge_hi_done;

      if (min_gallop > 1) --min_gallop;
    } while (left_wins >= min_gallop || right_wins >= min_gallop);

    ++min_gallop;
  }
merge_hi_done:
  state.min_gallop = (min_gallop < 1) ? 1 : min_gallop;
  // Copy remaining buffer (right-run) elements
  for (iter_diff_t<Iter> i = 0; i < len2; ++i)
    *dest-- = std::move_if_noexcept(*buf--);
  // Left-run remainder already in place
}

//----------------------------------------------------------------------------
// merge_collapse / merge_force_collapse (P1, position 8 in tim_detail)
// Run-stack invariant enforcement (D1.3).
//----------------------------------------------------------------------------
template <typename Iter, typename Compare>
void do_merge(
    std::vector<RunEntry<Iter>>& stack,
    typename std::vector<RunEntry<Iter>>::size_type n,
    SortState<iter_value_t<Iter>>& state,
    Compare comp)
{
  Iter              b1 = stack[n].base;
  iter_diff_t<Iter> l1 = stack[n].len;
  iter_diff_t<Iter> l2 = stack[n + 1].len;
  if (l1 <= l2)
    merge_lo<Iter, Compare>(b1, l1, b1 + l1, l2, state, comp);
  else
    merge_hi<Iter, Compare>(b1, l1, b1 + l1, l2, state, comp);
  stack[n].len = l1 + l2;
  stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(n) + 1);
}

template <typename Iter, typename Compare>
void merge_collapse(
    std::vector<RunEntry<Iter>>& stack,
    SortState<iter_value_t<Iter>>& state,
    Compare comp)
{
  while (stack.size() > 1) {
    typename std::vector<RunEntry<Iter>>::size_type n = stack.size() - 2;
    bool inv1_violated = stack[n].len <= stack[n + 1].len;
    bool inv2_violated = (n > 0) && (stack[n - 1].len <= stack[n].len + stack[n + 1].len);
    if (!inv1_violated && !inv2_violated) break;
    // AC6: merge smaller pair first
    if (n > 0 && stack[n - 1].len < stack[n + 1].len)
      --n;
    do_merge<Iter, Compare>(stack, n, state, comp);
  }
}

template <typename Iter, typename Compare>
void merge_force_collapse(
    std::vector<RunEntry<Iter>>& stack,
    SortState<iter_value_t<Iter>>& state,
    Compare comp)
{
  while (stack.size() > 1) {
    typename std::vector<RunEntry<Iter>>::size_type n = stack.size() - 2;
    if (n > 0 && stack[n - 1].len < stack[n + 1].len)
      --n;
    do_merge<Iter, Compare>(stack, n, state, comp);
  }
}

} // namespace tim_detail

//----------------------------------------------------------------------------
// Public API (D2.1, D2.2) — outside tim_detail, inside boost::sort
//----------------------------------------------------------------------------
template <typename Iter, typename Compare>
void timsort(Iter first, Iter last, Compare comp) {
  static_assert(
      std::is_base_of<
          std::random_access_iterator_tag,
          typename std::iterator_traits<Iter>::iterator_category
      >::value,
      "boost::sort::timsort requires RandomAccessIterator"
  );
  typedef typename std::iterator_traits<Iter>::value_type      value_type;  // P4
  typedef typename std::iterator_traits<Iter>::difference_type diff_t;      // P4

  const diff_t n = last - first;
  if (n < 2) return;  // FR-5: empty range, single element

  tim_detail::SortState<value_type> state;
  state.buffer.reserve(static_cast<std::size_t>(n / 2));  // D1.2: allocate once

  const diff_t minrun = tim_detail::compute_minrun<Iter>(n);
  std::vector<tim_detail::RunEntry<Iter>> run_stack;

  Iter   cur       = first;
  diff_t remaining = n;

  while (remaining > 0) {
    diff_t run_len = tim_detail::count_run(cur, last, comp);

    if (run_len < minrun) {
      const diff_t force = (remaining < minrun) ? remaining : minrun;
      tim_detail::binary_insertion_sort(cur, cur + force, cur + run_len, comp);
      run_len = force;
    }

    tim_detail::RunEntry<Iter> entry;
    entry.base = cur;
    entry.len  = run_len;
    run_stack.push_back(entry);

    tim_detail::merge_collapse(run_stack, state, comp);

    cur       += run_len;
    remaining -= run_len;
  }

  tim_detail::merge_force_collapse(run_stack, state, comp);
}

template <typename Iter>
void timsort(Iter first, Iter last) {
  typedef typename std::iterator_traits<Iter>::value_type value_type;  // P4
  timsort(first, last, std::less<value_type>());
}

} // namespace sort
} // namespace boost
