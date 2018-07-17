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
int counter;

template <typename Key, typename Val, typename Comp>
struct HeapItem;

template <typename K, typename Compare = std::less<K>>
class HollowHeap {
  using Index = int_fast32_t;
  using Counter = int_fast32_t;
  using Rank = int_fast32_t;

  struct Node {
    K key;
    Rank rank;  //negative rank means hollow node
    Index child, next, secondParent;
    Node(K k) : key(k),
                rank(0),
                child(-1),
                next(-1),
                secondParent(-1){};
  };

 private:
  Counter count_item, count_node;
  Index root;
  std::vector<Index> rankmap;
  static std::vector<Node> nodes;
  Compare comp;

 public:
  HollowHeap() : count_item(0), count_node(0), root(-1), rankmap(3, -1), comp(){};

  bool empty() const noexcept { return root == -1; }
  Counter size() const noexcept { return count_item; }

  Index push(K k) { return emplace(k); }
  Index emplace(K k) {
    ++count_item;
    ++count_node;
    nodes.emplace_back(k);
    Index now = (Index)nodes.size() - 1;
    root = meld_(now);
    return now;
  }

  //make v child of w
  void addChild(Index v, Index w) {
    nodes[v].next = nodes[w].child;
    nodes[w].child = v;
  }

  Index link(Index v, Index w) {
    if (comp(nodes[v].key, nodes[w].key)) {
      addChild(w, v);
      return v;
    } else {
      addChild(v, w);
      return w;
    }
  }

 private:
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
  Index meld(HollowHeap &g) {
    count_item += g.count_item;
    count_node += g.count_node;
    g.count_item = g.count_node = 0;
    root = meld_(g.root);
    g.root = -1;
    return root;
  }

  const K &top() const noexcept {
    //do not use when the heap is empty
    //assert(count_item>0);
    return nodes[root].key;
  }

  Index pop() { return deleteNode(root); }

  Index deleteNode(Index del) {
    count_item--;
    count_node--;
    nodes[del].rank = -1;
    if (nodes[root].rank >= 0) return root;  //if del!=r_oot, deletion is completed
    Rank maxRank = 0;
    while (root != -1) {
      Index w = nodes[root].child;
      Index x = root;
      root = nodes[root].next;  //root lists all hollow roots
      while (w != -1) {
        ++counter;
        Index u = w;
        w = nodes[w].next;
        if (nodes[u].rank < 0) {              //if the child of root (u) is hollow node
          if (nodes[u].secondParent == -1) {  //u became hollow by delete()
            //insert u to list of hollow nodes
            nodes[u].next = root;
            root = u;
          } else {  //u became hollow by decreaseKey()
            if (nodes[u].secondParent == x)
              w = -1;  //unnecessary?
            else
              nodes[u].next = -1;        //when x is deleted, u is the last child of u->second_parent
            nodes[u].secondParent = -1;  // u no longer have two parents
          }
        } else {               //ranked link
          nodes[u].next = -1;  //
          while (rankmap[nodes[u].rank] != -1) {
            u = link(u, rankmap[nodes[u].rank]);
            rankmap[nodes[u].rank] = -1;
            ++nodes[u].rank;
          }
          rankmap[nodes[u].rank] = u;
          maxRank = std::max(maxRank, nodes[u].rank);
          if (maxRank >= (int)rankmap.size() - 2) rankmap.resize((int)rankmap.size() * 2, -1);
        }
      }
      --count_node;
      if (x != -1) {
        //delete x;
        //rebuild
      }
    }
    //unranked link
    for (int i = 0; i <= maxRank; ++i) {
      if (rankmap[i] != -1) {
        if (root == -1)
          root = rankmap[i];
        else
          root = link(root, rankmap[i]);
        rankmap[i] = -1;
      }
    }
    return root;
  }

  Index decreaseKey(Index u, K k) {
    //assert(nodes[u].key>k)
    if (u == root) {
      nodes[u].key = k;
      return u;
    }
    nodes.emplace_back(k);
    Index v = (Index)nodes.size() - 1;
    count_node++;
    nodes[u].rank = -1;
    nodes[v].rank = std::max(0, nodes[u].rank - 2);
    nodes[v].child = u;
    nodes[u].secondParent = v;
    return root = link(v, root);
  }
  void swapHeap(HollowHeap &a) {
    std::swap(count_item, a.count_item);
    std::swap(count_node, a.count_node);
    std::swap(root, a.root);
    std::swap(rankmap, a.rankmap);
  }
  //rebuild
};
template <typename K, typename Compare>
std::vector<typename HollowHeap<K, Compare>::Node> HollowHeap<K, Compare>::nodes;

void heapSort(int n) {
  std::random_device rnd;
  std::mt19937 mt(rnd());
  HollowHeap<int> hh;
  std::priority_queue<int, std::vector<int>, std::greater<int>> pq;
  std::vector<int> a(n), b(n);

  for (int i = 0; i < (int)a.size(); ++i) {
    a[i] = i;
  }
  std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
  std::shuffle(a.begin(), a.end(), mt);
  for (int i = 0; i < (int)a.size(); ++i) {
    auto tmp = hh.emplace(a[i]);
    if (i % 10000 == 0) hh.decreaseKey(tmp, a[i] / 2);
  }
  for (int i = 0; i < (int)a.size(); ++i) {
    b[i] = hh.top();
    hh.pop();
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
