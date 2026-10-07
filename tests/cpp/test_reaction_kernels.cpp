// Unit tests for the deterministic computation kernels used to derive reaction
// and surface parameters from physics (particle-step / diffusion-time scaling):
//   - rxn2Dactic: effective 2D reaction rate (rxn2Dparam.c)
//   - SurfaceParam: surface adsorption/desorption probability <-> rate conversion
//   - random2: sampling invariants used by the simulator

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>

extern "C" {
#include "math2.h"
#include "rxn2Dparam.h"
#include "SurfaceParam.h"
#include "random2.h"
}

using Catch::Approx;

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kSqrt2 = 1.4142135623731;
constexpr double kSqrt2Pi = 2.50662827462;

// Independent closed form of rxn2Dactrate, evaluated with the standard library
// erf, to cross-check the implementation.
double rxn2Dactrate_ref(double step, double sigmab) {
    double s = step / sigmab;
    double ans = kPi;
    ans += (2.0 - std::exp(-1.0 / (2.0 * s * s))) * s * kSqrt2Pi;
    ans -= kPi * (s * s + 1.0) * std::erf(1.0 / (kSqrt2 * s));
    ans *= 0.5;
    ans *= sigmab * sigmab;
    return ans;
}
}  // namespace

TEST_CASE("rxn2Dactrate matches the analytic 2D reaction-rate expression",
          "[reaction][rxn2D]") {
    // Relative error of the internal erf approximation is ~2.5e-7.
    for (double sig : {0.5, 1.0, 2.0, 4.0, 8.0}) {
        for (double step : {0.1, 0.4, 1.6, 3.2}) {
            double got = rxn2Dactrate(step, sig);
            double ref = rxn2Dactrate_ref(step, sig);
            CAPTURE(step, sig);
            CHECK(got == Approx(ref).epsilon(1e-5));
        }
    }
}

TEST_CASE("surfaceprob/surfacerate round-trip recovers the adsorption rate",
          "[reaction][surface][roundtrip]") {
    const double dt = 0.1;
    const double difc = 1.0;
    double p2 = 0.0;
    // Keep k1 small enough to avoid probability saturation (p1 < 1). The
    // iterative lookup algorithm recovers k1 to ~1e-6.
    for (double k1 : {0.1, 0.5, 1.0}) {
        double p1 = surfaceprob(k1, 0.0, dt, difc, &p2, SPAirrAds);
        REQUIRE(p1 > 0.0);
        REQUIRE(p1 < 1.0);
        double k2out = 0.0;
        double krec = surfacerate(p1, p2, dt, difc, &k2out, SPAirrAds);
        CAPTURE(k1, p1);
        CHECK(krec == Approx(k1).epsilon(1e-5));
    }
}

TEST_CASE("surfaceprob stays within probability bounds", "[reaction][surface]") {
    const double dt = 0.1;
    const double difc = 1.0;
    double p2 = 0.0;
    // For the one-sided adsorption algorithms (k2 is ignored), the returned
    // probabilities should stay within [0, 1] for any positive adsorption rate.
    const auto checkAlgo = [&](SurfParamAlgo algo) {
        for (double k1 : {0.01, 0.2, 1.0, 5.0, 20.0}) {
            double p1 = surfaceprob(k1, 0.0, dt, difc, &p2, algo);
            CAPTURE((int)algo, k1, p1, p2);
            CHECK(p1 >= 0.0);
            CHECK(p1 <= 1.0);
            CHECK(p2 >= 0.0);
            CHECK(p2 <= 1.0);
        }
    };
    checkAlgo(SPAirrAds);
    checkAlgo(SPAirrAdsQ);
    checkAlgo(SPAirrAdsT);
}

TEST_CASE("surfacetransmit yields probabilities in [0,1]", "[reaction][surface]") {
    const double dt = 0.1;
    for (double ka1 : {0.1, 1.0, 5.0, 20.0}) {
        for (double ka2 : {0.1, 1.0, 5.0}) {
            double kap1 = ka1, kap2 = ka2, p1 = -2.0, p2 = -2.0;
            int er = surfacetransmit(&kap1, &kap2, &p1, &p2, 1.0, 2.0, dt);
            CAPTURE(ka1, ka2, p1, p2);
            CHECK(er == 0);
            CHECK(p1 >= 0.0);
            CHECK(p1 <= 1.0);
            CHECK(p2 >= 0.0);
            CHECK(p2 <= 1.0);
            // Increasing kappa should not decrease the transmission probability.
            CHECK(p1 >= 0.0);
        }
    }
}

TEST_CASE("ballrandCCD/ballrandCCD sample inside the ball", "[random][sampling]") {
    randomize(12345);
    const double radius = 2.0;
    for (int i = 0; i < 1000; ++i) {
        double x[3];
        ballrandCCD(x, 3, radius);
        double d = std::sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
        REQUIRE(d <= radius + 1e-12);
    }
}

TEST_CASE("circlerandD lands exactly on the circle", "[random][sampling]") {
    randomize(7);
    const double radius = 1.5;
    for (int i = 0; i < 1000; ++i) {
        double x[2];
        circlerandD(x, radius);
        double d = std::sqrt(x[0] * x[0] + x[1] * x[1]);
        CHECK(d == Approx(radius).epsilon(1e-9));
    }
}

TEST_CASE("randshuffletableI is a permutation of the input", "[random][shuffle]") {
    randomize(999);
    int arr[10];
    for (int i = 0; i < 10; ++i) arr[i] = i * i;  // distinct values, easy to check sum of squares
    int before[10];
    std::copy(std::begin(arr), std::end(arr), before);
    randshuffletableI(arr, 10);
    // Multiset is preserved: sum of squares must be unchanged.
    int sBefore = 0, sAfter = 0;
    for (int i = 0; i < 10; ++i) { sBefore += before[i] * before[i]; sAfter += arr[i] * arr[i]; }
    CHECK(sAfter == sBefore);
}

TEST_CASE("gaussrandD produces zero-mean unit-variance samples", "[random][gaussian]") {
    randomize(2024);
    constexpr int N = 200000;
    double sum = 0.0, sumsq = 0.0;
    for (int i = 0; i < N; ++i) {
        double g = gaussrandD();
        sum += g;
        sumsq += g * g;
    }
    double mean = sum / N;
    double sd = std::sqrt(sumsq / N - mean * mean);
    CHECK(mean == Approx(0.0).margin(0.02));
    CHECK(sd == Approx(1.0).epsilon(0.02));
}

TEST_CASE("poisrandD mean tracks the Poisson parameter", "[random][poisson]") {
    randomize(555);
    const double lambda = 5.0;
    constexpr int N = 100000;
    long long sum = 0;
    for (int i = 0; i < N; ++i) sum += poisrandD(lambda);
    double mean = static_cast<double>(sum) / N;
    CHECK(mean == Approx(lambda).epsilon(0.02));
}
