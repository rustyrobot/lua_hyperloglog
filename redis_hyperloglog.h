/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua HyperLogLog probabilistic cardinality approximation leveraging
 *  the Redis HLL dense represention/implementation @file */

#ifndef redis_hyperloglog_h_
#define redis_hyperloglog_h_

#include <stdint.h>

#define HLL_P 14 /* The greater is P, the smaller the error. */
#define HLL_REGISTERS (1<<HLL_P) /* With P=14, 16384 registers. */
#define HLL_BITS 6 /* Enough to count up to 63 leading zeroes. */
#define HLL_DENSE 0 /* Dense encoding. */

/*'registers' is expected to have room for HLL_REGISTERS plus an
 * additional byte on the right. */
#define HLL_REGISTERS_SIZE ((HLL_REGISTERS*HLL_BITS+7)/8) + 1

typedef struct hyperloglog {
    char magic[4];      /* "HYLL" */
    uint8_t encoding;   /* HLL_DENSE */
    uint8_t notused[3]; /* Reserved for future use, must be zero. */
    uint8_t card[8];    /* Cached cardinality, little endian. */
    uint8_t registers[HLL_REGISTERS_SIZE]; /* Data bytes. */
} hyperloglog;

/**
 * "Add" the element in the dense hyperloglog data structure.
 * Actually nothing is added, but the max 0 pattern counter of the subset
 * the element belongs to is incremented if needed.
 *
 * @param registers
 * @param ele
 * @param elesize
 *
 * @return int
 */
int hllDenseAdd(uint8_t *registers, unsigned char *ele, size_t elesize);

/**
 * Return cached cardinality or compute the current value.
 *
 * @param hll Pointer to the HyperLogLog object.
 *
 * @return uint64_t The approximated cardinality of the set.
 */
uint64_t hllCount(hyperloglog* hll);

#endif
