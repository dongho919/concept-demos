#include "concurrentbst.h"
#include <iostream>

ConcurrentBSTMap::ConcurrentBSTMap() : rootHolder(new Node()), debug(false), unlinkedNodes()
{}

ConcurrentBSTMap::~ConcurrentBSTMap()
{
    deleteTree(rootHolder);
    for (NodePtr& node : unlinkedNodes)
        delete node;
}

std::pair<Result,V> ConcurrentBSTMap::get(K k)
{
    return attemptGet(k, rootHolder, 1, 0);
}
std::pair<Result,V> ConcurrentBSTMap::put(K k, V v)
{
    return attemptPut(k, v, rootHolder, 1, 0);
}
std::pair<Result,V> ConcurrentBSTMap::remove(K k)
{
    return attemptRemove(k, rootHolder, 1, 0);
}
std::pair<Result,V> ConcurrentBSTMap::attemptGet(K k, NodePtr& node, int dir, long nodeV)
{
    while (true)
    {
        NodePtr child = node->child(dir);
        // validate inbound link
        if (((node->version ^ nodeV) & IgnoreGrow) != 0)
            return RetryPair;
        // basic null check
        if (child == nullptr)
            return NullPair;
        // condition to stop
        int nextD = compare(k, child->key);
        if (nextD == 0)
        {
            if (isRoutingNode(child))
                return NullPair;
            return std::make_pair(Result::Success, child->value);
        }
        // if didn't stop at this level, validate outbound link (child)
        long chV = child->version;
        if (chV != Unlinked && child == node->child(dir))
        {
            // revalidate inbound link
            if (((node->version ^ nodeV) & IgnoreGrow) != 0)
                return RetryPair;
            // hand the Retry baton to the inbound link (parent)
            // the parent will keep looping until retry succeeds
            // or itself or child becomes unlinked
            std::pair<Result,V> p = attemptGet(k, child, nextD, chV);
            if (p != RetryPair)
                return p;
        }
        // outbound link invalid
        else return NullPair;
    }
}

std::pair<Result,V> ConcurrentBSTMap::attemptPut(K k, V v, NodePtr& node, int dir, long nodeV)
{
    std::pair<Result,V> p = RetryPair;
    do
    {
        NodePtr child = node->child(dir);
        // validate inbound link
        if (((node->version ^ nodeV) & IgnoreGrow) != 0)
            return RetryPair;
        // basic null check; insert if outbound link (child) does not exist in the indicated direction
        if (child == nullptr)
            p = attemptInsert(k, v, node, dir, nodeV);
        else
        {
            // condition to stop and update the existing outbound link with the matching key
            int nextD = compare(k, child->key);
            if (nextD == 0)
                p = attemptUpdate(child, v);
            // continue to the next level if outbound link has a different key
            else
            {
                // validate outbound link
                long chV = child->version;
                if (chV != Unlinked && child == node->child(dir))
                {
                    // revalidate inbound link
                    if (((node->version ^ nodeV) & IgnoreGrow) != 0)
                        return RetryPair;
                    // the result of attemptInsert/attemptUpdate will come up
                    // from a lower level; keep looping or return accordingly
                    p = attemptPut(k, v, child, nextD, chV);
                }
            }
        }
    }
    while (p == RetryPair);
    return p;
}
std::pair<Result,V> ConcurrentBSTMap::attemptInsert(K k, V v, NodePtr& node, int dir, long nodeV)
{
    {
        std::unique_lock<std::mutex> nodeLock(node->m);
        // validate inbound link
        if (((node->version ^ nodeV) & IgnoreGrow) != 0 || node->child(dir) != nullptr)
            return RetryPair;
        node->child(dir) = new Node(k, v, node, 0, nullptr, nullptr);
    }
    return NullPair;
}
std::pair<Result,V> ConcurrentBSTMap::attemptUpdate(NodePtr& node, V v)
{
    std::unique_lock<std::mutex> nodeLock(node->m);
    // we're not concerned about nodes moving around (grow & shrink),
    // only check if it's unlinked or not
    if (node->version == Unlinked)
        return RetryPair;
    std::pair<Result,V> prev = (node->value == std::numeric_limits<V>::min() ? NullPair : std::make_pair(Result::Success, node->value));
    node->value = v;
    return prev;
}
std::pair<Result,V> ConcurrentBSTMap::attemptRemove(K k, NodePtr& node, int dir, long nodeV)
{
    std::pair<Result,V> p = RetryPair;
    do
    {
        NodePtr child = node->child(dir);
        // validate inbound link
        if (((node->version ^ nodeV) & IgnoreGrow) != 0)
            return RetryPair;
        // basic null check; NOOP when target is null
        if (child == nullptr)
            return NullPair;
        else
        {
            // condition to stop and remove the outbound link with the matching key
            int nextD = compare(k, child->key);
            if (nextD == 0)
                p = attemptRmNode(node, child);
            // continue to the next level if outbound link has a different key
            else
            {
                // validate outbound link
                long chV = child->version;
                if (chV != Unlinked && child == node->child(dir))
                {
                    // revalidate inbound link
                    if (((node->version ^ nodeV) & IgnoreGrow) != 0)
                        return RetryPair;
                    // the result of attemptRmNode will come up
                    // from a lower level; keep looping or return accordingly
                    p = attemptRemove(k, child, nextD, chV);
                }
            }
        }
    }
    while (p == RetryPair);
    return p;
}

std::pair<Result,V> ConcurrentBSTMap::attemptRmNode(NodePtr& par, NodePtr& n)
{
    // this is a routing node (physically present but logically deleted); NOOP
    if (isRoutingNode(n)) return NullPair;
    std::pair<Result,V> prev = RetryPair;
    // target has two children; show mercy and spare its life...
    if (!canUnlink(n))
    {
        std::unique_lock<std::mutex> nodeLock(n->m);
        // validate target
        if (n->version == Unlinked || canUnlink(n))
            return RetryPair;
        prev = (n->value == std::numeric_limits<V>::min() ? NullPair : std::make_pair(Result::Success, n->value));
        n->value = std::numeric_limits<V>::min();
    }
    // target has 0 or 1 children; kill it (I mean, unlink it)
    else
    {
        {
            std::unique_lock<std::mutex> parentLock(par->m);
            // validate target AND parent
            if (par->version == Unlinked || n->parent != par || n->version == Unlinked)
                return RetryPair;
            // scope for locking target
            {
                std::unique_lock<std::mutex> nodeLock(n->m);
                prev = (n->value == std::numeric_limits<V>::min() ? NullPair : std::make_pair(Result::Success, n->value));
                n->value = std::numeric_limits<V>::min();
                // recheck target state; target might have added more children
                // when we weren't looking if this part fails, then it's
                // logically equivalent to the block above for "!canUnlink(n)
                if (canUnlink(n))
                {
                    // proceed to unlink target from parent
                    // and replace target with its child (or null)
                    NodePtr& c = (n->left == nullptr ? n->right : n->left);
                    if (par->left == n)
                        par->left = c;
                    else
                        par->right = c;
                    if (c != nullptr) c->parent = par;
                    n->version = Unlinked;
                    // lock-based memory manager, ugh
                    std::unique_lock<std::mutex> unlinkLock(unlinkMutex);
                    unlinkedNodes.push_back(n);
                }
            }
        }
        // I don't have time for this
        // fixHeightAndRebalance(par);
    }
    return prev;
}

// has fewer than 2 children?
bool ConcurrentBSTMap::canUnlink(NodePtr& n)
{
    return !n->left || !n->right;
}

// -1 means a < b
// 0 means  a == b
// 1 means  a > b
int ConcurrentBSTMap::compare(K a, K b)
{
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

// is the node logically deleted?
bool ConcurrentBSTMap::isRoutingNode(NodePtr& node)
{
    return node->value == std::numeric_limits<V>::min();
}

// recursive tree deletion
void ConcurrentBSTMap::deleteTree(NodePtr& node)
{
    if (!node) return;
    deleteTree(node->left);
    deleteTree(node->right);
    delete node;
}

// prints all nodes recursively
void ConcurrentBSTMap::print()
{
    rootHolder->print(-1);
    std::cout << std::endl;
}

// deprecated; you can't use this technique for concurrent code
// because of Heisenbug or whatever
void ConcurrentBSTMap::enableDebugOutput(bool enable)
{
    debug = enable;
}