#pragma once

#include "geometry/TileShape.h"
#include "geometry/IntBox.h"
#include "geometry/Line.h"

namespace freerouting {

/**
 * Represents an axis-aligned rectangular shape for 90-degree routing.
 *
 * IntBoxShape is the simplest TileShape implementation, representing
 * rectangles with edges parallel to the X and Y axes. This supports
 * orthogonal (90°) routing.
 *
 * The shape is defined by an IntBox (minX, minY, maxX, maxY).
 * Empty boxes and degenerate boxes (1D lines, 0D points) are supported.
 *
 * Border lines are ordered counter-clockwise starting from the bottom edge:
 *   0: Bottom (y = minY, pointing right)
 *   1: Right  (x = maxX, pointing up)
 *   2: Top    (y = maxY, pointing left)
 *   3: Left   (x = minX, pointing down)
 */
class IntBoxShape : public TileShape {
public:
  // ========== Constructors ==========

  /**
   * Creates a box shape from an IntBox.
   */
  explicit IntBoxShape(const IntBox& box);

  /**
   * Creates a box shape from coordinates.
   */
  IntBoxShape(int minX, int minY, int maxX, int maxY);

  /**
   * Copy constructor.
   */
  IntBoxShape(const IntBoxShape& other);

  // ========== Shape Interface (inherited via TileShape) ==========

  IntBox getBoundingBox() const override;
  double area() const override;
  double circumference() const override;
  bool contains(IntPoint point) const override;
  bool containsInside(IntPoint point) const override;
  bool intersects(const Shape& other) const override;
  bool intersects(const IntBox& box) const override;
  bool isEmpty() const override;
  int dimension() const override;
  double distance(IntPoint point) const override;
  IntPoint nearestPoint(IntPoint fromPoint) const override;

  // ========== TileShape Interface ==========

  TileShape* intersection(const TileShape& other) const override;
  TileShape* intersectionWithHalfplane(const Line& line) const override;

  bool intersects(const TileShape& other) const;
  bool intersectsInterior(const Line& line) const override;

  double distanceToLeft(const Line& line) const override;

  int borderLineCount() const override;
  Line borderLine(int index) const override;
  int* touchingSides(const TileShape& other) const override;

  bool equals(const TileShape& other) const override;
  const char* getTypeName() const override;

  // ========== IntBoxShape-Specific Methods ==========

  /**
   * Returns the underlying IntBox.
   */
  const IntBox& getBox() const { return box; }

  /**
   * Returns the width of the box.
   */
  int width() const { return box.width(); }

  /**
   * Returns the height of the box.
   */
  int height() const { return box.height(); }

  /**
   * Returns the center point of the box.
   */
  FloatPoint center() const { return box.center(); }

  /**
   * Returns the corner points in counter-clockwise order:
   * 0: bottom-left, 1: bottom-right, 2: top-right, 3: top-left
   */
  IntPoint corner(int index) const;

  /**
   * Specialized intersection for two IntBoxShapes (more efficient).
   */
  IntBoxShape* intersectionWithBox(const IntBoxShape& other) const;

  /**
   * Returns true if this box's interior overlaps the other box's interior.
   * Boxes that only touch edges don't count as intersecting.
   */
  bool intersectsBox(const IntBoxShape& other) const;

  /**
   * Clips this box against a horizontal or vertical line.
   * Returns a new IntBoxShape representing the half-plane intersection.
   * Returns nullptr if result is empty.
   */
  IntBoxShape* clipWithAxisAlignedLine(const Line& line) const;

private:
  IntBox box;

  // ========== Helper Methods ==========

  /**
   * Determines which corner is farthest to the left of the line.
   * Returns the signed distance to that corner.
   */
  double maxCornerDistanceToLeft(const Line& line) const;

  /**
   * Checks if a line passes through the interior of this box.
   */
  bool linePassesThroughInterior(const Line& line) const;

  /**
   * Finds touching border edges between this box and another.
   * Returns array of touching edge indices, or nullptr if no 1D touches.
   */
  int* findTouchingEdges(const IntBoxShape& other) const;
};

} // namespace freerouting
