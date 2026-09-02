// Unit tests for libSteve string2 parsing routines. These handle number
// detection, word splitting, sequential string->number reads and math
// expression evaluation used throughout the Smoldyn command parser and
// simulation set-up.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cstring>

extern "C" {
#include "string2.h"
}

using Catch::Approx;

TEST_CASE("strisnumber distinguishes full numeric strings", "[string2][parse]") {
    CHECK(strisnumber("3.14") == 1);
    CHECK(strisnumber("-2.5") == 1);
    CHECK(strisnumber("1e5") == 1);
    CHECK(strisnumber("0") == 1);
    CHECK(strisnumber(".5") == 1);

    CHECK(strisnumber("") == 0);
    CHECK(strisnumber("3.14x") == 0);
    CHECK(strisnumber("abc") == 0);
    CHECK(strisnumber("1 2") == 0);
    CHECK(strisnumber("NaN") == 0);
}

TEST_CASE("wordcount counts whitespace separated tokens", "[string2][word]") {
    CHECK(wordcount("a b c") == 3);
    CHECK(wordcount("  a   b  ") == 2);
    CHECK(wordcount("") == 0);
    CHECK(wordcount("   ") == 0);
    CHECK(wordcount("single") == 1);
    CHECK(wordcount("one\ttwo\nthree") == 3);
}

TEST_CASE("strnwordc extracts the nth word", "[string2][word]") {
    const char* s = "alpha beta gamma";
    CHECK(std::strcmp(strnwordc(s, 1), "alpha") == 0);
    CHECK(std::strcmp(strnwordc(s, 2), "beta") == 0);
    CHECK(std::strcmp(strnwordc(s, 3), "gamma") == 0);
    const char* fourth = strnwordc(s, 4);
    const bool fourthIsEmpty = (fourth == nullptr) || (*fourth == '\0');
    CHECK(fourthIsEmpty);   // out-of-range word is empty
}

TEST_CASE("strbegin matches prefixes", "[string2][parse]") {
    CHECK(strbegin("abc", "abcdef", 1) == 1);      // case sensitive
    CHECK(strbegin("abc", "ABCdef", 0) == 1);      // case insensitive
    CHECK(strbegin("abc", "ab", 1) == 0);          // short is not a prefix of long target
    CHECK(strbegin("", "abc", 1) == 0);            // empty short string
    CHECK(strbegin("xyz", "abcdef", 1) == 0);
}

TEST_CASE("strreadnd parses doubles from a string", "[string2][read]") {
    char buf[] = "1.5  2.5\t3.5 tail";
    double a[3];
    char* end = nullptr;
    int n = strreadnd(buf, 3, a, &end);
    CHECK(n == 3);
    CHECK(a[0] == Approx(1.5));
    CHECK(a[1] == Approx(2.5));
    CHECK(a[2] == Approx(3.5));
    CHECK(std::strncmp(end, "tail", 4) == 0);
}

TEST_CASE("strreadnd stops early on a bad token", "[string2][read]") {
    char buf[] = "1.5 notanumber";
    double a[2];
    int n = strreadnd(buf, 2, a, nullptr);
    CHECK(n == 1);   // only the first value is consumable
    CHECK(a[0] == Approx(1.5));
}

TEST_CASE("strreadnf parses floats", "[string2][read]") {
    char buf[] = "0.25 0.5";
    float a[2];
    int n = strreadnf(buf, 2, a, nullptr);
    CHECK(n == 2);
    CHECK(a[0] == Approx(0.25f));
    CHECK(a[1] == Approx(0.5f));
}

TEST_CASE("strreadni parses integers", "[string2][read]") {
    char buf[] = "10 -20 30";
    int a[3];
    int n = strreadni(buf, 3, a, nullptr);
    CHECK(n == 3);
    CHECK(a[0] == 10);
    CHECK(a[1] == -20);
    CHECK(a[2] == 30);
}

TEST_CASE("strmatheval evaluates expressions with variables", "[string2][math]") {
    double x = 1.0;
    char xname[] = "x";
    char* vars[] = {xname};
    CHECK(strmatheval("x^2+1.0", vars, &x, 1) == Approx(2.0).epsilon(1e-12));
    CHECK(strmatheval("2+3*4", nullptr, nullptr, 0) == Approx(14.0).epsilon(1e-12));
    CHECK(strmatheval("10/4", nullptr, nullptr, 0) == Approx(2.5).epsilon(1e-12));
    CHECK(strmatheval("sin(0)", nullptr, nullptr, 0) == Approx(0.0).margin(1e-12));
    CHECK(strmatheval("2^10", nullptr, nullptr, 0) == Approx(1024.0).epsilon(1e-12));
}

TEST_CASE("strmatheval reports errors on invalid syntax", "[string2][math]") {
    // A malformed expression must raise the module's math-error flag.
    strmatherror(nullptr, 1);                       // clear any prior error
    strmatheval("2+(3", nullptr, nullptr, 0);
    CHECK(strmatherror(nullptr, 0) != 0);           // error is latched
    strmatherror(nullptr, 1);                       // clear it
    CHECK(strmatherror(nullptr, 0) == 0);
}
