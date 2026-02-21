# Critical Fix Needed: Board Items Not in Search Tree

## Problem

Maze search cannot expand because rooms have 0 doors.

```
expandToRoomDoors: room has 0 doors  ← STUCK!
createRoomToRoomDoors: found 1 overlapping objects  ← Only finds itself
```

## Root Cause

**Board items (traces, pads, vias) are NOT in the autoroute search tree.**

When `createRoomToRoomDoors()` queries for overlapping objects, it only finds other expansion rooms (usually just itself). Without board items in the tree, it cannot:
- Find obstacles to create doors around
- Navigate between free space areas  
- Detect conflicts
- Plan routes

## Java vs C++ Difference

**Java (correct)**:
```java
// During AutorouteEngine initialization:
for (Item item : board.get_items()) {
    if (item.is_routable()) {
        autoroute_search_tree.insert(item);  // ← Items go in tree!
    }
}
```

**C++ (broken)**:
```cpp
// AutorouteEngine constructor - MISSING item insertion!
// Only expansion rooms get added to tree later
```

## Solution

Add all board items to `autorouteSearchTree` during initialization.

### 1. Add Method to AutorouteEngine

**File:** `include/autoroute/AutorouteEngine.h`
```cpp
private:
  void initializeSearchTree();  // NEW method
```

**File:** `src/autoroute/AutorouteEngine.cpp`
```cpp
void AutorouteEngine::initializeSearchTree() {
  if (!autorouteSearchTree) {
    autorouteSearchTree = new ShapeSearchTree();
  }

  // Insert all routable board items into search tree
  const auto& items = board->getItems();
  for (const auto& itemPtr : items) {
    if (itemPtr && itemPtr->isRoutable()) {
      autorouteSearchTree->insert(itemPtr.get());
    }
  }

  std::cout << "Initialized search tree with " << items.size() << " board items\n";
}
```

### 2. Call During Initialization

**File:** `src/batch/BatchAutorouter.cpp`

Find the `autorouteConnection()` call and add initialization before it:

```cpp
void BatchAutorouter::runBatchLoop(Stoppable* stoppable) {
  // ... existing code ...

  // Initialize search tree with board items
  for (auto& engine : engines) {
    engine->initializeSearchTree();  // ← ADD THIS
  }

  // ... continue with routing ...
}
```

### 3. Expected Result

**Before**:
```
createRoomToRoomDoors: found 1 overlapping objects
expandToRoomDoors: room has 0 doors
```

**After**:
```
Initialized search tree with 1148 board items
createRoomToRoomDoors: found 47 overlapping objects
createRoomToRoomDoors: created door between rooms!
createRoomToRoomDoors: created door between rooms!
...
expandToRoomDoors: room has 12 doors
Maze search: 100 expansions...
Maze search: 200 expansions...
```

## Files to Modify

1. `include/autoroute/AutorouteEngine.h` - Declare initializeSearchTree()
2. `src/autoroute/AutorouteEngine.cpp` - Implement initializeSearchTree()
3. `src/batch/BatchAutorouter.cpp` - Call before routing

## Why This Fixes It

Once items are in the tree, the existing code in `createRoomToRoomDoors()` (lines 200-254) will:
1. Find overlapping Items via `overlappingEntries()`
2. Cast to `Item*`, check `isRoutable()`
3. Create `ObstacleExpansionRoom` for each item
4. Create doors between free space room and obstacle rooms
5. Maze search can now expand through these doors!

All the door generation logic is already implemented - it just needs items in the tree to work.
