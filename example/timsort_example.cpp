// boost::sort::timsort usage example
//
// Copyright (c) 2026 Ilya Sorokin
// Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//      http://www.boost.org/LICENSE_1_0.txt)

#include <boost/sort/timsort/timsort.hpp>
#include <iostream>
#include <vector>

int main() {
  // Default overload: sort ascending using operator<
  std::vector<int> v = {5, 3, 1, 4, 2};
  boost::sort::timsort(v.begin(), v.end());
  for (int x : v) std::cout << x << ' ';
  std::cout << '\n';

  // Custom comparator overload: sort descending
  std::vector<int> w = {5, 3, 1, 4, 2};
  boost::sort::timsort(w.begin(), w.end(), std::greater<int>());
  for (int x : w) std::cout << x << ' ';
  std::cout << '\n';

  return 0;
}
