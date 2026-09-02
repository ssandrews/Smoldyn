// Unit tests for libSteve Geometry routines: distances, areas, volumes and
// point-in-region tests used for surface/panel geometry in Smoldyn.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>

extern "C" {
#include "Geometry.h"
}

using Catch::Approx;

namespace {
constexpr double kPi = 3.14159265358979323846;
}

TEST_CASE("Geo_LineLength in 2D and 3D", "[geometry][distance]") {
    double p1[3] = {0, 0, 0};
    double p2[3] = {3, 4, 0};
    CHECK(Geo_LineLength(p1, p2, 2) == Approx(5.0));

    double p3[3] = {1, 2, 2};
    CHECK(Geo_LineLength(p1, p3, 3) == Approx(3.0)); // sqrt(1+4+4)
}

TEST_CASE("Geo_TriArea2 and Geo_TriArea3D agree for a right triangle", "[geometry][area]") {
    double p1[3] = {0, 0, 0};
    double p2[3] = {2, 0, 0};
    double p3[3] = {0, 2, 0};
    CHECK(Geo_TriArea2(p1, p2, p3) == Approx(2.0));       // |x2*y3|/2
    CHECK(Geo_TriArea3D(p1, p2, p3) == Approx(2.0));      // cross product / 2

    double p4[3] = {3, 0, 0};
    double p5[3] = {0, 4, 0};
    CHECK(Geo_TriArea3D(p1, p4, p5) == Approx(6.0));      // 3*4/2
}

TEST_CASE("Geo_SphVolume for 2D circles and 3D spheres", "[geometry][volume]") {
    CHECK(Geo_SphVolume(2.0, 2) == Approx(4.0 * kPi));        // pi r^2
    CHECK(Geo_SphVolume(3.0, 3) == Approx(36.0 * kPi));       // 4/3 pi r^3
    CHECK(Geo_SphVolume(1.0, 2) == Approx(kPi).epsilon(1e-12));
    CHECK(Geo_SphVolume(1.0, 3) == Approx(4.0 / 3.0 * kPi).epsilon(1e-12));
}

TEST_CASE("Geo_NearestSeg2SegDist between separated segments", "[geometry][distance]") {
    // Two parallel horizontal segments at y=0 and y=1 -> distance 1.
    double s1p1[2] = {0, 0}, s1p2[2] = {1, 0};
    double s2p1[2] = {0, 1}, s2p2[2] = {1, 1};
    CHECK(Geo_NearestSeg2SegDist(s1p1, s1p2, s2p1, s2p2) == Approx(1.0).epsilon(1e-12));
}

TEST_CASE("Geo_NearestSeg2SegDist for crossing segments", "[geometry][distance]") {
    // Segment A: (0,0)-(1,1); segment B: (0,1)-(1,0). They cross -> distance 0.
    double a1[2] = {0, 0}, a2[2] = {1, 1};
    double b1[2] = {0, 1}, b2[2] = {1, 0};
    CHECK(Geo_NearestSeg2SegDist(a1, a2, b1, b2) == Approx(0.0).margin(1e-12));
}

TEST_CASE("Geo_PtInSphere and Geo_PtInSlab", "[geometry][pointintest]") {
    double cent[3] = {0, 0, 0};
    CHECK(Geo_PtInSphere((double[]){0, 0, 0}, cent, 1.0, 3) == 1);
    CHECK(Geo_PtInSphere((double[]){0.5, 0, 0}, cent, 1.0, 3) == 1);
    CHECK(Geo_PtInSphere((double[]){1.1, 0, 0}, cent, 1.0, 3) == 0);

    double p1[2] = {0, 0}, p2[2] = {10, 0};
    CHECK(Geo_PtInSlab(p1, p2, (double[]){5, 100}, 2) == 1);   // x within segment
    CHECK(Geo_PtInSlab(p1, p2, (double[]){15, 0}, 2) == 0);    // x outside segment
}

TEST_CASE("Geo_NearestAabbPt closest point on/inside a box", "[geometry][nearest]") {
    double bpt1[2] = {0, 0};
    double bpt2[2] = {10, 10};
    double ans[2];

    // Point inside the box: nearest point is the point itself.
    double inside[2] = {5, 5};
    Geo_NearestAabbPt(bpt1, bpt2, 2, inside, ans);
    CHECK(ans[0] == Approx(5.0));
    CHECK(ans[1] == Approx(5.0));

    // Point outside: clamp to the box surface.
    double outside[2] = {15, -3};
    Geo_NearestAabbPt(bpt1, bpt2, 2, outside, ans);
    CHECK(ans[0] == Approx(10.0));
    CHECK(ans[1] == Approx(0.0));
}

TEST_CASE("Geo_PtInTriangle for a triangle in the plane", "[geometry][pointintest]") {
    double p1[3] = {0, 0, 0};
    double p2[3] = {2, 0, 0};
    double p3[3] = {0, 2, 0};
    double norm[3];
    Geo_TriNormal(p1, p2, p3, norm);
    CHECK(Geo_PtInTriangle(p1, p2, p3, norm, (double[]){0.5, 0.5, 0}) == 1);
    CHECK(Geo_PtInTriangle(p1, p2, p3, norm, (double[]){1.5, 1.5, 0}) == 0);
    CHECK(Geo_PtInTriangle(p1, p2, p3, norm, (double[]){0.1, 0.1, 0}) == 1);
}
