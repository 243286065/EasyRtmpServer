#pragma once

#include "config.h"

#include <string.h>
#include <stdio.h>
#include <string>


// struct Handshake {
// 	uint8_t flags[8];
// 	uint8_t random[RANDOM_LEN];
// } PACKED;

#define DEFAULT_CHUNK_LEN 128

struct RtmpHeader {
	uint8_t header_type;
    char timestamp[3];
    char amf_len[3];
    uint8_t amf_type;
	char stream_id[3]; /* Note, this is little-endian while others are BE */
};

struct RtmpMessage {
	uint8_t type;
	size_t len;
	unsigned long timestamp;
	uint32_t endpoint;
	std::string buf;
};

// 三次握手
// c0c1------->>
// <<-----------s0s1s2
// c2--------->>
#define HANDSHAKE_PLAINTEXT 0x03
struct RtmpHandSharkServer {
    uint8_t c0;
    char c1[1536];
    char c2[1536];
};

struct RtmpHandSharkClient {
    uint8_t s0;
    char s1[1536];
    char s2[1536];
};

struct Client {
    int fd;
    bool pullStream;    //是否拉流
    bool needKeyFrame;  //是否需要关键帧
    RtmpMessage messages[64];
    std::string buf;
    std::string send_queue;
    size_t chunk_len;
    uint32_t written_seq;
    uint32_t read_seq;
};

enum RtmpMessageType{
    MSG_SET_CHUNK_SIZE=0x01,
    MSG_ABORT,
    MSG_BYTES_READ,
    MSG_USER_CONTROL,
    MSG_WINDOWS_SIZE,
    MSG_SET_PEER_BAND_WIDTH,
    //Audio or video message
    MSG_AUDIO = 0x08,
    MSG_VIDEO,
    //Obeject message
    MSG_INVOKE3 = 0x11, /* AMF3 */
    MSG_NOTIFY,
    MSG_OBJECT,
    MSG_INVOKE,     /* AMF0 */
    MSG_FLASH_VIDEO = 0x16,
};