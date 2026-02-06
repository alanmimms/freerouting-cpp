# Canyon Routing with Cliff Vias - Design Document

**Feature Status:** Future Enhancement (Phase 5+)
**Prerequisites:** Controlled impedance routing, zone-aware routing, soft constraints
**Estimated Complexity:** High (50-100 hours implementation)
**Priority:** Low (after core routing stabilizes at 95%+ success)

---

## Overview

Enable automatic "canyon routing" technique for controlled impedance domains, where:
- A keepout zone defines the impedance-controlled region (the "canyon")
- Only specific layers are allowed inside the zone (e.g., top and bottom on 6-layer board)
- Automatic via "fencing" creates a barrier around the zone edges (the "cliffs")
- Violations are tracked but allowed with configurable tolerance ("squishy" rules)

**Use Case:** 200Ω differential impedance transmission lines that require isolation from other nets to maintain signal integrity.

---

## User Requirements

### Primary Use Case: 200Ω Impedance Domain on 6-Layer Board

**Board Structure:**
```
Layer 1: Top (F.Cu)      - 200Ω traces ALLOWED
Layer 2: Inner 1         - KEEPOUT
Layer 3: Inner 2         - KEEPOUT
Layer 4: Inner 3         - KEEPOUT
Layer 5: Inner 4         - KEEPOUT
Layer 6: Bottom (B.Cu)   - 200Ω traces ALLOWED
```

**Zone Requirements:**
- Define polygonal keepout region for 200Ω domain
- All nets except 200Ω nets must avoid the canyon (layers 2-5)
- 200Ω nets can use layers 1 and 6 freely
- Automatic via placement around zone boundary ("cliff vias")
- Soft constraints with configurable violation tolerance

**Cliff Via Requirements:**
- Automatically generated around zone boundary
- Connected to reference plane (ground/power)
- Act as electromagnetic barrier
- Typical spacing: 0.5-2.0mm (λ/10 at max frequency)
- Stitching pattern around perimeter

---

## Technical Design

### 1. KiCad 9 Rule Definition

#### Option A: Extended Rule Area (Recommended)

```lisp
(zone
  (net 0)  ; unconnected or reference net
  (name "200ohm_canyon")
  (layer "F.Cu" "B.Cu")
  (type impedance_canyon)

  ;; Keepout configuration
  (keepout
    (mode selective)  ; not global keepout
    (allowed_nets "200ohm_diff_p" "200ohm_diff_n")
    (allowed_layers "F.Cu" "B.Cu")
    (blocked_layers "In1.Cu" "In2.Cu" "In3.Cu" "In4.Cu")
  )

  ;; Impedance specification
  (impedance
    (type differential)
    (value 200)
    (tolerance 10)  ; ±10%
  )

  ;; Cliff via configuration
  (cliff_vias
    (enabled yes)
    (auto_generate yes)
    (spacing 1.0)           ; mm between vias
    (offset_from_edge 0.5)  ; mm inside zone boundary
    (via_type through)
    (via_drill 0.3)
    (via_diameter 0.6)
    (connect_to_plane "GND")
    (stitch_all_layers yes)
  )

  ;; Soft constraint configuration
  (violations
    (mode soft)
    (max_allowed 3)          ; Allow up to 3 violations
    (severity warning)       ; Report as warnings, not errors
    (cost_multiplier 100.0)  ; Heavy penalty but not forbidden
  )

  ;; Zone boundary polygon
  (polygon
    (pts
      (xy 100.0 50.0)
      (xy 150.0 50.0)
      (xy 150.0 100.0)
      (xy 100.0 100.0)
    )
  )
)
```

#### Option B: Net Class Extension

```lisp
(net_class "200ohm_controlled"
  (clearance 0.5)
  (track_width 0.2)
  (diff_pair_gap 0.15)
  (diff_pair_width 0.2)

  ;; Impedance control
  (impedance
    (single_ended 100)
    (differential 200)
  )

  ;; Canyon protection
  (canyon_zone "200ohm_canyon")
  (canyon_protection
    (auto_cliff_vias yes)
    (violation_mode soft)
  )
)

;; Separate zone definition references this net class
(zone
  (name "200ohm_canyon")
  (protected_net_class "200ohm_controlled")
  (cliff_vias ...)
  (polygon ...)
)
```

#### Option C: Constraint System

```lisp
(design_rules
  (rule "canyon_protection_200ohm"
    ;; Condition: applies to non-200ohm nets
    (condition "A.NetClass != '200ohm' && A.insideZone('200ohm_canyon')")

    ;; Constraint definition
    (constraint disallow)
    (layer_constraint "In1.Cu" "In2.Cu" "In3.Cu" "In4.Cu")

    ;; Soft constraint behavior
    (severity warning)
    (max_violations 3)
    (cost_penalty 100.0)
  )

  (rule "cliff_via_auto_generation"
    (condition "zone.type == 'impedance_canyon'")
    (action generate_cliff_vias)
    (parameters
      (spacing 1.0)
      (via_spec "standard_through_0.6_0.3")
      (connect_to "GND")
    )
  )
)
```

**Recommendation:** Option A (Extended Rule Area) provides the cleanest integration with existing KiCad zone concepts.

---

### 2. Data Structures

```cpp
// Impedance-controlled zone definition
class ImpedanceCanyon {
public:
  ImpedanceCanyon(const std::string& name, int impedance);

  // Zone properties
  std::string getName() const { return name_; }
  int getImpedance() const { return impedance_; }
  double getImpedanceTolerance() const { return impedanceTolerance_; }

  // Layer restrictions
  void setAllowedLayers(const std::vector<int>& layers);
  void setBlockedLayers(const std::vector<int>& layers);
  bool isLayerAllowed(int layer) const;

  // Net restrictions
  void addAllowedNet(int netNumber);
  bool isNetAllowed(int netNumber) const;

  // Geometry
  void setBoundary(const std::vector<IntPoint>& polygon);
  bool containsPoint(const IntPoint& p) const;
  double distanceToEdge(const IntPoint& p) const;

  // Cliff via configuration
  struct CliffViaConfig {
    bool autoGenerate;
    double spacing;           // mm
    double offsetFromEdge;    // mm
    int viaDrill;            // internal units
    int viaDiameter;         // internal units
    int connectToNet;        // reference plane net
    bool stitchAllLayers;
  };

  void setCliffViaConfig(const CliffViaConfig& config);
  const CliffViaConfig& getCliffViaConfig() const;

  // Soft constraint configuration
  struct ViolationConfig {
    bool softMode;           // true = warnings, false = errors
    int maxAllowed;          // max violations before hard failure
    double costMultiplier;   // routing cost penalty
  };

  void setViolationConfig(const ViolationConfig& config);
  const ViolationConfig& getViolationConfig() const;

private:
  std::string name_;
  int impedance_;
  double impedanceTolerance_;

  std::vector<int> allowedLayers_;
  std::vector<int> blockedLayers_;
  std::set<int> allowedNets_;

  std::vector<IntPoint> boundaryPolygon_;

  CliffViaConfig cliffViaConfig_;
  ViolationConfig violationConfig_;
};

// Soft constraint violation tracking
class SoftConstraintViolation {
public:
  enum class Type {
    CanyonLayerViolation,
    CanyonNetViolation,
    ImpedanceViolation,
    ClearanceViolation
  };

  SoftConstraintViolation(
    Type type,
    const IntPoint& location,
    int severity,
    const std::string& description
  );

  Type getType() const { return type_; }
  IntPoint getLocation() const { return location_; }
  int getSeverity() const { return severity_; }
  std::string getDescription() const { return description_; }

private:
  Type type_;
  IntPoint location_;
  int severity_;        // 1-10 (10 = most severe)
  std::string description_;
};

// Constraint checker with soft violation support
class CanyonConstraintChecker {
public:
  CanyonConstraintChecker(RoutingBoard* board);

  // Register canyon zones
  void addCanyonZone(std::unique_ptr<ImpedanceCanyon> canyon);

  // Check if routing action violates constraints
  bool checkTraceConstraints(
    const Trace* trace,
    std::vector<SoftConstraintViolation>& violations
  );

  bool checkViaConstraints(
    const Via* via,
    std::vector<SoftConstraintViolation>& violations
  );

  // Get all violations for reporting
  const std::vector<SoftConstraintViolation>& getViolations() const {
    return violations_;
  }

  // Check if we've exceeded max allowed violations for any zone
  bool hasExceededLimits() const;

private:
  RoutingBoard* board_;
  std::vector<std::unique_ptr<ImpedanceCanyon>> canyonZones_;
  std::vector<SoftConstraintViolation> violations_;

  // Per-zone violation counts
  std::map<std::string, int> violationCounts_;
};
```

---

### 3. Cliff Via Generation Algorithm

```cpp
class CliffViaGenerator {
public:
  CliffViaGenerator(RoutingBoard* board);

  // Generate cliff vias for a canyon zone
  std::vector<Via*> generateCliffVias(const ImpedanceCanyon& canyon);

private:
  RoutingBoard* board_;

  // Compute via placement points along boundary
  std::vector<IntPoint> computeViaLocations(
    const std::vector<IntPoint>& boundary,
    double spacing,
    double offset
  );

  // Create a stitching via at specified location
  Via* createStitchingVia(
    const IntPoint& location,
    const ImpedanceCanyon::CliffViaConfig& config
  );

  // Ensure vias connect to reference plane on all layers
  void connectToReferencePlane(Via* via, int refNet);
};

// Implementation outline:
std::vector<Via*> CliffViaGenerator::generateCliffVias(
    const ImpedanceCanyon& canyon) {

  std::vector<Via*> cliffVias;
  const auto& config = canyon.getCliffViaConfig();

  if (!config.autoGenerate) {
    return cliffVias;
  }

  // 1. Get boundary polygon points
  const auto& boundary = canyon.getBoundary();

  // 2. Compute via placement locations along perimeter
  //    - Offset inward by config.offsetFromEdge
  //    - Space vias at config.spacing intervals
  std::vector<IntPoint> viaLocations = computeViaLocations(
    boundary,
    config.spacing,
    config.offsetFromEdge
  );

  // 3. Create stitching vias at each location
  for (const auto& loc : viaLocations) {
    Via* via = createStitchingVia(loc, config);

    if (via) {
      // 4. Connect to reference plane
      connectToReferencePlane(via, config.connectToNet);

      // 5. Add to board
      board_->addItem(std::unique_ptr<Via>(via));
      cliffVias.push_back(via);
    }
  }

  return cliffVias;
}

std::vector<IntPoint> CliffViaGenerator::computeViaLocations(
    const std::vector<IntPoint>& boundary,
    double spacing,
    double offset) {

  std::vector<IntPoint> locations;

  // Convert spacing from mm to internal units
  int spacingUnits = static_cast<int>(spacing * 10000.0);
  int offsetUnits = static_cast<int>(offset * 10000.0);

  // For each edge of the polygon:
  for (size_t i = 0; i < boundary.size(); ++i) {
    IntPoint p1 = boundary[i];
    IntPoint p2 = boundary[(i + 1) % boundary.size()];

    // Calculate edge vector and length
    IntVector edge(p2.x - p1.x, p2.y - p1.y);
    double edgeLength = std::sqrt(edge.x * edge.x + edge.y * edge.y);

    // Calculate perpendicular inward normal (offset direction)
    IntVector normal(-edge.y, edge.x);
    double normalLen = std::sqrt(normal.x * normal.x + normal.y * normal.y);
    normal.x = static_cast<int>((normal.x / normalLen) * offsetUnits);
    normal.y = static_cast<int>((normal.y / normalLen) * offsetUnits);

    // Place vias along this edge
    int numVias = static_cast<int>(edgeLength / spacingUnits);
    for (int j = 0; j <= numVias; ++j) {
      double t = static_cast<double>(j) / numVias;
      IntPoint edgePoint(
        p1.x + static_cast<int>(edge.x * t),
        p1.y + static_cast<int>(edge.y * t)
      );

      // Offset inward by normal
      IntPoint viaPoint(
        edgePoint.x + normal.x,
        edgePoint.y + normal.y
      );

      locations.push_back(viaPoint);
    }
  }

  return locations;
}
```

---

### 4. Routing Integration

```cpp
// In AutorouteEngine pathfinding:

bool AutorouteEngine::expandNode(
    SearchNode* node,
    const std::vector<ImpedanceCanyon*>& canyons,
    CanyonConstraintChecker* constraintChecker) {

  // ... existing pathfinding logic ...

  // Check canyon constraints for this expansion
  for (const auto* canyon : canyons) {
    if (canyon->containsPoint(node->position)) {

      // Check if this net is allowed in this canyon
      if (!canyon->isNetAllowed(currentNet_)) {

        // Check if this layer is blocked
        if (!canyon->isLayerAllowed(node->layer)) {

          const auto& violConfig = canyon->getViolationConfig();

          if (violConfig.softMode) {
            // Soft constraint: heavily penalize but allow
            node->cost *= violConfig.costMultiplier;

            // Record violation
            std::vector<SoftConstraintViolation> violations;
            constraintChecker->recordViolation(
              SoftConstraintViolation::Type::CanyonLayerViolation,
              node->position,
              8,  // high severity
              "Non-200ohm net on blocked layer in canyon"
            );

            // Check if we've exceeded max violations
            if (constraintChecker->hasExceededLimits()) {
              return false;  // Hard block
            }

            // Otherwise continue with heavy penalty
          } else {
            // Hard constraint: block this expansion
            return false;
          }
        }
      }
    }
  }

  // ... continue pathfinding ...
}
```

---

### 5. DSN File Format Extension

For boards designed with canyon routing, the DSN file needs to convey this information:

```lisp
(structure
  (layer F.Cu (type signal))
  (layer In1.Cu (type signal))
  (layer In2.Cu (type signal))
  (layer In3.Cu (type signal))
  (layer In4.Cu (type signal))
  (layer B.Cu (type signal))

  ;; New: impedance canyon definition
  (impedance_canyon
    (name "200ohm_diff")
    (impedance 200 differential)
    (tolerance 10)
    (allowed_layers F.Cu B.Cu)
    (blocked_layers In1.Cu In2.Cu In3.Cu In4.Cu)
    (allowed_nets "diff_p" "diff_n")

    (cliff_vias
      (spacing 1.0)
      (offset 0.5)
      (via (padstack via_0.6_0.3))
      (connect_to GND)
    )

    (violations soft max_allowed 3)

    (boundary
      (path signal 0
        100.0 50.0
        150.0 50.0
        150.0 100.0
        100.0 100.0
        100.0 50.0
      )
    )
  )
)
```

**Parser Implementation:**
```cpp
// In DsnReader.cpp:
bool DsnReader::parseImpedanceCanyon(
    const SExprNode& expr,
    ImpedanceCanyon& canyon) {

  // Parse name
  canyon.name = parseString(*expr.getChild(1));

  // Parse impedance specification
  // Parse layer constraints
  // Parse cliff via configuration
  // Parse boundary polygon
  // Parse violation mode

  return true;
}
```

---

### 6. User Interface / Reporting

**Pre-Routing Report:**
```
Canyon Zone: 200ohm_canyon
  Impedance: 200Ω differential (±10%)
  Allowed layers: F.Cu, B.Cu
  Blocked layers: In1.Cu, In2.Cu, In3.Cu, In4.Cu
  Allowed nets: diff_p, diff_n

  Cliff vias: 42 vias generated
  Via spacing: 1.0mm
  Via spec: 0.6mm/0.3mm drill
  Reference plane: GND

  Constraint mode: Soft (max 3 violations)
```

**Post-Routing Report:**
```
Canyon Constraint Violations: 2 / 3 allowed

  Warning: Net clk_100mhz violated canyon on In1.Cu
    Location: (125.4, 73.2)
    Severity: 8/10
    Cost penalty: 100x

  Warning: Net usb_dp crossed canyon edge at shallow angle
    Location: (148.1, 51.3)
    Severity: 5/10
    Cost penalty: 100x

  Status: ✓ Within acceptable limits (2 ≤ 3)
```

---

## Implementation Phases

### Phase 1: Data Structures (8 hours)
- Implement `ImpedanceCanyon` class
- Implement `SoftConstraintViolation` class
- Implement `CanyonConstraintChecker` class
- Add to `RoutingBoard` management

### Phase 2: DSN Parser Extension (8 hours)
- Parse canyon zone definitions from DSN
- Parse cliff via specifications
- Parse violation modes
- Unit tests for parser

### Phase 3: Cliff Via Generation (12 hours)
- Implement `CliffViaGenerator` class
- Boundary polygon via placement algorithm
- Via creation and plane connection
- Integration with board setup
- Unit tests

### Phase 4: Constraint Checking (16 hours)
- Implement soft constraint checking
- Integrate with pathfinding cost function
- Violation tracking and counting
- Max violation enforcement
- Unit tests

### Phase 5: Routing Integration (12 hours)
- Integrate constraint checker into `AutorouteEngine`
- Add canyon-aware cost penalties
- Layer restriction enforcement
- Net-specific allowances
- Integration tests

### Phase 6: KiCad Output (8 hours)
- Write cliff vias to output PCB
- Preserve canyon zones in output
- Write violation reports
- Generate DRC markers for violations

### Phase 7: Testing & Validation (16 hours)
- Test with real 200Ω differential pairs
- Validate via fence effectiveness
- Measure signal integrity impact
- Performance benchmarking
- Documentation

**Total Estimated: 80 hours**

---

## Prerequisites (Must be completed first)

1. **Controlled Impedance Routing**
   - Parse impedance from net classes
   - Calculate trace widths for impedance
   - Differential pair routing
   - Status: Not implemented

2. **Zone-Aware Routing**
   - Parse keepout zones
   - Enforce zone boundaries
   - Report zone violations
   - Status: Basic implementation exists, needs enhancement

3. **Soft Constraints Framework**
   - Violation severity levels
   - Cost-based penalties vs hard blocks
   - Configurable violation thresholds
   - Status: Not implemented

4. **Core Routing Stability**
   - 95%+ routing success rate
   - Robust parser
   - Reliable pathfinding
   - Status: Currently 88.6% (needs improvement)

---

## Test Cases

### Test Case 1: Simple Canyon with Cliff Vias
**Setup:**
- 4-layer board
- Rectangular 200Ω zone (50mm × 50mm)
- Top and bottom layers allowed
- Inner layers blocked
- Cliff vias every 1mm

**Expected:**
- 200 cliff vias generated around perimeter
- 200Ω nets route on top/bottom only
- Other nets avoid inner layers in zone
- 0 violations

### Test Case 2: Soft Constraint Violation
**Setup:**
- Same as Test Case 1
- Add a high-speed clock net that must cross canyon
- Allow 2 violations

**Expected:**
- Clock net crosses canyon on inner layer (violation)
- Violation recorded and reported
- Routing completes successfully
- 1 violation count (< 2 allowed)

### Test Case 3: Complex Polygon Canyon
**Setup:**
- 6-layer board
- L-shaped canyon zone
- Multiple differential pairs
- Tight spacing (0.5mm cliff via spacing)

**Expected:**
- Vias placed correctly on angled boundaries
- Differential pairs maintain spacing
- No impedance violations
- All constraint checks pass

### Test Case 4: Multiple Canyons
**Setup:**
- 2 separate canyon zones (100Ω and 200Ω)
- Each with different layer restrictions
- Overlapping keepout regions

**Expected:**
- Independent cliff via fences
- Correct layer restrictions per zone
- Nets route to appropriate zones
- No crosstalk between zones

---

## Performance Considerations

**Cliff Via Generation:**
- O(n) where n = perimeter length / via spacing
- Typical: 200-500 vias for large zone
- Generation time: < 100ms

**Constraint Checking:**
- O(1) point-in-polygon test per routing expansion
- Use spatial indexing for multiple zones
- Minimal impact on routing speed (< 5% overhead)

**Memory:**
- ~1KB per canyon zone
- ~100 bytes per cliff via
- ~50 bytes per violation record
- Negligible for typical designs

---

## Future Enhancements

1. **Adaptive Via Spacing**
   - Adjust spacing based on frequency
   - Denser spacing at high-frequency areas
   - λ/10 calculation from stackup

2. **Via Pattern Optimization**
   - Staggered vs aligned patterns
   - Corner reinforcement
   - Gap minimization

3. **Electromagnetic Simulation Integration**
   - Pre-route EM field analysis
   - Validate via fence effectiveness
   - Suggest improvements

4. **Interactive Violation Resolution**
   - GUI highlight of violations
   - Suggest alternate routing
   - Manual override capabilities

5. **Multi-Zone Coordination**
   - Automatic buffer zones between canyons
   - Cross-zone routing optimization
   - Hierarchical impedance domains

---

## References

1. Howard Johnson, "High-Speed Digital Design" - Chapter on ground plane discontinuities
2. IPC-2221B - Generic Standard on Printed Board Design
3. Eric Bogatin, "Signal and Power Integrity - Simplified" - Via fencing techniques
4. Rick Hartley, "Ground Plane Stitching" white papers
5. KiCad 9 Design Rules documentation (when available)

---

## Notes from User Requirements

**Original requirement summary:**
- 6-layer board with 200Ω differential impedance domain
- "Canyon" routing technique with "cliff" vias
- Top and bottom layers allowed for 200Ω traces
- Inner 4 layers are keepout for 200Ω domain
- Cliff vias placed around edge to maintain quiet zone
- Soft constraints ("squishy" rules) with violation reporting
- Not 100% hard rules, more like guidelines

**Key insight:** This is fundamentally about **electromagnetic isolation** through physical barriers (cliff vias) rather than just geometric keepout. The vias create a Faraday cage effect around the sensitive impedance-controlled traces.

---

## Status

**Current Status:** Design document complete
**Next Step:** Wait for core routing to stabilize (95%+ success)
**Blocked By:**
- Impedance routing not implemented
- Soft constraint framework not implemented
- Core routing success at 88.6% (needs 95%+)

**Estimated Time to Ready:** 3-6 months at current pace
