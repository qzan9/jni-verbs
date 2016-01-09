/*
 * a naive demo for test.
 *
 * Author(s):
 *   azq  @qzan9  anzhongqi@ncic.ac.cn
 */

#ifdef __GNUC__

#ifndef _POSIX_C_SOURCE
#	define _POSIX_C_SOURCE 200809L
#endif /* _POSIX_C_SOURCE */

#ifndef _GNU_SOURCE
#	define _GNU_SOURCE
#endif /* _GNU_SOURCE */

#ifndef _SVID_SOURCE
#	define _SVID_SOURCE
#endif /* _SVID_SOURCE */

#endif /* __GNUC__ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <my_rdma.h>
#include <chk_err.h>

#define DEFAULT_IB_DEVICE       0
#define DEFAULT_IB_PORT         1
#define DEFAULT_BUFFER_SIZE   512
#define DEFAULT_SOCKET_PORT  9999

static struct rdma_context *rctx;
static struct user_config  *ucfg;

int main(int argc, char **argv)
{
	int op;
	int n0, n1;

	rctx = malloc(sizeof *rctx);
	ucfg = malloc(sizeof *ucfg);
	memset(rctx, 0, sizeof *rctx);
	memset(ucfg, 0, sizeof *ucfg);

	ucfg->ib_dev      = DEFAULT_IB_DEVICE;
	ucfg->ib_port     = DEFAULT_IB_PORT;
	ucfg->buffer_size = DEFAULT_BUFFER_SIZE;
	ucfg->server_name = NULL;
	ucfg->sock_port   = DEFAULT_SOCKET_PORT;

	while ((op = getopt(argc, argv, "b:s:")) != -1) {
		switch (op) {
		case 'b':
			ucfg->buffer_size = atoi(optarg);
			break;
		case 's':
			ucfg->server_name = optarg;
			break;
		default:
			printf("%s -b [buffer size] -s [server hostname]\n", argv[0]);
			return -1;
		}
	}

	//printf("buffer size: %d\n", ucfg->buffer_size);
	//printf("server name: %s\n", ucfg->server_name);

	if (!ucfg->server_name) {
		printf("%s -b [buffer size] -s [server hostname]\n", argv[0]);
		return -1;
	}

	CHK_NZPI(init_context(rctx, ucfg), "failed to initialize my RDMA context!");
	n0 = *((int *)(rctx->buf + rctx->size - sizeof n0));
	printf("N0: %d\n", n0);
	CHK_NZPI(connect_to_peer(rctx, ucfg), "failed to connect to peer server!");

	while(1) {
		printf("%d", n1 = *((int *)(rctx->buf + rctx->size - sizeof n1)));
		if (n0 != n1) {
			printf("\nDATA (maybe) WRITTEN!\n");
			break;
		}
	}

	destroy_context(rctx);
	free(rctx);
	free(ucfg);

	return 0;
}

