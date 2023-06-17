#include "cal_sha1.h"
#include "sha1.h"

void calc_sha1(unsigned char hash[20], const void *input, size_t len)
{
    CSHA1 sha1;
    sha1.Update((const uint8_t*)input, static_cast<unsigned int>(len));
    sha1.Final();
    sha1.GetHash(hash);
}
