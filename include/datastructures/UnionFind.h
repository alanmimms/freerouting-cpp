#ifndef FREEROUTING_DATASTRUCTURES_UNIONFIND_H
#define FREEROUTING_DATASTRUCTURES_UNIONFIND_H

#include <vector>
#include <numeric>

namespace freerouting {

// Disjoint Set / Union-Find data structure
// Efficiently tracks connectivity between elements
// Used for checking if all pins on a net are connected during routing
//
// Supports path compression and union by rank for near-constant time operations
class UnionFind {
public:
  // Create union-find structure with n elements (0 to n-1)
  explicit UnionFind(int nElements)
    : parent(nElements),
      rank(nElements, 0),
      componentCount(nElements) {
    // Initially each element is its own parent
    std::iota(parent.begin(), parent.end(), 0);
  }

  // Find the root/representative of element x with path compression
  int find(int x) {
    if (x < 0 || x >= static_cast<int>(parent.size())) {
      return -1; // Invalid element
    }

    // Path compression: make all nodes on path point directly to root
    if (parent[x] != x) {
      parent[x] = find(parent[x]);
    }
    return parent[x];
  }

  // Union two elements (merge their sets)
  // Returns true if they were in different sets, false if already in same set
  bool unite(int x, int y) {
    int rootX = find(x);
    int rootY = find(y);

    if (rootX == -1 || rootY == -1) {
      return false; // Invalid elements
    }

    if (rootX == rootY) {
      return false; // Already in same set
    }

    // Union by rank: attach smaller tree under larger tree
    if (rank[rootX] < rank[rootY]) {
      parent[rootX] = rootY;
    } else if (rank[rootX] > rank[rootY]) {
      parent[rootY] = rootX;
    } else {
      parent[rootY] = rootX;
      rank[rootX]++;
    }

    componentCount--;
    return true;
  }

  // Check if two elements are in the same set (connected)
  bool connected(int x, int y) {
    int rootX = find(x);
    int rootY = find(y);

    if (rootX == -1 || rootY == -1) {
      return false; // Invalid elements
    }

    return rootX == rootY;
  }

  // Get number of disjoint sets (connected components)
  int getComponentCount() const {
    return componentCount;
  }

  // Get total number of elements
  int size() const {
    return static_cast<int>(parent.size());
  }

  // Get size of the set containing element x
  int getSetSize(int x) {
    int root = find(x);
    if (root == -1) {
      return 0;
    }

    int count = 0;
    for (int i = 0; i < size(); i++) {
      if (find(i) == root) {
        count++;
      }
    }
    return count;
  }

  // Reset to initial state (all elements separate)
  void reset() {
    std::iota(parent.begin(), parent.end(), 0);
    std::fill(rank.begin(), rank.end(), 0);
    componentCount = static_cast<int>(parent.size());
  }

  // Check if all elements are in a single connected component
  bool isFullyConnected() const {
    return componentCount == 1;
  }

private:
  std::vector<int> parent;  // Parent of each element
  std::vector<int> rank;    // Rank for union by rank heuristic
  int componentCount;       // Number of disjoint sets
};

} // namespace freerouting

#endif // FREEROUTING_DATASTRUCTURES_UNIONFIND_H
