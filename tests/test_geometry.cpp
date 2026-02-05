#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "geometry/Vector2.h"
#include "geometry/IntBox.h"
#include "geometry/IntOctagon.h"
#include "geometry/Side.h"

using namespace freerouting;
using Catch::Approx;

TEST_CASE("Vector2 basic operations", "[geometry][vector2]") {
  SECTION("Construction and equality") {
    IntPoint p1(10, 20);
    IntPoint p2(10, 20);
    IntPoint p3(10, 21);

    REQUIRE(p1 == p2);
    REQUIRE(p1 != p3);
    REQUIRE(p1.x == 10);
    REQUIRE(p1.y == 20);
  }

  SECTION("Zero vector") {
    IntVector v;
    REQUIRE(v.isZero());
    REQUIRE(v.x == 0);
    REQUIRE(v.y == 0);

    IntVector v2(5, 0);
    REQUIRE_FALSE(v2.isZero());
  }

  SECTION("Vector arithmetic") {
    IntVector a(10, 20);
    IntVector b(5, 3);

    auto sum = a + b;
    REQUIRE(sum.x == 15);
    REQUIRE(sum.y == 23);

    auto diff = a - b;
    REQUIRE(diff.x == 5);
    REQUIRE(diff.y == 17);

    auto neg = -a;
    REQUIRE(neg.x == -10);
    REQUIRE(neg.y == -20);
  }

  SECTION("Scalar multiplication") {
    IntVector v(3, 4);
    auto scaled = v * 5;
    REQUIRE(scaled.x == 15);
    REQUIRE(scaled.y == 20);

    auto scaled2 = 2 * v;
    REQUIRE(scaled2.x == 6);
    REQUIRE(scaled2.y == 8);
  }

  SECTION("Dot product") {
    IntVector a(3, 4);
    IntVector b(5, 12);
    REQUIRE(a.dot(b) == 63); // 3*5 + 4*12 = 15 + 48
  }

  SECTION("Cross product") {
    IntVector a(3, 0);
    IntVector b(0, 4);
    REQUIRE(a.cross(b) == 12);  // 3*4 - 0*0

    IntVector c(1, 2);
    IntVector d(3, 4);
    REQUIRE(c.cross(d) == -2);  // 1*4 - 2*3 = 4 - 6
  }

  SECTION("Length operations") {
    IntVector v(3, 4);
    REQUIRE(v.lengthSquared() == 25);

    FloatVector fv(3.0, 4.0);
    REQUIRE(fv.length() == Approx(5.0));

    auto normalized = fv.normalized();
    REQUIRE(normalized.x == Approx(0.6));
    REQUIRE(normalized.y == Approx(0.8));
    REQUIRE(normalized.length() == Approx(1.0));
  }

  SECTION("Orthogonal and diagonal checks") {
    REQUIRE(IntVector(5, 0).isOrthogonal());
    REQUIRE(IntVector(0, 5).isOrthogonal());
    REQUIRE_FALSE(IntVector(3, 4).isOrthogonal());

    REQUIRE(IntVector(5, 5).isDiagonal());
    REQUIRE(IntVector(-3, 3).isDiagonal());
    REQUIRE_FALSE(IntVector(3, 4).isDiagonal());
  }

  SECTION("Rotation by 90 degrees") {
    IntVector v(3, 4);

    auto v90 = v.rotate90(1);
    REQUIRE(v90.x == -4);
    REQUIRE(v90.y == 3);

    auto v180 = v.rotate90(2);
    REQUIRE(v180.x == -3);
    REQUIRE(v180.y == -4);

    auto v270 = v.rotate90(3);
    REQUIRE(v270.x == 4);
    REQUIRE(v270.y == -3);

    auto v360 = v.rotate90(4);
    REQUIRE(v360 == v);
  }

  SECTION("Type conversion") {
    IntPoint ip(10, 20);
    auto fp = ip.toDouble();
    REQUIRE(fp.x == Approx(10.0));
    REQUIRE(fp.y == Approx(20.0));

    FloatPoint fp2(3.7, 4.9);
    auto ip2 = fp2.toInt();
    REQUIRE(ip2.x == 3);
    REQUIRE(ip2.y == 4);
  }

  SECTION("Manhattan and Chebyshev distance") {
    IntVector v(3, -4);
    REQUIRE(v.manhattanLength() == 7);
    REQUIRE(v.chebyshevLength() == 4);
  }
}

TEST_CASE("Side enumeration and predicates", "[geometry][side]") {
  SECTION("sideFrom creates correct side") {
    REQUIRE(sideFrom(5.0) == Side::OnTheLeft);
    REQUIRE(sideFrom(-5.0) == Side::OnTheRight);
    REQUIRE(sideFrom(0.0) == Side::Collinear);
  }

  SECTION("negateSide works correctly") {
    REQUIRE(negateSide(Side::OnTheLeft) == Side::OnTheRight);
    REQUIRE(negateSide(Side::OnTheRight) == Side::OnTheLeft);
    REQUIRE(negateSide(Side::Collinear) == Side::Collinear);
  }

  SECTION("Vector sideOf method") {
    IntVector v1(1, 0);    // Right
    IntVector v2(1, 1);    // Northeast
    IntVector v3(0, 1);    // Up
    IntVector v4(-1, 0);   // Left
    IntVector v5(1, -1);   // Southeast

    // v2 is left of v1 (northeast is left of east)
    REQUIRE(v2.sideOf(v1) == Side::OnTheLeft);
    // v3 is left of v1 (north is left of east)
    REQUIRE(v3.sideOf(v1) == Side::OnTheLeft);
    // v4 is collinear with v1 (opposite direction on same line)
    REQUIRE(v4.sideOf(v1) == Side::Collinear);
    // v5 is right of v1 (southeast is right of east)
    REQUIRE(v5.sideOf(v1) == Side::OnTheRight);
    // v1 is collinear with itself
    REQUIRE(v1.sideOf(v1) == Side::Collinear);

    // Cross-check: v1 is right of v2
    REQUIRE(v1.sideOf(v2) == Side::OnTheRight);
  }
}

TEST_CASE("IntBox operations", "[geometry][intbox]") {
  SECTION("Construction") {
    IntBox box(10, 20, 50, 60);
    REQUIRE(box.ll.x == 10);
    REQUIRE(box.ll.y == 20);
    REQUIRE(box.ur.x == 50);
    REQUIRE(box.ur.y == 60);
  }

  SECTION("Empty box") {
    IntBox empty = IntBox::empty();
    REQUIRE(empty.isEmpty());

    IntBox valid(0, 0, 10, 10);
    REQUIRE_FALSE(valid.isEmpty());

    IntBox invalid(10, 10, 5, 5);
    REQUIRE(invalid.isEmpty());
  }

  SECTION("Dimensions") {
    IntBox box(10, 20, 50, 80);
    REQUIRE(box.width() == 40);
    REQUIRE(box.height() == 60);
    REQUIRE(box.maxDimension() == 60);
    REQUIRE(box.minDimension() == 40);
  }

  SECTION("Area and perimeter") {
    IntBox box(0, 0, 10, 20);
    REQUIRE(box.area() == Approx(200.0));
    REQUIRE(box.perimeter() == Approx(60.0));
  }

  SECTION("Center point") {
    IntBox box(0, 0, 10, 20);
    auto center = box.center();
    REQUIRE(center.x == Approx(5.0));
    REQUIRE(center.y == Approx(10.0));
  }

  SECTION("Corners") {
    IntBox box(10, 20, 50, 60);
    REQUIRE(box.corner(0) == IntPoint(10, 20));  // lower-left
    REQUIRE(box.corner(1) == IntPoint(50, 20));  // lower-right
    REQUIRE(box.corner(2) == IntPoint(50, 60));  // upper-right
    REQUIRE(box.corner(3) == IntPoint(10, 60));  // upper-left
  }

  SECTION("Point containment") {
    IntBox box(10, 20, 50, 60);
    REQUIRE(box.contains(IntPoint(30, 40)));
    REQUIRE(box.contains(IntPoint(10, 20)));  // On boundary
    REQUIRE(box.contains(IntPoint(50, 60)));  // On boundary
    REQUIRE_FALSE(box.contains(IntPoint(5, 40)));
    REQUIRE_FALSE(box.contains(IntPoint(60, 40)));
  }

  SECTION("Box intersection test") {
    IntBox box1(0, 0, 10, 10);
    IntBox box2(5, 5, 15, 15);
    IntBox box3(20, 20, 30, 30);

    REQUIRE(box1.intersects(box2));
    REQUIRE(box2.intersects(box1));
    REQUIRE_FALSE(box1.intersects(box3));
  }

  SECTION("Box containment") {
    IntBox outer(0, 0, 100, 100);
    IntBox inner(10, 10, 50, 50);
    IntBox overlap(50, 50, 150, 150);

    REQUIRE(outer.contains(inner));
    REQUIRE_FALSE(inner.contains(outer));
    REQUIRE_FALSE(outer.contains(overlap));
  }

  SECTION("Union") {
    IntBox box1(0, 0, 10, 10);
    IntBox box2(5, 5, 15, 15);
    auto un = box1.unionWith(box2);

    REQUIRE(un.ll.x == 0);
    REQUIRE(un.ll.y == 0);
    REQUIRE(un.ur.x == 15);
    REQUIRE(un.ur.y == 15);
  }

  SECTION("Intersection") {
    IntBox box1(0, 0, 10, 10);
    IntBox box2(5, 5, 15, 15);
    auto inter = box1.intersection(box2);

    REQUIRE(inter.ll.x == 5);
    REQUIRE(inter.ll.y == 5);
    REQUIRE(inter.ur.x == 10);
    REQUIRE(inter.ur.y == 10);

    IntBox box3(20, 20, 30, 30);
    auto noInter = box1.intersection(box3);
    REQUIRE(noInter.isEmpty());
  }

  SECTION("Expand") {
    IntBox box(10, 20, 30, 40);
    auto expanded = box.expand(5);
    REQUIRE(expanded.ll.x == 5);
    REQUIRE(expanded.ll.y == 15);
    REQUIRE(expanded.ur.x == 35);
    REQUIRE(expanded.ur.y == 45);
  }

  SECTION("Translate") {
    IntBox box(10, 20, 30, 40);
    auto translated = box.translate(IntVector(5, -5));
    REQUIRE(translated.ll.x == 15);
    REQUIRE(translated.ll.y == 15);
    REQUIRE(translated.ur.x == 35);
    REQUIRE(translated.ur.y == 35);
  }
}

TEST_CASE("IntOctagon operations", "[geometry][intoctagon]") {
  SECTION("Construction") {
    IntOctagon oct(10, 20, 50, 60, 80, 40, 10, 70);
    REQUIRE(oct.leftX == 10);
    REQUIRE(oct.bottomY == 20);
    REQUIRE(oct.rightX == 50);
    REQUIRE(oct.topY == 60);
  }

  SECTION("Empty octagon") {
    IntOctagon empty = IntOctagon::empty();
    REQUIRE(empty.isEmpty());
  }

  SECTION("Octagon from point") {
    IntPoint p(10, 20);
    auto oct = IntOctagon::fromPoint(p);

    REQUIRE(oct.leftX == 10);
    REQUIRE(oct.bottomY == 20);
    REQUIRE(oct.rightX == 10);
    REQUIRE(oct.topY == 20);
    REQUIRE(oct.contains(p));
    REQUIRE(oct.dimension() == 0);  // Point
  }

  SECTION("Octagon from box") {
    IntBox box(10, 20, 50, 60);
    auto oct = IntOctagon::fromBox(box);

    REQUIRE(oct.leftX == 10);
    REQUIRE(oct.bottomY == 20);
    REQUIRE(oct.rightX == 50);
    REQUIRE(oct.topY == 60);
    REQUIRE(oct.contains(IntPoint(30, 40)));
  }

  SECTION("Bounding box") {
    IntOctagon oct(10, 20, 50, 60, 80, 40, 10, 70);
    auto box = oct.boundingBox();

    REQUIRE(box.ll.x == 10);
    REQUIRE(box.ll.y == 20);
    REQUIRE(box.ur.x == 50);
    REQUIRE(box.ur.y == 60);
  }

  SECTION("Point containment") {
    // Create a centered octagon
    IntPoint p(50, 50);
    auto oct = IntOctagon::fromPoint(p).expand(10);

    REQUIRE(oct.contains(p));
    REQUIRE(oct.contains(IntPoint(50, 55)));
    REQUIRE(oct.contains(IntPoint(55, 50)));
  }

  SECTION("Union") {
    auto oct1 = IntOctagon::fromPoint(IntPoint(10, 10)).expand(5);
    auto oct2 = IntOctagon::fromPoint(IntPoint(30, 30)).expand(5);
    auto un = oct1.unionWith(oct2);

    REQUIRE(un.contains(IntPoint(10, 10)));
    REQUIRE(un.contains(IntPoint(30, 30)));
  }

  SECTION("Intersection") {
    auto oct1 = IntOctagon::fromPoint(IntPoint(10, 10)).expand(10);
    auto oct2 = IntOctagon::fromPoint(IntPoint(15, 15)).expand(10);
    auto inter = oct1.intersection(oct2);

    REQUIRE_FALSE(inter.isEmpty());
    REQUIRE(inter.contains(IntPoint(15, 15)));
  }

  SECTION("Expand") {
    IntOctagon oct(10, 20, 50, 60, 80, 40, 10, 70);
    auto expanded = oct.expand(5);

    REQUIRE(expanded.leftX == oct.leftX - 5);
    REQUIRE(expanded.bottomY == oct.bottomY - 5);
    REQUIRE(expanded.rightX == oct.rightX + 5);
    REQUIRE(expanded.topY == oct.topY + 5);
  }

  SECTION("Translate") {
    IntOctagon oct(10, 20, 50, 60, 80, 40, 10, 70);
    auto translated = oct.translate(IntVector(5, 5));

    REQUIRE(translated.leftX == 15);
    REQUIRE(translated.bottomY == 25);
    REQUIRE(translated.rightX == 55);
    REQUIRE(translated.topY == 65);
  }
}
