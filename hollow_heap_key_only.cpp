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
  Counter countItem, countNode;
  Index root;
  std::vector<Index> rankArr;
  static std::vector<Node> nodes;
  Compare comp;

 public:
  HollowHeap() : countItem(0), countNode(0), root(-1), rankArr(3, -1), comp(){};

  bool empty() const noexcept { return root == -1; }
  Counter size() const noexcept { return countItem; }

  Index push(K k) { return emplace(k); }
  Index emplace(K k) {
    ++countItem;
    ++countNode;
    nodes.emplace_back(k);
    Index now = (Index)nodes.size() - 1;
    root = meld_(now);
    return now;
  }

  const K &top() const noexcept {
    //do not use when the heap is empty
    //assert(countItem>0);
    return nodes[root].key;
  }
  Index pop() { return deleteNode(root); }

  Index meld(HollowHeap &g) {
    countItem += g.countItem;
    countNode += g.countNode;
    g.countItem = g.countNode = 0;
    root = meld_(g.root);
    g.root = -1;
    return root;
  }

  Index deleteNode(Index del) {
    countItem--;
    countNode--;
    nodes[del].rank = -1;
    if (nodes[root].rank >= 0) return root;  //if del!=r_oot, deletion is completed
    Rank maxRank = 0;
    while (root != -1) {
      Index w = nodes[root].child;
      Index x = root;
      root = nodes[root].next;  //root lists all hollow roots
      while (w != -1) {
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
    //assert(nodes[u].key>k)
    if (u == root) {
      nodes[u].key = k;
      return u;
    }
    nodes.emplace_back(k);
    Index v = (Index)nodes.size() - 1;
    countNode++;
    nodes[u].rank = -1;
    nodes[v].rank = std::max(0, nodes[u].rank - 2);
    nodes[v].child = u;
    nodes[u].secondParent = v;
    return root = link(v, root);
  }
  void swap(HollowHeap &a) {
    std::swap(countItem, a.countItem);
    std::swap(countNode, a.countNode);
    std::swap(root, a.root);
    std::swap(rankArr, a.rankArr);
  }

 private:
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

  Index meld_(Index u) {
    if (u == -1) {
      return root;
    }
    if (root == -1) {
      return u;
    }
    return link(root, u);
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
