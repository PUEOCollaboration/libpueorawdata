// 16-bit CRC implementation (CRC-CCIT from Linux Kernel)


#ifndef _PUEO_CRC_
#define _PUEO_CRC_

#include <stdint.h>
#include <stddef.h>

#define CRC16_START 0xffff

uint16_t pueo_crc16_continue(uint16_t start, const void * buf, size_t len);

#define pueo_crc16(buf, len) pueo_crc16_continue(CRC16_START, buf, len)

#endif
