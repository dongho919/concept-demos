#ifndef CONCURRENTBST_H
#define CONCURRENTBST_H

#include <memory>
#include <utility>
#include <mutex>
#include <iostream>
#include <vector>

typedef int K;
typedef int V;

struct Node;
//typedef struct Node* NodePtr;
typedef Node* NodePtr;

struct Node
{
    std::mutex m;
    long version;
    const K key;
    V value;
    NodePtr parent;
    NodePtr left;
    NodePtr right;

    Node(K k = std::numeric_limits<K>::min())
        : m(), version(0), key(k), value(0), parent(nullptr), left(nullptr), right(nullptr) {}
    Node(K k, V v, const NodePtr& par, long nodeV, const NodePtr& l, const NodePtr& r)
        : m(), version(nodeV), key(k), value(v), parent(par), left(l), right(r) {}
    NodePtr& child(int dir)
    {
        if (dir == -1)
            return left;
        else
            return right;
    }
    void print(int depth) const
    {
        
        std::cerr << "(" << key << ", " << value << ") [depth=" << depth << "]\n";
        if (left) left->print(depth + 1);
        if (right) right->print(depth + 1);
    }
};

enum class Result { Null, Retry, Success };

class ConcurrentBSTMap
{
public:
    ConcurrentBSTMap();
    ~ConcurrentBSTMap();

    std::pair<Result,V> get(K k);
    std::pair<Result,V> put(K k, V v);
    std::pair<Result,V> remove(K k);
    
private:
    bool canUnlink(NodePtr& n);
    int compare(K a, K b);
    bool isRoutingNode(NodePtr& node);
    void deleteTree(NodePtr& node);
    ConcurrentBSTMap(const ConcurrentBSTMap& rhs);
    ConcurrentBSTMap& operator=(const ConcurrentBSTMap& rhs);
    void print();

    // non-blocking methods
    std::pair<Result,V> attemptGet(K k, NodePtr& node, int dir, long nodeV);
    std::pair<Result,V> attemptPut(K k, V v, NodePtr& node, int dir, long nodeV);
    std::pair<Result,V> attemptInsert(K k, V v, NodePtr& node, int dir, long nodeV);
    std::pair<Result,V> attemptUpdate(NodePtr& node, V v);
    std::pair<Result,V> attemptRemove(K k, NodePtr& node, int dir, long nodeV);
    std::pair<Result,V> attemptRmNode(NodePtr& par, NodePtr& n);
    void enableDebugOutput(bool enable);

private:
    std::mutex unlinkMutex;
    NodePtr rootHolder;
    bool debug;
    std::vector<NodePtr> unlinkedNodes;
    const std::pair<Result,V> RetryPair = std::make_pair(Result::Retry, 0);
    const std::pair<Result,V> NullPair = std::make_pair(Result::Null, 0);
};

const long Unlinked = 0x1L;
const long Growing = 0x2L;
const long GrowCountMask = 0xffL << 3;
const long IgnoreGrow = ~(Growing | GrowCountMask);


#endif