// Originally by Fränz Friederes, MIT License
// https://github.com/cryptii/cryptii/blob/main/src/Encoder/Ascii85.js

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean/

// Base85 (Ascii85) encoding/decoding

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Maximum value of a tuple (85^5 - 1 does not fit into 32 bits).
#define MAX_TUPLE UINT32_C(0xFFFFFFFF)

uint8_t* base85_encode(const uint8_t* src, size_t len, size_t* out_len) {
    // Each 4-byte group takes at most 5 characters, plus the null terminator.
    uint8_t* encoded = malloc(len * 5 / 4 + 5);
    if (encoded == NULL) {
        *out_len = 0;
        return NULL;
    }

    // Encode each group of 4 bytes
    uint32_t digits[5], tuple;
    size_t pos = 0;
    for (size_t i = 0; i < len; i += 4) {
        // The last group may be partial
        size_t group_len = (len - i < 4) ? len - i : 4;

        // Read 32-bit unsigned integer from bytes following the
        // big-endian convention (most significant byte first),
        // padding a partial group with zero bytes
        tuple = 0;
        for (size_t j = 0; j < group_len; j++) {
            tuple |= (uint32_t)src[i + j] << (24 - 8 * j);
        }

        if (tuple == 0 && group_len == 4) {
            // An all-zero tuple is encoded as a single character.
            // Only full groups qualify: for a partial group the shorthand
            // would decode into more bytes than the source has.
            encoded[pos++] = 'z';
            continue;
        }

        // Calculate 5 digits by repeatedly dividing
        // by 85 and taking the remainder
        for (size_t j = 0; j < 5; j++) {
            digits[4 - j] = tuple % 85;
            tuple = tuple / 85;
        }

        // A group of n bytes is encoded as n+1 characters,
        // so omit the characters added due to zero padding
        for (size_t j = 0; j < group_len + 1; j++) {
            encoded[pos++] = digits[j] + 33;
        }
    }

    *out_len = pos;
    encoded[pos] = '\0';
    return encoded;
}

uint8_t* base85_decode(const uint8_t* src, size_t len, size_t* out_len) {
    // Every 'z' expands into 4 bytes, while the other characters decode
    // in groups of 5 to 4 bytes. Since 'z' is only valid at a group
    // boundary (it is rejected as a digit otherwise), the remaining
    // characters form full groups plus at most one trailing group of
    // n characters, which decodes into n-1 bytes. Both cases are covered
    // by rounding down: (5k + n) * 4 / 5 = 4k + n - 1 for 2 <= n <= 4.
    size_t n_shorthand = 0;
    for (size_t i = 0; i < len; i++) {
        if (src[i] == 'z') {
            n_shorthand++;
        }
    }
    size_t decoded_len = (len - n_shorthand) * 4 / 5 + n_shorthand * 4;

    uint8_t* decoded = malloc(decoded_len > 0 ? decoded_len : 1);
    if (decoded == NULL) {
        *out_len = 0;
        return NULL;
    }

    uint8_t digits[5], tuple_bytes[4];
    uint64_t tuple;
    size_t pos = 0;
    for (size_t i = 0; i < len;) {
        if (src[i] == 'z') {
            // A single character encodes an all-zero tuple
            decoded[pos++] = 0;
            decoded[pos++] = 0;
            decoded[pos++] = 0;
            decoded[pos++] = 0;
            i++;
            continue;
        }

        // The last group may be partial, but a single character
        // encodes no bytes at all, so it is invalid
        size_t group_len = (len - i < 5) ? len - i : 5;
        if (group_len == 1) {
            free(decoded);
            *out_len = 0;
            return NULL;
        }

        // Retrieve radix-85 digits of tuple,
        // padding a partial group with the largest digit ('u')
        for (size_t k = 0; k < 5; k++) {
            if (k >= group_len) {
                digits[k] = 84;
                continue;
            }
            uint8_t digit = src[i + k] - 33;
            if (digit > 84) {
                free(decoded);
                *out_len = 0;
                return NULL;
            }
            digits[k] = digit;
        }

        // Create 32-bit binary number from digits and handle padding
        // tuple = a * 85^4 + b * 85^3 + c * 85^2 + d * 85 + e
        // (calculated as 64-bit, since the digits may overflow 32 bits)
        tuple = (uint64_t)digits[0] * 52200625 + (uint64_t)digits[1] * 614125 +
                (uint64_t)digits[2] * 7225 + (uint64_t)digits[3] * 85 + digits[4];
        if (tuple > MAX_TUPLE) {
            free(decoded);
            *out_len = 0;
            return NULL;
        }

        // Get bytes from tuple
        tuple_bytes[0] = (tuple >> 24) & 0xff;
        tuple_bytes[1] = (tuple >> 16) & 0xff;
        tuple_bytes[2] = (tuple >> 8) & 0xff;
        tuple_bytes[3] = tuple & 0xff;

        // Append bytes to result, dropping the ones
        // that came from the padding digits
        for (size_t k = 0; k + 1 < group_len; k++) {
            decoded[pos++] = tuple_bytes[k];
        }
        i += 5;
    }

    *out_len = pos;
    return decoded;
}
