#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <windows.h>
#include "miniaudio.h"

#pragma pack(push, 1)
typedef struct {
    char magic[4];
    float bpm, swing;
    int barCount, trackCount, sampleCount, clipCount, isLofi, quantizeEnabled;
} CSQHeader;

typedef struct {
    int trackIndex, isMuted;
    float volume, eqLow, eqMid, eqHigh, eqFreq[3], eqQ[3];
} CSQTrack;

typedef struct {
    char name[64];
    ma_uint64 frameCount;
    DWORD rawBytes, compBytes;
} CSQSampleHeader;
#pragma pack(pop)

/* Hash-accelerated LZ compression */
static inline unsigned char* csq_compress_lz(const unsigned char *src, size_t srcSize, size_t *outSize) {
    if (!src || srcSize == 0) return NULL;
    size_t maxDst = srcSize + (srcSize / 8) + 256;
    unsigned char *dst = (unsigned char*)malloc(maxDst);
    if (!dst) return NULL;

    #define CSQ_HASH_SIZE 8192
    int32_t head[CSQ_HASH_SIZE];
    memset(head, -1, sizeof(head));

    size_t inPos = 0, outPos = 0;
    size_t litStart = 0;

    while (inPos < srcSize) {
        if (outPos + 128 > maxDst) {
            maxDst *= 2;
            unsigned char *nDst = (unsigned char*)realloc(dst, maxDst);
            if (!nDst) { free(dst); return NULL; }
            dst = nDst;
        }

        size_t bestLen = 0;
        size_t bestOff = 0;

        if (inPos + 3 <= srcSize) {
            uint32_t h = ((src[inPos] << 10) ^ (src[inPos + 1] << 5) ^ src[inPos + 2]) & (CSQ_HASH_SIZE - 1);
            int32_t matchPos = head[h];
            head[h] = (int32_t)inPos;

            if (matchPos >= 0 && (inPos - (size_t)matchPos) <= 4095) {
                size_t off = inPos - (size_t)matchPos;
                size_t maxMatch = srcSize - inPos;
                if (maxMatch > 127) maxMatch = 127;

                size_t l = 0;
                while (l < maxMatch && src[matchPos + l] == src[inPos + l]) l++;

                if (l >= 3) {
                    bestLen = l;
                    bestOff = off;
                }
            }
        }

        if (bestLen >= 3) {
            // Flush preceding literals
            while (litStart < inPos) {
                size_t chunk = inPos - litStart;
                if (chunk > 127) chunk = 127;
                dst[outPos++] = (unsigned char)chunk;
                memcpy(&dst[outPos], &src[litStart], chunk);
                outPos += chunk;
                litStart += chunk;
            }

            dst[outPos++] = 0x80 | (unsigned char)(bestLen & 0x7F);
            dst[outPos++] = (unsigned char)(bestOff & 0xFF);
            dst[outPos++] = (unsigned char)((bestOff >> 8) & 0x0F);
            inPos += bestLen;
            litStart = inPos;
        } else {
            inPos++;
            if (inPos - litStart >= 127) {
                size_t chunk = inPos - litStart;
                dst[outPos++] = (unsigned char)chunk;
                memcpy(&dst[outPos], &src[litStart], chunk);
                outPos += chunk;
                litStart = inPos;
            }
        }
    }

    // Flush any remaining literals
    while (litStart < inPos) {
        size_t chunk = inPos - litStart;
        if (chunk > 127) chunk = 127;
        dst[outPos++] = (unsigned char)chunk;
        memcpy(&dst[outPos], &src[litStart], chunk);
        outPos += chunk;
        litStart += chunk;
    }

    #undef CSQ_HASH_SIZE
    *outSize = outPos;
    return dst;
}

/* Memory-safe decompression strictly bounded to origSize allocations */
static inline bool csq_decompress_lz(const unsigned char *src, size_t srcSize, unsigned char *dst, size_t origSize) {
    if (!src || !dst || srcSize == 0 || origSize == 0) return false;
    size_t inPos = 0, outPos = 0;
    while (inPos < srcSize && outPos < origSize) {
        unsigned char tag = src[inPos++];
        if (tag & 0x80) {
            size_t len = (size_t)(tag & 0x7F);
            if (inPos + 1 >= srcSize) return false;
            size_t off = (size_t)src[inPos] | ((size_t)(src[inPos + 1] & 0x0F) << 8);
            inPos += 2;
            if (off == 0 || off > outPos || outPos + len > origSize) return false;
            for (size_t i = 0; i < len; ++i) {
                dst[outPos] = dst[outPos - off];
                outPos++;
            }
        } else {
            size_t len = (size_t)tag;
            if (inPos + len > srcSize || outPos + len > origSize) return false;
            memcpy(&dst[outPos], &src[inPos], len);
            inPos += len;
            outPos += len;
        }
    }
    return (outPos == origSize);
}