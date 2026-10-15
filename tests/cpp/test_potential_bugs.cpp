// Tests that assert the CORRECT behaviour for a handful of known defects in the
// Smoldyn core / libSteve. These are written against the intended semantics, so
// they currently FAIL, deliberately flagging the bug. Once the underlying
// functions are fixed (see the doc-comments on each case) these tests should
// start passing.

#include <cstring>
#include <cmath>

extern "C" {
#include "math2.h"
#include "string2.h"
#include "SurfaceParam.h"
#include "smoldynfuncs.h"
}

// smoldynfuncs.h defines its own CHECK macro (used for internal error handling)
// which collides with Catch2's assertion macro. Undefine the whole family before
// the Catch2 headers are brought in.
#undef CHECK
#undef CHECKMEM
#undef CHECKM
#undef CHECKBUG
#undef CHECKS

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

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

TEST_CASE("BUG: surfaceprob yields non-finite probabilities for a zero rate",
          "[known-bug][surface][surfaceprob]") {
    // A "no adsorption, only desorption" rate (k1 == 0) is a valid physical
    // input, but surfaceprob returns p2 == inf for SPArevAds
    // (SurfaceParam.c:133 uses lookuprevads which divides by the adsorption
    // capacity). All returned probabilities must be finite and in [0, 1].
    const double dt = 0.1;
    const double difc = 1.0;
    double p2 = 0.0;
    double p1 = surfaceprob(0.0, 0.5, dt, difc, &p2, SPArevAds);
    CHECK(p1 >= 0.0);
    CHECK(p1 <= 1.0);
    CHECK(std::isfinite(p1));
    CHECK(std::isfinite(p2));      // currently inf
    CHECK(p2 >= 0.0);
    CHECK(p2 <= 1.0);
}

TEST_CASE("BUG: molserno2string/molserno2string lose serial numbers with a zero part",
          "[known-bug][serno][parse]") {
    // The "hi.lo" serial-number encoding can't be parsed back when either part
    // is 0: 0xFFFFFFFF -> "0.4294967295" and 0x100000000 -> "1.0" both parse to
    // 0 (smolmolec.c:625 returns 0 when i1==0 || i2==0). Round-trip must be exact.
    char buf[STRCHARLONG];
    for (unsigned long long s : {0xFFFFFFFFULL, 0x100000000ULL}) {
        molserno2string(s, buf);
        CAPTURE(s, buf);
        CHECK(molstring2serno(buf) == s);   // currently returns 0
    }
}
