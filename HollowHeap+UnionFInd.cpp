#include <algorithm>
#include <cassert>
#include <iostream>
#include <queue>
#include <random>
#include <unordered_map>
#include <utility>
#include <vector>

#if MYDEBUG
#include "lib/cp_debug.hpp"
#else
#define DBG(...) ;
#endif

class None {};

template <typename K, typename V = None, typename Compare = std::less<K>>
class HollowHeap {
  using Index = int_fast32_t;
  using Counter = int_fast32_t;

 public:
  struct Node {
    K key;
    V value;
    Index rank;  //if rank is negative, this node is hollow node
    Index child, next, secondParent;
    Node(K k, V v) : key(k),
                     value(v),
                     rank(0),
                     child(-1),
                     next(-1),
                     secondParent(-1){};
  };
  struct UnionFind {
    std::vector<int> par;
    UnionFind(int n) : par(n, -1){};
    int root(int x) { return par[x] < 0 ? x : (par[x] = root(par[x])); }
    void unite(int x, int y) {
      x = root(x);
      y = root(y);
      if (x != y) {
        if (par[x] < par[y]) std::swap(x, y);
        par[y] += par[x];
        par[x] = y;
      }
    }
    void append() {
      par.push_back(-1);
    }
    bool isRoot(int x) { return root(x) == x; }
    bool same(int x, int y) { return root(x) == root(y); }
    int size(int x) { return -par[root(x)]; }
  };

 private:
  Counter countItem, countNode;
  Index root;
  std::vector<Index> rankArr;
  static std::vector<Node> nodes;
  static UnionFind uf;
  static std::vector<HollowHeap> heaps;

 public:
  HollowHeap() : countItem(0), countNode(0), root(-1), rankArr(3, -1) { uf.append(); };
  bool empty() const noexcept { return root == -1; }
  int size() const noexcept { return countItem; }
  Index push(K k) { return emplace(k, V()); }
  Index push(std::pair<K, V> &p) { emplace(p.first, p.second); }
  Index emplace(K k) { return emplace(k, V()); }
  Index emplace(K k, V v) {
    ++countItem;
    ++countNode;
    nodes.emplace_back(k, v);
    Index now = (Index)nodes.size() - 1;
    root = meld_(now);
    return now;
  }
  const K &top_key() {
    //do not use when the heap is empty
    //assert(countItem>0);
    return nodes[root].key;
  }
  const std::pair<K, V> &top() {
    return make_pair(nodes[root].key, nodes[root].value);
  }
  Index pop() {
    return deleteNode(root);
  }

  Index meld(HollowHeap &g) {
    root = meld_(g.root);
    g.root = -1;
    if (g.size() > size()) swap(g);
    uf.unite(g - heaps[0], this - heaps[0]);
    countItem += g.countItem;
    countNode += g.countNode;
    g.countItem = g.countNode = 0;
    g.root = -1;
    g.rankArr = std::vector<Index>(3, -1);
    return root;
  }

  Index deleteNode(Index del) {
    countItem--;
    countNode--;
    nodes[del].rank = -1;
    if (nodes[root].rank >= 0) return root;  //if del!=r_oot, deletion is completed
    int maxRank = 0;
    while (root != -1) {
      Index w = nodes[root].child;
      Index x = root;
      root = nodes[root].next;  //root lists all hollow roots
      while (w != -1) {
        Index u = w;
        w = nodes[w].next;
        if (nodes[u].rank < 0) {              //if the child of root (u) is hollow node
          if (nodes[u].secondParent == -1) {  //u became hollow by delete op
            //insert u to list of hollow nodes
            nodes[u].next = root;
            root = u;
          } else {  //u became hollow by decrease-key operation
            if (nodes[u].secondParent == x)
              w = -1;  //unnecessary?
            else
              nodes[u].next = -1;        //when x is deleted, u is the last child of u->second_parent
            nodes[u].secondParent = -1;  // u no longer have two parents
          }
        } else {               //ranked link
          nodes[u].next = -1;  //
          while (rankArr[nodes[u].rank] != -1) {
            u = link(u, rankArr[nodes[u].rank]);
            rankArr[nodes[u].rank] = -1;
            ++nodes[u].rank;
          }
          rankArr[nodes[u].rank] = u;
          maxRank = std::max(maxRank, nodes[u].rank);
          if (maxRank >= (int)rankArr.size() - 2) rankArr.resize((int)rankArr.size() * 2, -1);
        }
      }
      --countNode;
      if (x != -1) {
        //delete x;
        //rebuild
      }
    }
    //unranked link
    for (int i = 0; i <= maxRank; ++i) {
      if (rankArr[i] != -1) {
        if (root == -1)
          root = rankArr[i];
        else
          root = link(root, rankArr[i]);
        rankArr[i] = -1;
      }
    }
    return root;
  }

  Index decreaseKey(Index u, K k) {
    if (u == root) {
      nodes[u].key = k;
      return u;
    }
    nodes.emplace_back(k, std::move(nodes[u].value));
    Index v = (Index)nodes.size() - 1;
    countNode++;
    nodes[u].rank = -1;
    nodes[v].rank = std::max(0, nodes[u].rank - 2);
    nodes[v].child = u;
    nodes[u].secondParent = v;
    return root = link(v, root);
  }

 private:
  void addChild(Index v, Index w) {  //make v child of w
    nodes[v].next = nodes[w].child;
    nodes[w].child = v;
  }
  Index link(Index v, Index w) {
    if (Compare()(nodes[v].key, nodes[w].key)) {
      addChild(w, v);
      return v;
    } else {
      addChild(v, w);
      return w;
    }
  }
  Index meld_(Index u) {
    if (u == -1) {
      return root;
    }
    if (root == -1) {
      return u;
    }
    return link(root, u);
  }

 public:
  //static member functions
  static HollowHeap &idx(int h) { return heaps[uf.root(h)]; }
  static int newHeap() {
    heaps.emplace_back();
    return (int)heaps.size() - 1;
  }
  static Index deleteNode(int heapIdx, Index del) { return idx(heapIdx).deleteNode(del); }
  static Index decraseKey(int heapIdx, Index u, K k) { return idx(heapIdx).decreaseKey(u, k); }
  static int countHeap() { return (int)uf.par.size(); }
  //rebuild
};
template <typename K, typename V, typename Compare>
std::vector<typename HollowHeap<K, V, Compare>::Node> HollowHeap<K, V, Compare>::nodes;
template <typename K, typename V, typename Compare>
typename HollowHeap<K, V, Compare>::UnionFind HollowHeap<K, V, Compare>::uf(0);
template <typename K, typename V, typename Compare>
typename std::vector<HollowHeap<K, V, Compare>> HollowHeap<K, V, Compare>::heaps(0);

void heapSort(int n) {
  std::random_device rnd;
  std::mt19937 mt(rnd());
  using HHi = HollowHeap<int>;
  int heapId = HHi::newHeap();
  //HHi hh ;
  std::priority_queue<int, std::vector<int>, std::greater<int>> pq;
  std::vector<int> a(n), b(n);

  for (int i = 0; i < (int)a.size(); ++i) {
    a[i] = i;
  }
  std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
  std::shuffle(a.begin(), a.end(), mt);
  for (int i = 0; i < (int)a.size(); ++i) {
    auto tmp = HHi::idx(heapId).emplace(a[i]);
    if (i % 10000 == 0) HHi::idx(heapId).decreaseKey(tmp, a[i] / 2);
  }
  for (int i = 0; i < (int)a.size(); ++i) {
    b[i] = HHi::idx(heapId).top_key();
    HHi::idx(heapId).pop();
  }
  assert(std::is_sorted(b.begin(), b.end()));
  std::chrono::system_clock::time_point p1 = std::chrono::system_clock::now();

  for (int i = 0; i < a.size(); ++i) {
    pq.emplace(a[i]);
  }
  for (int i = 0; i < a.size(); ++i) {
    b[i] = pq.top();
    pq.pop();
  }
  assert(std::is_sorted(b.begin(), b.end()));
  std::chrono::system_clock::time_point p2 = std::chrono::system_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(p1 - start).count() << std::endl;
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(p2 - p1).count() << std::endl;
}
int main() {
  long long n = 0;
  std::cin >> n;
  heapSort(n);
  return 0;
}
