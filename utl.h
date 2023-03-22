/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * utl.h
 *
 * Copyright (C) 2022,2023 Bryan Hinton
 *
 */

#ifndef _UTL_H
#define _UTL_H
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#define BDMAX 8192

#define debug(M, ...) syslog(LOG_NOTICE, "DEBUG %s:%d: " M "\n",\
       __FILE__, __LINE__, ##__VA_ARGS__)

#define log_errno() (errno == 0 ? "None" : strerror(errno))

#define log_err(M, ...) syslog(LOG_NOTICE, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__,\
        log_errno(), ##__VA_ARGS__)

#define log_wrn(M, ...) syslog(LOG_NOTICE, "[WARN] (%s:%d: errno: %s) " M "\n",\
        __FILE__, __LINE__, log_errno(), ##__VA_ARGS__);

#define log_inf(M, ...) syslog(LOG_NOTICE, "[INFO] (%s) " M "\n",\
        __FUNCTION__, ##__VA_ARGS__)

#define log_dbg(M, ...) syslog(LOG_NOTICE, "[INFO] " M "\n", ##__VA_ARGS__)

#define log_lbr() log_dbg("%.*s", 22, "===================");

uint8_t valid(void *t);

#endif
