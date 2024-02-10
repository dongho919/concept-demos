#include <iostream>
#include <string>
#include <thread>
#include <random>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <sstream>
#include <iomanip>

#include "concurrentbst.h"
#include "driver-helper.h"

const int numCores = (std::thread::hardware_concurrency() > 0 ? static_cast<int>(std::thread::hardware_concurrency()) : 4);

// test0 (small): put 6-4-7-3-1-2-5-8-9-0, check 0-9 are all in it
void test0()
{
    std::cout << "-------------- TEST0 --------------\n";
    std::vector<Operation> operations
    {
        {Put, std::make_pair(6, 6)},
        {Put, std::make_pair(4, 4)},
        {Put, std::make_pair(7, 7)},
        {Put, std::make_pair(3, 3)},
        {Put, std::make_pair(1, 1)},
        {Put, std::make_pair(2, 2)},
        {Put, std::make_pair(5, 5)},
        {Put, std::make_pair(8, 8)},
        {Put, std::make_pair(9, 9)},
        {Put, std::make_pair(0, 0)}
    };

    ConcurrentBSTMap bst;
    run(bst, operations, TestMode::Verbose, false);
}

void test1()
{
    std::cout << "-------------- TEST1 --------------\n";
    std::vector<Operation> operations
    {
        {Put, std::make_pair(6, 6)},
        {Put, std::make_pair(4, 4)},
        {Put, std::make_pair(7, 7)},
        {Put, std::make_pair(3, 3)},
        {Put, std::make_pair(1, 1)},
        {Put, std::make_pair(2, 2)},
        {Put, std::make_pair(5, 5)},
        {Put, std::make_pair(8, 8)},
        {Put, std::make_pair(9, 9)},
        {Put, std::make_pair(0, 0)},

        {Put, std::make_pair(6, -6)},
        {Put, std::make_pair(4, -4)},
        {Put, std::make_pair(7, -7)},
        {Put, std::make_pair(3, -3)},
        {Put, std::make_pair(1, -1)},
        {Put, std::make_pair(2, -2)},
        {Put, std::make_pair(5, -5)},
        {Put, std::make_pair(8, -8)},
        {Put, std::make_pair(9, -9)},
        {Put, std::make_pair(0, 0)}
    };

    ConcurrentBSTMap bst;
    run(bst, operations, TestMode::Verbose, false);
}

void test2()
{
    std::cout << "-------------- TEST2 --------------\n";
    std::vector<Operation> operations
    {
        {Put, std::make_pair(6, 6)},
        {Put, std::make_pair(4, 4)},
        {Put, std::make_pair(7, 7)},
        {Put, std::make_pair(3, 3)},
        {Put, std::make_pair(1, 1)},
        {Put, std::make_pair(2, 2)},
        {Put, std::make_pair(5, 5)},
        {Put, std::make_pair(8, 8)},
        {Put, std::make_pair(9, 9)},
        {Put, std::make_pair(0, 0)},

        {Remove, std::make_pair(2, 0)},
        {Remove, std::make_pair(5, 0)},
        {Remove, std::make_pair(8, 0)},
        {Remove, std::make_pair(9, 0)},
        {Remove, std::make_pair(0, 0)}
    };

    ConcurrentBSTMap bst;
    run(bst, operations, TestMode::Verbose, false);
}

void test3()
{
    std::cout << "-------------- TEST3 --------------\n";
    std::vector<Operation> operations
    {
        {Remove, std::make_pair(1, 0)},
        {Remove, std::make_pair(2, 0)},
        {Remove, std::make_pair(3, 0)},
        {Remove, std::make_pair(4, 0)},
        {Remove, std::make_pair(5, 0)}
    };

    ConcurrentBSTMap bst;
    run(bst, operations, TestMode::Verbose, false);
}

void test4()
{
    std::cout << "-------------- TEST4 --------------\n";
    std::vector<Operation> operations
    {
        {Put, std::make_pair(6, 6)},
        {Put, std::make_pair(4, 4)},
        {Put, std::make_pair(7, 7)},
        {Put, std::make_pair(3, 3)},
        {Put, std::make_pair(1, 1)},
        {Put, std::make_pair(2, 2)},
        {Put, std::make_pair(5, 5)},
        {Put, std::make_pair(8, 8)},
        {Put, std::make_pair(9, 9)},
        {Put, std::make_pair(0, 0)},

        {Remove, std::make_pair(0, 0)},
        {Remove, std::make_pair(1, 0)},
        {Remove, std::make_pair(2, 0)},
        {Remove, std::make_pair(3, 0)},
        {Remove, std::make_pair(4, 0)},
        {Remove, std::make_pair(5, 0)},
        {Remove, std::make_pair(6, 0)},
        {Remove, std::make_pair(7, 0)},
        {Remove, std::make_pair(8, 0)},
        {Remove, std::make_pair(9, 0)},

        {Remove, std::make_pair(9, 0)},
        {Remove, std::make_pair(8, 0)},
        {Remove, std::make_pair(7, 0)},
        {Remove, std::make_pair(6, 0)},
        {Remove, std::make_pair(5, 0)},
        {Remove, std::make_pair(4, 0)},
        {Remove, std::make_pair(3, 0)},
        {Remove, std::make_pair(2, 0)},
        {Remove, std::make_pair(1, 0)},
        {Remove, std::make_pair(0, 0)}
    };

    ConcurrentBSTMap bst;
    run(bst, operations, TestMode::Verbose, false);
}

void test5()
{
    std::cout << "-------------- TEST5 --------------\n";

    const int numThreads = numCores;
    const int numPutsPerThread = 100; 
    const int numRemovesPerThread = 50;
    const int numGetsPerThread = 100;

    ConcurrentBSTMap bst;

    // the threads will move through the three phases of testing in lockstep
    std::vector<Operation> puts = generateRandomOps(numPutsPerThread * numThreads, 1, 0, 0);
    std::vector<Operation> removes = generateRandomOps(numRemovesPerThread * numThreads, 0, 1, 0);
    std::vector<Operation> gets = generateRandomOps(numGetsPerThread * numThreads, 0, 0, 1);

    // divvy the operations for individual threads
    std::vector<std::vector<Operation> > distPuts;
    std::vector<std::vector<Operation> > distRemoves;
    std::vector<std::vector<Operation> > distGets;
    for (int i = 0; i < numThreads; ++i)
    {
        distPuts.push_back( std::vector<Operation>(puts.begin() + i*numPutsPerThread, puts.begin() + (i+1)*numPutsPerThread) );
        distRemoves.push_back( std::vector<Operation>(removes.begin() + i*numRemovesPerThread, removes.begin() + (i+1)*numRemovesPerThread) );
        distGets.push_back( std::vector<Operation>(gets.begin() + i*numGetsPerThread, gets.begin() + (i+1)*numGetsPerThread) );
    }

    std::map<K,V> tracker;

    // phase 1 part 1: concurrent put operations
    std::cout << numThreads << " threads * " << numPutsPerThread << " puts/thread = " << (numThreads * numPutsPerThread) << " puts...\n";
    run(tracker, puts, TestMode::None, false);
    std::vector<std::thread> threads;
    for (const std::vector<Operation>& ops : distPuts)
        threads.push_back( std::thread(run<ConcurrentBSTMap>, std::ref(bst), ops, TestMode::None, true) );
    for (std::thread& th : threads)
        th.join();
    threads.clear();

    // phase 1 part 2: check that all concurrent put operations succeeded
    bool res1 = checkElemsInBST(bst, tracker);
    if (res1)
        std::cout << "All puts successful\n";
    else
        std::cout << "Couldn't find some elements that should be in the BST\n";

    // phase 2 part 1: concurrent remove operations
    std::cout << numThreads << " threads * " << numRemovesPerThread << " removes/thread = " << (numThreads * numRemovesPerThread) << " removes...\n";
    std::map<K,V> removed(tracker);
    run(tracker, removes, TestMode::None, false);
    for (const std::vector<Operation>& ops : distRemoves)
        threads.push_back( std::thread(run<ConcurrentBSTMap>, std::ref(bst), ops, TestMode::None, true) );
    for (std::thread& th : threads)
        th.join();
    threads.clear();

    // phase 2 part 2: check that all concurrent remove operations succeeded
    for (const std::pair<K,V>& remaining : tracker)
        removed.erase(remaining.first);
    bool res2 = checkElemsInBST(bst, tracker);
    bool res3 = checkElemsNotInBST(bst, removed);
    if (res2 && res3)
        std::cout << "All removes successful\n";
    else if (!res2)
        std::cout << "Couldn't find some elements that should be in the BST\n";
    else
        std::cout << "Found some elements that shouldn't be in the BST\n";

    // phase 3: concurrent get operations (no modification)
    std::cout << numThreads << " threads * " << numGetsPerThread << " gets/thread = " << (numThreads * numGetsPerThread) << " gets...\n";
    for (const std::vector<Operation>& ops : distGets)
        threads.push_back( std::thread(run<ConcurrentBSTMap>, std::ref(bst), ops, TestMode::None, true) );
    for (std::thread& th : threads)
        th.join();
    threads.clear();
    
    if (res1 && res2 && res3)
        std::cout << "\nAll good\n";
}

void test6()
{
    std::cout << "-------------- TEST6 --------------\n";

    const int numThreads = numCores;
    const int numOpsPerThread = 1000;
    const float ratioPut = 0.333;
    const float ratioRemove = 0.333;
    const float ratioGet = 0.334;
    const int numPutsPerThread = static_cast<int>(numOpsPerThread * ratioPut);
    const int numRemovesPerThread = static_cast<int>(numOpsPerThread * ratioRemove);
    const int numGetsPerThread = numOpsPerThread - numPutsPerThread - numRemovesPerThread;

    ConcurrentBSTMap bst;

    std::cout << numThreads << " threads * " << numPutsPerThread << " puts/thread = " << (numThreads * numPutsPerThread) << " puts...\n";
    std::cout << numThreads << " threads * " << numRemovesPerThread << " removes/thread = " << (numThreads * numRemovesPerThread) << " removes...\n";
    std::cout << numThreads << " threads * " << numGetsPerThread << " gets/thread = " << (numThreads * numGetsPerThread) << " gets...\n";
    std::cout << "All " << numThreads << " threads perform " << numOpsPerThread << " operations each, in the same key range\n";

    std::vector<std::vector<Operation> > concurrentOps;
    for (int i = 0; i < numThreads; ++i)
        concurrentOps.push_back( generateRandomOps(numOpsPerThread, ratioPut, ratioRemove, ratioGet) );

    std::vector<std::thread> threads;
    for (const std::vector<Operation>& ops : concurrentOps)
        threads.push_back( std::thread(run<ConcurrentBSTMap>, std::ref(bst), ops, TestMode::None, true) );
    for (std::thread& th : threads)
        th.join();
    threads.clear();

    std::cout << "\nAll good\n";
}

void timeTest(int numOpsPerThread, int numThreads, float ratioPut, float ratioRemove, float ratioGet)
{
    std::cout << (numThreads * numOpsPerThread) << " operations with (put, remove, get) frequencies of ("
              << std::setprecision(4) << ratioPut << ", " << ratioRemove << ", " << ratioGet << ")\n";

    std::vector<Operation> allOps = generateRandomOps(numOpsPerThread * numThreads, ratioPut, ratioRemove, ratioGet);
    std::vector<std::vector<Operation> > distOps;
    for (int i = 0; i < numThreads; ++i)
        distOps.emplace_back(allOps.begin() + i*numOpsPerThread, allOps.begin() + (i+1)*numOpsPerThread);

    auto start1 = std::chrono::high_resolution_clock::now();
    ConcurrentBSTMap bst;
    std::vector<std::thread> threads;
    for (const std::vector<Operation> ops : distOps)
        threads.push_back( std::thread(run<ConcurrentBSTMap>, std::ref(bst), ops, TestMode::None, true) );
    for (std::thread& th : threads)
        th.join();
    auto stop1 = std::chrono::high_resolution_clock::now();

    auto start2 = std::chrono::high_resolution_clock::now();
    std::map<K,V> comparison;
    run(comparison, allOps, TestMode::None, false);
    auto stop2 = std::chrono::high_resolution_clock::now();

    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(stop1 - start1);
    auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(stop2 - start2);

    std::stringstream threaded;
    threaded << numThreads << "-threaded ConcurrentBSTMap = ";
    std::cout << std::setw(33) << threaded.str()
              << std::setw(7) << duration1.count() << " microseconds\n"
              << std::setw(33) << "Single-threaded std::map = "
              << std::setw(7) << duration2.count() << " microseconds\n\n"
              << "Performance improvement = " << std::setprecision(4) << ( duration2.count() / static_cast<double>(duration1.count()) ) << std::endl;
}

void test7()
{
    std::cout << "-------------- TEST7 --------------\n";
    timeTest(50000, numCores, 0.333, 0.333, 0.334);
}

void test8()
{
    std::cout << "-------------- TEST8 --------------\n";
    timeTest(50000, numCores, 0.8, 0.1, 0.1);
}

void test9()
{
    std::cout << "-------------- TEST9 --------------\n";
    timeTest(50000, numCores, 0.1, 0.8, 0.1);
}

void test10()
{
    std::cout << "-------------- TEST10 -------------\n";
    timeTest(50000, numCores, 0.1, 0.1, 0.8);
}

void test11()
{
    std::cout << "-------------- TEST11 -------------\n";
    timeTest(10000000, numCores / 2, 0.333, 0.333, 0.334);
    std::cout << "\n";
    timeTest(10000000 / 2, numCores, 0.333, 0.333, 0.334);
    std::cout << "\n";
    timeTest(10000000 / 4, numCores * 2, 0.333, 0.333, 0.334);
    std::cout << "\n";
    timeTest(10000000 / 8, numCores * 4, 0.333, 0.333, 0.334);
    std::cout << "\n";
    timeTest(10000000 / 16, numCores * 8, 0.333, 0.333, 0.334);
    std::cout << "\n";
}


void (*pTests[])() = { test0, test1, test2, test3, test4, test5, test6, test7, test8, test9, test10, test11 };

int main(int argc, char** argv)
{
    std::stringstream description;
    description << "Run \"" << argv[0] << " [test #]\" to run a specific test. The tests are as follows:\n"
                << "0:        (small) put 10 nodes, check that all 10 of them are in the BST\n"
                << "1:        (small) put 10 nodes then update all of their values, check that they were updated correctly\n"
                << "2:        (small) put 10 nodes then remove 5, check that the removes were performed correctly\n"
                << "3:        (small) call remove 5 times on an empty BST, check that nothing unusual happens\n"
                << "4:        (small) put 10 nodes then remove all of them *twice*, check that the removes were performed correctly\n"
                << "-------------------------------------------------- THE REAL TESTS(TM) --------------------------------------------------\n"
                << "5:  (correctness) " << numCores << " threads perform put, remove, get to check for correctness (no contention for same nodes)\n"
                << "6:       (stress) " << numCores << " threads perform put, remove, get to check for deadlocks or infinite loops (contention for same nodes)\n"
                << "7:  (performance) " << numCores << "-threaded concurrent BST vs single-threaded std::map with operation frequencies (1, 1, 1)\n"
                << "8:  (performance) " << numCores << "-threaded concurrent BST vs single-threaded std::map with operation frequencies (8, 1, 1)\n"
                << "9:  (performance) " << numCores << "-threaded concurrent BST vs single-threaded std::map with operation frequencies (1, 8, 1)\n"
                << "10: (performance) " << numCores << "-threaded concurrent BST vs single-threaded std::map with operation frequencies (1, 1, 8)\n"
                << "---------------------------------------- FINDING THE SWEET SPOT (HUGE AND SLOW) ----------------------------------------\n"
                << "11: (performance) " << (numCores / 2) << "-, " << numCores << "-, " << (numCores * 2) << "-, " << (numCores * 4) << "-, and " << (numCores * 8)
                << "-threaded concurrent BST vs single-threaded std::map with operation frequencies (1, 1, 1)\n";
    if (argc != 2)
    {
        std::cout << description.str() << std::endl;
        return 1;
    }
    int testNum = std::atoi(argv[1]);
    pTests[testNum]();
    return 0;
}