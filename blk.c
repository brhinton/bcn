// SPDX-License-Identifier: GPL-2.0-only
/*
 * blk.c
 *
 * Copyright (C) 2022,2023,2024,2025 Bryan Hinton
 *
 */

#include <blk.h>
#include <utl.h>

static uint32_t ctr = 0;

uint64_t tsm_get(void)
{
    int res;
    uint64_t nsec;
    struct timespec tp;
    clockid_t clk_id;

    clk_id = CLOCK_REALTIME;

    errno = 0;
    res = clock_gettime(clk_id, &tp);
    if(res != 0) {
        perror("clock_gettime()");
        exit(EXIT_FAILURE);
    }

    nsec = tp.tv_sec*1000000000 + tp.tv_nsec;

    return nsec;
}

struct blk* blk_add(struct blk *const l)
{
    struct blk *n;

    if(ctr > UINT_MAX-2) {
        log_err("invalid block num");
        _exit(EXIT_FAILURE);
    }

    if(ctr == 0) {
        if(l != INIT) {
            log_err("ctr == 0 && l==INIT");
            _exit(EXIT_FAILURE);
        }
    } else {
        if(!valid(l)) {
            log_err("ctr > 0 && !valid(l)");
            _exit(EXIT_FAILURE);
        }
    }

    errno = 0;

    n = (struct blk *)malloc(sizeof(struct blk));
    if(!valid(n)) {
        log_err("!valid(n)");
        _exit(EXIT_FAILURE);
    }

    if(ctr == 0)
        INIT_LST_HEAD(&n->lst);
    else
        lst_add_tail(&n->lst, &l->lst);

    n->bnm = ctr++;
    n->tsm = tsm_get();
    n->tdx = 0;
    n->tta = (struct txn *)malloc(sizeof(struct txn) * TPB);
    if(!valid(n->tta)) {
        log_err("!valid(b->tta)");
        _exit(EXIT_FAILURE);
    }

    return (n);
}

void blk_itr(struct blk *const b)
{
    struct lst_head *itr;
    void *p;
    struct blk *etr;
    uint32_t i,j;

    if(!valid(b)) {
        log_err("!valid(b)");
        _exit(EXIT_FAILURE);
    }

    /* check if list is empty */
    if (lst_empty(&b->lst)) {
            log_err("list is empty");
        _exit(EXIT_FAILURE);
        }

    /* iterate over each node in list */
    lst_for_each(itr, &b->lst) {
        etr = lst_entry(itr, struct blk, lst);

        if(!valid(etr)) {
            log_err("!valid(etr)");
            _exit(EXIT_FAILURE);
        }

        if (etr->tdx < 0 || etr->tdx >= TPB) {
                log_err("b->tdx is out of bounds");
                _exit(EXIT_FAILURE);
        }

        for(i = 0; i < etr->tdx; ++i) {
            for(j = 0; j < etr->tta[i].cdx; ++j) {
                /* execute function if array indexed function pointer is not null */
                if((etr->tta[i].cmd +j) != NULL)
                    ((void (*)(uint64_t))*(*(*(etr->tta[i].cmd +j)
                        +0) +0))(etr->tsm);
                /* execute function if array indexed function pointer is not null */
                if((etr->tta[i].cmd +j +1) != NULL)
                    p = (*(*(*(etr->tta[i].cmd +j) +1) +0));
                if((etr->tta[i].cmd +j +0 +0) != NULL)
                    ((void (*)(uint64_t))*(*(*(etr->tta[i].cmd +j)
                        +0) +0))((uint64_t)p+etr->tsm);
            }
        }
    }
}

void txn_add(struct blk *const b)
{
    struct txn *x;
    uint32_t i,j;

    if(!valid(b)) {
        log_err("!valid(b)");
        _exit(EXIT_FAILURE);
    }

    if(!valid(b->tta)) {
            log_err("b->tta is NULL");
            _exit(EXIT_FAILURE);
    }

    if (b->tdx < 0 || b->tdx >= TPB) {
            log_err("b->tdx is out of bounds");
            _exit(EXIT_FAILURE);
    }

    x = &b->tta[b->tdx];
    x->cdx = 0;
    errno = 0;
    b->tta[b->tdx].cmd = (void****)malloc(sizeof(void***)*CPT);
    if(!valid(b->tta[b->tdx].cmd)) {
        log_err("!valid(b->tta[b->tdx].cmd)");
        _exit(EXIT_FAILURE);
    }
    for(i = 0; i < CPT; ++i) {
        errno = 0;
        b->tta[b->tdx].cmd[i] = (void***)malloc(sizeof(void**)*CPU);
        if(!valid(b->tta[b->tdx].cmd[i])) {
            log_err("!valid(b->tta[b->tdx].cmd[i])");
            _exit(EXIT_FAILURE);
        }
        for(j = 0; j < CPU; ++j) {
            errno = 0;
            b->tta[b->tdx].cmd[i][j] =
                    (void**)malloc(sizeof(void*)*CPV);
            if(!valid(b->tta[b->tdx].cmd[i][j])) {
                log_err("!valid(b->tta[b->tdx].cmd[i])");
                _exit(EXIT_FAILURE);
            }
        }
    }

    errno = 0;
    b->tta[b->tdx].gtr = (uint32_t *)malloc(sizeof(uint32_t)*CPT);
    if(!valid(b->tta[b->tdx].gtr)) {
        log_err("!valid(b->tta[b->tdx].gtr)");
        _exit(EXIT_FAILURE);
    }

    errno = 0;
    b->tta[b->tdx].str = (uint32_t *)malloc(sizeof(uint32_t)*CPT);
    if(!valid(b->tta[b->tdx].str)) {
        log_err("!valid(b->tta->str)");
        _exit(EXIT_FAILURE);
    }
    b->tdx++;
}

void txn_addcmd(struct blk *const b, void(*c)(void), void *d, uint64_t t)
{
    struct txn *x;
    if(!valid(b)) {
        log_err("!valid(b)");
        _exit(EXIT_FAILURE);
    }

    if(!valid(b->tta)) {
        log_err("!valid(b->tta)");
            _exit(EXIT_FAILURE);
    }

    if (b->tdx < 0 || b->tdx >= TPB) {
            log_err("b->tdx is out of bounds");
            _exit(EXIT_FAILURE);
    }

    x = &b->tta[b->tdx-1];

    if(!valid(x->cmd)) {
        log_err("!valid(x->cmd)");
            _exit(EXIT_FAILURE);
    }

    if(c == NULL) {
        log_err("c is NULL");
            _exit(EXIT_FAILURE);
    }

    *(*(*(x->cmd + x->cdx) + 0) + 0) = (void*)c;
    *(*(*(x->cmd + x->cdx++) + 1) + 0) = (void*)t;
}
