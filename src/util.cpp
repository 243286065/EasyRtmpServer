#include "util.h"

uint32_t ReadBigEndian32(const void *p)
{
	uint32_t val;
	memcpy(&val, p, sizeof val);
	return ntohl(val);
}

uint32_t ReadBigEndian24(const void *p) {
	const uint8_t *data = (const uint8_t *) p;
	return data[2] | ((uint32_t) data[1] << 8) | ((uint32_t) data[0] << 16);
}


uint16_t ReadBigEndian16(const void *p)
{
	uint16_t val;
	memcpy(&val, p, sizeof val);
	return ntohs(val);
}

uint32_t ReadLittleEndian32(const void *p)
{
	const uint8_t *data = (const uint8_t *) p;
	return data[0] | ((uint32_t) data[1] << 8) |
		((uint32_t) data[2] << 16) | ((uint32_t) data[3] << 24);
}

uint32_t ReadLittleEndian24(const void *p) {
    const uint8_t *data = (const uint8_t *) p;
	return data[0] | ((uint32_t) data[1] << 8) | ((uint32_t) data[2] << 16);
}

void SetBigEndian24(void *p, uint32_t val)
{
	uint8_t *data = (uint8_t *) p;
	data[0] = val >> 16;
	data[1] = val >> 8;
	data[2] = val;
}

void SetLittleEndian32(void *p, uint32_t val)
{
	uint8_t *data = (uint8_t *) p;
	data[0] = val;
	data[1] = val >> 8;
	data[2] = val >> 16;
	data[3] = val >> 24;
}

void PrintfAll(void *p, size_t len) {
	char* pos = (char*)p;
	for(int i=0; i< len; i++) {
		printf("%02x", pos[i]);
	}
	printf("\n");
}