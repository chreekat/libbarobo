#include "net_byte_order.h"

uint16_t netByteOrder16 (uint16_t value) {
    int i;
    uint16_t result;
    uint8_t *presult = (uint8_t *)&result;
    for (i = sizeof(value) - 1; i >= 0; --i) {
        presult[i] = value & 0xff;
        value >>= 8;
    }
    return result;
}

uint32_t netByteOrder32 (uint32_t value) {
    int i;
    uint32_t result;
    uint8_t *presult = (uint8_t *)&result;
    for (i = sizeof(value) - 1; i >= 0; --i) {
        presult[i] = value & 0xff;
        value >>= 8;
    }
    return result;
}

uint64_t netByteOrder64 (uint64_t value) {
    int i;
    uint64_t result;
    uint8_t *presult = (uint8_t *)&result;
    for (i = sizeof(value) - 1; i >= 0; --i) {
        presult[i] = value & 0xff;
        value >>= 8;
    }
    return result;
}
