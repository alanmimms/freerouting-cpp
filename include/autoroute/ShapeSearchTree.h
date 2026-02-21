#ifndef FREEROUTING_AUTOROUTE_SHAPESEARCHTREE_H
#define FREEROUTING_AUTOROUTE_SHAPESEARCHTREE_H

#include "geometry/Shape.h"
#include "geometry/IntBox.h"
#include <vector>
#include <memory>

namespace freerouting {

// Forward declarations
class ExpansionRoom;
class Item;

// Base class for objects that can be stored in the search tree
// Java: ShapeTree.Storable interface
class SearchTreeObject {
public:
  virtual ~SearchTreeObject() = default;

  // Get the shape for spatial indexing (bounding box for now)
  virtual const Shape* getTreeShape(int shapeIndex) const = 0;

  // Get the layer this shape is on
  virtual int getShapeLayer(int shapeIndex) const = 0;

  // Check if this object is an obstacle for a given net
  virtual bool isTraceObstacle(int netNo) const = 0;

  // How many shapes does this object have?
  virtual int treeShapeCount() const = 0;
};

// Entry returned by search queries
// Java: ShapeTree.TreeEntry
struct TreeEntry {
  SearchTreeObject* object;  // The object (ExpansionRoom or Item)
  int shapeIndex;           // Which shape within the object

  TreeEntry(SearchTreeObject* obj, int idx)
    : object(obj), shapeIndex(idx) {}
};

// Simple binary spatial search tree
// Java: MinAreaTree
class ShapeSearchTree {
public:
  ShapeSearchTree() = default;
  ~ShapeSearchTree();

  // Insert an object into the tree
  void insert(SearchTreeObject* object);

  // Find all objects whose shapes overlap with the query box on a given layer
  // Java: overlapping_tree_entries()
  void overlappingEntries(const IntBox& queryBox, int layer,
                         std::vector<TreeEntry>& results) const;

  // Find all objects on any layer (layer = -1 means all layers)
  void overlappingEntries(const IntBox& queryBox,
                         std::vector<TreeEntry>& results) const;

  // Clear the tree
  void clear();

  // Get number of objects in tree
  int size() const { return leafCount; }

private:
  // Tree node types
  struct TreeNode {
    IntBox boundingBox;
    TreeNode* parent = nullptr;

    virtual ~TreeNode() = default;
    virtual bool isLeaf() const = 0;
  };

  struct InnerNode : public TreeNode {
    TreeNode* firstChild = nullptr;
    TreeNode* secondChild = nullptr;

    bool isLeaf() const override { return false; }
  };

  struct Leaf : public TreeNode {
    SearchTreeObject* object = nullptr;
    int shapeIndex = 0;

    bool isLeaf() const override { return true; }
  };

  // Find overlapping leaves (internal helper)
  void findOverlaps(TreeNode* node, const IntBox& queryBox,
                   std::vector<Leaf*>& leaves) const;

  // Insert a leaf into the tree
  void insertLeaf(Leaf* leaf);

  // Find position for new leaf using minimum area increase heuristic
  Leaf* positionLocate(TreeNode* node, Leaf* newLeaf);

  // Calculate area increase
  int areaIncrease(const IntBox& box1, const IntBox& box2) const;

  TreeNode* root = nullptr;
  int leafCount = 0;
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_SHAPESEARCHTREE_H
