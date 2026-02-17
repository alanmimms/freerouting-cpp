#include "autoroute/DrillPageArray.h"
#include "board/RoutingBoard.h"
#include <cmath>
#include <algorithm>

namespace freerouting {

DrillPageArray::DrillPageArray(RoutingBoard* routingBoard, int maxPageWidth)
  : bounds(routingBoard->getBoundingBox()) {

  double length = bounds.ur.x - bounds.ll.x;
  double height = bounds.ur.y - bounds.ll.y;

  columnCount = static_cast<int>(std::ceil(length / maxPageWidth));
  rowCount = static_cast<int>(std::ceil(height / maxPageWidth));

  pageWidth = static_cast<int>(std::ceil(length / columnCount));
  pageHeight = static_cast<int>(std::ceil(height / rowCount));

  // Allocate 2D array
  pageArray.resize(rowCount);
  for (int j = 0; j < rowCount; j++) {
    pageArray[j].resize(columnCount);

    for (int i = 0; i < columnCount; i++) {
      int llX = bounds.ll.x + i * pageWidth;
      int urX;
      if (i == columnCount - 1) {
        urX = bounds.ur.x;
      } else {
        urX = llX + pageWidth;
      }

      int llY = bounds.ll.y + j * pageHeight;
      int urY;
      if (j == rowCount - 1) {
        urY = bounds.ur.y;
      } else {
        urY = llY + pageHeight;
      }

      IntBox pageShape(IntPoint(llX, llY), IntPoint(urX, urY));
      pageArray[j][i] = new DrillPage(pageShape, routingBoard);
    }
  }
}

DrillPageArray::~DrillPageArray() {
  for (int j = 0; j < rowCount; j++) {
    for (int i = 0; i < columnCount; i++) {
      delete pageArray[j][i];
    }
  }
}

void DrillPageArray::invalidate(const Shape* shape) {
  std::vector<DrillPage*> overlaps = overlappingPages(shape);
  for (DrillPage* currPage : overlaps) {
    currPage->invalidate();
  }
}

std::vector<DrillPage*> DrillPageArray::overlappingPages(const Shape* shape) {
  std::vector<DrillPage*> result;

  IntBox shapeBox = shape->getBoundingBox().intersection(bounds);

  int minJ = static_cast<int>(std::floor(static_cast<double>(shapeBox.ll.y - bounds.ll.y) / static_cast<double>(pageHeight)));
  double maxJ = static_cast<double>(shapeBox.ur.y - bounds.ll.y) / static_cast<double>(pageHeight);

  int minI = static_cast<int>(std::floor(static_cast<double>(shapeBox.ll.x - bounds.ll.x) / static_cast<double>(pageWidth)));
  double maxI = static_cast<double>(shapeBox.ur.x - bounds.ll.x) / static_cast<double>(pageWidth);

  for (int j = minJ; j < static_cast<int>(maxJ); j++) {
    if (j < 0 || j >= rowCount) continue;

    for (int i = minI; i < static_cast<int>(maxI); i++) {
      if (i < 0 || i >= columnCount) continue;

      DrillPage* currPage = pageArray[j][i];
      // Check if shape actually intersects with this page
      // Simplified - in full version would check dimension of intersection
      result.push_back(currPage);
    }
  }

  return result;
}

void DrillPageArray::reset() {
  for (int j = 0; j < rowCount; j++) {
    for (int i = 0; i < columnCount; i++) {
      pageArray[j][i]->reset();
    }
  }
}

} // namespace freerouting
