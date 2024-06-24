#pragma once
#include <stdint.h>

void calc_sha_256(uint8_t hash[32], const void *input, size_t len);
