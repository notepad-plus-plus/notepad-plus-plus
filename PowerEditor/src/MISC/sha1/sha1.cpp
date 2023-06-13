#include <cstdint>
#include <cstring>
#include <iostream>

uint32_t leftrotate(uint32_t x, uint32_t c) {
    return (x << c) | (x >> (32-c));
}

uint32_t from_bytes(const unsigned char * p) {
    return (((uint32_t)p[3]) << 24)
         | (((uint32_t)p[2]) << 16)
         | (((uint32_t)p[1]) << 8)
         | ((uint32_t)p[0]);
}

void to_bytes(uint32_t val, unsigned char * p) {
    p[3] = unsigned char(val >> 24);
    p[2] = unsigned char(val >> 16);
    p[1] = unsigned char(val >> 8);
    p[0] = unsigned char(val >> 0);
}

void calc_sha1(uint8_t hash[20], const void *input, size_t len) {
    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xC3D2E1F0;

    unsigned char *msg = NULL;

    size_t new_len, offset;
    uint32_t w[80];
    uint32_t a, b, c, d, e, f, k, temp;

    new_len = len + 1;
    while (new_len % 64 != 56) {
        new_len++;
    }

    msg = (unsigned char*)malloc(new_len + 8);
    memcpy(msg, input, len);
    msg[len] = 0x80;
    for (offset = len + 1; offset < new_len; offset++) {
        msg[offset] = 0;
    }

    to_bytes(uint32_t(len * 8), msg + new_len);
    new_len += 8;

    for(offset=0; offset<new_len; offset += (512/8)) {
        for (unsigned int i = 0; i < 16; i++) {
            w[i] = from_bytes(msg + offset + i*4);
        }
        for (unsigned int i = 16; i < 80; i++) {
            w[i] = leftrotate(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
        }

        a = h0;
        b = h1;
        c = h2;
        d = h3;
        e = h4;

        for (unsigned int i = 0; i < 80; i++) {
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }

            temp = leftrotate(a,5) + f + e + k + w[i];
            e = d;
            d = c;
            c = leftrotate(b,30);
            b = a;
            a = temp;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    free(msg);

    to_bytes(h0, hash);
    to_bytes(h1, hash + 4);
    to_bytes(h2, hash + 8);
    to_bytes(h3, hash +12);
    to_bytes(h4, hash +16);
}


