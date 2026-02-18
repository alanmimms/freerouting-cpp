# Room Completion Infrastructure - Porting Plan

**Date**: February 17, 2026
**Status**: Planning phase after MazeSearchAlgo core completion

## Overview

Room completion is the critical missing piece preventing the MazeSearchAlgo from finding routes. Without completed rooms with doors, the maze search has no graph to traverse.

## Current Blocker

The MazeSearchAlgo calls `autorouteEngine->completeNeighbourRooms(room)` which is currently stubbed:

```cpp
void AutorouteEngine::completeNeighbourRooms(ExpansionRoom* room) {
  // TODO: Complete implementation
  (void)room;
}
```

This causes all routing attempts to fail with "Pathfinding failed" because no doors are generated between rooms.

## Java Implementation Analysis

### 1. completeNeighbourRooms() - Entry Point
**File**: AutorouteEngine.java, ~27 lines
**Purpose**: Iterate through a room's doors and complete/calculate all neighbor rooms

**Logic**:
1. Snapshot door list (avoid ConcurrentModificationException)
2. For each door, get the neighbor room
3. If neighbor is `IncompleteFreeSpaceExpansionRoom`: call `complete_expansion_room()`
4. If neighbor is `ObstacleExpansionRoom` without doors: call `calculate_doors()`

**Dependencies**: complete_expansion_room(), calculate_doors()

### 2. complete_expansion_room() - Shape Finalization
**File**: AutorouteEngine.java, lines 384-434 (~50 lines)
**Purpose**: Convert IncompleteFreeSpaceExpansionRoom to CompleteFreeSpaceExpansionRoom(s)

**Logic**:
1. Call `autoroute_search_tree.complete_shape()` to finalize shape by obstacle detection
2. For each resulting shape (may split into multiple):
   - Create CompleteFreeSpaceExpansionRoom
   - Call `add_complete_room()` to calculate doors and add to database
3. Remove incomplete room from incomplete list
4. Return collection of complete rooms

**Key insight**: A single incomplete room can become multiple complete rooms if obstacles split it.

**Dependencies**:
- ShapeSearchTree.complete_shape() - obstacle detection and shape finalization
- add_complete_room() - door calculation wrapper
- calculate_doors() - actual door generation

### 3. calculate_doors() - Door Generation Dispatcher
**File**: AutorouteEngine.java, lines 459-469 (~10 lines)
**Purpose**: Delegate to specialized door calculation algorithm based on routing geometry

**Logic**:
```java
if (autoroute_search_tree instanceof ShapeSearchTree90Degree) {
  return SortedOrthogonalRoomNeighbours.calculate(...);
} else if (autoroute_search_tree instanceof ShapeSearchTree45Degree) {
  return Sorted45DegreeRoomNeighbours.calculate(...);
} else {
  return SortedRoomNeighbours.calculate(...);
}
```

**Dependencies**: SortedRoomNeighbours and variants

### 4. SortedRoomNeighbours.calculate() - Core Algorithm
**File**: SortedRoomNeighbours.java (~680 lines total)
**Purpose**: Generate all doors for a room by finding neighbors and creating connections

**Main method** (lines 52-86, ~35 lines):
1. `calculate_neighbours()` - Find overlapping rooms/obstacles, sort counterclockwise
2. `try_remove_edge()` - Remove edges without neighbors (recursive optimization)
3. `calculate_new_incomplete_rooms()` - Create doors to new expansion areas (MOST COMPLEX)
4. `calculate_target_doors()` - Create doors to start/destination items

**Most complex part**: `calculate_new_incomplete_rooms()` (lines 369-531, ~162 lines)
- Handles corner cutting to avoid obstacles
- Creates new incomplete rooms for unexplored free space
- Manages door creation between rooms
- Uses geometric intersection and edge tracking

**Key data structures**:
- `SortedRoomNeighbour`: neighbour_shape, intersection, touching_side_no
- Edge tracking: room_touching_sides_table, neighbour_touching_sides
- Border traces: TileShape.get_border() for counterclockwise traversal

## Estimated Porting Effort

| Component | Java Lines | Est. C++ Lines | Complexity | Time Est. |
|-----------|-----------|----------------|------------|-----------|
| completeNeighbourRooms() | 27 | 30 | Low | 30 min |
| complete_expansion_room() | 50 | 60 | Medium | 2 hrs |
| calculate_doors() dispatcher | 10 | 15 | Low | 15 min |
| SortedRoomNeighbours class | 680 | 800 | **HIGH** | 8-12 hrs |
| ShapeSearchTree.complete_shape() | ~100 | 120 | High | 3-4 hrs |
| Supporting utilities | 50 | 60 | Medium | 1-2 hrs |
| **TOTAL** | **~917** | **~1085** | - | **15-20 hrs** |

## Critical Dependencies Still Missing

### 1. ShapeSearchTree Infrastructure
**Purpose**: Spatial indexing for obstacles and rooms

**Key methods needed**:
- `complete_shape()` - Finalize room shape by obstacle detection
- `overlapping_tree_entries()` - Find all objects overlapping a shape
- `get_default_tree_entries()` - Get tree entries for an item

**Estimated size**: ~500 lines for basic functionality

### 2. Shape/TileShape Methods
**Purpose**: Geometric operations on room shapes

**Key methods needed**:
- `get_border()` - Get counterclockwise border trace
- `split_to_convex()` - Split concave shapes
- `intersection()` - Calculate shape intersections
- `offset()` - Expand/contract shapes

**Status**: Some exist, many missing

### 3. TreeEntry Management
**Purpose**: Track items in spatial index

**Key methods needed**:
- Creation and removal from search tree
- Overlap queries
- Layer filtering

**Status**: Stub class exists

## Recommended Phased Approach

### Phase 1: Minimal Door Generation (Week 1)
**Goal**: Get ANY doors generated, even if incomplete

**Tasks**:
1. Implement simple ShapeSearchTree.complete_shape() that returns input shape unchanged
2. Port completeNeighbourRooms() basic structure
3. Port complete_expansion_room() without obstacle detection
4. Port calculate_doors() dispatcher using simple fallback
5. Port minimal SortedRoomNeighbours that creates doors only to existing rooms (no new incomplete rooms)

**Expected result**: Some simple traces might route, but complex routes still fail

**Validation**: Issue026 should route at least 1-2 connections successfully

### Phase 2: Obstacle Detection (Week 2)
**Goal**: Proper obstacle avoidance and room splitting

**Tasks**:
1. Implement ShapeSearchTree.overlapping_tree_entries()
2. Implement ShapeSearchTree.complete_shape() with obstacle detection
3. Add room splitting when obstacles divide shapes
4. Handle multiple complete rooms from one incomplete room

**Expected result**: Routes avoid obstacles correctly

**Validation**: Issue026 should route 50%+ of connections

### Phase 3: New Room Creation (Week 3)
**Goal**: Full expansion into unexplored areas

**Tasks**:
1. Port calculate_new_incomplete_rooms() logic
2. Implement corner cutting algorithm
3. Implement edge removal optimization
4. Add proper border traversal

**Expected result**: Complete maze search functionality

**Validation**: Issue026 should route 90%+ of connections, NexRx should start routing

### Phase 4: Optimization (Week 4)
**Goal**: Match Java performance

**Tasks**:
1. Profile and optimize hotspots
2. Add 45-degree and 90-degree specialized variants
3. Implement caching where beneficial
4. Memory pool optimizations

**Expected result**: Performance at or better than Java

**Validation**: NexRx routes in minutes instead of hours

## Next Immediate Steps

1. Create ShapeSearchTree stub class with minimal methods
2. Implement Phase 1 minimal door generation
3. Test on simplest possible board (2 pads, single trace)
4. Iterate until first successful route

## Open Questions

1. Should we implement the general SortedRoomNeighbours first, or start with the simpler SortedOrthogonalRoomNeighbours (90-degree)?
   - **Recommendation**: Start with general case, it's only ~100 lines more and handles all cases

2. How much of ShapeSearchTree do we need immediately?
   - **Recommendation**: Start with stubs that return empty results, add functionality as needed

3. Should we port all geometric Shape methods first?
   - **Recommendation**: Port on-demand as compilation errors appear

4. Test strategy?
   - **Recommendation**: Unit tests for door creation, integration test for simple 2-pad route

## Success Criteria

**Phase 1 Complete**: At least 1 trace routes successfully on simple board
**Phase 2 Complete**: Issue026 routes 50%+ successfully
**Phase 3 Complete**: Issue026 routes 90%+ successfully
**Phase 4 Complete**: NexRx routes in reasonable time (<10 minutes)

## Current Session Summary

**Completed**:
- ✅ Analyzed room completion infrastructure requirements
- ✅ Identified 3 main components and dependencies
- ✅ Created 4-phase porting plan
- ✅ Estimated 15-20 hours of development effort

**Next session should start with**: Phase 1 minimal door generation
