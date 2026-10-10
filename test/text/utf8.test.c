// Copyright (c) 2024 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "text/utf8/utf8.h"

static void test_icmp(void) {
    printf("test_icmp...");
    {
        const char* s1 = "Hello, 世界!";
        const char* s2 = "hello, 世界!";
        assert(utf8_icmp(s1, strlen(s1), s2, strlen(s2)) == 0);
    }
    {
        const char* s1 = "Hello, 世界!";
        const char* s2 = "HELLO, 世界!";
        assert(utf8_icmp(s1, strlen(s1), s2, strlen(s2)) == 0);
    }
    {
        const char* s1 = "Hello, 世界!";
        const char* s2 = "HELLO, 世界";
        assert(utf8_icmp(s1, strlen(s1), s2, strlen(s2)) > 0);
    }
    {
        const char* s1 = "Hello, 世界";
        const char* s2 = "HELLO, 世界!";
        assert(utf8_icmp(s1, strlen(s1), s2, strlen(s2)) < 0);
    }
    // a casefold may change the encoded width: ſ (two bytes) folds to
    // s (one byte), K (U+212A, three bytes) folds to k. the comparison
    // is over decoded folded codepoints, not bytes
    {
        const char* s1 = "ſ";
        const char* s2 = "s";
        assert(utf8_icmp(s1, strlen(s1), s2, strlen(s2)) == 0);
    }
    {
        const char* s1 = "\xe2\x84\xaa";  // K, the kelvin sign
        const char* s2 = "k";
        assert(utf8_icmp(s1, strlen(s1), s2, strlen(s2)) == 0);
    }
    // the order must be total: ſx < sxx < sxy, transitively.
    // comparing byte lengths made ſx "equal" to both sxx and sxy
    {
        const char* a = "ſx";
        const char* b = "sxx";
        const char* c = "sxy";
        assert(utf8_icmp(a, strlen(a), b, strlen(b)) < 0);
        assert(utf8_icmp(b, strlen(b), c, strlen(c)) < 0);
        assert(utf8_icmp(a, strlen(a), c, strlen(c)) < 0);
    }
    // a string that folds to a prefix of the other is the smaller one
    {
        const char* s1 = "s";
        const char* s2 = "ſx";
        assert(utf8_icmp(s1, strlen(s1), s2, strlen(s2)) < 0);
        assert(utf8_icmp(s2, strlen(s2), s1, strlen(s1)) > 0);
    }
    printf("OK\n");
}

// utf8_next must consume every byte exactly once and decode each maximal
// ill-formed subpart as U+FFFD, never reading past the given length.
static void test_next(void) {
    printf("test_next...");
    static const struct {
        const char* input;
        const uint32_t runes[4];
        size_t count;
    } tests[] = {
        {"a\xd0\xb1" "c", {'a', 0x0431, 'c'}, 3},  // valid
        {"\xc3", {0xFFFD}, 1},                     // truncated at the end
        {"a\xe4\xb8" "b", {'a', 0xFFFD, 'b'}, 3},  // truncated in the middle
        {"\x80\x80", {0xFFFD, 0xFFFD}, 2},         // stray continuation bytes
        {"\xc3" "a", {0xFFFD, 'a'}, 2},            // the breaking byte survives
    };
    for (size_t t = 0; t < sizeof(tests) / sizeof(*tests); t++) {
        size_t n = strlen(tests[t].input);
        // exact-sized allocation, unterminated: utf8_next takes an explicit
        // length, so a sanitizer build catches it touching s[n]
        char* s = malloc(n);
        assert(s != NULL);
        memcpy(s, tests[t].input, n);
        size_t i = 0, count = 0;
        while (i < n) {
            size_t prev = i;
            uint32_t c = utf8_next(s, n, &i);
            assert(i > prev && i <= n);
            assert(c == tests[t].runes[count]);
            count++;
        }
        assert(count == tests[t].count);
        free(s);
    }
    printf("OK\n");
}

// utf8_icmp must stay within the given lengths even when the strings are not
// valid utf8. Exact-sized unterminated allocations let a sanitizer build
// catch any overrun.
static void test_icmp_invalid(void) {
    printf("test_icmp_invalid...");
    static const char* inputs[] = {"\x80", "\xc3", "a\xe4\xb8", "\xff\xff", "A\xffZ"};
    for (size_t t = 0; t < sizeof(inputs) / sizeof(*inputs); t++) {
        size_t n = strlen(inputs[t]);
        char* s = malloc(n);
        assert(s != NULL);
        memcpy(s, inputs[t], n);
        assert(utf8_icmp(s, n, s, n) == 0);
        assert(utf8_icmp(s, n, "b", 1) != 0);
        free(s);
    }
    // invalid sequences compare equal case-insensitively around them
    assert(utf8_icmp("A\xff" "B", 3, "a\xff" "b", 3) == 0);
    // both decode to a single U+FFFD, so they compare equal even though
    // their byte lengths differ
    assert(utf8_icmp("\xe4\xb8", 2, "\xff", 1) == 0);
    printf("OK\n");
}

// check_case converts src using fn and asserts the result equals want.
static void check_case(bool (*fn)(const char*, size_t, char*, size_t, size_t*),
                       const char* src,
                       const char* want) {
    char dst[256];
    size_t n = strlen(src);
    assert(n * 2 + 1 <= sizeof(dst));
    size_t len = 0;
    assert(fn(src, n, dst, sizeof(dst), &len));
    assert(len == strlen(want));
    assert(strcmp(dst, want) == 0);
}

static void test_tolower(void) {
    printf("test_tolower...");
    check_case(utf8_tolower, "Hello, WORLD!", "hello, world!");
    check_case(utf8_tolower, "Hello, 世界!", "hello, 世界!");
    check_case(utf8_tolower, "CÓMO ESTÁS", "cómo estás");
    check_case(utf8_tolower, "Привет, МИР!", "привет, мир!");
    // conversions that change the utf8 width of a character
    check_case(utf8_tolower, "İ", "i");
    check_case(utf8_tolower, "İstanbul", "istanbul");
    check_case(utf8_tolower, "K", "k");
    check_case(utf8_tolower, "Ⱥbcd", "ⱥbcd");
    printf("OK\n");
}

static void test_toupper(void) {
    printf("test_toupper...");
    check_case(utf8_toupper, "Hello, world!", "HELLO, WORLD!");
    check_case(utf8_toupper, "Hello, 世界!", "HELLO, 世界!");
    check_case(utf8_toupper, "cómo estás", "CÓMO ESTÁS");
    check_case(utf8_toupper, "Привет, мир!", "ПРИВЕТ, МИР!");
    // conversions that change the utf8 width of a character
    check_case(utf8_toupper, "ß", "ẞ");
    check_case(utf8_toupper, "ſ", "S");
    check_case(utf8_toupper, "ⱥbcd", "ȺBCD");
    printf("OK\n");
}

static void test_totitle(void) {
    printf("test_totitle...");
    check_case(utf8_totitle, "hello, world!", "Hello, World!");
    check_case(utf8_totitle, "hello, 世界!", "Hello, 世界!");
    check_case(utf8_totitle, "cómo estás", "Cómo Estás");
    check_case(utf8_totitle, "привет, мир!", "Привет, Мир!");
    // conversions that change the utf8 width of a character
    check_case(utf8_totitle, "aK", "Ak");
    check_case(utf8_totitle, "straße", "Straße");
    check_case(utf8_totitle, "ßx", "ẞx");
    printf("OK\n");
}

static void test_case_embedded_zero(void) {
    printf("test_case_embedded_zero...");
    // sqlite text may contain zero bytes; they are part of the value
    char dst[64];
    size_t len = 0;
    assert(utf8_tolower("A\0BC", 4, dst, sizeof(dst), &len));
    assert(len == 4);
    assert(memcmp(dst, "a\0bc", 4) == 0);
    assert(utf8_toupper("a\0bc", 4, dst, sizeof(dst), &len));
    assert(len == 4);
    assert(memcmp(dst, "A\0BC", 4) == 0);
    printf("OK\n");
}

static void test_casefold(void) {
    printf("test_casefold...");
    check_case(utf8_casefold, "Hello, WORLD!", "hello, world!");
    check_case(utf8_casefold, "Hello, 世界!", "hello, 世界!");
    check_case(utf8_casefold, "CÓMO ESTÁS", "cómo estás");
    check_case(utf8_casefold, "Привет, МИР!", "привет, мир!");
    // conversions that change the utf8 width of a character
    check_case(utf8_casefold, "ẞ", "ß");
    check_case(utf8_casefold, "K", "k");
    check_case(utf8_casefold, "Ⱥbcd", "ⱥbcd");
    printf("OK\n");
}

static void test_case_invalid(void) {
    printf("test_case_invalid...");
    char dst[64];
    size_t len = 0;
    // a stray continuation byte and a truncated sequence are both rejected
    // rather than sending the decoder past the end of the input
    assert(utf8_tolower("\x80", 1, dst, sizeof(dst), &len) == false);
    assert(utf8_toupper("\xc3", 1, dst, sizeof(dst), &len) == false);
    assert(utf8_totitle("a\xe4\xb8", 3, dst, sizeof(dst), &len) == false);
    assert(utf8_casefold("\xff", 1, dst, sizeof(dst), &len) == false);
    // a result that does not fit is rejected too
    assert(utf8_tolower("abc", 3, dst, 2, &len) == false);
    printf("OK\n");
}

int main(void) {
    test_icmp();
    test_icmp_invalid();
    test_next();
    test_tolower();
    test_toupper();
    test_totitle();
    test_casefold();
    test_case_invalid();
    test_case_embedded_zero();
}
