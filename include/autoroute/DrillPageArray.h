#ifndef FREEROUTING_AUTOROUTE_DRILLPAGEARRAY_H
#define FREEROUTING_AUTOROUTE_DRILLPAGEARRAY_H

#include "autoroute/DrillPage.h"
#include "geometry/IntBox.h"
#include "geometry/Shape.h"
#include <vector>

namespace freerouting {

// Forward declaration
class RoutingBoard;

// Describes the 2 dimensional array of pages of ExpansionDrills used in the maze search algorithm
// The pages are rectangles of about equal width and height covering the bounding box of the board area
class DrillPageArray {
public:
  // Creates a new DrillPageArray
  // maxPageWidth determines the granularity of the spatial subdivision
  DrillPageArray(RoutingBoard* routingBoard, int maxPageWidth);

  ~DrillPageArray();

  // Invalidates all drill pages intersecting with shape
  // So they must be recalculated at the next call of getDrills()
  void invalidate(const Shape* shape);

  // Collects all drill pages with a 2-dimensional overlap with shape
  std::vector<DrillPage*> overlappingPages(const Shape* shape);

  // Resets all drill pages for autorouting the next connection
  void reset();

private:
  IntBox bounds;

  // The number of columns in the array
  int columnCount;

  // The number of rows in the array
  int rowCount;

  // The width of a single page in this array
  int pageWidth;

  // The height of a single page in this array
  int pageHeight;

  // 2D array of drill pages [row][column]
  std::vector<std::vector<DrillPage*>> pageArray;
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_DRILLPAGEARRAY_H
