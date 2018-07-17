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

class None {};

template <typename T, typename U>
class PairHash {
 public:
  size_t operator()(const std::pair<T, U> &p) const { return std::hash<T>()(p.first) ^ std::hash<U>()(p.second); };
};
template <typename K, typename V = None, typename Compare = std::less<K>>
class HollowHeap {
  using Index = int_fast32_t;
  using Counter = int_fast32_t;

 public:
  struct Node {
    K key;
    V value;
    Index rank;  //negative rank means hollow node
    Node *c_hild, *n_ext, *s_econd_parent;
    Index child, next, secondParent;
    Node(K k, V v) : key(k),
                     value(v),
                     rank(0),
                     c_hild(nullptr),
                     n_ext(nullptr),
                     s_econd_parent(nullptr),
                     child(-1),
                     next(-1),
                     secondParent(-1){};
  };

  struct DisjointSet {  //Union-Find
    int size;
    DisjointSet *par;
    HeapItem<K, V, Compare> *item;    //pointer to Heapitem
    HollowHeap<K, V, Compare> *heap;  //pointer to Heap
    DisjointSet() : size(1), par(nullptr), item(nullptr), heap(nullptr){};
    DisjointSet(HeapItem<K, V, Compare> *it) : size(1), par(nullptr), item(it), heap(nullptr){};
    DisjointSet *UFroot() {
      return par == nullptr ? this : par = par->UFroot();
    }
    void unite(DisjointSet *ds) {
      DisjointSet *root_this = UFroot();
      DisjointSet *root_ds = ds->UFroot();
      if (root_this == root_ds) return;
      if (root_this->size < root_ds->size) std::swap(root_this, root_ds);
      root_ds->par = root_this;
      root_this->size += root_ds->size;
    }
  };

  HollowHeap() : count_item(0), count_node(0), r_oot(nullptr), root(-1), r_ankmap(3), rankmap(3, -1){};
  bool empty() const noexcept { return root == -1; }
  int size() const noexcept { return count_item; }
  Index push(K k) { return emplace(k, V()); }
  Index push(std::pair<K, V> &p) { emplace(p.first, p.second); }
  Index emplace(K k) { return emplace(k, V()); }
  Index emplace(K k, V v) {
    ++count_item;
    ++count_node;
    nodes.emplace_back(k, v);
    Index now = (Index)nodes.size() - 1;
    root = meld_(now);
    return now;
  }
  void add_child(Index v, Index w) {  //make v child of w
    nodes[v].next = nodes[w].child;
    nodes[w].child = v;
  }
  Index link(Index v, Index w) {
    if (Compare()(nodes[v].key, nodes[w].key)) {
      add_child(w, v);
      return v;
    } else {
      add_child(v, w);
      return w;
    }
  }
  Index meld_(Index u) {
    if (u == -1) {
      //r_oot->item->uf->UFroot()->heap = this;
      return root;
    }
    if (root == -1) {
      //u->item->uf->UFroot()->heap = this;
      return u;
    }
    //r_oot->item->uf->unite(u->item->uf);
    //r_oot->item->uf->UFroot()->heap = this;
    return link(root, u);
  }
  Index meld(HollowHeap &g) {
    count_item += g.count_item;
    count_node += g.count_node;
    g.count_item = g.count_node = 0;
    root = meld_(g.root);
    g.root = -1;
    return root;
  }
  const K &top_key() {
    //do not use when the heap is empty
    //assert(count_item>0);
    return nodes[root].key;
  }
  const std::pair<K, V> &top() {
    return make_pair(nodes[root].key, nodes[root].value);
  }
  Index pop() {
    return delete_node(root);
  }
  //static Node* delete_node(){}
  Index delete_node(Index del) {
    count_item--;
    count_node--;
    nodes[del].rank = -1;
    if (nodes[root].rank >= 0) return root;  //if del!=r_oot, deletion is completed
    int maxRank = 0;
    while (root != -1) {
      Index w = nodes[root].child;
      Index x = root;
      root = nodes[root].next;  //root lists all hollow roots
      while (w != -1) {
        ++counter;
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
  //static Node* decrease_key(){}//
  /*Node *decrease_key(HeapItem<K, V, Compare> &i, K k) {
    return decrease_key(i.node, k);
  }*/
  Index decrease_key(Index u, K k) {
    if (u == root) {
      nodes[u].key = k;
      return u;
    }
    nodes.emplace_back(k, std::move(nodes[u].value));
    Index v = (Index)nodes.size() - 1;
    count_node++;
    nodes[u].rank = -1;
    nodes[v].rank = std::max(0, nodes[u].rank - 2);
    nodes[v].child = u;
    nodes[u].secondParent = v;
    return root = link(v, root);
  }
  void swap_heap(HollowHeap &a) {
    std::swap(count_item, a.count_item);
    std::swap(count_node, a.count_node);
    std::swap(root, a.root);
    std::swap(rankmap, a.rankmap);
  }
  //rebuild
  //static std::unordered_multimap<std::pair<K, V>, HeapItem<K, V, Compare> *, PairHash<K, V>> table;  //hash function needed
 private:
  Counter count_item, count_node;
  Node *r_oot;
  Index root;
  std::vector<Node *> r_ankmap;
  std::vector<Index> rankmap;
  static std::vector<Node> nodes;
};
template <typename K, typename V, typename Compare>
std::vector<typename HollowHeap<K, V, Compare>::Node> HollowHeap<K, V, Compare>::nodes;
//template <typename K, typename V, typename Compare>
//std::unordered_multimap<std::pair<K, V>, HeapItem<K, V, Compare> *, PairHash<K, V>> HollowHeap<K, V, Compare>::table;

/*template <typename Key, typename Val, typename Comp>
struct HeapItem {
  Val val;
  typename HollowHeap<Key, Val, Comp>::Node *node;
  typename HollowHeap<Key, Val, Comp>::DisjointSet *uf;
  HeapItem(Val v) : val(v), node(nullptr){};
  HeapItem() : val(), node(nullptr){};
  HollowHeap<Key, Val, Comp> *which_heap() {
    return uf->UFroot()->heap;
  }
};*/

void heapSort(int n) {
  std::random_device rnd;
  std::mt19937 mt(rnd());
  HollowHeap<int> hh;
  std::vector<int> a(n), b(n);

  for (int i = 0; i < (int)a.size(); ++i) {
    a[i] = i;
  }
  std::shuffle(a.begin(), a.end(), mt);
  for (int i = 0; i < (int)a.size(); ++i) {
    auto tmp = hh.emplace(a[i]);
    if (i % 10000 == 0) hh.decrease_key(tmp, a[i] / 2);
  }
  for (int i = 0; i < (int)a.size(); ++i) {
    b[i] = hh.top_key();
    hh.pop();
  }
  assert(std::is_sorted(b.begin(), b.end()));
  std::cout << counter << std::endl;
}
int main() {
  long long n = 0;
  std::cin >> n;
  heapSort(n);
  return 0;
}
