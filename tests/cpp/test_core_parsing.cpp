// Unit tests for self-contained Smoldyn core parsing helpers: enum <-> string
// conversions used when reading simulation commands. Linked against the real
// smoldyn_static library so this exercises production Smoldyn code.

#include <cstring>

extern "C" {
#include "smoldynfuncs.h"
}

// smoldynfuncs.h defines its own CHECK macro (used for its internal error
// handling) which collides with Catch2's assertion macro. Undefine the whole
// family before the Catch2 headers are brought in.
#undef CHECK
#undef CHECKMEM
#undef CHECKM
#undef CHECKBUG
#undef CHECKS

#include <catch2/catch_test_macros.hpp>

TEST_CASE("rxnstring2rp maps short and long reversible-parameter strings", "[core][parse][rxn]") {
    CHECK(rxnstring2rp("i") == RPirrev);
    CHECK(rxnstring2rp("irrev") == RPirrev);
    CHECK(rxnstring2rp("a") == RPconfspread);
    CHECK(rxnstring2rp("confspread") == RPconfspread);
    CHECK(rxnstring2rp("p") == RPpgem);
    CHECK(rxnstring2rp("pgem") == RPpgem);
    CHECK(rxnstring2rp("ratio") == RPratio);
    CHECK(rxnstring2rp("s") == RPratio2);
    CHECK(rxnstring2rp("none") == RPnone);      // unknown token
    CHECK(rxnstring2rp("bogus") == RPnone);
}

TEST_CASE("rxnrp2string reverses rxnstring2rp", "[core][parse][rxn]") {
    char buf[STRCHARLONG];
    CHECK(std::strcmp(rxnrp2string(RPirrev, buf), "irrev") == 0);
    CHECK(std::strcmp(rxnrp2string(RPpgem, buf), "pgem") == 0);
    CHECK(std::strcmp(rxnrp2string(RPbounce, buf), "bounce") == 0);
    CHECK(std::strcmp(rxnrp2string(RPnone, buf), "none") == 0);   // none / unmapped
}

TEST_CASE("molstring2ms and molms2string translate molecule states", "[core][parse][mol]") {
    char buf[STRCHARLONG];
    char solution[] = "solution";
    char soln[] = "soln";
    char front[] = "front";
    char bsoln[] = "bsoln";
    char free[] = "free";

    CHECK(molstring2ms(solution) == MSsoln);
    CHECK(molstring2ms(soln) == MSsoln);
    CHECK(molstring2ms(front) == MSfront);
    CHECK(molstring2ms(bsoln) == MSbsoln);
    CHECK(molstring2ms(free) == MSnone);        // unrecognised
    CHECK(std::strcmp(molms2string(MSsoln, buf), "solution") == 0);
    CHECK(std::strcmp(molms2string(MSfront, buf), "front") == 0);
    CHECK(std::strcmp(molms2string(MSsome, buf), "some") == 0);
    CHECK(std::strcmp(molms2string(MSnone, buf), "none") == 0);
}

TEST_CASE("rxnstring2sr parses species representations", "[core][parse][rxn]") {
    CHECK(rxnstring2sr("particle") == SRparticle);
    CHECK(rxnstring2sr("lattice") == SRlattice);
    CHECK(rxnstring2sr("both") == SRboth);
    CHECK(rxnstring2sr("all") == SRboth);       // alias
    CHECK(rxnstring2sr("petri") == SRnone);
}

TEST_CASE("simversionnumber returns a parseable version", "[core][version]") {
    double v = simversionnumber();
    CHECK(v >= 2.0);
    CHECK(v == v);  // not NaN
}

TEST_CASE("molstring2serno / molserno2string round-trip", "[core][parse][serno]") {
    char buf[STRCHARLONG];
    // Small serial numbers are printed as a single integer.
    for (unsigned long long s : {0ULL, 1ULL, 123456ULL, 0xFFFFFFFEULL}) {
        molserno2string(s, buf);
        CHECK(molstring2serno(buf) == s);
    }
    // Large serial numbers use the "hi.lo" encoding; round-trip only works when
    // both the high and low parts are non-zero.
    for (unsigned long long s : {0x100000001ULL, 0x123456789ABCDEF0ULL}) {
        molserno2string(s, buf);
        CAPTURE(s, buf);
        CHECK(molstring2serno(buf) == s);
    }
}
