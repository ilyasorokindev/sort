//----------------------------------------------------------------------------
/// @file benchmark_strings.cpp
/// @brief Benchmark of several sort methods with strings
///
/// @author Copyright (c) 2017 Francisco José Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanying file LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt )
///
///         This program use for comparison purposes, the Threading Building
///         Blocks which license is the GNU General Public License, version 2
///         as  published  by  the  Free Software Foundation.
///
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <random>
#include <stdlib.h>
#include <vector>

#include <boost/sort/common/time_measure.hpp>
#include <boost/sort/common/file_vector.hpp>
#include "boost/sort/common/int_array.hpp"

#include <boost/sort/sort.hpp>


#define NMAXSTRING 10000000
const int NELEM_STR = 100000;

using namespace std;
namespace bsort = boost::sort;
namespace bsc = boost::sort::common;

using bsc::time_point;
using bsc::now;
using bsc::subtract_time;
using bsc::fill_vector_uint64;
using bsc::write_file_uint64;

using bsort::spinsort;
using bsort::flat_stable_sort;
using bsort::spreadsort::spreadsort;
using bsort::pdqsort;
using bsort::timsort;

void Generator_random (void);
void Generator_sorted (void);
void Generator_sorted_end (size_t n_last);
void Generator_sorted_middle (size_t n_last);
void Generator_reverse_sorted (void);
void Generator_reverse_sorted_end (size_t n_last);
void Generator_reverse_sorted_middle (size_t n_last);

int Test (std::vector <std::string> &B);
void RunTimsortStringBenchmark();
void TestTimsortStr(const std::vector<std::string>& B);


int main (int argc, char *argv [])
{
    RunTimsortStringBenchmark();

    cout << "\n\n";
    cout << "************************************************************\n";
    cout << "**                                                        **\n";
    cout << "**               B O O S T      S O R T                   **\n";
    cout << "**              S I N G L E    T H R E A D                **\n";
    cout << "**            S T R I N G S   B E N C H M A R K           **\n";
    cout << "**                                                        **\n";
    cout << "**       S O R T   O F  10 000 000   S T R I N G S        **\n";
    cout << "**                                                        **\n";
    cout << "************************************************************\n";
    cout << std::endl;

    cout << "[ 1 ] std::sort   [ 2 ] pdqsort          [ 3 ] std::stable_sort\n";
    cout << "[ 4 ] spinsort    [ 5 ] flat_stable_sort [ 6 ] spreadsort\n\n";
    cout << "                    |      |      |      |      |      |      |\n";
    cout << "                    | [ 1 ]| [ 2 ]| [ 3 ]| [ 4 ]| [ 5 ]| [ 6 ]|\n";
    cout << "--------------------+------+------+------+------+------+------+\n";
    std::string empty_line =
           "                    |      |      |      |      |      |      |\n";

    cout << "random              |";
    Generator_random ();

    cout << empty_line;
    cout << "sorted              |";
    Generator_sorted ();

    cout << "sorted + 0.1% end   |";
    Generator_sorted_end (NMAXSTRING / 1000);

    cout << "sorted +   1% end   |";
    Generator_sorted_end (NMAXSTRING / 100);

    cout << "sorted +  10% end   |";
    Generator_sorted_end (NMAXSTRING / 10);

    cout <<empty_line;
    cout << "sorted + 0.1% mid   |";
    Generator_sorted_middle (NMAXSTRING / 1000);

    cout << "sorted +   1% mid   |";
    Generator_sorted_middle (NMAXSTRING / 100);

    cout << "sorted +  10% mid   |";
    Generator_sorted_middle (NMAXSTRING / 10 );

    cout <<empty_line;
    cout << "reverse sorted      |";
    Generator_reverse_sorted ();

    cout << "rv sorted + 0.1% end|";
    Generator_reverse_sorted_end (NMAXSTRING / 1000);

    cout << "rv sorted +   1% end|";
    Generator_reverse_sorted_end (NMAXSTRING / 100);

    cout << "rv sorted +  10% end|";
    Generator_reverse_sorted_end (NMAXSTRING / 10);

    cout <<empty_line;
    cout << "rv sorted + 0.1% mid|";
    Generator_reverse_sorted_middle (NMAXSTRING / 1000);

    cout << "rv sorted +   1% mid|";
    Generator_reverse_sorted_middle (NMAXSTRING / 100);

    cout << "rv sorted +  10% mid|";
    Generator_reverse_sorted_middle (NMAXSTRING / 10);

    cout << "--------------------+------+------+------+------+------+------+\n";
    cout << endl<<endl ;
    return 0;
}

void Generator_random(void)
{
    std::vector <std::string> A;
    A.reserve (NMAXSTRING);
    A.clear ();
    if (bsc::fill_vector_string ("input.bin", A, NMAXSTRING) != 0)
    {
        std::cout << "Error in the input file\n";
        std::exit (EXIT_FAILURE);
    };
    Test (A);
};
void Generator_sorted (void)
{
    std::vector <std::string> A;
    A.reserve (NMAXSTRING);
    A.clear ();
    if (bsc::fill_vector_string ("input.bin", A, NMAXSTRING) != 0)
    {
        std::cout << "Error in the input file\n";
        std::exit (EXIT_FAILURE);
    };
    std::sort (A.begin (), A.end ());
    Test (A);
};

void Generator_sorted_end (size_t n_last)
{
    std::vector <std::string> A;
    A.reserve (NMAXSTRING);
    A.clear ();
    if (bsc::fill_vector_string ("input.bin", A, NMAXSTRING+ n_last) != 0)
    {
        std::cout << "Error in the input file\n";
        std::exit (EXIT_FAILURE);
    };
    std::sort (A.begin (), A.begin () + NMAXSTRING );
    Test (A);
};
void Generator_sorted_middle (size_t n_middle)
{
    assert (n_middle > 1 && NMAXSTRING >= (n_middle -1));
    vector <std::string> A, aux;
    A.reserve (NMAXSTRING + n_middle);
    aux.reserve (n_middle);

    if (bsc::fill_vector_string ("input.bin", A, NMAXSTRING + n_middle) != 0)
    {
        std::cout << "Error in the input file\n";
        std::exit (EXIT_FAILURE);
    };
    for (size_t i = 0; i < n_middle; ++i)   aux.push_back (std::move (A [i]));

    std::sort (A.begin () + n_middle, A.end ());
    //------------------------------------------------------------------------
    // To insert n_middle elements, must have (n_middle - 1) intervals between
    // them. The size of the interval is step
    // The elements after the last element of aux don't need to be moved
    //-------------------------------------------------------------------------
    size_t step = NMAXSTRING / (n_middle - 1);
    A [0] = std::move (aux [0]);
    size_t pos_read = n_middle, pos_write = 1;

    for (size_t i = 1; i < n_middle; ++i)
    {
        for (size_t k = 0 ; k < step; ++k)
            A [pos_write ++] = std::move (A [pos_read ++]);
        A [pos_write ++] = std::move (aux [i]);    
    };
    aux.clear ();
    aux.reserve (0);
    Test (A);
};
void Generator_reverse_sorted (void)
{
    std::vector <std::string> A;
    A.reserve (NMAXSTRING);
    {
        std::vector <std::string> B;
        B.reserve (NMAXSTRING);
        if (bsc::fill_vector_string ("input.bin", B, NMAXSTRING) != 0)
        {
            std::cout << "Error in the input file\n";
            std::exit (EXIT_FAILURE);
        };
        std::sort (B.begin(), B.end());
        A.clear ();
        for (size_t i = 0; i < NMAXSTRING; ++i)
            A.push_back (B [NMAXSTRING - 1 - i]);
    };
    Test (A);
};
void Generator_reverse_sorted_end (size_t n_last)
{
    std::vector <std::string> A;
    A.reserve (NMAXSTRING);
    A.clear ();
    if (bsc::fill_vector_string ("input.bin", A, NMAXSTRING + n_last) != 0)
    {
        std::cout << "Error in the input file\n";
        std::exit (EXIT_FAILURE);
    };
    std::sort (A.begin (), A.begin () + NMAXSTRING);
    for (size_t i = 0; i < (NMAXSTRING >> 1); ++i)
        std::swap (A [i], A [NMAXSTRING - 1 - i]);

    Test (A);
};
void Generator_reverse_sorted_middle (size_t n_middle)
{
    assert (n_middle > 1 && NMAXSTRING >= (n_middle -1));
    vector <std::string> A, aux;
    A.reserve (NMAXSTRING + n_middle);
    aux.reserve (n_middle);

    if (bsc::fill_vector_string ("input.bin", A, NMAXSTRING + n_middle) != 0)
    {
        std::cout << "Error in the input file\n";
        std::exit (EXIT_FAILURE);
    };
    for (size_t i = 0; i < n_middle; ++i)   aux.push_back (std::move (A [i]));

    std::sort (A.begin () + n_middle, A.end ());
    
    size_t pos1 = n_middle, pos2 = A.size () - 1;
    for (size_t i = 0; i < (NMAXSTRING >> 1); ++i)
        std::swap (A [pos1 ++], A [pos2 --]);
        
    //------------------------------------------------------------------------
    // To insert n_middle elements, must have (n_middle - 1) intervals between
    // them. The size of the interval is step
    // The elements after the last element of aux don't need to be moved
    //-------------------------------------------------------------------------
    size_t step = NMAXSTRING / (n_middle - 1);
    A [0] = std::move (aux [0]);
    size_t pos_read = n_middle, pos_write = 1;

    for (size_t i = 1; i < n_middle; ++i)
    {
        for (size_t k = 0 ; k < step; ++k)
            A [pos_write ++] = std::move (A [pos_read ++]);
        A [pos_write ++] = std::move (aux [i]);    
    };
    aux.clear ();
    aux.reserve (0);
    Test (A);
};

int Test (std::vector <std::string> &B)
{   
    //---------------------------- begin -----------------------------
    std::less <std::string> comp;
    double duration;
    time_point start, finish;
    std::vector <std::string> A (B);
    std::vector <double> V;

    A = B;
    start = now ();
    std::sort (A.begin (), A.end (), comp);
    finish = now ();
    duration = subtract_time (finish, start);
    V.push_back (duration);

    A = B;
    start = now ();
    pdqsort (A.begin (), A.end (), comp);
    finish = now ();
    duration = subtract_time (finish, start);
    V.push_back (duration);

    A = B;
    start = now ();
    std::stable_sort (A.begin (), A.end (), comp);
    finish = now ();
    duration = subtract_time (finish, start);
    V.push_back (duration);

    A = B;
    start = now ();
    spinsort (A.begin (), A.end (), comp);
    finish = now ();
    duration = subtract_time (finish, start);
    V.push_back (duration);

    A = B;
    start = now ();
    flat_stable_sort (A.begin (), A.end (), comp);
    finish = now ();
    duration = subtract_time (finish, start);
    V.push_back (duration);

    A = B;
    start = now ();
    spreadsort (A.begin (), A.end ());
    finish = now ();
    duration = subtract_time (finish, start);
    V.push_back (duration);

    //-----------------------------------------------------------------------
    // printing the vector
    //-----------------------------------------------------------------------
    std::cout << std::setprecision (2) << std::fixed;
    for (uint32_t i = 0; i < V.size () ; ++i)
    {   
        std::cout << std::right << std::setw (5) << V [i] << " |";
    };
    std::cout <<std::endl;
    return 0;
};
void TestTimsortStr(const std::vector<std::string>& B)
{
    std::less<std::string> comp;
    double duration;
    time_point start, finish;
    std::vector<std::string> A;
    std::vector<double> V;

    A = B;
    start = now();
    timsort(A.begin(), A.end(), comp);
    finish = now();
    duration = subtract_time(finish, start);
    V.push_back(duration);

    A = B;
    start = now();
    spinsort(A.begin(), A.end(), comp);
    finish = now();
    duration = subtract_time(finish, start);
    V.push_back(duration);

    A = B;
    start = now();
    flat_stable_sort(A.begin(), A.end(), comp);
    finish = now();
    duration = subtract_time(finish, start);
    V.push_back(duration);

    A = B;
    start = now();
    stable_sort(A.begin(), A.end(), comp);
    finish = now();
    duration = subtract_time(finish, start);
    V.push_back(duration);

    cout << std::setprecision(2) << std::fixed;
    for (uint32_t i = 0; i < V.size(); ++i)
        cout << std::right << std::setw(5) << V[i] << " |";
    cout << endl;
}
void RunTimsortStringBenchmark()
{
    cout << "\n";
    cout << "************************************************************\n";
    cout << "**                                                        **\n";
    cout << "**  Timsort vs. Stable-Sort Algorithms (N=100K, string)   **\n";
    cout << "**   timsort | spinsort | flat_stable_sort | stable_sort  **\n";
    cout << "**                                                        **\n";
    cout << "************************************************************\n";
    cout << endl;
    cout << "[ 1 ] timsort  [ 2 ] spinsort  [ 3 ] flat_stable_sort  [ 4 ] std::stable_sort\n\n";
    cout << "                    |      |      |      |      |\n";
    cout << "                    | [ 1 ]| [ 2 ]| [ 3 ]| [ 4 ]|\n";
    cout << "--------------------+------+------+------+------+\n";

    const char charset[] = "abcdefghijklmnopqrstuvwxyz";

    // random
    {
        vector<std::string> A;
        A.reserve(NELEM_STR);
        std::mt19937 rng(123);
        for (int i = 0; i < NELEM_STR; ++i) {
            std::string s(8, ' ');
            for (int j = 0; j < 8; ++j)
                s[j] = charset[rng() % 26];
            A.push_back(s);
        }
        cout << "random              |";
        TestTimsortStr(A);
    }

    // already-sorted
    {
        vector<std::string> A;
        A.reserve(NELEM_STR);
        std::mt19937 rng(123);
        for (int i = 0; i < NELEM_STR; ++i) {
            std::string s(8, ' ');
            for (int j = 0; j < 8; ++j)
                s[j] = charset[rng() % 26];
            A.push_back(s);
        }
        std::sort(A.begin(), A.end());
        cout << "already-sorted      |";
        TestTimsortStr(A);
    }

    // reverse-sorted
    {
        vector<std::string> A;
        A.reserve(NELEM_STR);
        std::mt19937 rng(123);
        for (int i = 0; i < NELEM_STR; ++i) {
            std::string s(8, ' ');
            for (int j = 0; j < 8; ++j)
                s[j] = charset[rng() % 26];
            A.push_back(s);
        }
        std::sort(A.begin(), A.end());
        std::reverse(A.begin(), A.end());
        cout << "reverse-sorted      |";
        TestTimsortStr(A);
    }

    // Nearly-sorted: sorted array, N*0.05 random adjacent swaps, mt19937(42)
    {
        vector<std::string> A;
        A.reserve(NELEM_STR);
        std::mt19937 rng(123);
        for (int i = 0; i < NELEM_STR; ++i) {
            std::string s(8, ' ');
            for (int j = 0; j < 8; ++j)
                s[j] = charset[rng() % 26];
            A.push_back(s);
        }
        std::sort(A.begin(), A.end());
        std::mt19937 rng2(42);
        const int nswaps = static_cast<int>(NELEM_STR * 0.05);
        for (int k = 0; k < nswaps; ++k) {
            int idx = static_cast<int>(rng2() % (NELEM_STR - 1));
            std::swap(A[idx], A[idx + 1]);
        }
        cout << "nearly-sorted       |";
        TestTimsortStr(A);
    }

    // pipe-organ: ascending first half, descending second half
    {
        vector<std::string> sorted_base;
        sorted_base.reserve(NELEM_STR);
        std::mt19937 rng(123);
        for (int i = 0; i < NELEM_STR; ++i) {
            std::string s(8, ' ');
            for (int j = 0; j < 8; ++j)
                s[j] = charset[rng() % 26];
            sorted_base.push_back(s);
        }
        std::sort(sorted_base.begin(), sorted_base.end());
        const int half = NELEM_STR / 2;
        vector<std::string> A;
        A.reserve(NELEM_STR);
        for (int i = 0; i < half; ++i)
            A.push_back(sorted_base[i]);
        for (int i = half - 1; i >= 0; --i)
            A.push_back(sorted_base[i]);
        cout << "pipe-organ          |";
        TestTimsortStr(A);
    }

    cout << "--------------------+------+------+------+------+\n";
    cout << endl;
}

