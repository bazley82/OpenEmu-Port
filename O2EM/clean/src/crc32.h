#include <stdint.h>
#ifndef __CRC32_H
#define __CRC32_H

uint32_t crc32_buf(const void *buf, size_t len);
uint32_t crc32_file(const char *filename);

#endif
