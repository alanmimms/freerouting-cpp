#include <catch2/catch_test_macros.hpp>
#include "geometry/SpatialIndex.h"
#include "geometry/ShapeTree.h"
#include "geometry/CollisionDetector.h"
#include "geometry/Circle.h"
#include "geometry/IntBox.h"
#include "board/Item.h"
#include "board/FixedState.h"
#include "datastructures/Stoppable.h"
#include "datastructures/TimeLimit.h"
#include "datastructures/UnionFind.h"
#include <thread>

using namespace freerouting;

// ============================================================================
// SpatialIndex Tests
// ============================================================================

// Simple test object for spatial indexing
struct TestObject {
  int id;
  IntBox bounds;

  TestObject(int idVal, IntBox boundsVal)
    : id(idVal), bounds(boundsVal) {}
};

TEST_CASE("SpatialIndex - Construction", "[datastructures][spatialindex]") {
  SpatialIndex<TestObject> index(1000);

  REQUIRE(index.cellCount() == 0);
  REQUIRE(index.itemReferenceCount() == 0);
}

TEST_CASE("SpatialIndex - Insert and query single item", "[datastructures][spatialindex]") {
  SpatialIndex<TestObject> index(100);  // Smaller cell size for precise queries

  TestObject obj(1, IntBox(0, 0, 100, 100));
  index.insert(&obj, obj.bounds);

  // Query region containing the item
  auto results = index.query(IntBox(0, 0, 200, 200));
  REQUIRE(results.size() == 1);
  REQUIRE(results[0]->id == 1);

  // Query region not containing the item
  auto empty = index.query(IntBox(500, 500, 600, 600));
  REQUIRE(empty.empty());
}

TEST_CASE("SpatialIndex - Insert multiple items", "[datastructures][spatialindex]") {
  SpatialIndex<TestObject> index(100);  // Smaller cell size for precise queries

  TestObject obj1(1, IntBox(0, 0, 100, 100));
  TestObject obj2(2, IntBox(200, 200, 300, 300));
  TestObject obj3(3, IntBox(400, 400, 500, 500));

  index.insert(&obj1, obj1.bounds);
  index.insert(&obj2, obj2.bounds);
  index.insert(&obj3, obj3.bounds);

  // Query all
  auto all = index.query(IntBox(0, 0, 600, 600));
  REQUIRE(all.size() == 3);

  // Query subset
  auto subset = index.query(IntBox(0, 0, 250, 250));
  REQUIRE(subset.size() == 2);
}

TEST_CASE("SpatialIndex - Remove items", "[datastructures][spatialindex]") {
  SpatialIndex<TestObject> index(1000);

  TestObject obj1(1, IntBox(0, 0, 100, 100));
  TestObject obj2(2, IntBox(200, 200, 300, 300));

  index.insert(&obj1, obj1.bounds);
  index.insert(&obj2, obj2.bounds);

  auto before = index.query(IntBox(0, 0, 400, 400));
  REQUIRE(before.size() == 2);

  index.remove(&obj1, obj1.bounds);

  auto after = index.query(IntBox(0, 0, 400, 400));
  REQUIRE(after.size() == 1);
  REQUIRE(after[0]->id == 2);
}

TEST_CASE("SpatialIndex - Query near point", "[datastructures][spatialindex]") {
  SpatialIndex<TestObject> index(1000);

  TestObject obj1(1, IntBox(0, 0, 100, 100));
  TestObject obj2(2, IntBox(2000, 2000, 2100, 2100));

  index.insert(&obj1, obj1.bounds);
  index.insert(&obj2, obj2.bounds);

  // Query near first object
  auto near1 = index.queryNear(IntPoint(50, 50), 500);
  REQUIRE(near1.size() == 1);
  REQUIRE(near1[0]->id == 1);

  // Query near second object
  auto near2 = index.queryNear(IntPoint(2050, 2050), 500);
  REQUIRE(near2.size() == 1);
  REQUIRE(near2[0]->id == 2);
}

TEST_CASE("SpatialIndex - No duplicates in results", "[datastructures][spatialindex]") {
  SpatialIndex<TestObject> index(100);  // Small cell size

  // Large object spanning multiple cells
  TestObject obj(1, IntBox(0, 0, 500, 500));
  index.insert(&obj, obj.bounds);

  // Query should return object only once even though it's in many cells
  auto results = index.query(IntBox(0, 0, 500, 500));
  REQUIRE(results.size() == 1);
}

TEST_CASE("SpatialIndex - Clear", "[datastructures][spatialindex]") {
  SpatialIndex<TestObject> index(1000);

  TestObject obj1(1, IntBox(0, 0, 100, 100));
  TestObject obj2(2, IntBox(200, 200, 300, 300));

  index.insert(&obj1, obj1.bounds);
  index.insert(&obj2, obj2.bounds);

  REQUIRE(index.cellCount() > 0);

  index.clear();

  REQUIRE(index.cellCount() == 0);
  REQUIRE(index.itemReferenceCount() == 0);

  auto results = index.query(IntBox(0, 0, 400, 400));
  REQUIRE(results.empty());
}

// ============================================================================
// ShapeTree Tests
// ============================================================================

// Test item for ShapeTree
class TreeTestItem : public Item {
public:
  TreeTestItem(int id, int layer, IntBox box, int netNo = 0)
    : Item((netNo == 0) ? std::vector<int>{} : std::vector<int>{netNo},
           0, id, 0, FixedState::NotFixed, nullptr),
      box_(box),
      layer_(layer) {}

  IntBox getBoundingBox() const override { return box_; }
  int firstLayer() const override { return layer_; }
  int lastLayer() const override { return layer_; }

  bool isObstacle(const Item& other) const override {
    if (!isOnLayer(other.firstLayer())) {
      return false;
    }
    return !sharesNet(other);
  }

  Item* copy(int newId) const override {
    int netNo = getNets().empty() ? 0 : getNets()[0];
    return new TreeTestItem(newId, layer_, box_, netNo);
  }

private:
  IntBox box_;
  int layer_;
};

TEST_CASE("ShapeTree - Construction", "[datastructures][shapetree]") {
  ShapeTree tree(1000);

  REQUIRE(tree.cellCount() == 0);
  REQUIRE(tree.itemReferenceCount() == 0);
}

TEST_CASE("ShapeTree - Insert and query", "[datastructures][shapetree]") {
  ShapeTree tree(1000);

  TreeTestItem item1(1, 0, IntBox(0, 0, 100, 100), 1);
  TreeTestItem item2(2, 0, IntBox(200, 200, 300, 300), 2);

  tree.insert(&item1);
  tree.insert(&item2);

  auto results = tree.queryRegion(IntBox(0, 0, 400, 400));
  REQUIRE(results.size() == 2);
}

TEST_CASE("ShapeTree - Find obstacles", "[datastructures][shapetree]") {
  ShapeTree tree(1000);

  TreeTestItem item1(1, 0, IntBox(0, 0, 100, 100), 1);
  TreeTestItem item2(2, 0, IntBox(50, 50, 150, 150), 2);  // Overlaps, different net
  TreeTestItem item3(3, 0, IntBox(200, 200, 300, 300), 1);  // Same net as item1

  tree.insert(&item1);
  tree.insert(&item2);
  tree.insert(&item3);

  // Find obstacles for item1 (different net = obstacle)
  auto obstacles = tree.findObstacles(item1);
  REQUIRE(obstacles.size() == 1);
  REQUIRE(obstacles[0]->getId() == 2);
}

TEST_CASE("ShapeTree - Find items on layer", "[datastructures][shapetree]") {
  ShapeTree tree(1000);

  TreeTestItem item1(1, 0, IntBox(0, 0, 100, 100), 1);
  TreeTestItem item2(2, 1, IntBox(50, 50, 150, 150), 2);
  TreeTestItem item3(3, 0, IntBox(200, 200, 300, 300), 1);

  tree.insert(&item1);
  tree.insert(&item2);
  tree.insert(&item3);

  auto layer0Items = tree.findItemsOnLayer(0, IntBox(0, 0, 400, 400));
  REQUIRE(layer0Items.size() == 2);

  auto layer1Items = tree.findItemsOnLayer(1, IntBox(0, 0, 400, 400));
  REQUIRE(layer1Items.size() == 1);
  REQUIRE(layer1Items[0]->getId() == 2);
}

TEST_CASE("ShapeTree - Find items by net", "[datastructures][shapetree]") {
  ShapeTree tree(1000);

  TreeTestItem item1(1, 0, IntBox(0, 0, 100, 100), 1);
  TreeTestItem item2(2, 0, IntBox(50, 50, 150, 150), 2);
  TreeTestItem item3(3, 0, IntBox(200, 200, 300, 300), 1);

  tree.insert(&item1);
  tree.insert(&item2);
  tree.insert(&item3);

  auto net1Items = tree.findItemsByNet(1, IntBox(0, 0, 400, 400));
  REQUIRE(net1Items.size() == 2);

  auto net2Items = tree.findItemsByNet(2, IntBox(0, 0, 400, 400));
  REQUIRE(net2Items.size() == 1);
  REQUIRE(net2Items[0]->getId() == 2);
}

TEST_CASE("ShapeTree - Find trace obstacles", "[datastructures][shapetree]") {
  ShapeTree tree(1000);

  TreeTestItem item1(1, 0, IntBox(0, 0, 100, 100), 1);
  TreeTestItem item2(2, 0, IntBox(50, 50, 150, 150), 2);
  TreeTestItem item3(3, 0, IntBox(100, 100, 200, 200), 1);

  tree.insert(&item1);
  tree.insert(&item2);
  tree.insert(&item3);

  // Find obstacles for net 1 (items on different nets)
  auto obstacles = tree.findTraceObstacles(1, IntBox(0, 0, 200, 200), 0, 0);
  REQUIRE(obstacles.size() == 1);
  REQUIRE(obstacles[0]->getId() == 2);
}

// ============================================================================
// CollisionDetector Tests
// ============================================================================

TEST_CASE("CollisionDetector - Box intersection", "[datastructures][collision]") {
  IntBox box1(0, 0, 100, 100);
  IntBox box2(50, 50, 150, 150);
  IntBox box3(200, 200, 300, 300);

  REQUIRE(CollisionDetector::boxesIntersect(box1, box2));
  REQUIRE_FALSE(CollisionDetector::boxesIntersect(box1, box3));
}

TEST_CASE("CollisionDetector - Circle intersection", "[datastructures][collision]") {
  Circle c1(IntPoint(0, 0), 100);
  Circle c2(IntPoint(150, 0), 100);
  Circle c3(IntPoint(500, 0), 100);

  REQUIRE(CollisionDetector::circlesIntersect(c1, c2));
  REQUIRE_FALSE(CollisionDetector::circlesIntersect(c1, c3));
}

TEST_CASE("CollisionDetector - Circle-box intersection", "[datastructures][collision]") {
  Circle circle(IntPoint(100, 100), 50);
  IntBox box1(0, 0, 200, 200);
  IntBox box2(500, 500, 600, 600);

  REQUIRE(CollisionDetector::circleBoxIntersect(circle, box1));
  REQUIRE_FALSE(CollisionDetector::circleBoxIntersect(circle, box2));
}

TEST_CASE("CollisionDetector - Point containment", "[datastructures][collision]") {
  IntPoint point(50, 50);
  IntBox box(0, 0, 100, 100);
  Circle circle(IntPoint(100, 100), 75);

  REQUIRE(CollisionDetector::pointInBox(point, box));
  REQUIRE(CollisionDetector::pointInCircle(point, circle));

  IntPoint outsidePoint(500, 500);
  REQUIRE_FALSE(CollisionDetector::pointInBox(outsidePoint, box));
  REQUIRE_FALSE(CollisionDetector::pointInCircle(outsidePoint, circle));
}

TEST_CASE("CollisionDetector - Box distance", "[datastructures][collision]") {
  IntBox box1(0, 0, 100, 100);
  IntBox box2(150, 0, 250, 100);
  IntBox box3(50, 50, 150, 150);

  // Adjacent boxes
  double dist = CollisionDetector::boxDistance(box1, box2);
  REQUIRE(dist == 50.0);

  // Overlapping boxes
  double overlapDist = CollisionDetector::boxDistance(box1, box3);
  REQUIRE(overlapDist == 0.0);
}

TEST_CASE("CollisionDetector - Circle distance", "[datastructures][collision]") {
  Circle c1(IntPoint(0, 0), 100);
  Circle c2(IntPoint(300, 0), 100);
  Circle c3(IntPoint(150, 0), 100);

  // Non-overlapping circles
  double dist = CollisionDetector::circleDistance(c1, c2);
  REQUIRE(dist == 100.0);

  // Overlapping circles
  double overlapDist = CollisionDetector::circleDistance(c1, c3);
  REQUIRE(overlapDist == 0.0);
}

TEST_CASE("CollisionDetector - Box clearance", "[datastructures][collision]") {
  IntBox box1(0, 0, 100, 100);
  IntBox box2(150, 0, 250, 100);

  double clearance = CollisionDetector::boxClearance(box1, box2);
  REQUIRE(clearance == 50.0);

  // Overlapping boxes (negative clearance)
  IntBox box3(50, 50, 150, 150);
  double negClearance = CollisionDetector::boxClearance(box1, box3);
  REQUIRE(negClearance < 0.0);
}

TEST_CASE("CollisionDetector - Boxes within clearance", "[datastructures][collision]") {
  IntBox box1(0, 0, 100, 100);
  IntBox box2(150, 0, 250, 100);

  REQUIRE(CollisionDetector::boxesWithinClearance(box1, box2, 50));
  REQUIRE(CollisionDetector::boxesWithinClearance(box1, box2, 100));
  REQUIRE_FALSE(CollisionDetector::boxesWithinClearance(box1, box2, 20));
}

TEST_CASE("CollisionDetector - Segment-box intersection", "[datastructures][collision]") {
  IntBox box(100, 100, 200, 200);

  // Segment crossing box
  REQUIRE(CollisionDetector::segmentBoxIntersect(IntPoint(50, 150), IntPoint(250, 150), box));

  // Segment with endpoint inside box
  REQUIRE(CollisionDetector::segmentBoxIntersect(IntPoint(150, 150), IntPoint(300, 300), box));

  // Segment not intersecting box
  REQUIRE_FALSE(CollisionDetector::segmentBoxIntersect(IntPoint(0, 0), IntPoint(50, 50), box));
}

TEST_CASE("CollisionDetector - Segment-circle intersection", "[datastructures][collision]") {
  Circle circle(IntPoint(100, 100), 50);

  // Segment crossing circle
  REQUIRE(CollisionDetector::segmentCircleIntersect(IntPoint(50, 100), IntPoint(150, 100), circle));

  // Segment with endpoint inside circle
  REQUIRE(CollisionDetector::segmentCircleIntersect(IntPoint(100, 100), IntPoint(200, 200), circle));

  // Segment not intersecting circle
  REQUIRE_FALSE(CollisionDetector::segmentCircleIntersect(IntPoint(0, 0), IntPoint(10, 10), circle));
}

// ============================================================================
// Stoppable Tests
// ============================================================================

TEST_CASE("SimpleStoppable - Basic functionality", "[datastructures][stoppable]") {
  SimpleStoppable stoppable;

  REQUIRE_FALSE(stoppable.isStopRequested());

  stoppable.requestStop();
  REQUIRE(stoppable.isStopRequested());

  stoppable.reset();
  REQUIRE_FALSE(stoppable.isStopRequested());
}

// ============================================================================
// TimeLimit Tests
// ============================================================================

TEST_CASE("TimeLimit - Unlimited", "[datastructures][timelimit]") {
  TimeLimit limit;

  REQUIRE_FALSE(limit.isExceeded());
  REQUIRE(limit.getRemainingMs() == -1);
}

TEST_CASE("TimeLimit - With limit", "[datastructures][timelimit]") {
  TimeLimit limit(100);  // 100ms limit

  REQUIRE_FALSE(limit.isExceeded());
  REQUIRE(limit.getRemainingMs() > 0);

  // Sleep to exceed limit
  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  REQUIRE(limit.isExceeded());
  REQUIRE(limit.getElapsedMs() >= 100);
}

TEST_CASE("TimeLimit - Reset", "[datastructures][timelimit]") {
  TimeLimit limit(50);

  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  REQUIRE(limit.isExceeded());

  limit.reset();
  REQUIRE_FALSE(limit.isExceeded());
}

TEST_CASE("TimeLimit - Set limit", "[datastructures][timelimit]") {
  TimeLimit limit(1000);

  REQUIRE_FALSE(limit.isExceeded());

  limit.setLimit(10);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  REQUIRE(limit.isExceeded());
}

// ============================================================================
// UnionFind Tests
// ============================================================================

TEST_CASE("UnionFind - Construction", "[datastructures][unionfind]") {
  UnionFind uf(10);

  REQUIRE(uf.size() == 10);
  REQUIRE(uf.getComponentCount() == 10);
  REQUIRE_FALSE(uf.isFullyConnected());
}

TEST_CASE("UnionFind - Initial state", "[datastructures][unionfind]") {
  UnionFind uf(5);

  // Each element should be its own component
  for (int i = 0; i < 5; i++) {
    REQUIRE(uf.find(i) == i);
  }

  // No elements should be connected
  for (int i = 0; i < 5; i++) {
    for (int j = i + 1; j < 5; j++) {
      REQUIRE_FALSE(uf.connected(i, j));
    }
  }
}

TEST_CASE("UnionFind - Simple union", "[datastructures][unionfind]") {
  UnionFind uf(5);

  REQUIRE(uf.unite(0, 1));
  REQUIRE(uf.connected(0, 1));
  REQUIRE(uf.getComponentCount() == 4);
}

TEST_CASE("UnionFind - Multiple unions", "[datastructures][unionfind]") {
  UnionFind uf(10);

  uf.unite(0, 1);
  uf.unite(1, 2);
  uf.unite(3, 4);

  // 0, 1, 2 should be connected
  REQUIRE(uf.connected(0, 1));
  REQUIRE(uf.connected(0, 2));
  REQUIRE(uf.connected(1, 2));

  // 3, 4 should be connected
  REQUIRE(uf.connected(3, 4));

  // But 0-2 group should not be connected to 3-4 group
  REQUIRE_FALSE(uf.connected(0, 3));
  REQUIRE_FALSE(uf.connected(2, 4));

  REQUIRE(uf.getComponentCount() == 7);
}

TEST_CASE("UnionFind - Union of same set", "[datastructures][unionfind]") {
  UnionFind uf(5);

  uf.unite(0, 1);
  int initialCount = uf.getComponentCount();

  // Uniting already connected elements should return false
  REQUIRE_FALSE(uf.unite(0, 1));

  // Component count should not change
  REQUIRE(uf.getComponentCount() == initialCount);
}

TEST_CASE("UnionFind - Transitive connectivity", "[datastructures][unionfind]") {
  UnionFind uf(10);

  uf.unite(0, 1);
  uf.unite(1, 2);
  uf.unite(2, 3);
  uf.unite(3, 4);

  // All elements 0-4 should be connected through transitive unions
  REQUIRE(uf.connected(0, 4));
  REQUIRE(uf.connected(1, 3));
  REQUIRE(uf.connected(2, 4));
}

TEST_CASE("UnionFind - Get set size", "[datastructures][unionfind]") {
  UnionFind uf(10);

  REQUIRE(uf.getSetSize(0) == 1);

  uf.unite(0, 1);
  uf.unite(1, 2);

  REQUIRE(uf.getSetSize(0) == 3);
  REQUIRE(uf.getSetSize(1) == 3);
  REQUIRE(uf.getSetSize(2) == 3);
  REQUIRE(uf.getSetSize(3) == 1);
}

TEST_CASE("UnionFind - Fully connected", "[datastructures][unionfind]") {
  UnionFind uf(5);

  REQUIRE_FALSE(uf.isFullyConnected());

  uf.unite(0, 1);
  uf.unite(1, 2);
  uf.unite(2, 3);
  uf.unite(3, 4);

  REQUIRE(uf.isFullyConnected());
  REQUIRE(uf.getComponentCount() == 1);
}

TEST_CASE("UnionFind - Reset", "[datastructures][unionfind]") {
  UnionFind uf(5);

  uf.unite(0, 1);
  uf.unite(2, 3);
  REQUIRE(uf.getComponentCount() == 3);

  uf.reset();
  REQUIRE(uf.getComponentCount() == 5);
  REQUIRE_FALSE(uf.connected(0, 1));
  REQUIRE_FALSE(uf.connected(2, 3));
}

TEST_CASE("UnionFind - Path compression", "[datastructures][unionfind]") {
  UnionFind uf(100);

  // Create a long chain
  for (int i = 0; i < 99; i++) {
    uf.unite(i, i + 1);
  }

  // All elements should be connected
  REQUIRE(uf.connected(0, 99));
  REQUIRE(uf.getComponentCount() == 1);

  // Path compression should make subsequent finds faster
  // (Can't directly test performance, but verify correctness)
  int root = uf.find(99);
  REQUIRE(root == uf.find(0));
}

TEST_CASE("UnionFind - Invalid elements", "[datastructures][unionfind]") {
  UnionFind uf(10);

  // Invalid element indices
  REQUIRE(uf.find(-1) == -1);
  REQUIRE(uf.find(10) == -1);
  REQUIRE(uf.find(100) == -1);

  REQUIRE_FALSE(uf.unite(-1, 5));
  REQUIRE_FALSE(uf.unite(5, 15));
  REQUIRE_FALSE(uf.connected(-1, 5));
}
