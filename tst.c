// SPDX-License-Identifier: GPL-2.0-only
/*
 * tst.c
 *
 * Copyright (C) 2022 Bryan Hinton
 *
 */

#include <blk.h>
#include <utl.h>
#include <fcntl.h>
#include <signal.h>

void tst(uint64_t t)
{
	log_lbr()
	log_dbg("%lu", t);
}

void tsta(uint64_t t)
{
	uint64_t a;
	log_lbr()
	a = (t & 0xffff) ^ ((t >> 16UL) & 0xffff);
	log_dbg("%lu", a);
}

void tstb(uint64_t t)
{
	log_lbr()
	log_dbg("%lu", t);
}

void tstc(void)
{
	log_lbr()
	log_dbg("");
}

void tstd(uint8_t u)
{
	log_lbr()
	log_dbg("%u", u);
}

enum {CTA, CTB, CTC};

int main(int argc, char **argv)
{
	uint32_t i, m;
	struct sigaction sa;
	struct timespec req, rem;
	struct blk *b, *r;
	static const time_t ssc = 1;

	switch(fork()) {
	case -1:
		_exit(EXIT_FAILURE);
	case 0:
		break;
	default:
		_exit(EXIT_SUCCESS);
	}

	if(setsid() < 0)
		_exit(EXIT_FAILURE);

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGHUP, &sa, NULL) < 0)
		_exit(EXIT_FAILURE);

	switch (fork()) {
	case -1:
		_exit(EXIT_FAILURE);
	case 0:
		break;
	default:
		_exit(EXIT_SUCCESS);
	}

	umask(0);
	chdir("/");

	m = sysconf(_SC_OPEN_MAX);
	if(m == -1)
		m = BDMAX;
	for(i = 0; i < m; ++i)
		close(i);

	close(STDIN_FILENO);
	i = open("/dev/null", O_RDWR);
	if (i != STDIN_FILENO)
		_exit(EXIT_FAILURE);
	if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
		_exit(EXIT_FAILURE);
	if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
		_exit(EXIT_FAILURE);

	b = blk_add(INIT);
	if(!valid(b))
		_exit(EXIT_FAILURE);
	txn_add(b);
	txn_addcmd(b,(fcnt_t)&tst,0,CTB);
	txn_addcmd(b,(fcnt_t)&tsta,0,CTB);
	txn_addcmd(b,(fcnt_t)&tstb,0,CTB);
	r = b;
	openlog("bcn", LOG_PID, LOG_DAEMON);

	for(i = 0; i < 400; ++i) {
		b = blk_add(b);
		txn_add(b);
		txn_addcmd(b,(fcnt_t)&tst,0,CTB);
		txn_addcmd(b,(fcnt_t)&tsta,0,CTB);
		txn_addcmd(b,(fcnt_t)&tstb,0,CTB);
		if(!valid(b)) {
			log_err("b==NULL");
			_exit(EXIT_FAILURE);
		}
	}

	blk_itr(r);
	req.tv_sec = ssc;
	req.tv_nsec = 0;
	while(1) {
		nanosleep(&req, &rem);
		log_dbg("%ld",tsm_get());
	}
}
