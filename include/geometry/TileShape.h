#pragma once

#include "geometry/Shape.h"
#include "geometry/Vector2.h"
#include "geometry/IntBox.h"

namespace freerouting {

// Forward declarations
class Line;

/**
 * Base interface for geometric shapes used in routing.
 *
 * TileShape represents convex polygons that form the basis of the maze search
 * algorithm. Rooms are represented as TileShapes, and the router expands from
 * room to room by finding 1D intersections (doors) between adjacent shapes.
 *
 * The hierarchy supports different routing geometries:
 * - IntBoxShape: Axis-aligned rectangles (90° routing)
 * - IntOctagon: Octagons (45° routing) - TODO Phase 3B
 * - Simplex: Arbitrary convex polygons (any-angle routing) - TODO Phase 3C
 *
 * Key operations:
 * - intersection(): Find overlap between two shapes
 * - intersectionWithHalfplane(): Cut shape with a line
 * - contains(), intersects(): Geometric queries
 * - borderLine(), touchingSides(): Edge detection for door creation
 */
class TileShape : public Shape {
public:
  virtual ~TileShape() = default;

  // ========== Core Geometric Operations ==========

  /**
   * Returns the intersection of this shape with another shape.
   * Result may be a 2D area, 1D line, 0D point, or empty.
   * Returns nullptr if shapes don't intersect.
   */
  virtual TileShape* intersection(const TileShape& other) const = 0;

  /**
   * Returns the intersection of this shape with a half-plane.
   * The half-plane is defined by a directed line - points to the left
   * of the line (in its direction) are included.
   * Returns nullptr if result is empty.
   */
  virtual TileShape* intersectionWithHalfplane(const Line& line) const = 0;

  /**
   * Returns the dimension of this shape.
   * 0 = point, 1 = line segment, 2 = area
   * Returns -1 for empty shapes.
   */
  virtual int dimension() const = 0;

  /**
   * Returns true if this shape is empty (no area, no line, no point).
   */
  virtual bool isEmpty() const = 0;

  // ========== Containment and Intersection Queries ==========

  /**
   * Returns true if this shape contains the given point.
   * For 2D shapes: point must be in interior or on boundary.
   * For 1D shapes: point must be on the line segment.
   * For 0D shapes: point must equal the single point.
   * Note: This overrides Shape::contains() - signature must match exactly.
   */
  // Inherited from Shape: bool contains(IntPoint point) const override;

  /**
   * Returns true if this shape's interior overlaps with the other shape.
   * 1D intersections (shared edges) return false - only 2D overlap counts.
   */
  virtual bool intersects(const TileShape& other) const = 0;

  /**
   * Returns true if the line intersects the interior of this shape.
   * Lines that touch only the boundary return false.
   */
  virtual bool intersectsInterior(const Line& line) const = 0;

  /**
   * Returns the bounding box of this shape.
   * For empty shapes, returns an empty IntBox.
   */
  virtual IntBox getBoundingBox() const = 0;

  // ========== Distance Queries ==========

  /**
   * Returns the signed distance from this shape to the left side of the line.
   * Positive = shape is to the left of the line.
   * Negative = shape is to the right of the line.
   * Zero = line passes through the shape.
   *
   * Used by room restraining to choose the best edge to cut with.
   */
  virtual double distanceToLeft(const Line& line) const = 0;

  // ========== Border Operations for Door Creation ==========

  /**
   * Returns the number of border edges this shape has.
   * IntBoxShape: 4 edges (top, right, bottom, left)
   * IntOctagon: 8 edges
   * Simplex: variable
   */
  virtual int borderLineCount() const = 0;

  /**
   * Returns the i-th border edge as a directed Line.
   * Lines are ordered counter-clockwise around the shape.
   * Index must be in range [0, borderLineCount()).
   */
  virtual Line borderLine(int index) const = 0;

  /**
   * Returns an array of indices of border edges that touch the other shape.
   * Returns nullptr if shapes don't touch.
   *
   * Used to detect 1D intersections for door creation:
   * - If intersection is 1D (shared edge), returns the touching edge indices
   * - If intersection is 2D (overlap), returns nullptr
   * - If shapes don't touch, returns nullptr
   *
   * Caller must free the returned array.
   */
  virtual int* touchingSides(const TileShape& other) const = 0;

  // ========== Comparison ==========

  /**
   * Returns true if this shape is geometrically equivalent to the other.
   */
  virtual bool equals(const TileShape& other) const = 0;

  // ========== Type Identification ==========

  /**
   * Returns a string identifying the shape type for debugging.
   */
  virtual const char* getTypeName() const = 0;
};

} // namespace freerouting
