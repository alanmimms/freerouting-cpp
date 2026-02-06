#ifndef FREEROUTING_BOARD_ROUTEOPTIMIZER_H
#define FREEROUTING_BOARD_ROUTEOPTIMIZER_H

#include "board/Trace.h"
#include "board/Via.h"
#include "geometry/Vector2.h"
#include <vector>
#include <algorithm>

namespace freerouting {

// Utility class for optimizing routed traces
// Provides algorithms for simplifying and improving routes
class RouteOptimizer {
public:
  // Merge collinear traces that share an endpoint
  // Returns the merged trace, or nullptr if traces cannot be merged
  static Trace* mergeCollinearTraces(const Trace& trace1, const Trace& trace2) {
    if (!trace1.canMergeWith(trace2)) {
      return nullptr;
    }

    // Find the two endpoints that are farthest apart
    std::vector<IntPoint> points = {
      trace1.getStart(), trace1.getEnd(),
      trace2.getStart(), trace2.getEnd()
    };

    IntPoint p1 = points[0];
    IntPoint p2 = points[0];
    int maxDistSq = 0;

    for (size_t i = 0; i < points.size(); i++) {
      for (size_t j = i + 1; j < points.size(); j++) {
        IntVector delta = points[j] - points[i];
        int distSq = delta.lengthSquared();
        if (distSq > maxDistSq) {
          maxDistSq = distSq;
          p1 = points[i];
          p2 = points[j];
        }
      }
    }

    // Create merged trace
    return new Trace(
      p1, p2,
      trace1.getLayer(),
      trace1.getHalfWidth(),
      trace1.getNets(),
      trace1.getClearanceClass(),
      trace1.getId(),  // Keep ID of first trace
      trace1.getFixedState(),
      trace1.getBoard()
    );
  }

  // Find redundant traces in a list (traces that are subsumed by others)
  static std::vector<const Trace*> findRedundantTraces(
      const std::vector<Trace*>& traces) {
    std::vector<const Trace*> redundant;

    for (size_t i = 0; i < traces.size(); i++) {
      for (size_t j = 0; j < traces.size(); j++) {
        if (i == j) continue;

        const Trace* t1 = traces[i];
        const Trace* t2 = traces[j];

        // Check if t1 is subsumed by t2
        if (isTraceSubsumed(t1, t2)) {
          redundant.push_back(t1);
          break;
        }
      }
    }

    return redundant;
  }

  // Check if trace1 is completely contained within trace2
  static bool isTraceSubsumed(const Trace* trace1, const Trace* trace2) {
    // Must be on same layer with same width and nets
    if (trace1->getLayer() != trace2->getLayer() ||
        trace1->getHalfWidth() != trace2->getHalfWidth() ||
        trace1->getNets() != trace2->getNets()) {
      return false;
    }

    // Check if both endpoints of trace1 are on trace2's centerline
    bool startOnLine = isPointOnSegment(
      trace1->getStart(), trace2->getStart(), trace2->getEnd());
    bool endOnLine = isPointOnSegment(
      trace1->getEnd(), trace2->getStart(), trace2->getEnd());

    return startOnLine && endOnLine;
  }

  // Calculate total length of a route
  static double calculateRouteLength(const std::vector<Trace*>& traces) {
    double totalLength = 0.0;
    for (const Trace* trace : traces) {
      totalLength += trace->getLength();
    }
    return totalLength;
  }

  // Count number of direction changes in a route
  static int countDirectionChanges(const std::vector<Trace*>& traces) {
    if (traces.size() < 2) {
      return 0;
    }

    int changes = 0;
    for (size_t i = 1; i < traces.size(); i++) {
      IntVector v1 = traces[i-1]->getEnd() - traces[i-1]->getStart();
      IntVector v2 = traces[i]->getEnd() - traces[i]->getStart();

      // Check if vectors point in different directions
      // (Normalize by comparing cross products)
      if (v1.cross(v2) != 0) {
        changes++;
      }
    }

    return changes;
  }

  // Straighten a route by removing unnecessary bends
  // Returns optimized list of traces
  static std::vector<Trace*> straightenRoute(const std::vector<Trace*>& traces) {
    if (traces.size() < 3) {
      // Nothing to optimize
      std::vector<Trace*> result;
      for (Trace* t : traces) {
        result.push_back(new Trace(*t));
      }
      return result;
    }

    std::vector<Trace*> result;
    size_t i = 0;

    while (i < traces.size()) {
      // Start with current trace
      IntPoint start = traces[i]->getStart();
      IntPoint end = traces[i]->getEnd();
      size_t j = i + 1;

      // Try to extend as far as possible in a straight line
      while (j < traces.size()) {
        IntPoint nextEnd = traces[j]->getEnd();

        // Check if we can make a straight trace from start to nextEnd
        bool canStraighten = true;
        IntVector straightVec = nextEnd - start;

        // Check if all intermediate points are collinear
        for (size_t k = i; k <= j; k++) {
          IntVector v1 = traces[k]->getStart() - start;
          IntVector v2 = traces[k]->getEnd() - start;

          if (straightVec.cross(v1) != 0 || straightVec.cross(v2) != 0) {
            canStraighten = false;
            break;
          }
        }

        if (canStraighten) {
          end = nextEnd;
          j++;
        } else {
          break;
        }
      }

      // Create optimized trace
      const Trace* template_trace = traces[i];
      result.push_back(new Trace(
        start, end,
        template_trace->getLayer(),
        template_trace->getHalfWidth(),
        template_trace->getNets(),
        template_trace->getClearanceClass(),
        template_trace->getId(),
        template_trace->getFixedState(),
        template_trace->getBoard()
      ));

      i = (j > i + 1) ? j : i + 1;
    }

    return result;
  }

  // Check if a point lies on a line segment
  static bool isPointOnSegment(IntPoint p, IntPoint segStart, IntPoint segEnd) {
    // Check if p is collinear with segment
    IntVector v1 = segEnd - segStart;
    IntVector v2 = p - segStart;

    if (v1.cross(v2) != 0) {
      return false;  // Not collinear
    }

    // Check if p is between segStart and segEnd
    int dotProduct = v1.dot(v2);
    if (dotProduct < 0) {
      return false;  // p is before segStart
    }

    int segLenSq = v1.lengthSquared();
    if (dotProduct > segLenSq) {
      return false;  // p is after segEnd
    }

    return true;
  }

  // Find mergeable trace pairs in a list
  static std::vector<std::pair<int, int>> findMergeableTracePairs(
      const std::vector<Trace*>& traces) {
    std::vector<std::pair<int, int>> pairs;

    for (size_t i = 0; i < traces.size(); i++) {
      for (size_t j = i + 1; j < traces.size(); j++) {
        if (traces[i]->canMergeWith(*traces[j])) {
          pairs.push_back({static_cast<int>(i), static_cast<int>(j)});
        }
      }
    }

    return pairs;
  }

  // Calculate wire length metric for route quality
  // Lower is better
  static double calculateWireLengthMetric(const std::vector<Trace*>& traces) {
    double length = calculateRouteLength(traces);
    int bends = countDirectionChanges(traces);

    // Penalize bends (each bend adds equivalent of 100 units)
    return length + (bends * 100.0);
  }

private:
  RouteOptimizer() = delete;  // Static class, no instances
};

} // namespace freerouting

#endif // FREEROUTING_BOARD_ROUTEOPTIMIZER_H
