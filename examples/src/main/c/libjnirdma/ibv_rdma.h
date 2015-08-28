/*
 * Copyright (c) 2015, AZQ. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

#ifndef _IBV_RDMA_H_
#define _IBV_RDMA_H_

#include <infiniband/verbs.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct user_config {
	int       ib_dev;
	int       ib_port;
	unsigned  buffer_size;

	char     *server_name;
	int       sock_port;
};

struct ib_conn {
	int                 lid;
	int                 qpn;
	int                 psn;
	unsigned            rkey;
	unsigned long long  vaddr;
};

struct rdma_context {
	struct ibv_device       *dev;
	struct ibv_context      *dev_ctx;
	struct ibv_pd           *pd;
	struct ibv_mr           *mr;
	struct ibv_comp_channel *ch;
	struct ibv_cq           *scq;
	struct ibv_cq           *rcq;
	struct ibv_qp           *qp;
	struct ibv_sge           sge_list;
	struct ibv_send_wr       wr;
	int                      port_num;
	void                    *buf;
	unsigned                 size;

	int                      sockfd;
	struct ib_conn          *local_conn;
	struct ib_conn          *remote_conn;
};

int init_context(struct rdma_context *, struct user_config *);

int connect_to_peer(struct rdma_context *, struct user_config *);

int rdma_write(struct rdma_context *);

int destroy_context(struct rdma_context *);

#ifdef __cplusplus
}
#endif /*__cplusplus */

#endif /* _IBV_RDMA_H_ */

