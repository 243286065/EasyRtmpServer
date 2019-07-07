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