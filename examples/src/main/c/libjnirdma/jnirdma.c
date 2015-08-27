/*
 * Copyright (c) 2015, AZQ. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

#ifdef __GNUC__
#	define _POSIX_C_SOURCE 200809L
#	define _SVID_SOURCE
#	define _GNU_SOURCE
#endif /* __GNUC__ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <inttypes.h>
#include <errno.h>

#include <infiniband/verbs.h>

#include <jni.h>
#include "jnirdma.h"

#define CHK_NZI(x) do { if ( (x) ) return -1; } while (0)    /* if x is NON-ZERO, return -1.   */
#define CHK_ZI(x)  do { if (!(x) ) return -1; } while (0)    /* if x is ZERO,     return -1.   */
#define CHK_NI(x)  do { if ((x)<0) return -1; } while (0)    /* if x is NEGATIVE, return -1.   */

#define CHKP_NZI(x,msg) do { if ( (x) ) { fprintf(stderr, "error: %s\n", (msg)); return -1; } } while (0)
#define CHKP_ZI(x,msg)  do { if (!(x) ) { fprintf(stderr, "error: %s\n", (msg)); return -1; } } while (0)
#define CHKP_NI(x,msg)  do { if ((x)<0) { fprintf(stderr, "error: %s\n", (msg)); return -1; } } while (0)

#define CHKE_NZI(x,msg) do { if ( (x) ) { fprintf(stderr, "error: %s - %s\n", strerror(errno), (msg)); return -1; } } while (0)
#define CHKE_ZI(x,msg)  do { if (!(x) ) { fprintf(stderr, "error: %s - %s\n", strerror(errno), (msg)); return -1; } } while (0)
#define CHKE_NI(x,msg)  do { if ((x)<0) { fprintf(stderr, "error: %s - %s\n", strerror(errno), (msg)); return -1; } } while (0)

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
	struct ib_conn           local_conn;
	struct ib_conn          *remote_conn;
};

static int parse_user_config(JNIEnv *, jobject, struct user_config *);
static int init_rdma_context(struct rdma_context *, struct user_config *);
static int connect_to_peer(struct rdma_context *, struct user_config *);
static int rdma_write(struct rdma_context *);
static int destroy(struct rdma_context *);

static int modify_qp_state_init(struct rdma_context *);
static int modify_qp_state_rtr (struct rdma_context *);
static int modify_qp_state_rts (struct rdma_context *);

static int set_local_ib_conn(struct rdma_context *);
static int print_ib_conn(const char *, struct ib_conn *);

static int tcp_server_listen(struct user_config *);
static int tcp_client_connect(struct user_config *);
static int tcp_exch_ib_conn_info(struct rdma_context *);

static int die(const char *);
static void throwException(JNIEnv *, const char *cls_name, const char *msg);

static const JNINativeMethod methods[] = {
	{ "rdmaSetUp", "(Lac/ncic/syssw/azq/JniExamples/RdmaUserConfig;)Ljava/nio/ByteBuffer;", (void *)rdmaSetUp },
	{  "rdmaFree", "()V",                                                                   (void *)rdmaFree  },
	{ "rdmaWrite", "()V",                                                                   (void *)rdmaWrite },
};

static struct rdma_context rctx;
static struct user_config  ucfg;

/* ------------------------------------------------------------------------- */

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved)
{
	JNIEnv *env = NULL;

//	if (jvm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK)
	if ((*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_1_6) != JNI_OK) {    // there is no JNI_VERSION_1_7 constant.
		return JNI_ERR;
	}

//	if (env->RegisterNatives(env->FindClass("Lac/ncic/syssw/azq/JniExamples/JniRdma;"),
	if ((*env)->RegisterNatives(env,
	                            (*env)->FindClass(env, "Lac/ncic/syssw/azq/JniExamples/JniRdma;"),
	                            methods,
	                            sizeof(methods) / sizeof(methods[0])
	                           ) < -1) {
		return JNI_ERR;
	}

	return JNI_VERSION_1_6;
}

//JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *jvm, void *reserved)
//{
//}

JNIEXPORT jobject JNICALL rdmaSetUp(JNIEnv *env, jobject this, jobject userConfig)
{
	srand48(getpid() * time(NULL));

	memset(&rctx, 0, sizeof rctx);
	memset(&ucfg, 0, sizeof ucfg);

	if (parse_user_config(env, userConfig, &ucfg)) {
		throwException(env, "Lac/ncic/syssw/azq/JniExamples/RdmaException;", "failed to parse user config!");
		return NULL;
	}

	if (init_rdma_context(&rctx, &ucfg)) {
		throwException(env, "Lac/ncic/syssw/azq/JniExamples/RdmaException;", "failed to set up RDMA context!");
		return NULL;
	}

	if (connect_to_peer(&rctx, &ucfg)) {
		throwException(env, "Lac/ncic/syssw/azq/JniExamples/RdmaException;", "failed to make IB connection!");
		return NULL;
	}

	return (*env)->NewDirectByteBuffer(env, rctx.buf, rctx.size*2);
}

JNIEXPORT void JNICALL rdmaWrite(JNIEnv *env, jobject this)
{
	if (rdma_write(&rctx))
		throwException(env, "Lac/ncic/syssw/azq/JniExamples/RdmaException;", "failed to perform RDMA write operation!");
}

JNIEXPORT void JNICALL rdmaFree(JNIEnv *env, jobject this)
{
	if (ucfg.server_name)
		(*env)->ReleaseStringUTFChars(env, NULL, ucfg.server_name);

	if (destroy(&rctx))
//		exit(EXIT_FAILURE);
		;
}

/* ------------------------------------------------------------------------- */

static int parse_user_config(JNIEnv *env, jobject userConfig, struct user_config *ucfg)
{
	jclass userConfigCls;
	jmethodID userConfigMethodId;

	CHKP_ZI(userConfig, "user config is NULL!");

	CHK_ZI(userConfigCls = (*env)->GetObjectClass(env, userConfig));

	ucfg->ib_dev  = 0;    // note: pick your properly working IB device
	ucfg->ib_port = 1;    //       and port.

	CHK_ZI(userConfigMethodId = (*env)->GetMethodID(env, userConfigCls, "getBufferSize", "()I"));
	ucfg->buffer_size = (*env)->CallIntMethod(env, userConfig, userConfigMethodId);

	CHK_ZI(userConfigMethodId = (*env)->GetMethodID(env, userConfigCls, "getSocketPort", "()I"));
	ucfg->sock_port = (*env)->CallIntMethod(env, userConfig, userConfigMethodId);

	CHK_ZI(userConfigMethodId = (*env)->GetMethodID(env, userConfigCls, "getServerName", "()Ljava/lang/String;"));
	jstring serverName = (jstring)(*env)->CallObjectMethod(env, userConfig, userConfigMethodId);
	if (serverName) {
		ucfg->server_name = (char *)(*env)->GetStringUTFChars(env, serverName, NULL);
			// instance pinned, copy allocated: need be freed.
			// here's also buggy: if GetStringUTFChars returns NULL, the program would consider herself as the server.
	}
	(*env)->ReleaseStringUTFChars(env, serverName, NULL);    // not sure whether or not this would work.

	(*env)->DeleteLocalRef(env, userConfigCls);

	if ((*env)->ExceptionOccurred(env)) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}

	return 0;
}

static int init_rdma_context(struct rdma_context *rctx, struct user_config *ucfg)
{
	struct ibv_device **dev_list;
	struct ibv_device_attr dev_attr;
	struct ibv_port_attr port_attr;

	int i, num_devices;

//	CHKE_NZ(ibv_fork_init(), "ibv_fork_init failed!");

	// check local IB devices.
	CHKE_ZI(dev_list = ibv_get_device_list(&num_devices), "ibv_get_device_list failed!");
	printf("found %d devices.\n", num_devices);
	for (i = 0; i < num_devices; i++) {
		printf("\t%d: %s 0x%" PRIx64 "\n", i, ibv_get_device_name(dev_list[i]), ibv_get_device_guid(dev_list[i]));
	}

	// open IB device.
	rctx->dev = dev_list[ucfg->ib_dev];
	CHKE_ZI(rctx->dev_ctx = ibv_open_device(rctx->dev), "ibv_open_device failed!");
	printf("using device %d.\n", ucfg->ib_dev);

	// check device port.
	CHKE_NZI(ibv_query_device(rctx->dev_ctx, &dev_attr), "ibv_query_device failed!");
	printf("device has %d port(s).\n", dev_attr.phys_port_cnt);

	rctx->port_num = ucfg->ib_port;
	CHKE_NZI(ibv_query_port(rctx->dev_ctx, rctx->port_num, &port_attr), "ibv_query_port failed!");
	printf("\tusing port %d.\n", rctx->port_num);
	printf("\tstatus: %s.\n", ibv_port_state_str(port_attr.state));
	if (port_attr.state != IBV_PORT_ACTIVE) {
		fprintf(stderr, "IB port %d is not ready!\n", rctx->port_num);
		return -1;
	}

	// allocate a protection domain.
	CHKE_ZI(rctx->pd = ibv_alloc_pd(rctx->dev_ctx), "ibv_alloc_pd failed!");

	// allocate and prepare the RDMA buffer.
	rctx->size = ucfg->buffer_size;
	CHKE_NZI(posix_memalign(&rctx->buf, sysconf(_SC_PAGESIZE), rctx->size*2), "failed to allocate buffer.");
	memset(rctx->buf, 0, rctx->size*2);

	// register the buffer and assoicate with the allocated protection domain.
	CHKE_ZI(rctx->mr = ibv_reg_mr(rctx->pd, rctx->buf, rctx->size*2, IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE),
	        "ibv_reg_mr failed! do you have root access?");

	// set up S/R completion queue and CQE notification receiving.
	CHKE_ZI(rctx->ch = ibv_create_comp_channel(rctx->dev_ctx), "ibv_create_comp_channel failed!");
	CHKE_ZI(rctx->scq = ibv_create_cq(rctx->dev_ctx, 64, rctx, rctx->ch, 0), "ibv_create_cq (scq) failed!");
	CHKE_ZI(rctx->rcq = ibv_create_cq(rctx->dev_ctx, 1, NULL, NULL, 0), "ibv_create_cq (rcq) failed!");

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
	CHKE_ZI(rctx->qp = ibv_create_qp(rctx->pd, &qp_init_attr), "ibv_create_qp failed!");

	// set QP status to INIT.
	struct ibv_qp_attr qp_attr = {
		.qp_state        = IBV_QPS_INIT,
		.pkey_index      = 0,
		.port_num        = rctx->port_num,
		.qp_access_flags = IBV_ACCESS_REMOTE_WRITE
	};
	CHKE_NZI(ibv_modify_qp(rctx->qp, &qp_attr,
	                       IBV_QP_STATE      |
	                       IBV_QP_PKEY_INDEX |
	                       IBV_QP_PORT       |
	                       IBV_QP_ACCESS_FLAGS),
	         "ibv_modify_qp (INIT) failed!");

	// record local IB info to exchange with peer.
	rctx->local_conn.lid   = port_attr.lid;
	rctx->local_conn.qpn   = rctx->qp->qp_num;
	rctx->local_conn.psn   = lrand48() & 0xffffff;
	rctx->local_conn.rkey  = rctx->mr->rkey;
	rctx->local_conn.vaddr = (uintptr_t)rctx->buf + rctx->size;

	return 0;
}

static int connect_to_peer(struct rdma_context *rctx, struct user_config *ucfg)
{
	// make TCP connection.
	if (!ucfg->server_name) {
		CHKP_NI(rctx->sockfd = tcp_server_listen(ucfg), "server failed!");
	} else {
		CHKP_NI(rctx->sockfd = tcp_client_connect(ucfg), "client failed!");
	}

	// exchange ib connection info.
	CHKP_NZI(tcp_exch_ib_conn_info(rctx), "failed to exchange IB info!");
	print_ib_conn("local  connection", &rctx->local_conn);
	print_ib_conn("remote connection", rctx->remote_conn);

	if (!ucfg->server_name) {
		modify_qp_state_rts(rctx);
	} else {
		modify_qp_state_rtr(rctx);
	}

	close(rctx->sockfd);

	return 0;
}

static int rdma_write(struct rdma_context *rctx)
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

	CHKE_NZI(ibv_post_send(rctx->qp, &rctx->wr, &bad_wr), "ibv_post_send failed! bad mkay!");

	do ne = ibv_poll_cq(rctx->scq, 1, &wc); while (ne == 0);
	CHKE_NI(ne, "ibv_poll_cq failed!");

	if (wc.status != IBV_WC_SUCCESS) {
		fprintf(stderr, "error completion! status: %d, WR id: %d.\n", wc.status, (int) wc.wr_id);
		return -1;
	}

	return 0;
}

static int destroy(struct rdma_context *rctx)
{
	if (rctx->qp)  CHKE_NZI(ibv_destroy_qp(rctx->qp), "ibv_destroy_qp failed!");
	if (rctx->scq) CHKE_NZI(ibv_destroy_cq(rctx->scq), "ibv_destroy_cq (scp) failed!");
	if (rctx->rcq) CHKE_NZI(ibv_destroy_cq(rctx->rcq), "ibv_destroy_cq (rcp) failed!");
	if (rctx->ch)  CHKE_NZI(ibv_destroy_comp_channel(rctx->ch), "ibv_destroy_comp_channel failed!");
	if (rctx->mr)  CHKE_NZI(ibv_dereg_mr(rctx->mr), "ibv_dereg_mr failed!");
	if (rctx->pd)  CHKE_NZI(ibv_dealloc_pd(rctx->pd), "ibv_dealloc_pd failed!");

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

	CHKE_NZI(ibv_modify_qp(rctx->qp, &qp_attr,
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

	CHKE_NZI(ibv_modify_qp(rctx->qp, &qp_attr,
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
		.sq_psn        = rctx->local_conn.psn,
		.max_rd_atomic = 1
	};

	CHKE_NZI(ibv_modify_qp(rctx->qp, &qp_attr,
	                       IBV_QP_STATE     |
	                       IBV_QP_TIMEOUT   |
	                       IBV_QP_RETRY_CNT |
	                       IBV_QP_RNR_RETRY |
	                       IBV_QP_SQ_PSN    |
	                       IBV_QP_MAX_QP_RD_ATOMIC),
	         "ibv_modify_qp (set RTS) failed!");

	return 0;
}

static int set_local_ib_conn(struct rdma_context *rctx)
{
	struct ibv_port_attr attr;

	CHKE_NZI(ibv_query_port(rctx->dev_ctx, rctx->port_num, &attr), "ibv_query_port failed!");

	rctx->local_conn.lid   = attr.lid;
	rctx->local_conn.qpn   = rctx->qp->qp_num;
	rctx->local_conn.psn   = lrand48() & 0xffffff;
	rctx->local_conn.rkey  = rctx->mr->rkey;
	rctx->local_conn.vaddr = (uintptr_t)rctx->buf + rctx->size;

	return 0;
}

static int print_ib_conn(const char *conn_name, struct ib_conn *conn)
{
	printf("%s: LID %#04x, QPN %#06x, PSN %#06x, RKey %#08x, VAddr %#016Lx\n",
	       conn_name, conn->lid, conn->qpn, conn->psn, conn->rkey, conn->vaddr);
	return 0;
}

static int tcp_server_listen(struct user_config *ucfg)
{
	char *port;
	int s, connfd, sockfd = -1;
	struct addrinfo *res, *rp;
	struct addrinfo hints = {
		.ai_family   = AF_UNSPEC,
		.ai_flags    = AI_PASSIVE,
		.ai_socktype = SOCK_STREAM
	};

	CHK_NI(asprintf(&port, "%d", ucfg->sock_port));
	CHKE_NZI(s = getaddrinfo(NULL, port, &hints, &res), "getaddrinfo failed!");
	for (rp = res; rp != NULL; rp = rp->ai_next) {
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sockfd == -1) continue;

		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &s, sizeof s);
		if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) break;

		close(sockfd);
	}
	CHKE_NI(rp, "failed to create or bind socket!");

	free(port);
	freeaddrinfo(res);

	printf("server: waiting for client to connect ...\n");
	CHKE_NZI(listen(sockfd, 1), "listen failed!");
	CHKE_NI(connfd = accept(sockfd, NULL, 0), "accept failed!");

	return connfd;
}

static int tcp_client_connect(struct user_config *ucfg)
{
	char *port;
	int sockfd = -1;
	struct addrinfo *res, *rp;
	struct addrinfo hints = {
		.ai_family   = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM
	};

	CHK_NI(asprintf(&port, "%d", ucfg->sock_port));
	CHKE_NZI(getaddrinfo(ucfg->server_name, port, &hints, &res), "getaddrinfo failed!");

	printf("client: connecting to server ...\n");
	for (rp = res; rp != NULL; rp = rp->ai_next) {
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sockfd == -1) continue;

		if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) break;

		close(sockfd);
	}
	CHKE_NI(rp, "failed to create socket or connect to server!");

	free(port);
	freeaddrinfo(res);

	return sockfd;
}

static int tcp_exch_ib_conn_info(struct rdma_context *rctx)
{
	char msg[sizeof "0000:000000:000000:00000000:0000000000000000"];
	struct ib_conn *local, *remote;
	int parsed;

	// send local IB connection info.
	local = &rctx->local_conn;
	sprintf(msg, "%04x:%06x:%06x:%08x:%016Lx",
	              local->lid, local->qpn, local->psn, local->rkey, local->vaddr);
	if (write(rctx->sockfd, msg, sizeof msg) != sizeof msg) {
		perror("failed to send IB connection details to peer!");
		goto ERROR;
	}

	// receive and parse remote IB connection info.
	if (read(rctx->sockfd, msg, sizeof msg) != sizeof msg) {
		perror("failed to receive connection details from peer!");
		goto ERROR;
	}
	if (rctx->remote_conn) free(rctx->remote_conn);
	rctx->remote_conn = malloc(sizeof(struct ib_conn));
	remote = rctx->remote_conn;
	parsed = sscanf(msg, "%x:%x:%x:%x:%Lx",
	                      &remote->lid, &remote->qpn, &remote->psn, &remote->rkey, &remote->vaddr);
	if (parsed != 5) {
		fprintf(stderr, "failed to parse message from peer.");
		free(rctx->remote_conn);
		goto ERROR;
	}

	close(rctx->sockfd);
	return 0;

ERROR:
	close(rctx->sockfd);
	return -1;
}

/* ------------------------------------------------------------------------- */

static int die(const char *reason)
{
	fprintf(stderr, "error: %s - %s\n ", strerror(errno), reason);
	exit(EXIT_FAILURE);
	return -1;
}

static void throwException(JNIEnv *env, const char *cls_name, const char *msg)
{
	jclass cls = (*env)->FindClass(env, cls_name);
	if (cls) (*env)->ThrowNew(env, cls, msg);
	(*env)->DeleteLocalRef(env, cls);
}

