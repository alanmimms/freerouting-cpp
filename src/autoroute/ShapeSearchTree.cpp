#include "autoroute/ShapeSearchTree.h"
#include <algorithm>
#include <limits>
#include <iostream>

namespace freerouting {

ShapeSearchTree::~ShapeSearchTree() {
  clear();
}

void ShapeSearchTree::clear() {
  if (!root) return;

  // Delete all nodes using post-order traversal
  std::vector<TreeNode*> stack;
  std::vector<TreeNode*> toDelete;
  stack.push_back(root);

  while (!stack.empty()) {
    TreeNode* node = stack.back();
    stack.pop_back();
    toDelete.push_back(node);

    if (!node->isLeaf()) {
      auto* inner = static_cast<InnerNode*>(node);
      if (inner->firstChild) stack.push_back(inner->firstChild);
      if (inner->secondChild) stack.push_back(inner->secondChild);
    }
  }

  // Delete in reverse order (children before parents)
  for (auto it = toDelete.rbegin(); it != toDelete.rend(); ++it) {
    delete *it;
  }

  root = nullptr;
  leafCount = 0;
}

void ShapeSearchTree::insert(SearchTreeObject* object) {
  if (!object) return;

  int shapeCount = object->treeShapeCount();
  for (int i = 0; i < shapeCount; ++i) {
    const Shape* shape = object->getTreeShape(i);
    if (!shape) {
      std::cout << "ShapeSearchTree::insert: object has null shape at index " << i << "\n";
      continue;
    }

    // Create leaf for this shape
    Leaf* leaf = new Leaf();
    leaf->object = object;
    leaf->shapeIndex = i;
    leaf->boundingBox = shape->getBoundingBox();
    leaf->parent = nullptr;

    insertLeaf(leaf);
  }
}

void ShapeSearchTree::insertLeaf(Leaf* leaf) {
  leafCount++;

  if (!root) {
    // Empty tree - leaf becomes root
    root = leaf;
    return;
  }

  // Find leaf to replace using minimum area increase heuristic
  Leaf* leafToReplace = positionLocate(root, leaf);

  // Create new internal node containing both leaves
  IntBox newBounds = leafToReplace->boundingBox.unionWith(leaf->boundingBox);
  InnerNode* newNode = new InnerNode();
  newNode->boundingBox = newBounds;
  newNode->parent = leafToReplace->parent;

  // Update parent pointers
  if (leafToReplace->parent) {
    auto* parentInner = static_cast<InnerNode*>(leafToReplace->parent);
    if (leafToReplace == parentInner->firstChild) {
      parentInner->firstChild = newNode;
    } else {
      parentInner->secondChild = newNode;
    }
  }

  leafToReplace->parent = newNode;
  leaf->parent = newNode;

  newNode->firstChild = leafToReplace;
  newNode->secondChild = leaf;

  if (root == leafToReplace) {
    root = newNode;
  }
}

ShapeSearchTree::Leaf* ShapeSearchTree::positionLocate(TreeNode* node, Leaf* newLeaf) {
  // Base case: reached a leaf
  if (node->isLeaf()) {
    return static_cast<Leaf*>(node);
  }

  // Recurse: choose child with minimum area increase
  auto* inner = static_cast<InnerNode*>(node);

  int increaseFirst = areaIncrease(inner->firstChild->boundingBox, newLeaf->boundingBox);
  int increaseSecond = areaIncrease(inner->secondChild->boundingBox, newLeaf->boundingBox);

  if (increaseFirst <= increaseSecond) {
    return positionLocate(inner->firstChild, newLeaf);
  } else {
    return positionLocate(inner->secondChild, newLeaf);
  }
}

int ShapeSearchTree::areaIncrease(const IntBox& box1, const IntBox& box2) const {
  IntBox unionBox = box1.unionWith(box2);

  // Calculate areas (avoiding overflow with int64_t)
  int64_t originalArea = static_cast<int64_t>(box1.width()) * box1.height();
  int64_t unionArea = static_cast<int64_t>(unionBox.width()) * unionBox.height();

  return static_cast<int>(unionArea - originalArea);
}

void ShapeSearchTree::overlappingEntries(const IntBox& queryBox, int layer,
                                        std::vector<TreeEntry>& results) const {
  if (!root) return;

  // Find overlapping leaves
  std::vector<Leaf*> leaves;
  findOverlaps(root, queryBox, leaves);

  // Convert to TreeEntry and filter by layer
  for (Leaf* leaf : leaves) {
    // Filter by layer if specified
    if (layer >= 0 && leaf->object->getShapeLayer(leaf->shapeIndex) != layer) {
      continue;
    }

    results.emplace_back(leaf->object, leaf->shapeIndex);
  }
}

void ShapeSearchTree::overlappingEntries(const IntBox& queryBox,
                                        std::vector<TreeEntry>& results) const {
  overlappingEntries(queryBox, -1, results);  // -1 = all layers
}

void ShapeSearchTree::findOverlaps(TreeNode* node, const IntBox& queryBox,
                                  std::vector<Leaf*>& leaves) const {
  if (!node) return;

  // Check if node's bounding box intersects query box
  if (!node->boundingBox.intersects(queryBox)) {
    return;  // Prune this subtree
  }

  if (node->isLeaf()) {
    // Found overlapping leaf
    leaves.push_back(static_cast<Leaf*>(node));
  } else {
    // Recurse on children
    auto* inner = static_cast<InnerNode*>(node);
    findOverlaps(inner->firstChild, queryBox, leaves);
    findOverlaps(inner->secondChild, queryBox, leaves);
  }
}

} // namespace freerouting
