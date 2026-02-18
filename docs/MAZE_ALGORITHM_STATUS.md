# Maze Search Algorithm Porting Status

## Executive Summary

**Current Status**: MazeSearchAlgo is using SimpleGridRouter as a fallback (src/autoroute/MazeSearchAlgo.cpp:87-138).

**Performance Impact**: SimpleGridRouter takes HOURS for complex boards; Java expansion room algorithm takes MINUTES.

**Root Cause**: Grid-based A* explores millions of cells vs. expansion rooms exploring hundreds of rooms/doors.

---

## Java Algorithm Complexity

- **Source**: freerouting.app/src/main/java/app/freerouting/autoroute/MazeSearchAlgo.java (1290 lines)
- **Algorithm**: Weighted A* search through expansion rooms connected by doors
- **Data Structures**: Priority queue, object pooling, spatial indexing (drill pages)
- **Optimizations**: Ripup/retry, trace shoving, layer changes, neckdown, small door filtering

---

## Infrastructure Status

### ✅ Completed (WIP commit d3506f1)

**Core Data Structures:**
- `MazeSearchElement` - Per-door section state (occupied, backtrack, ripup)
- `MazeListElement` - Priority queue elements with object pooling
- `ExpandableObject` - Base interface for doors/drills
- `ExpansionRoom` hierarchy - Room base classes
- `ExpansionDoor` - Borders between rooms
- `DrillPage`/`DrillPageArray` - Via spatial indexing

**Files Created:**
```
include/autoroute/MazeListElement.h
include/autoroute/MazeSearchElement.h
include/autoroute/ExpansionRoom.h
include/autoroute/ExpansionDoor.h
include/autoroute/CompleteExpansionRoom.h
include/autoroute/FreeSpaceExpansionRoom.h
include/autoroute/CompleteFreeSpaceExpansionRoom.h
include/autoroute/IncompleteFreeSpaceExpansionRoom.h
include/autoroute/ObstacleExpansionRoom.h
include/autoroute/TargetItemExpansionDoor.h
include/autoroute/ExpansionDrill.h
include/autoroute/DrillPage.h
include/autoroute/DrillPageArray.h
include/autoroute/ExpansionRoomGenerator.h
```

### ✅ Completed Components

**1. DestinationDistance (A* Heuristic)**
- ✅ Ported in src/autoroute/DestinationDistance.cpp (346 lines)
- Calculates admissible lower-bound distance to destination
- Layer-aware Manhattan distance with via costs
- Bounding boxes for component/solder/inner layers

**2. Main Algorithm Loop**
- ✅ Ported in src/autoroute/MazeSearchAlgo.cpp (~700 lines)
- `findConnection()` - Entry point
- `occupyNextElement()` - Main expansion step (lines 218-292)
- `expandToRoomDoors()` - Critical expansion logic (lines 298-469)
- `expandToDoorSection()` - Add candidates to priority queue
- `checkRipup()` - Ripup cost calculation with randomization
- Priority queue (MazeExpansionList wrapper)
- Helper methods (doorIsSmall, roomShapeIsThick, etc.)

### ❌ Missing Critical Components

**3. Room Completion (BLOCKER)**
- ⬜ `complete_neighbour_rooms()` - Currently stubbed in AutorouteEngine
- ⬜ Thickness checking (`room_shape_is_thick()`) - Stubbed, always returns true
- ⬜ Nearest border point calculations - Missing
- ⬜ **Java**: Integrated with AutorouteEngine
- **Why it fails**: Without room completion, no doors are generated, so maze search finds no path

**4. Supporting Infrastructure**
- ⬜ ShapeSearchTree - Spatial indexing for obstacles
- ⬜ PolylineTrace - Trace representation with multiple segments
- `shove_trace_room()` - Try pushing traces sideways (lines 1093-1144)
- Fanout via protection
- Detour factor calculations
- **Java**: ~200 lines

**5. Layer Change Handling**
- `expand_to_other_layers()` - Via expansion (lines 748-880)
- Via mask compatibility
- SMD attachment rules
- Forced via algorithm integration
- **Java**: ~130 lines

**6. Small Door Optimization**
- `door_is_small()` - Identify narrow gaps
- `check_leaving_ripped_item()` - Validate entry through small doors
- Prevents routing through tight spaces
- **Java**: ~80 lines

---

## Implementation Roadmap

### Phase 1: Minimal Viable Algorithm (1-2 weeks)

**Goal**: Replace SimpleGridRouter with basic expansion room search

**Tasks:**
1. Implement DestinationDistance heuristic calculator
   - Simple Manhattan distance to start
   - Add layer change costs
   - Bounding box calculations

2. Implement occupy_next_element() main loop
   - Priority queue processing
   - Backtrack information storage
   - Destination detection

3. Implement expand_to_room_doors() core expansion
   - Room completion stubs
   - Door iteration
   - Cost calculation (distance only, no ripup)

4. Implement expand_to_door_section()
   - Calculate expansion_value (g(n))
   - Calculate sorting_value (f(n) = g(n) + h(n))
   - Add MazeListElement to priority queue

5. Path reconstruction from backtrack information

**Testing**: Simple 2-layer boards with < 50 pads

**Expected Performance**: 10-100x faster than SimpleGridRouter on simple boards

---

### Phase 2: Multi-Layer Routing (1 week)

**Goal**: Add via support

**Tasks:**
1. Implement expand_to_other_layers()
   - Drill page iteration
   - Layer mask checking
   - Via cost addition

2. Integrate DrillPageArray spatial indexing

3. Test on boards requiring vias

**Testing**: 4-layer boards with 50-200 pads

---

### Phase 3: Obstacle Handling (2 weeks)

**Goal**: Add ripup and shove capabilities

**Tasks:**
1. Implement check_ripup()
   - Base ripup cost calculation
   - Already-ripped item detection
   - Fanout via protection

2. Implement shove_trace_room()
   - MazeShoveTraceAlgo integration
   - LEFT/RIGHT adjustment handling

3. Implement small door optimization
   - door_is_small() detection
   - check_leaving_ripped_item() validation

**Testing**: Dense boards with 200-500 pads

---

### Phase 4: Optimization & Polish (1-2 weeks)

**Goal**: Match Java performance

**Tasks:**
1. Room thickness checking
2. Neckdown at pins
3. Detour factor calculations
4. Randomization for escaping loops
5. Performance profiling and optimization

**Testing**: Complex boards (NexRx: 1148 items)

---

## Current SimpleGridRouter Bottlenecks

**File**: src/autoroute/SimpleGridRouter.cpp

**Issues:**
1. **Grid-based search**: O(W×H×L) cells vs. O(R×D) rooms/doors
   - NexRx board: ~1M grid cells vs. ~1000 rooms

2. **Iteration limit**: 100,000 (recently increased from 10,000)
   - Still too low for complex boards

3. **Linear parent search**: O(N) per expansion
   - Lines 118-125, 169-178, 199-207
   - Should be O(1) with hash map

4. **Poor obstacle checking**: Only checks endpoints
   - Lines 155-158
   - Misses intermediate collisions

5. **No ripup/shove**: Fails when path blocked
   - Java handles this elegantly

---

## Algorithm Comparison

### Java Expansion Room Algorithm
```
Time: O(R log R + D)
  R = rooms (~100-1000)
  D = doors (~500-5000)

Optimizations:
- Priority queue: O(log R) insertions
- Drill pages: Spatial indexing
- Door pruning: Skip occupied
- Ripup/shove: Handle obstacles
- Small doors: Filter tight gaps
```

### C++ SimpleGridRouter
```
Time: O((W/G) × (H/G) × L × log N)
  W, H = board dimensions
  G = grid size (4× trace width)
  L = layers
  N = priority queue size

For NexRx (100mm × 100mm, 1mm grid):
  ≈ 1000 × 1000 × 2 × log(10000)
  ≈ 26 million operations PER CONNECTION

With 1148 items and iteration limit:
  Hits limit on every complex route
  Takes HOURS total
```

---

## Recommended Approach

**Option A: Full Port (4-6 weeks)**
- Port entire Java algorithm methodically
- Best long-term solution
- High confidence in correctness
- Matches Java performance

**Option B: Incremental (2-3 weeks to MVP)**
- Implement Phase 1 first (minimal viable)
- Test and validate
- Add features incrementally
- Risk: May miss subtle interactions

**Option C: Hybrid**
- Keep SimpleGridRouter for simple routes
- Use expansion rooms for complex routes
- Fallback strategy
- Lower risk but more code to maintain

---

## Testing Strategy

### Unit Tests
1. DestinationDistance calculations
2. Room creation and door generation
3. Priority queue ordering
4. Path reconstruction

### Integration Tests
1. Simple 2-pad connections
2. Multi-layer via routing
3. Obstacle avoidance
4. Complex board (NexRx)

### Performance Benchmarks
Compare against Java freerouting:
- Time per connection
- Success rate
- Memory usage
- DRC violations

---

## Current Blockers

1. **Scope**: 1290 lines of complex Java code
2. **Testing**: Need test boards at each complexity level
3. **Dependencies**: Room completion requires AutorouteEngine integration
4. **Time**: Full port is 4-6 week effort

---

## Next Steps

### Immediate (Completed Feb 17, 2026)
1. ✅ Document current status
2. ✅ Decided on approach: Full port (Option A) per user directive
3. ✅ Ported DestinationDistance (346 lines)
4. ✅ Ported MazeSearchAlgo core (~700 lines)
5. ✅ Fixed linker errors - enabled WIP expansion room classes
6. ✅ Build succeeds - algorithm compiles and links
7. ✅ Tested on Issue026 - algorithm runs but finds no routes (expected - room completion stubbed)

### Short-term (Next Session)
1. ⬜ Implement room completion infrastructure
2. ⬜ Port ShapeSearchTree for spatial queries
3. ⬜ Test routing success on simple boards

### Medium-term (Week 1-2)
1. ⬜ Complete Phase 1 (MVP)
2. ⬜ Validate on test suite
3. ⬜ Compare performance to SimpleGridRouter

---

## File Manifest

### Expansion Room Infrastructure (Created but not integrated)
- `include/autoroute/CompleteExpansionRoom.h` (abstract)
- `include/autoroute/FreeSpaceExpansionRoom.h` (abstract)
- `include/autoroute/CompleteFreeSpaceExpansionRoom.h`
- `include/autoroute/IncompleteFreeSpaceExpansionRoom.h`
- `include/autoroute/ObstacleExpansionRoom.h`
- `include/autoroute/ExpansionDoor.h`
- `include/autoroute/TargetItemExpansionDoor.h`
- `include/autoroute/ExpansionDrill.h`
- `include/autoroute/DrillPage.h`
- `include/autoroute/DrillPageArray.h`
- `include/autoroute/MazeListElement.h`
- `include/autoroute/MazeSearchElement.h`

### Java Reference
- `freerouting.app/src/main/java/app/freerouting/autoroute/MazeSearchAlgo.java` (1290 lines)
- `freerouting.app/src/main/java/app/freerouting/autoroute/CompleteExpansionRoom.java`
- `freerouting.app/src/main/java/app/freerouting/autoroute/ExpansionDoor.java`

### Current Fallback
- `src/autoroute/MazeSearchAlgo.cpp` (lines 87-138 use SimpleGridRouter)
- `src/autoroute/SimpleGridRouter.cpp` (naive grid-based A*)

---

## Conclusion

The expansion room algorithm is the RIGHT way to route PCBs. SimpleGridRouter was never intended for production - it's fundamentally the wrong algorithm.

**We must port the Java expansion room algorithm** to achieve acceptable performance on complex boards.

The infrastructure exists. The roadmap is clear. The effort is significant but achievable.

**Estimated effort**: 4-6 weeks for full parity, 2-3 weeks for minimal viable product.
