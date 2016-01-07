/*
 * Copyright (c) 2015, AZQ. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

#ifdef __GNUC__
#	define _POSIX_C_SOURCE 200809L
#	define _GNU_SOURCE
#endif /* __GNUC__ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include <unistd.h>
#include <netdb.h>

#include <infiniband/verbs.h>

#include "ibv_rdma.h"
#include "chk_err.h"

static int modify_qp_state_init(struct rdma_context *);
static int modify_qp_state_rtr (struct rdma_context *);
static int modify_qp_state_rts (struct rdma_context *);

static int tcp_server_listen(int);
static int tcp_client_connect(const char *, int);
static int tcp_exch_ib_conn_info(int, struct ib_conn *, struct ib_conn *);

int init_context(struct rdma_context *rctx, struct user_config *ucfg)
{
	struct ibv_device **dev_list;
	struct ibv_device_attr dev_attr;
	struct ibv_port_attr port_attr;

	int i, num_devices;

//	CHKE_NZ(ibv_fork_init(), "ibv_fork_init failed!");

	// check local IB devices.
	CHK_ZEI(dev_list = ibv_get_device_list(&num_devices), "ibv_get_device_list failed!");
	printf("found %d devices.\n", num_devices);
	for (i = 0; i < num_devices; i++) {
		printf("\t%d: %s 0x%" PRIx64 "\n", i, ibv_get_device_name(dev_list[i]), ibv_get_device_guid(dev_list[i]));
	}

	// open IB device.
	rctx->dev = dev_list[ucfg->ib_dev];
	CHK_ZEI(rctx->dev_ctx = ibv_open_device(rctx->dev), "ibv_open_device failed!");
	printf("using device %d.\n", ucfg->ib_dev);

	// query device port(s).
	CHK_NZEI(ibv_query_device(rctx->dev_ctx, &dev_attr), "ibv_query_device failed!");
	printf("device %d has %d port(s).\n", ucfg->ib_dev, dev_attr.phys_port_cnt);
	for (i = 1; i <= dev_attr.phys_port_cnt; i++) {
		CHK_NZEI(ibv_query_port(rctx->dev_ctx, i, &port_attr), "ibv_query_port failed!");
		printf("\t%d: %s 0x%" PRIx16 "\n", i, ibv_port_state_str(port_attr.state), port_attr.lid);
	}

	// check device port.
	rctx->port_num = ucfg->ib_port;
	printf("using port %d.\n", rctx->port_num);
	CHK_NZEI(ibv_query_port(rctx->dev_ctx, rctx->port_num, &port_attr), "ibv_query_port failed!");
	if (port_attr.state != IBV_PORT_ACTIVE) {
		fprintf(stderr, "IB port %d is not ready!\n", rctx->port_num);
		return -1;
	}

	// allocate protection domain.
	CHK_ZEI(rctx->pd = ibv_alloc_pd(rctx->dev_ctx), "ibv_alloc_pd failed!");

	// allocate and prepare the RDMA buffer.
	rctx->size = ucfg->buffer_size;
	CHK_NZEI(posix_memalign(&rctx->buf, sysconf(_SC_PAGESIZE), rctx->size*2), "failed to allocate buffer.");
	memset(rctx->buf, 0, rctx->size*2);

	// register the buffer and associate with the allocated protection domain.
	CHK_ZEI(rctx->mr = ibv_reg_mr(rctx->pd, rctx->buf, rctx->size*2, IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE),
	        "ibv_reg_mr failed! do you have root access?");

	// set up S/R completion queue and CQE notification receiving.
	CHK_ZEI(rctx->ch = ibv_create_comp_channel(rctx->dev_ctx), "ibv_create_comp_channel failed!");
	CHK_ZEI(rctx->scq = ibv_create_cq(rctx->dev_ctx, 64, rctx, rctx->ch, 0), "ibv_create_cq (scq) failed!");
	CHK_ZEI(rctx->rcq = ibv_create_cq(rctx->dev_ctx, 1, NULL, NULL, 0), "ibv_create_cq (rcq) failed!");

	// create the queue pair.
	struct ibv_qp_init_attr qp_init_attr = {
		.send_cq = rctx->scq,
		.recv_cq = rctx->rcq,
		.qp_type = IBV_QPT_RC,
		.cap = {
			.max_send_wr     = 64,
			.max_recv_wr     = 1,
			.max_send_sge    = 1,
			.max_recv_sge    = 1,
			.max_inline_data = 0
		}
	};
	CHK_ZEI(rctx->qp = ibv_create_qp(rctx->pd, &qp_init_attr), "ibv_create_qp failed!");

	// set QP status to INIT.
	struct ibv_qp_attr qp_attr = {
		.qp_state        = IBV_QPS_INIT,
		.pkey_index      = 0,
		.port_num        = rctx->port_num,
		.qp_access_flags = IBV_ACCESS_REMOTE_WRITE
	};
	CHK_NZEI(ibv_modify_qp(rctx->qp, &qp_attr,
	                       IBV_QP_STATE      |
	                       IBV_QP_PKEY_INDEX |
	                       IBV_QP_PORT       |
	                       IBV_QP_ACCESS_FLAGS),
	         "ibv_modify_qp (INIT) failed!");

	// allocate space for IB info.
	CHK_ZPI(rctx->local_conn  = malloc(sizeof(struct ib_conn)), "failed to allocate local_conn!");
	CHK_ZPI(rctx->remote_conn = malloc(sizeof(struct ib_conn)), "failed to allocate remote_conn!");
	memset(rctx->local_conn , 0, sizeof(struct ib_conn));
	memset(rctx->remote_conn, 0, sizeof(struct ib_conn));

	// record local IB info to exchange with peer.
	rctx->local_conn->lid   = port_attr.lid;
	rctx->local_conn->qpn   = rctx->qp->qp_num;
	rctx->local_conn->psn   = lrand48() & 0xffffff;
	rctx->local_conn->rkey  = rctx->mr->rkey;
	rctx->local_conn->vaddr = (uintptr_t)rctx->buf + rctx->size;

	return 0;
}

int connect_to_peer(struct rdma_context *rctx, struct user_config *ucfg)
{
	// make TCP connection.
	if (!ucfg->server_name) {
		CHK_NPI(rctx->sockfd = tcp_server_listen (ucfg->sock_port), "server failed!");
	} else {
		CHK_NPI(rctx->sockfd = tcp_client_connect(ucfg->server_name, ucfg->sock_port), "client failed!");
	}

	// exchange IB connection info.
	CHK_NZPI(tcp_exch_ib_conn_info(rctx->sockfd, rctx->local_conn, rctx->remote_conn), "failed to exchange IB info!");

	if (!ucfg->server_name) {
		modify_qp_state_rts(rctx);
	} else {
		modify_qp_state_rtr(rctx);
	}

	return 0;
}

int rdma_write(struct rdma_context *rctx)
{
	struct ibv_send_wr *bad_wr;
	struct ibv_wc wc;
	int ne;

	rctx->sge_list.addr   = (uintptr_t)rctx->buf;
	rctx->sge_list.length = rctx->size;
	rctx->sge_list.lkey   = rctx->mr->lkey;

	rctx->wr.wr.rdma.remote_addr = rctx->remote_conn->vaddr;
	rctx->wr.wr.rdma.rkey        = rctx->remote_conn->rkey;
	rctx->wr.wr_id               = 3;
	rctx->wr.sg_list             = &rctx->sge_list;
	rctx->wr.num_sge             = 1;
	rctx->wr.opcode              = IBV_WR_RDMA_WRITE;
	rctx->wr.send_flags          = IBV_SEND_SIGNALED;
	rctx->wr.next                = NULL;

	CHK_NZEI(ibv_post_send(rctx->qp, &rctx->wr, &bad_wr), "ibv_post_send failed! bad mkay!");

	do ne = ibv_poll_cq(rctx->scq, 1, &wc); while (ne == 0);
	CHK_NEI(ne, "ibv_poll_cq failed!");

	if (wc.status != IBV_WC_SUCCESS) {
		fprintf(stderr, "error completion! status: %d, WR id: %d.\n", wc.status, (int) wc.wr_id);
		return -1;
	}

	return 0;
}

int destroy_context(struct rdma_context *rctx)
{
	if (rctx->qp)  CHK_NZEI(ibv_destroy_qp(rctx->qp), "ibv_destroy_qp failed!");
	if (rctx->scq) CHK_NZEI(ibv_destroy_cq(rctx->scq), "ibv_destroy_cq (scp) failed!");
	if (rctx->rcq) CHK_NZEI(ibv_destroy_cq(rctx->rcq), "ibv_destroy_cq (rcp) failed!");
	if (rctx->ch)  CHK_NZEI(ibv_destroy_comp_channel(rctx->ch), "ibv_destroy_comp_channel failed!");
	if (rctx->mr)  CHK_NZEI(ibv_dereg_mr(rctx->mr), "ibv_dereg_mr failed!");
	if (rctx->pd)  CHK_NZEI(ibv_dealloc_pd(rctx->pd), "ibv_dealloc_pd failed!");
	if (rctx->dev_ctx)  CHK_NZEI(ibv_close_device(rctx->dev_ctx), "ibv_close_device failed!");

	if (rctx->local_conn ) free(rctx->local_conn );
	if (rctx->remote_conn) free(rctx->remote_conn);
	if (rctx->buf) free(rctx->buf);

	return 0;
}

static int modify_qp_state_init(struct rdma_context *rctx)
{
	struct ibv_qp_attr qp_attr = {
		.qp_state        = IBV_QPS_INIT,
		.pkey_index      = 0,
		.port_num        = rctx->port_num,
		.qp_access_flags = IBV_ACCESS_REMOTE_WRITE
	};

	CHK_NZEI(ibv_modify_qp(rctx->qp, &qp_attr,
	                       IBV_QP_STATE      |
	                       IBV_QP_PKEY_INDEX |
	                       IBV_QP_PORT       |
	                       IBV_QP_ACCESS_FLAGS),
	         "ibv_modify_qp (set INIT) failed!");

	return 0;
}

static int modify_qp_state_rtr(struct rdma_context *rctx)
{
	struct ibv_qp_attr qp_attr = {
		.qp_state              = IBV_QPS_RTR,
		.path_mtu              = IBV_MTU_2048,
		.dest_qp_num           = rctx->remote_conn->qpn,
		.rq_psn                = rctx->remote_conn->psn,
		.max_dest_rd_atomic    = 1,
		.min_rnr_timer         = 12,
		.ah_attr = {
			.is_global     = 0,
			.dlid          = rctx->remote_conn->lid,
			.sl            = 1,
			.src_path_bits = 0,
			.port_num      = rctx->port_num
		}
	};

	CHK_NZEI(ibv_modify_qp(rctx->qp, &qp_attr,
	                       IBV_QP_STATE              |
	                       IBV_QP_AV                 |
	                       IBV_QP_PATH_MTU           |
	                       IBV_QP_DEST_QPN           |
	                       IBV_QP_RQ_PSN             |
	                       IBV_QP_MAX_DEST_RD_ATOMIC |
	                       IBV_QP_MIN_RNR_TIMER),
	         "ibv_modify_qp (set RTR) failed!");

	return 0;
}

static int modify_qp_state_rts(struct rdma_context *rctx)
{
	CHK_NI(modify_qp_state_rtr(rctx));

	struct ibv_qp_attr qp_attr = {
		.qp_state      = IBV_QPS_RTS,
		.timeout       = 14,
		.retry_cnt     = 7,
		.rnr_retry     = 7,
		.sq_psn        = rctx->local_conn->psn,
		.max_rd_atomic = 1
	};

	CHK_NZEI(ibv_modify_qp(rctx->qp, &qp_attr,
	                       IBV_QP_STATE     |
	                       IBV_QP_TIMEOUT   |
	                       IBV_QP_RETRY_CNT |
	                       IBV_QP_RNR_RETRY |
	                       IBV_QP_SQ_PSN    |
	                       IBV_QP_MAX_QP_RD_ATOMIC),
	         "ibv_modify_qp (set RTS) failed!");

	return 0;
}

static int tcp_server_listen(int sock_port)
{
	char *port;
	int s, connfd, sockfd = -1;
	struct addrinfo *res, *rp;
	struct addrinfo hints = {
		.ai_family   = AF_UNSPEC,
		.ai_flags    = AI_PASSIVE,
		.ai_socktype = SOCK_STREAM
	};

	CHK_NI(asprintf(&port, "%d", sock_port));
	CHK_NZEI(s = getaddrinfo(NULL, port, &hints, &res), "getaddrinfo failed!");
	for (rp = res; rp != NULL; rp = rp->ai_next) {
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sockfd == -1) continue;

		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &s, sizeof s);
		if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) break;

		close(sockfd);
	}
	CHK_NEI(rp, "failed to create or bind socket!");

	free(port);
	freeaddrinfo(res);

	printf("server: waiting for client to connect ...\n");
	CHK_NZEI(listen(sockfd, 1), "listen failed!");
	CHK_NEI(connfd = accept(sockfd, NULL, 0), "accept failed!");

	return connfd;
}

static int tcp_client_connect(const char *server_name, int sock_port)
{
	char *port;
	int sockfd = -1;
	struct addrinfo *res, *rp;
	struct addrinfo hints = {
		.ai_family   = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM
	};

	CHK_NI(asprintf(&port, "%d", sock_port));
	CHK_NZEI(getaddrinfo(server_name, port, &hints, &res), "getaddrinfo failed!");

	printf("client: connecting to server ...\n");
	for (rp = res; rp != NULL; rp = rp->ai_next) {
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sockfd == -1) continue;

		if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) break;

		close(sockfd);
	}
	CHK_NEI(rp, "failed to create socket or connect to server!");

	free(port);
	freeaddrinfo(res);

	return sockfd;
}

static int tcp_exch_ib_conn_info(int sockfd, struct ib_conn *local_conn, struct ib_conn *remote_conn)
{
	char msg[sizeof "0000:000000:000000:00000000:0000000000000000"];
	int parsed;
	struct ib_conn *local = local_conn, *remote = remote_conn;

	if (!local || !remote) {
		fprintf(stderr, "empty space for local or remote connection!\n");
		goto ERROR;
	}

	// write down and send local IB connection info.
	sprintf(msg, "%04x:%06x:%06x:%08x:%016Lx",
	              local->lid, local->qpn, local->psn, local->rkey, local->vaddr);

	if (write(sockfd, msg, sizeof msg) != sizeof msg) {
		perror("failed to send IB connection details to peer!");
		goto ERROR;
	}

	// receive and parse remote IB connection info.
	memset(msg, 0, sizeof msg);
	if (read(sockfd, msg, sizeof msg) != sizeof msg) {
		perror("failed to receive connection details from peer!");
		goto ERROR;
	}

	memset(remote, 0, sizeof *remote);
	parsed = sscanf(msg, "%x:%x:%x:%x:%Lx",
	                      &remote->lid, &remote->qpn, &remote->psn, &remote->rkey, &remote->vaddr);
	if (parsed != 5) {
		fprintf(stderr, "failed to parse message from peer!\n");
		goto ERROR;
	}

	// shut down socket.
	close(sockfd);

	// print both local and remote IB connection info.
	printf("%s: LID %#04x, QPN %#06x, PSN %#06x, RKey %#08x, VAddr %#016Lx\n",
	       "local  connection", local ->lid, local ->qpn, local ->psn, local ->rkey, local ->vaddr);
	printf("%s: LID %#04x, QPN %#06x, PSN %#06x, RKey %#08x, VAddr %#016Lx\n",
	       "remote connection", remote->lid, remote->qpn, remote->psn, remote->rkey, remote->vaddr);

	return 0;

ERROR:
	close(sockfd);
	return -1;
}

