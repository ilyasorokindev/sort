//----------------------------------------------------------------------------
/// @file test_timsort.cpp
/// @brief test program of the timsort algorithm
///
/// @author Copyright (c) 2026 Boost Contributors\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanying file LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#include <algorithm>
#include <iostream>
#include <vector>
#include <random>
#include <boost/sort/timsort/timsort.hpp>
#include <boost/test/included/test_exec_monitor.hpp>
#include <boost/test/test_tools.hpp>

void test_correctness();
void test_stability();
void test_edge_cases();
void test_type_coverage();
void test_adversarial();

void test_correctness() {
  typedef std::vector<int> Vec;
  const int NElem = 10000;
  std::mt19937 my_rand(0);

  // Random
  Vec v1;
  for (int i = 0; i < NElem; ++i) v1.push_back(my_rand() % NElem);
  boost::sort::timsort(v1.begin(), v1.end());
  BOOST_REQUIRE(static_cast<int>(v1.size()) == NElem);
  BOOST_CHECK(std::is_sorted(v1.begin(), v1.end()));

  // Already sorted
  Vec v2;
  for (int i = 0; i < NElem; ++i) v2.push_back(i);
  boost::sort::timsort(v2.begin(), v2.end());
  BOOST_CHECK(std::is_sorted(v2.begin(), v2.end()));

  // Reverse sorted
  Vec v3;
  for (int i = 0; i < NElem; ++i) v3.push_back(NElem - i);
  boost::sort::timsort(v3.begin(), v3.end());
  BOOST_CHECK(std::is_sorted(v3.begin(), v3.end()));

  // Many duplicates
  Vec v4;
  for (int i = 0; i < NElem; ++i) v4.push_back(i % 100);
  boost::sort::timsort(v4.begin(), v4.end());
  BOOST_CHECK(std::is_sorted(v4.begin(), v4.end()));

  // Single contiguous sorted run (exercises O(N) path)
  Vec v5;
  for (int i = 0; i < NElem; ++i) v5.push_back(i);
  boost::sort::timsort(v5.begin(), v5.end());
  BOOST_CHECK(std::is_sorted(v5.begin(), v5.end()));

  // Two-arg overload with explicit comparator
  Vec v6;
  for (int i = 0; i < NElem; ++i) v6.push_back(my_rand() % NElem);
  boost::sort::timsort(v6.begin(), v6.end(), std::less<int>());
  BOOST_CHECK(std::is_sorted(v6.begin(), v6.end()));
}

void test_stability() {
  typedef std::pair<int, int> KeyIdx;
  struct CmpFirst {
    bool operator()(const KeyIdx& a, const KeyIdx& b) const {
      return a.first < b.first;
    }
  };
  const int NElem = 10000;
  std::vector<KeyIdx> v1, v2;
  v1.reserve(NElem);
  for (int i = 0; i < NElem; ++i)
    v1.push_back(KeyIdx(i % 100, i));

  std::mt19937 my_rand(0);
  std::shuffle(v1.begin(), v1.end(), my_rand);

  v2 = v1;

  boost::sort::timsort(v1.begin(), v1.end(), CmpFirst());
  std::stable_sort(v2.begin(), v2.end(), CmpFirst());

  BOOST_REQUIRE(static_cast<int>(v1.size()) == NElem);
  for (int i = 0; i < NElem; ++i) {
    BOOST_CHECK(v1[i].first  == v2[i].first);
    BOOST_CHECK(v1[i].second == v2[i].second);
  }
}

void test_edge_cases()    { /* Story 2.2 */ }
void test_type_coverage() { /* Story 2.2 */ }
void test_adversarial()   { /* Story 2.2 */ }

int test_main(int, char*[]) {
  test_correctness();
  test_stability();
  test_edge_cases();
  test_type_coverage();
  test_adversarial();
  return 0;
}
