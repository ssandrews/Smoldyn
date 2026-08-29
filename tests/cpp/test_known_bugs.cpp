// Tests that assert the CORRECT behaviour for a handful of known defects in
// libSteve. These are written against the intended semantics, so they currently
// FAIL, deliberately flagging the bug. Once the underlying functions are fixed
// (see the doc-comments on each case) these tests should start passing.
//
// Kept in a separate target so the intentionally-failing cases are easy to
// spot and don't mask regressions in the rest of the suite.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

extern "C" {
#include "math2.h"
#include "string2.h"
}

using Catch::Approx;

TEST_CASE("BUG: gcomdiv returns 1 for a zero argument instead of the other value",
          "[known-bug][math2][gcomdiv]") {
    // gcd(0, n) == |n| and gcd(n, 0) == |n|; only gcd(0, 0) is 0 by convention.
    // Current implementation: "if(m==0||n==0) return 1;"  (math2.c).
    CHECK(gcomdiv(0, 5) == 5);
    CHECK(gcomdiv(0, 3) == 3);
    CHECK(gcomdiv(12, 0) == 12);
    CHECK(gcomdiv(-6, 0) == 6);
}

TEST_CASE("BUG: choose/chooseD yields 1 instead of 0 when k > n",
          "[known-bug][math2][choose]") {
    // C(n, k) == 0 for k > n. Current code sets m = n - m which goes negative
    // and the loop body never runs, so it returns 1 (math2.c).
    CHECK(chooseD(3, 5) == 0);
    CHECK(chooseD(2, 4) == 0);
    CHECK(chooseD(10, 11) == 0);
    CHECK(choose(2, 4) == 0.0f);        // float variant has the same defect
    // Sanity: the valid direction still works.
    CHECK(chooseD(5, 3) == 10);
}

TEST_CASE("BUG: erfncD loses all significance for large x (underflows to 0)",
          "[known-bug][math2][erfnc]") {
    // erfncD is implemented as 1 - gammpD(0.5, x*x); for large x the result is
    // computed by cancellation and collapses to 0.0 (from roughly x >= 6). The
    // accurate complementary error function is erfccD (math2.c:395 vs math2.c:408).
    CHECK(erfncD(6.0) > 0.0);
    CHECK(erfncD(6.0) == Approx(erfccD(6.0)).epsilon(1e-6));
    CHECK(erfncD(10.0) > 0.0);
    CHECK(erfncD(10.0) == Approx(erfccD(10.0)).epsilon(1e-6));
}

TEST_CASE("BUG: strisnumber accepts non-finite special values as numbers",
          "[known-bug][string2][parse]") {
    // strisnumber is used to validate numeric config values; 'NaN'/'inf' are
    // not sensible numbers and should be rejected, but the strtod-based test
    // currently accepts them (string2.c:48).
    CHECK(strisnumber("NaN") == 0);
    CHECK(strisnumber("inf") == 0);
    CHECK(strisnumber("-inf") == 0);
    CHECK(strisnumber("Infinity") == 0);
}
