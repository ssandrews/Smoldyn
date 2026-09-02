// Unit tests for libSteve math2 numerical and special-function routines.
// These are the pure numeric kernels used throughout Smoldyn (diffusion/reaction
// rates, Gaussian sampling, Hill response curves, etc.).

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>

extern "C" {
#include "math2.h"
}

using Catch::Approx;

TEST_CASE("gaussD returns the standard normal density", "[math2][gaussian]") {
    // 1/sqrt(2*pi)
    const double invSqrt2Pi = 0.3989422804014327;
    SECTION("unit normal at the mean") {
        // The implementation uses a truncated SQRT2PI constant, so allow 1e-10.
        CHECK(gaussD(0.0, 0.0, 1.0) == Approx(invSqrt2Pi).epsilon(1e-10));
    }
    SECTION("mean, sd=2") {
        CHECK(gaussD(1.0, 1.0, 2.0) == Approx(invSqrt2Pi / 2.0).epsilon(1e-10));
    }
    SECTION("symmetric about the mean") {
        CHECK(gaussD(0.5, 0.0, 1.0) == Approx(gaussD(-0.5, 0.0, 1.0)).epsilon(1e-12));
    }
}

TEST_CASE("gammalnD matches the natural log of Gamma", "[math2][special]") {
    CHECK(gammalnD(1.0) == Approx(lgamma(1.0)).epsilon(1e-10));     // log(1) = 0
    CHECK(gammalnD(5.0) == Approx(3.1780538303479458).epsilon(1e-10)); // log(24)
    // 0.5 is a hard-coded 9-digit constant (0.572364942) in the implementation.
    CHECK(gammalnD(0.5) == Approx(0.5723649429247001).epsilon(1e-8));
    CHECK(gammalnD(10.0) == Approx(lgamma(10.0)).epsilon(1e-10));     // log(362880)
}

TEST_CASE("error functions are self-consistent", "[math2][special]") {
    // erfD is a rational approximation accurate to ~1e-8, so tolerances are
    // relaxed accordingly.
    CHECK(erfD(0.0) == Approx(0.0).margin(1e-7));
    CHECK(erfD(1.0) == Approx(0.8427007929497149).epsilon(1e-7));
    // erf(x) + erfc(x) = 1
    CHECK(erfD(1.0) + erfncD(1.0) == Approx(1.0).epsilon(1e-6));
    CHECK(erfccD(1.0) == Approx(erfncD(1.0)).epsilon(1e-7));
    CHECK(erfccD(1.0) == Approx(0.15729920705028513).epsilon(1e-7));
}

TEST_CASE("bessj0D at known points", "[math2][special]") {
    CHECK(bessj0D(0.0) == Approx(1.0).epsilon(1e-12));
    CHECK(bessj0D(2.0) == Approx(0.22389077914123567).epsilon(1e-7));
}

TEST_CASE("factorialD and chooseD for small arguments", "[math2][combinatorics]") {
    CHECK(factorialD(0) == Approx(1.0));
    CHECK(factorialD(1) == Approx(1.0));
    CHECK(factorialD(6) == Approx(720.0));
    CHECK(factorialD(10) == Approx(3628800.0));

    CHECK(chooseD(5, 2) == Approx(10.0));
    CHECK(chooseD(10, 3) == Approx(120.0));
    CHECK(chooseD(52, 5) == Approx(2598960.0)); // number of poker hands
    CHECK(chooseD(7, 0) == Approx(1.0));
    CHECK(chooseD(7, 7) == Approx(1.0));
}

TEST_CASE("gcomdiv computes the greatest common divisor", "[math2][combinatorics]") {
    CHECK(gcomdiv(12, 18) == 6);
    CHECK(gcomdiv(17, 5) == 1);
    CHECK(gcomdiv(0, 5) == 1);
    CHECK(gcomdiv(100, 25) == 25);
}

TEST_CASE("sincD and boxD", "[math2][special]") {
    CHECK(sincD(0.0) == Approx(1.0));
    CHECK(sincD(PI / 2.0) == Approx(2.0 / PI).epsilon(1e-10));
    CHECK(sincD(PI) == Approx(0.0).margin(1e-10));

    CHECK(boxD(0.0) == Approx(1.0));
    CHECK(boxD(0.99) == Approx(1.0));
    CHECK(boxD(-1.0) == Approx(1.0));  // |x|<=1 -> 1
    CHECK(boxD(1.01) == Approx(0.0));
    CHECK(boxD(3.0) == Approx(0.0));
}

TEST_CASE("reflectD folds a value back into the interval", "[math2][interval]") {
    CHECK(reflectD(5.0, 0.0, 10.0) == Approx(5.0));
    CHECK(reflectD(-3.0, 0.0, 10.0) == Approx(6.0));
    CHECK(reflectD(15.0, 0.0, 10.0) == Approx(5.0));
    // Repeated reflection keeps the result inside [lo, hi].
    CHECK(reflectD(-25.0, 0.0, 10.0) >= 0.0);
    CHECK(reflectD(-25.0, 0.0, 10.0) <= 10.0);
}

TEST_CASE("diffgreen2D is symmetric", "[math2][greens]") {
    CHECK(diffgreen2D(1.0, 2.0) == Approx(diffgreen2D(2.0, 1.0)).epsilon(1e-12));
    // r1 == 0: 1/(2*pi) * exp(-r2^2/2)
    CHECK(diffgreen2D(0.0, 1.0) == Approx(0.0965308856131924).epsilon(1e-10));
}

TEST_CASE("linefitD computes a least-squares line", "[math2][regression]") {
    // Data generated from y = 2x + 3.
    double x[4] = {0.0, 1.0, 2.0, 3.0};
    double y[4] = {3.0, 5.0, 7.0, 9.0};
    double m, b;
    linefitD(x, y, 4, &m, &b);
    CHECK(m == Approx(2.0).epsilon(1e-12));
    CHECK(b == Approx(3.0).epsilon(1e-12));
}

TEST_CASE("HillFnD evaluates the Hill activation curve", "[math2][hill]") {
    double hp[4];
    SetHillParamD(hp, /*a=*/1.0, /*e=*/1.0, /*n=*/2.0, /*b=*/0.0);
    CHECK(HillFnD(hp, 0.0) == Approx(0.0));
    CHECK(HillFnD(hp, 1.0) == Approx(0.5));     // half-saturation at x = e
    CHECK(HillFnD(hp, 2.0) == Approx(0.8));     // 1*4/(1+4)
    // Saturation: as x -> infinity the Hill response approaches the amplitude a.
    CHECK(HillFnD(hp, 1e6) == Approx(1.0).epsilon(1e-9));

    SetHillParamD(hp, /*a=*/2.0, /*e=*/1.0, /*n=*/1.0, /*b=*/0.5);
    CHECK(HillFnD(hp, 1.0) == Approx(1.5));     // b + a/2
    CHECK(HillFnD(hp, 0.0) == Approx(0.5));     // offset term b
}
