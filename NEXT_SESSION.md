# Next Session: SortedRoomNeighbours Implementation

**Date**: February 17, 2026
**Current Status**: Phase 1 scaffolding complete, ready to implement door generation

## Where We Left Off

Successfully implemented:
1. ✅ MazeSearchAlgo core algorithm (~700 lines) - executes in 4ms
2. ✅ DestinationDistance A* heuristic (346 lines)
3. ✅ completeNeighbourRooms() entry point (40 lines)
4. ✅ calculateDoors() stub method

**Current blocker**: calculateDoors() is stubbed and returns no doors, so maze search has no graph to traverse. All routes fail as expected.

## What Needs to Be Done

### Immediate Task: Implement calculateDoors() Dispatcher

**File**: src/autoroute/AutorouteEngine.cpp line 742

**Current code**:
```cpp
void AutorouteEngine::calculateDoors(ObstacleExpansionRoom* room) {
  // TODO Phase 1: Minimal implementation - do nothing for now
  (void)room;
}
```

**Should become** (Java lines 459-469):
```cpp
void AutorouteEngine::calculateDoors(ObstacleExpansionRoom* room) {
  // Dispatch to appropriate algorithm based on search tree type
  // For now, use general case SortedRoomNeighbours
  SortedRoomNeighbours::calculate(room, this);
}
```

### Main Task: Port SortedRoomNeighbours Class

**Estimated effort**: 8-12 hours for full port
**Lines to port**: ~540 lines
**Complexity**: HIGH - involves geometric algorithms and complex sorting

#### Class Structure to Create

**File**: include/autoroute/SortedRoomNeighbours.h

```cpp
namespace freerouting {

// Forward declarations
class ExpansionRoom;
class CompleteExpansionRoom;
class AutorouteEngine;
class Shape;
class ShapeSearchTree;

class SortedRoomNeighbours {
public:
  // Main entry point - calculates doors for a room
  static CompleteExpansionRoom* calculate(ExpansionRoom* room,
                                          AutorouteEngine* autorouteEngine);

private:
  // Nested helper class for sorting neighbours counterclockwise
  struct SortedRoomNeighbour {
    const Shape* neighbourShape;
    const Shape* intersection;
    int touchingSideNoOfRoom;
    int touchingSideNoOfNeighbourRoom;
    bool roomTouchIsCorner;
    bool neighbourRoomTouchIsCorner;

    // Cached corner calculations
    mutable IntPoint* precalculatedFirstCorner;
    mutable IntPoint* precalculatedLastCorner;

    // Comparison for std::set ordering (counterclockwise)
    bool operator<(const SortedRoomNeighbour& other) const;

    // Corner calculation helpers
    IntPoint firstCorner() const;
    IntPoint lastCorner() const;
  };

  // Member variables
  ExpansionRoom* fromRoom;
  CompleteExpansionRoom* completedRoom;
  const Shape* roomShape;
  std::set<SortedRoomNeighbour> sortedNeighbours;
  std::vector<void*> ownNetObjects;  // ShapeTree::TreeEntry

  // Constructor (private - use calculate() factory method)
  SortedRoomNeighbours(ExpansionRoom* room, AutorouteEngine* engine);

  // Helper methods
  static CompleteExpansionRoom* calculateNeighbours(
    ExpansionRoom* room, AutorouteEngine* engine);

  static void calculateIncompleteRoomsWithEmptyNeighbours(
    ObstacleExpansionRoom* room, AutorouteEngine* engine);

  static void calculateTargetDoors(
    CompleteFreeSpaceExpansionRoom* room, AutorouteEngine* engine);

  bool tryRemoveEdge(int edgeNo);

  void calculateNewIncompleteRooms(AutorouteEngine* engine);

  void addSortedNeighbour(const SortedRoomNeighbour& neighbour);

  static bool insertDoorOk(/* various overloads */);
};

} // namespace freerouting
```

#### Implementation Priority (Do in Order)

**Phase 2A: Minimal Viable Door Generation** (3-4 hours)
1. Create SortedRoomNeighbours skeleton class
2. Implement calculate() stub that returns the input room
3. Implement calculateNeighbours() that finds overlapping rooms but doesn't sort yet
4. Create simple doors to existing complete rooms only (no new incomplete rooms)
5. Test - should get SOME routes working on simple boards

**Phase 2B: Sorting and Edge Removal** (2-3 hours)
6. Implement SortedRoomNeighbour comparison operator for counterclockwise sorting
7. Implement tryRemoveEdge() to enlarge rooms when edges have no neighbours
8. Implement calculateTargetDoors() to create doors to start/destination items
9. Test - should improve route success rate

**Phase 2C: New Room Creation** (3-4 hours)
10. Implement calculateNewIncompleteRooms() - creates doors to unexplored areas
11. Implement corner calculation and border traversal logic
12. Implement insertDoorOk() validation methods
13. Test - should match Java functionality

**Phase 2D: Specialized Variants** (2-3 hours, optional)
14. Implement SortedOrthogonalRoomNeighbours for 90-degree routing
15. Implement Sorted45DegreeRoomNeighbours for 45-degree routing
16. Update calculateDoors() dispatcher to choose based on search tree type
17. Performance profiling and optimization

## Testing Strategy

### Test 1: Verify calculateDoors() is Called
- Add debug printf in calculateDoors() stub
- Run Issue026 board
- Verify method is called during completeNeighbourRooms()

### Test 2: First Successful Route
- After Phase 2A implementation
- Create simplest possible test board (2 pads, one trace)
- Should successfully route at least one connection
- Validates basic door generation works

### Test 3: Simple Board Success Rate
- After Phase 2B implementation
- Run Issue026 board (45 connections)
- Should route 50%+ successfully
- Validates sorting and edge removal works

### Test 4: Complex Board Performance
- After Phase 2C implementation
- Run Issue026 board - should route 90%+ successfully
- Run NexRx board - should start routing (may not complete all)
- Validates full algorithm correctness

## Key Dependencies Still Missing

These will need stub implementations or basic versions:

1. **ShapeSearchTree::overlapping_tree_entries()**
   - Returns empty vector for now
   - Phase 2B should implement basic version

2. **Shape::get_border()**
   - Returns empty border for now
   - Phase 2C needs real implementation for counterclockwise traversal

3. **Shape geometric operations**
   - intersection(), offset(), split_to_convex()
   - Can stub with simpler versions initially

4. **TreeEntry management**
   - Basic structure exists, needs query methods

## Expected Outcomes

**After Phase 2A**: First successful route on simple 2-pad board
**After Phase 2B**: Issue026 routes 50%+ connections
**After Phase 2C**: Issue026 routes 90%+, NexRx starts routing
**After Phase 2D**: Performance matches or exceeds Java

## Files to Create/Modify

**New files**:
- include/autoroute/SortedRoomNeighbours.h (~150 lines)
- src/autoroute/SortedRoomNeighbours.cpp (~600 lines)

**Modified files**:
- src/autoroute/AutorouteEngine.cpp (implement calculateDoors dispatcher)
- CMakeLists.txt (add SortedRoomNeighbours.cpp)

## Commit Strategy

Make small, incremental commits:
1. "Add SortedRoomNeighbours skeleton structure"
2. "Implement calculateNeighbours basic version"
3. "Add simple door generation to existing rooms"
4. "Test Phase 2A - first successful route"
5. Continue incrementally...

## Notes from This Session

- MazeSearchAlgo core is complete and executes correctly
- completeNeighbourRooms() infrastructure is in place
- Build is clean and stable
- Ready to start actual door generation
- SortedRoomNeighbours is the last major piece blocking functional routing

## Time Estimate

- Phase 2A: 3-4 hours → First route working
- Phase 2B: 2-3 hours → 50% success rate
- Phase 2C: 3-4 hours → 90% success rate
- Phase 2D: 2-3 hours → Optimization

**Total**: 10-14 hours of focused development

**Recommendation**: Start with Phase 2A in next session, aim to get first successful route. This will validate the entire approach before investing in the complex sorting logic.
