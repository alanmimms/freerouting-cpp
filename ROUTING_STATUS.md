# FreeRouting C++ - Routing Implementation Status

## Current Status: Phase 2 Complete - Infrastructure Ready

**Last Updated:** 2026-02-18

### What Works ✓

1. **KiCad I/O** - Full PCB file parsing and writing
2. **Board Model** - Items (traces, vias, pads), nets, layers, clearances
3. **Spatial Indexing** - ShapeSearchTree for efficient overlap queries
4. **Maze Search Framework** - Priority queue, expansion lists, backtracking
5. **Basic Routing** - Creates direct point-to-point connections (77.7% success on test boards)
6. **Visualization** - SDL2 real-time rendering of board state
7. **Command Line** - Full option parsing, batch mode, visualization mode

### What's Missing ✗

**CRITICAL: Room Shape Generation**

Currently, expansion rooms have `nullptr` shapes, causing:
- No proper room expansion (maze search falls back to direct connections)
- No obstacle avoidance (routes go through pads/components)
- No multi-layer routing (all routes on layer 0)
- Routes fail when direct connection impossible (23% failure rate)

### Current Routing Behavior

**File:** `tests/NexRx/NexRx.kicad_pcb` (889 connections)
```
Pass 1: 619 routed, 63 failed  (direct connections work)
Pass 2: 333 routed, 66 failed  (retrying failures)
Pass 3: 297 routed, 66 failed  (stuck - no improvement)
...
Final: 680 routed, 209 failed (77.7% success)
```

**All traces:**
- On layer 0 (F.Cu) only
- Direct point-to-point lines
- Go through obstacles
- No bends or routing around components

## Phase 3: Implement Proper Room Shape Generation

### Required Components

#### 1. TileShape Hierarchy (~500 lines)

**File:** `include/geometry/TileShape.h`

```cpp
// Base interface for routing shapes
class TileShape {
public:
  virtual ~TileShape() = default;

  // Core operations
  virtual TileShape* intersection(const TileShape& other) const = 0;
  virtual TileShape* intersectionWithHalfplane(const Line& line) const = 0;
  virtual int dimension() const = 0;  // 0=point, 1=line, 2=area
  virtual bool contains(const IntPoint& point) const = 0;
  virtual IntBox getBoundingBox() const = 0;

  // Geometric queries
  virtual bool intersects(const TileShape& other) const = 0;
  virtual bool intersectsInterior(const Line& line) const = 0;
  virtual double distanceToLeft(const Line& line) const = 0;

  // Border operations for door creation
  virtual int borderLineCount() const = 0;
  virtual Line borderLine(int index) const = 0;
  virtual int* touchingSides(const TileShape& other) const = 0;
};

// Axis-aligned rectangle (90° routing)
class IntBoxShape : public TileShape {
  IntBox box;
  // Implement all TileShape operations using IntBox logic
};

// TODO Phase 3B: IntOctagon (45° routing)
// TODO Phase 3C: Simplex (any-angle routing)
```

**Java Reference:** `freerouting.app/src/main/java/app/freerouting/geometry/planar/TileShape.java`

#### 2. Room Shape Completion (~400 lines)

**File:** `src/autoroute/ExpansionRoomGenerator.cpp`

**Algorithm:** (from Java `ShapeSearchTree.complete_shape()`)

```cpp
TileShape* ExpansionRoomGenerator::completeRoomShape(
    IncompleteFreeSpaceExpansionRoom* incompleteRoom,
    const TileShape* containedShape,
    int layer,
    int netNo,
    ShapeSearchTree* searchTree) {

  // Start with board bounding box
  TileShape* roomShape = new IntBoxShape(board->getBoundingBox());

  // If room has initial shape constraint, intersect
  if (incompleteRoom->getShape()) {
    roomShape = roomShape->intersection(*incompleteRoom->getShape());
  }

  // Find all obstacles that overlap this room
  std::vector<TreeEntry> overlappingObjects;
  searchTree->overlappingEntries(roomShape->getBoundingBox(), layer, overlappingObjects);

  // Restrain room around each obstacle
  for (const TreeEntry& entry : overlappingObjects) {
    if (!entry.object->isTraceObstacle(netNo)) continue;

    TileShape* obstacleShape = entry.object->getTreeShape(entry.shapeIndex);
    TileShape* intersection = roomShape->intersection(*obstacleShape);

    if (intersection->dimension() == 2) {
      // Room overlaps obstacle - must restrain
      roomShape = restrainShape(roomShape, obstacleShape, containedShape);
    }
  }

  return roomShape;
}

TileShape* ExpansionRoomGenerator::restrainShape(
    TileShape* roomShape,
    const TileShape* obstacle,
    const TileShape* containedShape) {

  // Find the obstacle edge that best separates contained_shape from obstacle
  Line bestCutLine;
  double maxDistance = -std::numeric_limits<double>::infinity();

  for (int i = 0; i < obstacle->borderLineCount(); i++) {
    Line edgeLine = obstacle->borderLine(i);

    // Check if this edge intersects room interior
    if (!roomShape->intersectsInterior(edgeLine)) continue;

    // Check if contained_shape is on the correct side
    double distance = containedShape->distanceToLeft(edgeLine);
    if (distance > maxDistance) {
      maxDistance = distance;
      bestCutLine = edgeLine.opposite();  // Use opposite half-plane
    }
  }

  // Cut room with the best edge line
  return roomShape->intersectionWithHalfplane(bestCutLine);
}
```

**Java Reference:** `freerouting.app/src/main/java/app/freerouting/board/ShapeSearchTree.java` lines 549-689

#### 3. Neighbor Sorting and Gap Detection (~300 lines)

**File:** `src/autoroute/SortedRoomNeighbours.cpp`

**Algorithm:** (from Java `SortedRoomNeighbours.calculate()`)

After finding overlapping rooms via search tree:

```cpp
// Sort neighbors counter-clockwise around room border
std::vector<RoomNeighbor> sortedNeighbors;
for (TreeEntry& entry : overlappingEntries) {
  TileShape* neighborShape = entry.object->getTreeShape(...);
  int* touchingSides = room->getShape()->touchingSides(*neighborShape);

  if (touchingSides) {
    // 1D intersection = neighbor
    sortedNeighbors.push_back({
      .neighbor = entry.object,
      .shape = neighborShape,
      .touchingSide = touchingSides[0]
    });
  }
}

// Sort by touching side number (counter-clockwise)
std::sort(sortedNeighbors.begin(), sortedNeighbors.end(),
  [](const RoomNeighbor& a, const RoomNeighbor& b) {
    return a.touchingSide < b.touchingSide;
  });

// Find gaps between neighbors and create new incomplete rooms
for (size_t i = 0; i < sortedNeighbors.size(); i++) {
  auto& curr = sortedNeighbors[i];
  auto& next = sortedNeighbors[(i + 1) % sortedNeighbors.size()];

  if (!neighborsAdjacent(curr, next)) {
    // Gap found - create new incomplete room in the gap
    TileShape* gapShape = calculateGapShape(room, curr, next);
    auto* newRoom = autorouteEngine->addIncompleteExpansionRoom(
      gapShape, layer, room->getShape());
  }
}
```

**Java Reference:** `freerouting.app/src/main/java/app/freerouting/autoroute/SortedRoomNeighbours.java` lines 369-520

#### 4. Integration Points

**File:** `src/autoroute/MazeSearchAlgo.cpp`

**Changes needed:**

Line 176 - Replace `nullptr` with actual shape:
```cpp
// OLD:
const Shape* roomShape = nullptr;  // TODO: Create shape from itemBox

// NEW:
TileShape* containedShape = new IntBoxShape(itemBox);
TileShape* roomShape = roomGenerator->createInitialRoomShape(
  layer, itemBox, containedShape);
```

Line 189-195 - Actually complete the room shape:
```cpp
// Ensure completeExpansionRoom() calls ExpansionRoomGenerator
CompleteFreeSpaceExpansionRoom* completed =
  this->autorouteEngine->completeExpansionRoom(room);

// This should internally call:
// TileShape* finalShape = roomGenerator->completeRoomShape(
//   room, room->getContainedShape(), layer, netNo, searchTree);
```

### Implementation Phases

#### Phase 3A: TileShape Foundation (First Session)
1. Create `TileShape` base interface
2. Implement `IntBoxShape` with basic operations
3. Implement `Line` class for half-plane intersections
4. Test intersection and restraining operations
5. **Deliverable:** Working IntBox geometric operations

#### Phase 3B: Room Shape Completion (Second Session)
1. Implement `ExpansionRoomGenerator::completeRoomShape()`
2. Implement `restrainShape()` obstacle avoidance
3. Update `AutorouteEngine::completeExpansionRoom()` to call generator
4. **Deliverable:** Rooms have proper shapes that avoid obstacles

#### Phase 3C: Initial Room Creation (Third Session)
1. Implement `createInitialRoomShape()` for start items
2. Update `MazeSearchAlgo::init()` to create shaped rooms
3. Test room expansion through doors
4. **Deliverable:** Maze search expands through proper room shapes

#### Phase 3D: Neighbor Sorting (Fourth Session)
1. Implement `touchingSides()` edge detection
2. Implement counter-clockwise neighbor sorting
3. Implement gap detection and new room creation
4. **Deliverable:** Full room network with all gaps filled

#### Phase 3E: Multi-Layer Routing (Fifth Session)
1. Implement via placement at room boundaries
2. Implement layer-change doors
3. Update cost calculation for layer changes
4. **Deliverable:** Routes can use multiple layers with vias

### Expected Improvements

After Phase 3 complete:
- **Success Rate:** 77.7% → 95%+ (proper obstacle avoidance)
- **Multi-Layer:** 0% → 80%+ (vias for layer changes)
- **Route Quality:** Direct lines → Proper orthogonal routing with bends
- **DRC Violations:** High → Low (routes avoid obstacles properly)

## Testing Strategy

### Test Cases
1. **Simple 2-point route** - Single layer, no obstacles
2. **Route around obstacle** - Verify room restraining works
3. **Multi-layer route** - Start and end on different layers
4. **Complex board** - NexRx.kicad_pcb (889 connections)

### Success Criteria
- All test cases route successfully
- No routes go through obstacles
- Proper use of vias for layer changes
- 95%+ success rate on complex boards

## Files Modified

### Current Session (Phase 2)
- `include/autoroute/ShapeSearchTree.h` (NEW)
- `src/autoroute/ShapeSearchTree.cpp` (NEW)
- `include/autoroute/ExpansionRoom.h` (MODIFIED - SearchTreeObject interface)
- `src/autoroute/AutorouteEngine.cpp` (MODIFIED - search tree integration)
- `src/autoroute/SortedRoomNeighbours.cpp` (MODIFIED - door creation)
- `include/cli/CommandLineArgs.h` (MODIFIED - --remove-existing option)
- `src/cli/CommandLineArgs.cpp` (MODIFIED)
- `src/main.cpp` (MODIFIED - route removal)

### Next Session (Phase 3A)
- `include/geometry/TileShape.h` (NEW)
- `include/geometry/IntBoxShape.h` (NEW)
- `include/geometry/Line.h` (NEW)
- `src/geometry/IntBoxShape.cpp` (NEW)
- `src/geometry/Line.cpp` (NEW)

## Build and Test

```bash
# Build
cmake --build build -j8

# Test basic routing
./build/freerouting-cli tests/NexRx/NexRx.kicad_pcb /tmp/output.kicad_pcb

# Test with visualization
./build/freerouting-cli --visualize tests/NexRx/NexRx.kicad_pcb /tmp/output.kicad_pcb

# Test with route removal
./build/freerouting-cli --remove-existing routed_board.kicad_pcb /tmp/rerouted.kicad_pcb
```

## References

### Java Source (Original Implementation)
- `freerouting.app/src/main/java/app/freerouting/autoroute/MazeSearchAlgo.java`
- `freerouting.app/src/main/java/app/freerouting/autoroute/SortedRoomNeighbours.java`
- `freerouting.app/src/main/java/app/freerouting/board/ShapeSearchTree.java`
- `freerouting.app/src/main/java/app/freerouting/geometry/planar/TileShape.java`
- `freerouting.app/src/main/java/app/freerouting/geometry/planar/IntBox.java`

### Key Algorithms
1. **Room Completion:** ShapeSearchTree.complete_shape() lines 549-631
2. **Shape Restraining:** ShapeSearchTree.restrain_shape() lines 640-689
3. **Neighbor Sorting:** SortedRoomNeighbours.calculate() lines 52-520
4. **Maze Expansion:** MazeSearchAlgo.expand_to_room_doors() lines 298-469

## Known Limitations

### Current (Phase 2)
- ✗ No room shapes (all nullptr)
- ✗ Direct point-to-point routing only
- ✗ Routes go through obstacles
- ✗ No multi-layer routing
- ✗ 23% failure rate on complex boards

### After Phase 3A
- ✓ Basic room shapes (IntBox)
- ✗ Only 90° routing (no 45° or any-angle)
- ✗ May have some edge cases in geometric operations

### After Full Phase 3
- ✓ Proper obstacle avoidance
- ✓ Multi-layer routing with vias
- ✓ High success rate (95%+)
- ✗ Only 90° routing (IntOctagon/Simplex needed for 45°/any-angle)
