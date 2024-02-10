#ifndef DRIVER_HELPER_H
#define DRIVER_HELPER_H

#include <map>
#include <mutex>
#include <iostream>
#include <string>
#include <utility>

std::mutex coutMutex;


enum class TestMode { None, Errors, Verbose };
// using const ints for denoting the operations because I don't want to use static_cast to turn
// numbers from the RNG to enum values
const int Put = 0;
const int Remove = 1;
const int Get = 2;
const int TypesOfOps = 3;

struct Operation
{
    int op;
    std::pair<K,V> elem;
};

std::vector<Operation> generateRandomOps(unsigned n, float ratioPut, float ratioRemove, float ratioGet)
{
    (void)ratioGet;
    std::random_device rd;
    std::mt19937 gen(rd());
    unsigned numPut = static_cast<unsigned>( n*ratioPut );
    unsigned numRemove = static_cast<unsigned>( n*ratioRemove );
    unsigned numGet = n - numPut - numRemove;
    unsigned keyPoolSize = std::max( std::max(numPut,numRemove), numGet );
    std::vector<K> keyPool( keyPoolSize );
    std::iota( keyPool.begin(), keyPool.end(), 1 );
    std::vector<Operation> result;
    
    std::shuffle( keyPool.begin(), keyPool.end(), gen );
    for (unsigned i = 0; i < numPut; ++i)
        result.push_back( {Put, std::make_pair(keyPool[i], keyPool[i])} );

    std::shuffle( keyPool.begin(), keyPool.end(), gen );
    for (unsigned i = 0; i < numRemove; ++i)
        result.push_back( {Remove, std::make_pair(keyPool[i], keyPool[i])} );

    std::shuffle( keyPool.begin(), keyPool.end(), gen );
    for (unsigned i = 0; i < numGet; ++i)
        result.push_back( {Get, std::make_pair(keyPool[i], keyPool[i])} );
    
    std::shuffle( result.begin(), result.end(), gen );
    return result;
}

bool get(ConcurrentBSTMap& bst, K k, V& v)
{
    std::pair<Result,V> res(Result::Null, 0);
    do
    {
        res = bst.get(k);
    }
    while (res.first == Result::Retry);
    if (res.first == Result::Success) v = res.second;
    return res.first == Result::Success;
}

void put(ConcurrentBSTMap& bst, K k, V v)
{
    std::pair<Result,V> res(Result::Null, 0);
    do
    {
        res = bst.put(k, v);
    }
    while (res.first == Result::Retry);
}

void remove(ConcurrentBSTMap& bst, K k)
{
    std::pair<Result,V> res(Result::Null, 0);
    do
    {
        res = bst.remove(k);
    }
    while (res.first == Result::Retry);
}

/*****************  overloaded for std::map  *****************/

bool get(std::map<K,V>& bst, K k, V& v)
{
    std::map<K,V>::iterator it = bst.find(k);
    if (it != bst.end()) v = it->second;
    return it != bst.end();
}

void put(std::map<K,V>& bst, K k, V v)
{
    bst[k] = v;
}

void remove(std::map<K,V>& bst, K k)
{
    bst.erase(k);
}

template <typename BST, typename Container>
bool checkElemsInBST(BST& bst, const Container& container)
{
    typename Container::const_iterator rangeBegin = container.cbegin();
    typename Container::const_iterator rangeEnd = container.cend();
    bool success = false;
    V value = 0;
    while (rangeBegin != rangeEnd)
    {
        success = get(bst, rangeBegin->first, value);
        if (!success || value != rangeBegin->second) return false;
        ++rangeBegin;
    }
    return true;
}

template <typename BST, typename Container>
bool checkElemsNotInBST(BST& bst, const Container& container)
{
    typename Container::const_iterator rangeBegin = container.cbegin();
    typename Container::const_iterator rangeEnd = container.cend();
    bool success = false;
    V value = 0;
    while (rangeBegin != rangeEnd)
    {
        success = get(bst, rangeBegin->first, value);
        if (success) return false;
        ++rangeBegin;
    }
    return true;
}

template <typename BST>
void run(BST& bst, const std::vector<Operation>& ops, TestMode v, bool concurrent)
{
    V value = 0;
    bool getRes = false;
    bool res1 = false, res2 = false;
    std::map<K,V> remainingTracker;
    std::map<K,V> removedTracker;
    std::vector<Operation>::const_iterator it = ops.cbegin();
    switch (v)
    {
///////////////////////////////////////////////////////////////////////////////
        case TestMode::None:
        for (; it != ops.cend(); ++it)
        {
            switch (it->op)
            {
                case Put:
                put(bst, it->elem.first, it->elem.second);
                break;

                case Remove:
                remove(bst, it->elem.first);
                break;

                case Get:
                get(bst, it->elem.first, value);
                break;
            }
        }
        break;
///////////////////////////////////////////////////////////////////////////////
        case TestMode::Errors:
        for (; it != ops.cend(); ++it)
        {
            switch (it->op)
            {
                case Put:
                put(bst, it->elem.first, it->elem.second);
                remainingTracker[it->elem.first] = it->elem.second;
                removedTracker.erase(it->elem.first);
                break;

                case Remove:
                remove(bst, it->elem.first);
                remainingTracker.erase(it->elem.first);
                removedTracker[it->elem.first] = 1;
                break;

                case Get:
                get(bst, it->elem.first, value);
                break;
            }
        }
        {
            std::unique_lock<std::mutex> coutLock(coutMutex);
            res1 = checkElemsInBST(bst, remainingTracker);
            res2 = checkElemsNotInBST(bst, removedTracker);
            if (concurrent && !res1)
                std::cout << "[Thread #" << std::hex << std::this_thread::get_id() << "] ";
            if (!res1)
                std::cout << "Found some elements that shouldn't be in the BST\n";
            if (concurrent && !res2)
                std::cout << "[Thread #" << std::hex << std::this_thread::get_id() << "] ";
            if (!res2)
                std::cout << "Could not find some elements that should be in the BST\n";
            if (res1 && res2)
            {
                if (concurrent)
                    std::cout << "[Thread #" << std::hex << std::this_thread::get_id() << "] ";
                std::cout << "\nAll good\n";
            }
        }
        break;
///////////////////////////////////////////////////////////////////////////////
        case TestMode::Verbose:
        for (; it != ops.cend(); ++it)
        {
            switch (it->op)
            {
                case Put:
                put(bst, it->elem.first, it->elem.second);
                {
                    std::unique_lock<std::mutex> coutLock(coutMutex);
                    if (concurrent)
                        std::cout << "[Thread #" << std::hex << std::this_thread::get_id() << "] ";
                    std::cout << std::dec << "put(" << it->elem.first << ", " << it->elem.second << ")\n";
                }
                break;

                case Remove:
                remove(bst, it->elem.first);
                {
                    std::unique_lock<std::mutex> coutLock(coutMutex);
                    if (concurrent)
                        std::cout << "[Thread #" << std::hex << std::this_thread::get_id() << "] ";
                    std::cout << std::dec << "remove(" << it->elem.first << ")\n";
                }
                break;

                case Get:
                getRes = get(bst, it->elem.first, value);
                {
                    std::unique_lock<std::mutex> coutLock(coutMutex);
                    if (concurrent)
                        std::cout << "[Thread #" << std::hex << std::this_thread::get_id() << "] ";
                    if (getRes)
                        std::cout << std::dec << "get(" << it->elem.first << ") = " << value << "\n";
                    else
                        std::cout << std::dec << "get(" << it->elem.first << ") = null\n";
                }
                break;
            }
        }
        {
            std::unique_lock<std::mutex> coutLock(coutMutex);
            res1 = checkElemsInBST(bst, remainingTracker);
            res2 = checkElemsNotInBST(bst, removedTracker);
            if (concurrent && !res1)
                std::cout << "[Thread #" << std::hex << std::this_thread::get_id() << "] ";
            if (!res1)
                std::cout << "Found some elements that shouldn't be in the BST\n";
            if (concurrent && !res2)
                std::cout << "[Thread #" << std::hex << std::this_thread::get_id() << "] ";
            if (!res2)
                std::cout << "Could not find some elements that should be in the BST\n";
            if (res1 && res2)
            {
                if (concurrent)
                    std::cout << "[Thread #" << std::hex << std::this_thread::get_id() << "] ";
                std::cout << "\nAll good\n";
            }
        }
        break;
    }
}

#endif