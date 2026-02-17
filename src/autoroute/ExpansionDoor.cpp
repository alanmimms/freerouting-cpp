#include "autoroute/ExpansionDoor.h"
#include "autoroute/CompleteExpansionRoom.h"
#include "autoroute/CompleteFreeSpaceExpansionRoom.h"
#include "autoroute/AutorouteEngine.h"

namespace freerouting {

ExpansionDoor::ExpansionDoor(ExpansionRoom* first, ExpansionRoom* second)
  : firstRoom(first), secondRoom(second), cachedShape(nullptr) {
  // Calculate dimension based on shape intersection
  // For now, assume dimension 1 (will be refined when we have proper shape intersection)
  dimension = 1;
}

const Shape* ExpansionDoor::getShape() const {
  // Return cached shape if available
  // TODO: Implement proper shape intersection when we have TileShape
  return cachedShape;
}

ExpansionRoom* ExpansionDoor::otherRoom(ExpansionRoom* room) {
  if (room == firstRoom) {
    return secondRoom;
  } else if (room == secondRoom) {
    return firstRoom;
  }
  return nullptr;
}

CompleteExpansionRoom* ExpansionDoor::otherRoom(CompleteExpansionRoom* room) {
  ExpansionRoom* result = nullptr;
  if (room == firstRoom) {
    result = secondRoom;
  } else if (room == secondRoom) {
    result = firstRoom;
  }

  // Check if result is a CompleteExpansionRoom
  return dynamic_cast<CompleteExpansionRoom*>(result);
}

void ExpansionDoor::reset() {
  for (auto& section : sectionArray) {
    section.reset();
  }
}

void ExpansionDoor::allocateSections(int sectionCount) {
  if (static_cast<int>(sectionArray.size()) == sectionCount) {
    return;  // Already allocated
  }
  sectionArray.resize(sectionCount);
}

std::vector<FloatLine> ExpansionDoor::getSectionSegments(double offset) {
  constexpr double traceWidthTolerance = 0.5;  // AutorouteEngine::TRACE_WIDTH_TOLERANCE
  double adjustedOffset = offset + traceWidthTolerance;

  const Shape* doorShape = this->getShape();
  if (!doorShape || doorShape->isEmpty()) {
    return {};  // Empty result
  }

  FloatLine doorLineSegment;
  FloatLine shrinkedLineSegment;

  if (this->dimension == 1) {
    // 1-dimensional door - get diagonal corner segment
    // For now, simplified to use shape center (need full Shape implementation)
    FloatPoint center = FloatPoint::fromInt(doorShape->getCentroid());
    doorLineSegment = FloatLine(center, center);
    shrinkedLineSegment = doorLineSegment.shrinkSegment(adjustedOffset);
  } else if (this->dimension == 2) {
    // 2-dimensional door
    CompleteFreeSpaceExpansionRoom* firstFreeRoom =
      dynamic_cast<CompleteFreeSpaceExpansionRoom*>(firstRoom);
    CompleteFreeSpaceExpansionRoom* secondFreeRoom =
      dynamic_cast<CompleteFreeSpaceExpansionRoom*>(secondRoom);

    if (firstFreeRoom && secondFreeRoom) {
      // Both rooms are free space - calculate restraint line
      // For now, simplified to use center point
      FloatPoint center = FloatPoint::fromInt(doorShape->getCentroid());
      doorLineSegment = FloatLine(center, center);

      if (doorLineSegment.length() < 2.0 * adjustedOffset) {
        // Door is too small
        return {};
      }

      shrinkedLineSegment = doorLineSegment.shrinkSegment(adjustedOffset);
    } else {
      // Obstacle room - use gravity point
      FloatPoint gravityPoint = FloatPoint::fromInt(doorShape->getCentroid());
      doorLineSegment = FloatLine(gravityPoint, gravityPoint);
      shrinkedLineSegment = doorLineSegment;
    }
  } else {
    // Unexpected dimension
    return {};
  }

  // Calculate number of sections based on door length
  constexpr double maxDoorSectionWidth = 10.0;  // Multiplied by offset
  double doorLength = doorLineSegment.length();
  int sectionCount = static_cast<int>(doorLength / (maxDoorSectionWidth * adjustedOffset)) + 1;

  // Allocate sections
  this->allocateSections(sectionCount);

  // Divide the shrinked line segment into sections
  return shrinkedLineSegment.divideSegmentIntoSections(sectionCount);
}

} // namespace freerouting
