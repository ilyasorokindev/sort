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
#include <string>
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

void test_edge_cases() {
  typedef std::vector<int> Vec;

  // Empty range — no crash, no-op
  {
    Vec v;
    boost::sort::timsort(v.begin(), v.end());
    BOOST_CHECK(v.empty());
  }

  // Single element — no-op
  {
    Vec v(1, 42);
    boost::sort::timsort(v.begin(), v.end());
    BOOST_REQUIRE(v.size() == 1u);
    BOOST_CHECK(v[0] == 42);
  }

  // Two elements — already ordered
  {
    Vec v;
    v.push_back(1); v.push_back(2);
    boost::sort::timsort(v.begin(), v.end());
    BOOST_REQUIRE(v.size() == 2u);
    BOOST_CHECK(v[0] == 1);
    BOOST_CHECK(v[1] == 2);
  }

  // Two elements — reversed
  {
    Vec v;
    v.push_back(2); v.push_back(1);
    boost::sort::timsort(v.begin(), v.end());
    BOOST_REQUIRE(v.size() == 2u);
    BOOST_CHECK(v[0] == 1);
    BOOST_CHECK(v[1] == 2);
  }

  // All-equal elements (1000 x 7)
  {
    const int N = 1000;
    Vec v(N, 7);
    boost::sort::timsort(v.begin(), v.end());
    BOOST_REQUIRE(static_cast<int>(v.size()) == N);
    BOOST_CHECK(std::is_sorted(v.begin(), v.end()));
    for (int i = 0; i < N; ++i)
      BOOST_CHECK(v[i] == 7);
  }

  // Fully reversed range
  {
    const int N = 10000;
    Vec v;
    for (int i = N; i > 0; --i) v.push_back(i);
    boost::sort::timsort(v.begin(), v.end());
    BOOST_REQUIRE(static_cast<int>(v.size()) == N);
    BOOST_CHECK(std::is_sorted(v.begin(), v.end()));
  }
}

void test_type_coverage() {
  // int
  {
    std::vector<int> v;
    std::mt19937 rng(7);
    for (int i = 0; i < 1000; ++i) v.push_back(static_cast<int>(rng() % 1000));
    boost::sort::timsort(v.begin(), v.end());
    BOOST_REQUIRE(v.size() == 1000u);
    BOOST_CHECK(std::is_sorted(v.begin(), v.end()));
  }

  // std::string (lexicographic default sort)
  {
    const char* words[] = {
      "banana","apple","cherry","date","elderberry","fig","grape"
    };
    const int NW = 7;
    std::vector<std::string> v;
    std::mt19937 rng(42);
    for (int i = 0; i < 700; ++i)
      v.push_back(words[rng() % NW]);
    boost::sort::timsort(v.begin(), v.end());
    BOOST_REQUIRE(v.size() == 700u);
    BOOST_CHECK(std::is_sorted(v.begin(), v.end()));
  }

  // User-defined struct with custom comparator (FR-13)
  {
    struct Rec { int key; std::string val; };
    struct CmpKey {
      bool operator()(const Rec& a, const Rec& b) const {
        return a.key < b.key;
      }
    };
    std::vector<Rec> v;
    std::mt19937 rng(99);
    for (int i = 0; i < 500; ++i) {
      Rec r;
      r.key = static_cast<int>(rng() % 50);
      r.val = "x";
      v.push_back(r);
    }
    boost::sort::timsort(v.begin(), v.end(), CmpKey());
    BOOST_REQUIRE(v.size() == 500u);
    for (std::size_t i = 1; i < v.size(); ++i)
      BOOST_CHECK(!(v[i].key < v[i-1].key));
  }
}

void test_adversarial() {
  typedef std::vector<int> Vec;

  // --- A: Gallop trigger ---
  // Run1=[100..199], Run2=[0..99]; right run wins 100 consecutive → gallop activates.
  {
    const int N = 200;
    Vec v;
    for (int i = N/2; i < N; ++i) v.push_back(i);   // [100..199]
    for (int i = 0;   i < N/2; ++i) v.push_back(i);  // [0..99]
    boost::sort::timsort(v.begin(), v.end());
    BOOST_REQUIRE(static_cast<int>(v.size()) == N);
    BOOST_CHECK(std::is_sorted(v.begin(), v.end()));
  }

  // --- B: Both run-stack invariants violated simultaneously ---
  // Three runs: A(60), B(40), C(40). After pushing C: inv1 and inv2 both violated.
  {
    const int N = 140;
    Vec v;
    for (int i = 0;   i < 60;  ++i) v.push_back(i);          // Run-A: [0..59]
    for (int i = 200; i < 240; ++i) v.push_back(i);           // Run-B: [200..239]
    for (int i = 100; i < 140; ++i) v.push_back(i);           // Run-C: [100..139]
    boost::sort::timsort(v.begin(), v.end());
    BOOST_REQUIRE(static_cast<int>(v.size()) == N);
    BOOST_CHECK(std::is_sorted(v.begin(), v.end()));
  }

  // --- C: Minrun boundary N=63 ---
  // minrun(63)=63; entire range sorted by binary_insertion_sort, no merge exercised.
  {
    const int N = 63;
    Vec v;
    std::mt19937 rng(42);
    for (int i = 0; i < N; ++i) v.push_back(static_cast<int>(rng() % N));
    boost::sort::timsort(v.begin(), v.end());
    BOOST_REQUIRE(static_cast<int>(v.size()) == N);
    BOOST_CHECK(std::is_sorted(v.begin(), v.end()));
  }

  // --- D: Minrun boundary N=64 ---
  // minrun(64)=32; two runs of ~32 pushed and merged via merge path.
  {
    const int N = 64;
    Vec v;
    std::mt19937 rng(42);
    for (int i = 0; i < N; ++i) v.push_back(static_cast<int>(rng() % N));
    boost::sort::timsort(v.begin(), v.end());
    BOOST_REQUIRE(static_cast<int>(v.size()) == N);
    BOOST_CHECK(std::is_sorted(v.begin(), v.end()));
  }
}

int test_main(int, char*[]) {
  test_correctness();
  test_stability();
  test_edge_cases();
  test_type_coverage();
  test_adversarial();
  return 0;
}
