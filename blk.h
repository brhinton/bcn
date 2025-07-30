/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * blk.h
 *
 * Copyright (C) 2022,2023,2024,2025 Bryan Hinton
 *
 */

#ifndef _BLK_H
#define _BLK_H
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <lst.h>

#define CPT     1024
#define CPU     7
#define CPV     6
#define TPB     4096
#define BFL     32

struct txn {
    void ****cmd;
    uint32_t *str;
    uint32_t *gtr;
    uint32_t cdx;
    uint16_t sta;
    uint32_t cpt;
    uint32_t fee;
    uint32_t gsl;
    uint32_t gsu;
    uint32_t gsp;
    uint64_t nce;
    uint64_t val;
    uint8_t ato[42];
    uint8_t afr[42];
    uint8_t hsh[BFL];
};

struct blk {
    uint32_t tdx;
    uint32_t bnm;
    uint32_t bfp;
    uint32_t gsl;
    uint32_t gsu;
    uint64_t tsm;
    uint64_t dif;
    uint64_t nce;
    uint8_t msh[BFL];
    uint8_t psh[BFL];
    uint8_t osh[BFL];
    uint8_t trh[BFL];
    uint8_t srh[BFL];
    uint8_t rrh[BFL];
    uint8_t lsb[BFL*8];
    uint8_t edt[BFL];
    uint8_t bfc[BFL];
    struct blk *ucr;
    struct lst_head lst;
    struct txn *tta;
};

typedef void (*fcnt_t)(void);

uint64_t tsm_get(void);
struct blk* blk_add(struct blk *const l);
void blk_itr(struct blk *const b);
void txn_add(struct blk *const b);
void txn_addcmd(struct blk *const b, void(*c)(void), void *d, uint64_t t);

#endif
