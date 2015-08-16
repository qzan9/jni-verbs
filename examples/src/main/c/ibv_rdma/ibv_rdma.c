#if HAVE_CONFIG_H
#	include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef __GNUC__
#	define _POSIX_C_SOURCE 200809L
#	define _SVID_SOURCE
#	define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <netdb.h>

#include <infiniband/verbs.h>

#define RDMA_WRID 3

#define TEST_NZ(x,y) do { if ( (x) ) die(y); } while (0)  /* if x is NON-ZERO, error is printed */
#define TEST_Z(x,y)  do { if (!(x) ) die(y); } while (0)  /* if x is ZERO, error is printed */
#define TEST_N(x,y)  do { if ((x)<0) die(y); } while (0)  /* if x is NEGATIVE, error is printed */

static int sl = 1;
static pid_t pid;

struct app_context {    // full ibv context.
	struct ibv_context      *context;
	struct ibv_pd           *pd;
	struct ibv_mr           *mr;
	struct ibv_cq           *rcq;
	struct ibv_cq           *scq;
	struct ibv_qp           *qp;
	struct ibv_comp_channel *ch;
	void                    *buf;
	unsigned                 size;
	int                      tx_depth;
	struct ibv_sge           sge_list;
	struct ibv_send_wr       wr;
};

struct ib_connection {    // ib connection info (to be exchanged with peer).
	int                 lid;
	int                 qpn;
	int                 psn;
	unsigned            rkey;
	unsigned long long  vaddr;
};

struct app_data {    // user specified data.
	int                   port;
	int                   ib_port;
	unsigned              size;
	int                   tx_depth;
	int                   sockfd;
	char                 *servername;
	struct ib_connection  local_connection;
	struct ib_connection *remote_connection;
	struct ibv_device    *ib_dev;
};

static struct app_context *init_ctx(struct app_data *data);
static void destroy_ctx(struct app_context *ctx);

static void set_local_ib_connection(struct app_context *ctx, struct app_data *data);
static void print_ib_connection(char *conn_name, struct ib_connection *conn);

static int qp_change_state_init(struct ibv_qp *qp, struct app_data *data);
static int qp_change_state_rtr (struct ibv_qp *qp, struct app_data *data);
static int qp_change_state_rts (struct ibv_qp *qp, struct app_data *data);

static void rdma_write(struct app_context *ctx, struct app_data *data);

static int tcp_client_connect(struct app_data *data);
static int tcp_server_listen(struct app_data *data);
static int tcp_exch_ib_connection_info(struct app_data *data);

static int die(const char *reason);

int main(int argc, char *argv[])
{
	struct app_context *ctx = NULL;
	struct app_data data = {
		.port              = 9999,
		.ib_port           = 1,    // note: pick your active IB port.
		.size              = 65536,
		.tx_depth          = 64,
		.servername        = NULL,
		.remote_connection = NULL,
		.ib_dev            = NULL
	};

	if (argc == 2) data.servername = argv[1];
	if (argc >  2) die("usage: ibv_rdma <server>\n");

	pid = getpid();
	srand48(pid * time(NULL));    // later needed to create a random number for PSN.

	if (!data.servername) {
		printf("PID=%d | port=%d | ib_port=%d | size=%d | tx_depth=%d | sl=%d\n",
			pid, data.port, data.ib_port, data.size, data.tx_depth, sl);
	}

	// initialize IB context.
	TEST_Z(ctx = init_ctx(&data), "init_ctx failed to create ctx.");

	// prepare IB info and exchange with peer through socket.
	set_local_ib_connection(ctx, &data);

	if (data.servername) data.sockfd = tcp_client_connect(&data);
	else TEST_N(data.sockfd = tcp_server_listen(&data), "tcp_server_listen failed.");

	TEST_NZ(tcp_exch_ib_connection_info(&data), "tcp_exch_ib_connection failed.");
	print_ib_connection("local  connection", &data.local_connection);
	print_ib_connection("remote connection", data.remote_connection);

	// change QP status: client -> RTR, server -> RTS.
	if (data.servername) qp_change_state_rtr(ctx->qp, &data);
	else qp_change_state_rts(ctx->qp, &data);

	// performance RDMA write.
	if (!data.servername) {
		printf("server: writing to client-buffer (RDMA-WRITE).\n");

		char *chPtr = ctx->buf;
		strcpy(chPtr, "read the f**ing source code :-)");    // note: edit the message to be written into clients buffer.

		rdma_write(ctx, &data);
	} else {
		printf("client: reading local-buffer (buffer that was registered with MR).\n");

		char *chPtr = (char *)data.local_connection.vaddr;

		while(1) if(strlen(chPtr) > 0) break;

		printf("local buffer: %s\n", chPtr);
	}

	printf("closing socket\n");
	close(data.sockfd);

	printf("destroying IB context\n");
	destroy_ctx(ctx);

	return 0;
}

/*
 * init_ctx initializes the Infiniband context.
 *
 * it creates structures for: ProtectionDomain, MemoryRegion, CompletionChannel,
 * Completion Queues, and Queue Pair.
 */
static struct app_context *init_ctx(struct app_data *data)
{
	struct app_context *ctx;
	struct ibv_device **dev_list;
	struct ibv_device  *ib_dev;

	int page_size;

	ctx = malloc(sizeof *ctx);
	memset(ctx, 0, sizeof *ctx);

	ctx->tx_depth = data->tx_depth;

	// allocate and prepare the buffer.
	page_size = sysconf(_SC_PAGESIZE);
	ctx->size = data->size;
	TEST_NZ(posix_memalign(&ctx->buf, page_size, ctx->size * 2),
	        "could not allocate working buffer ctx->buf.");
	memset(ctx->buf, 0, ctx->size * 2);

	// open an IB device to get a context.
	TEST_Z(dev_list = ibv_get_device_list(NULL),
	       "no IB-device available. get_device_list returned NULL.");
	TEST_Z(data->ib_dev = dev_list[0],    /* note: pick your properly working device. */
	       "IB-device could not be assigned. maybe dev_list array is empty.");
	TEST_Z(ctx->context = ibv_open_device(data->ib_dev),
	       "open_device failed to create context.");

	// allocate a protection domain.
	TEST_Z(ctx->pd = ibv_alloc_pd(ctx->context),
	       "alloc_pd failed to allocate a protection domain.");

	// register the buffer and assoicate with the allocated protection domain.
	TEST_Z(ctx->mr = ibv_reg_mr(ctx->pd, ctx->buf, ctx->size * 2, IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE),
	       "reg_mr failed to register memory. do you have root access?");

	// set up S/R completion queue and CQE notification receiving.
	TEST_Z(ctx->ch = ibv_create_comp_channel(ctx->context),
	       "create_comp_channel failed to create completion channel.");
	TEST_Z(ctx->rcq = ibv_create_cq(ctx->context, 1, NULL, NULL, 0),
	       "create_cq failed to create receive completion queue.");
	TEST_Z(ctx->scq = ibv_create_cq(ctx->context, ctx->tx_depth, ctx, ctx->ch, 0),
	       "create_cq failed to create send completion queue.");

	// create the queue pair.
	struct ibv_qp_init_attr qp_init_attr = {
		.send_cq = ctx->scq,
		.recv_cq = ctx->rcq,
		.qp_type = IBV_QPT_RC,
		.cap = {
			.max_send_wr     = ctx->tx_depth,
			.max_recv_wr     = 1,
			.max_send_sge    = 1,
			.max_recv_sge    = 1,
			.max_inline_data = 0
		}
	};
	TEST_Z(ctx->qp = ibv_create_qp(ctx->pd, &qp_init_attr),
	       "create_qp failed to create queue pair.");

	// initialize QP and set status to INIT.
	qp_change_state_init(ctx->qp, data);

	return ctx;
}

static void destroy_ctx(struct app_context *ctx)
{

	TEST_NZ(ibv_destroy_qp(ctx->qp), "destroy_qp failed to destroy queue pair.");
	TEST_NZ(ibv_destroy_cq(ctx->scq), "destroy_cq failed to destroy send completion queue.");
	TEST_NZ(ibv_destroy_cq(ctx->rcq), "destroy_cq failed to destroy receive completion queue.");
	TEST_NZ(ibv_destroy_comp_channel(ctx->ch), "destroy_comp_channel failed to destroy completion channel.");
	TEST_NZ(ibv_dereg_mr(ctx->mr), "dereg_mr failed to de-register memory region.");
	TEST_NZ(ibv_dealloc_pd(ctx->pd), "dealloc_pd failed to deallocate protection domain.");

	free(ctx->buf);
	free(ctx);
}

/*
 * set_local_ib_connection sets all relevant attributes needed for an IB
 * connection. those are then sent to the peer via TCP.
 *
 * information needed to exchange data over IB include:
 *
 * - lid - Local Identifier, 16 bit address assigned to end node by subnet
 *   manager.
 *
 * - qpn - Queue Pair Number, identifies qpn within channel adapter (HCA).
 *
 * - psn - Packet Sequence Number, used to verify correct delivery sequence of
 *   packages (similar to ACK).
 *
 * - rkey - Remote Key, together with 'vaddr' identifies and grants access to
 *   memory region.
 *
 * - vaddr - Virtual Address, memory address that peer can later write to.
 */
static void set_local_ib_connection(struct app_context *ctx, struct app_data *data)
{
	struct ibv_port_attr attr;

	TEST_NZ(ibv_query_port(ctx->context, data->ib_port, &attr),
	        "query_port failed to get port attributes.");

	data->local_connection.lid   = attr.lid;
	data->local_connection.qpn   = ctx->qp->qp_num;
	data->local_connection.psn   = lrand48() & 0xffffff;
	data->local_connection.rkey  = ctx->mr->rkey;
	data->local_connection.vaddr = (uintptr_t)ctx->buf + ctx->size;
}

static void print_ib_connection(char *conn_name, struct ib_connection *conn)
{
	printf("%s: LID %#04x, QPN %#06x, PSN %#06x, RKey %#08x, VAddr %#016Lx\n",
	       conn_name, conn->lid, conn->qpn, conn->psn, conn->rkey, conn->vaddr);

}

/*
 * qp_change_state_init sets Queue Pair status to INIT.
 */
static int qp_change_state_init(struct ibv_qp *qp, struct app_data *data)
{
	struct ibv_qp_attr *attr;

	attr = malloc(sizeof *attr);
	memset(attr, 0, sizeof *attr);

	attr->qp_state        = IBV_QPS_INIT;
	attr->pkey_index      = 0;
	attr->port_num        = data->ib_port;
	attr->qp_access_flags = IBV_ACCESS_REMOTE_WRITE;

	TEST_NZ(ibv_modify_qp(qp, attr,
	                      IBV_QP_STATE      |
	                      IBV_QP_PKEY_INDEX |
	                      IBV_QP_PORT       |
	                      IBV_QP_ACCESS_FLAGS),
	        "modify_qp failed to modify QP to INIT.");

	free(attr);

	return 0;
}

/*
 * qp_change_state_rtr sets Queue Pair status to RTR (Ready to Receive).
 */
static int qp_change_state_rtr(struct ibv_qp *qp, struct app_data *data)
{
	struct ibv_qp_attr *attr;

	attr = malloc(sizeof *attr);
	memset(attr, 0, sizeof *attr);

	attr->qp_state              = IBV_QPS_RTR;
	attr->path_mtu              = IBV_MTU_2048;
	attr->dest_qp_num           = data->remote_connection->qpn;
	attr->rq_psn                = data->remote_connection->psn;
	attr->max_dest_rd_atomic    = 1;
	attr->min_rnr_timer         = 12;
	attr->ah_attr.is_global     = 0;
	attr->ah_attr.dlid          = data->remote_connection->lid;
	attr->ah_attr.sl            = sl;
	attr->ah_attr.src_path_bits = 0;
	attr->ah_attr.port_num      = data->ib_port;

	TEST_NZ(ibv_modify_qp(qp, attr,
	                      IBV_QP_STATE              |
	                      IBV_QP_AV                 |
	                      IBV_QP_PATH_MTU           |
	                      IBV_QP_DEST_QPN           |
	                      IBV_QP_RQ_PSN             |
	                      IBV_QP_MAX_DEST_RD_ATOMIC |
	                      IBV_QP_MIN_RNR_TIMER),
                "could not modify QP to RTR state.");

	free(attr);

	return 0;
}

/*
 * qp_change_state_rts sets Queue Pair status to RTS (Ready to send).
 *
 * note that QP status has to be RTR before changing it to RTS
 */
static int qp_change_state_rts(struct ibv_qp *qp, struct app_data *data)
{
	qp_change_state_rtr(qp, data);

	struct ibv_qp_attr *attr;

	attr = malloc(sizeof *attr);
	memset(attr, 0, sizeof *attr);

	attr->qp_state      = IBV_QPS_RTS;
	attr->timeout       = 14;
	attr->retry_cnt     = 7;
	attr->rnr_retry     = 7;
	attr->sq_psn        = data->local_connection.psn;
	attr->max_rd_atomic = 1;

	TEST_NZ(ibv_modify_qp(qp, attr,
	                      IBV_QP_STATE     |
	                      IBV_QP_TIMEOUT   |
	                      IBV_QP_RETRY_CNT |
	                      IBV_QP_RNR_RETRY |
	                      IBV_QP_SQ_PSN    |
	                      IBV_QP_MAX_QP_RD_ATOMIC),
	        "could not modify QP to RTS state.");

	free(attr);

	return 0;
}

/*
 *  rdma_write writes 'ctx-buf' into buffer of peer.
 */
static void rdma_write(struct app_context *ctx, struct app_data *data)
{
	struct ibv_send_wr *bad_wr;

	ctx->sge_list.addr   = (uintptr_t)ctx->buf;
   	ctx->sge_list.length = ctx->size;
   	ctx->sge_list.lkey   = ctx->mr->lkey;

  	ctx->wr.wr.rdma.remote_addr = data->remote_connection->vaddr;
	ctx->wr.wr.rdma.rkey        = data->remote_connection->rkey;
	ctx->wr.wr_id               = RDMA_WRID;
	ctx->wr.sg_list             = &ctx->sge_list;
	ctx->wr.num_sge             = 1;
	ctx->wr.opcode              = IBV_WR_RDMA_WRITE;
	ctx->wr.send_flags          = IBV_SEND_SIGNALED;
	ctx->wr.next                = NULL;

	TEST_NZ(ibv_post_send(ctx->qp, &ctx->wr, &bad_wr), "ibv_post_send failed. this is bad mkay");

	/*
	 * checks if messages are completely sent. but fails if client destroys her context too early.
	 * this would have to be timed by server telling client that rdma_write has been completed.
	 */
/*
	int ne;
	struct ibv_wc wc;

	do ne = ibv_poll_cq(ctx->scq, 1, &wc);
	while(ne == 0);

	if (ne < 0) fprintf(stderr, "%s: poll CQ failed %d.\n", __func__, ne);

	if (wc.status != IBV_WC_SUCCESS) {
		fprintf(stderr, "%d:%s: completion with error at %s.\n",
		                 pid, __func__, data->servername ? "client" : "server");
		fprintf(stderr, "%d:%s: failed status %d: wr_id %d.\n",
		                 pid, __func__, wc.status, (int) wc.wr_id);
        }

	if (wc.status == IBV_WC_SUCCESS) {
		printf("wrid: %i successfull.\n", (int)wc.wr_id);
		printf("%i bytes transfered.\n" , (int)wc.byte_len);
	}
*/
}

static int tcp_client_connect(struct app_data *data)
{
	char *service;
	int n;
	int sockfd = -1;
	struct sockaddr_in sin;

	struct addrinfo *res, *t;
	struct addrinfo hints = {
		.ai_family   = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM
	};

	TEST_N(asprintf(&service, "%d", data->port), "error writing port-number to port-string.");

	TEST_N(getaddrinfo(data->servername, service, &hints, &res), "getaddrinfo failed.");

	for (t = res; t; t = t->ai_next) {
		TEST_N(sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol), "failed to create client socket.");
		TEST_N(connect(sockfd, t->ai_addr, t->ai_addrlen), "failed to connect to server");
	}

	freeaddrinfo(res);

	return sockfd;
}

static int tcp_server_listen(struct app_data *data)
{
	char *service;
	int n, connfd;
	int sockfd = -1;
	struct sockaddr_in sin;

	struct addrinfo *res, *t;
	struct addrinfo hints = {
		.ai_flags    = AI_PASSIVE,
		.ai_family   = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM
	};

	TEST_N(asprintf(&service, "%d", data->port), "error writing port-number to port-string.");

	TEST_N(n = getaddrinfo(NULL, service, &hints, &res), "getaddrinfo failed.");

	TEST_N(sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol), "failed to create server socket.");

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof n);

	TEST_N(bind(sockfd, res->ai_addr, res->ai_addrlen), "failed to bind addr to server socket.");

	listen(sockfd, 1);

	TEST_N(connfd = accept(sockfd, NULL, 0), "server accept failed.");

	freeaddrinfo(res);

	return connfd;
}

static int tcp_exch_ib_connection_info(struct app_data *data)
{
	char msg[sizeof "0000:000000:000000:00000000:0000000000000000"];
	int parsed;
	struct ib_connection *local = &data->local_connection;

	sprintf(msg, "%04x:%06x:%06x:%08x:%016Lx",
	              local->lid, local->qpn, local->psn, local->rkey, local->vaddr);

	if (write(data->sockfd, msg, sizeof msg) != sizeof msg) {
		perror("failed to send connection_details to peer.");
		return -1;
	}

	if (read(data->sockfd, msg, sizeof msg) != sizeof msg) {
		perror("failed to receive connection_details from peer.");
		return -1;
	}

	if (!data->remote_connection) free(data->remote_connection);

	TEST_Z(data->remote_connection = malloc(sizeof(struct ib_connection)),
	       "failed to allocate memory for remote connection.");

	struct ib_connection *remote = data->remote_connection;

	parsed = sscanf(msg, "%x:%x:%x:%x:%Lx",
	                      &remote->lid, &remote->qpn, &remote->psn, &remote->rkey, &remote->vaddr);

	if (parsed != 5) {
		fprintf(stderr, "failed to parse message from peer.");
		free(data->remote_connection);
	}

	return 0;
}

static int die(const char *reason)
{
	fprintf(stderr, "error: %s - %s\n ", strerror(errno), reason);
	exit(EXIT_FAILURE);
	return -1;
}

