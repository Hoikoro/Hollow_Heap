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
//int counter;

template <typename Key, typename Val, typename Comp>
struct HeapItem;

class PairHash {
 public:
  size_t operator()(const std::pair<long long, int> &p) const { return std::hash<long long>()(p.first) ^ std::hash<int>()(p.second); };
};
template <typename K, typename V = int, typename compare = std::less<K>>
class HollowHeap {
 public:
  struct Node {
    K key;
    HeapItem<K, V, compare> *item;
    int rank;
    Node *child, *next, *second_parent;
    HollowHeap *hh;
    Node(int k, HeapItem<K, V, compare> *item_) : key(k),
                                                  item(item_),
                                                  rank(0),
                                                  child(nullptr),
                                                  next(nullptr),
                                                  second_parent(nullptr),
                                                  hh(nullptr){};
  };

  struct DisjointSet {  //Union-Find
    int size;
    DisjointSet *par;
    HeapItem<K, V, compare> *item;    //pointer to Heapitem
    HollowHeap<K, V, compare> *heap;  //pointer to Heap
    DisjointSet() : size(1), par(nullptr), item(nullptr), heap(nullptr){};
    DisjointSet(HeapItem<K, V, compare> *it) : size(1), par(nullptr), item(it), heap(nullptr){};
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

  HollowHeap() : count_item(0), count_node(0), root(nullptr), rankmap(3){};
  bool empty() { return root == nullptr; }
  int size() { return count_item; }
  HeapItem<K, V, compare> *push(K k) { return emplace(k, V()); }
  HeapItem<K, V, compare> *push(std::pair<K, HeapItem<K, V, compare> &> p) { emplace(p.first, p.second); }
  HeapItem<K, V, compare> *push(std::pair<K, V> &p) { emplace(p.first, p.second); }
  HeapItem<K, V, compare> *emplace(K k) { return emplace(k, V()); }
  HeapItem<K, V, compare> *emplace(K k, V v) {
    auto pr = new HeapItem<K, V, compare>(v);
    return emplace(k, *pr);
    //return pr;
  }
  HeapItem<K, V, compare> *emplace(K k, HeapItem<K, V, compare> &e) {
    ++count_item;
    ++count_node;
    e.node = new Node(k, &e);
    e.uf = new DisjointSet(&e);
    root = meld_(e.node);
    return &e;
  }
  void add_child(Node *v, Node *w) {  //make v child of w
    v->next = w->child;
    w->child = v;
  }
  Node *link(Node *v, Node *w) {
    if (compare()(v->key, w->key)) {
      add_child(w, v);
      return v;
    } else {
      add_child(v, w);
      return w;
    }
  }
  Node *meld_(Node *u) {
    if (u == nullptr) {
      root->item->uf->UFroot()->heap = this;
      return root;
    }
    if (root == nullptr) {
      u->item->uf->UFroot()->heap = this;
      return u;
    }
    root->item->uf->unite(u->item->uf);
    root->item->uf->UFroot()->heap = this;
    return link(root, u);
  }
  Node *meld(HollowHeap &g) {
    count_item += g.count_item;
    count_node += g.count_node;
    g.count_item = g.count_node = 0;
    root = meld_(g.root);
    g.root = nullptr;
    return root;
  }
  K top() {
    //do not use when the heap is empty
    //assert(count_item>0);
    return root->key;
  }
  std::pair<K, HeapItem<K, V, compare> *> top_item() {
    return make_pair(root->key, root->item);
  }
  Node *pop() {
    return delete_node(root);
  }
  //static Node* delete_node(){}
  Node *delete_node(Node *u) {
    count_item--;
    count_node--;
    u->item->node = nullptr;
    u->item = nullptr;
    if (root->item != nullptr) return root;  //if u!=root, deletion is completed
    int max_rank = 0;
    while (root != nullptr) {
      auto w = root->child;
      auto x = root;
      root = root->next;  //root lists all hollow roots
      while (w != nullptr) {
        //counter ++;
        auto u = w;
        w = w->next;
        if (u->item == nullptr) {             //if the child of root (u) is hollow node
          if (u->second_parent == nullptr) {  //u became hollow by delete op
            //insert u to list of hollow nodes
            u->next = root;
            root = u;
          } else {  //u became hollow by decrease-key operation
            if (u->second_parent == x)
              w = nullptr;  //unnecessary?
            else
              u->next = nullptr;         //when x is deleted, u is the last child of u->second_parent
            u->second_parent = nullptr;  // u no longer have two parents
          }
        } else {              //ranked link
          u->next = nullptr;  //
          while (rankmap[u->rank] != nullptr) {
            u = link(u, rankmap[u->rank]);
            rankmap[u->rank] = nullptr;
            ++u->rank;
          }
          rankmap[u->rank] = u;
          max_rank = std::max(max_rank, u->rank);
          if (max_rank >= (int)rankmap.size() - 2) rankmap.resize((int)rankmap.size() * 2);
        }
      }
      --count_node;
      if (x != nullptr) delete x;
    }
    //unranked link
    for (int i = 0; i <= max_rank; ++i) {
      if (rankmap[i] != nullptr) {
        if (root == nullptr)
          root = rankmap[i];
        else
          root = link(root, rankmap[i]);
        rankmap[i] = nullptr;
      }
    }
    return root;
  }
  //static Node* decrease_key(){}//
  Node *decrease_key(HeapItem<K, V, compare> &i, K k) {
    return decrease_key(i.node, k);
  }
  Node *decrease_key(Node *u, K k) {
    if (u == root) {
      u->key = k;
      return u;
    }
    Node *v = new Node(k, std::move(u->item));
    count_node++;
    u->item = nullptr;
    v->rank = std::max(0, u->rank - 2);
    v->child = u;
    u->second_parent = v;
    return root = link(v, root);
  }
  void swap_heap(HollowHeap &a) {
    std::swap(count_item, a.count_item);
    std::swap(count_node, a.count_node);
    std::swap(root, a.root);
  }
  //rebuild
  //static multimap<V*,Node*> table;
  static std::unordered_multimap<std::pair<K, V>, HeapItem<K, V, compare> *, PairHash> table;  //hash function needed
 private:
  int count_item, count_node;
  Node *root;
  std::vector<Node *> rankmap;
};
template <typename K, typename V, typename compare>
std::unordered_multimap<std::pair<K, V>, HeapItem<K, V, compare> *, PairHash> HollowHeap<K, V, compare>::table;

template <typename Key, typename Val, typename Comp>
struct HeapItem {
  Val val;
  typename HollowHeap<Key, Val, Comp>::Node *node;
  typename HollowHeap<Key, Val, Comp>::DisjointSet *uf;
  HeapItem(Val v) : val(v), node(nullptr){};
  HeapItem() : val(), node(nullptr){};
  HollowHeap<Key, Val, Comp> *which_heap() {
    return uf->UFroot()->heap;
  }
};

void heap_sort(int n) {
  std::random_device rnd;
  std::mt19937 mt(rnd());
  HollowHeap<int, int> hh, hh3;
  std::vector<int> a(n), b(n);

  for (int i = 0; i < (int)a.size(); ++i) {
    a[i] = i;
  }
  std::shuffle(a.begin(), a.end(), mt);
  for (int i = 0; i < (int)a.size(); ++i) {
    hh.emplace(a[i], i);
  }
  for (int i = 0; i < (int)a.size(); ++i) {
    b[i] = hh.top();
    hh.pop();
  }
  assert(std::is_sorted(b.begin(), b.end()));
}
int main() {
  long long n = 0;
  std::cin >> n;
  heap_sort(n);
  return 0;
}
