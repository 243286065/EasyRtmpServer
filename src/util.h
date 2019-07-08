
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

// 读取大端4bytes，转化为uint32_t
extern uint32_t ReadBigEndian32(const void *p);

// 读取小端3bytes，转换为uint32_t
extern uint32_t ReadBigEndian24(const void *p);

// 读取大端2bytes，转换为uint16_t
extern uint16_t ReadBigEndian16(const void *p);

// 读取小端4bytes，转换为uint32_t
extern uint32_t ReadLittleEndian32(const void *p);

// 读取小端3bytes，转换为uint32_t
extern uint32_t ReadLittleEndian24(const void *p);

// 将uint32_t表示成大端24位
extern void SetBigEndian24(void *p, uint32_t val);

// 将uint32_t表示成小端32位
void SetLittleEndian32(void *p, uint32_t val);

//按byte输出
void PrintfAll(void *p, size_t len);