/*
 * Copyright (c) 2016  AZQ
 */

#ifdef __GNUC__
#	define _POSIX_C_SOURCE 200809L
#	define _GNU_SOURCE
#endif /* __GNUC__ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <my_rdma.h>
#include <chk_err.h>

static struct rdma_context *rctx;
static struct user_config  *ucfg;

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("usage: ...\n");
		return -1;
	}

	rctx = malloc(sizeof *rctx);
	ucfg = malloc(sizeof *ucfg);
	memset(rctx, 0, sizeof *rctx);
	memset(ucfg, 0, sizeof *ucfg);

	ucfg->ib_dev  = 0;
	ucfg->ib_port = 1;
	ucfg->buffer_size = 512;
	ucfg->server_name = argv[1];
	ucfg->sock_port = 9999;

	printf("server_name: %s\n", ucfg->server_name);

	srand48(getpid() * time(NULL));

	CHK_NZPI(init_context(rctx, ucfg), "failed to initialize my RDMA context!");
	CHK_NZPI(connect_to_peer(rctx, ucfg), "failed to connect to peer server!");

	destroy_context(rctx);
	free(rctx);
	free(ucfg);

	return 0;
}

