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

#define IBV_NZI(env,ibv,msg) do { if ( (ibv) ) return bpill((env),(msg)); } while (0)    /* if ibv is NON-ZERO, throw exception and return -1. */
#define IBV_ZI(env,ibv,msg)  do { if (!(ibv) ) return bpill((env),(msg)); } while (0)    /* if ibv is ZERO,     throw exception and return -1. */
#define IBV_NI(env,ibv,msg)  do { if ((ibv)<0) return bpill((env),(msg)); } while (0)    /* if ibv is NEGATIVE, throw exception and return -1. */

#define CHK_NZN(x) do { if ((x)) return NULL; } while (0)    /* if x is NON-ZERO, return -1. */
#define CHK_NZI(x) do { if ((x)) return -1; } while (0)    /* if x is NON-ZERO, return -1. */
#define CHK_ZI(x)  do { if (!(x)) return -1; } while (0)    /* if x is ZERO, return -1. */
#define CHK_NI(x)  do { if ((x)<0) return -1; } while (0)    /* if x is NEGATIVE, return -1. */

struct ib_connection {
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
	struct ibv_cq           *rcq;
	struct ibv_cq           *scq;
	struct ibv_comp_channel *ch;
	struct ibv_qp           *qp;
	struct ibv_sge           sge_list;
	struct ibv_send_wr       wr;
	void                    *buf;
	unsigned                 size;
	int                      sockfd;
	struct ib_connection     local_connection;
	struct ib_connection    *remote_connection;
};

struct user_config {
	char     *servername;
	int       sock_port;
	int       ib_dev;
	int       ib_port;
	unsigned  size;
	int       tx_depth;
};

static int init_user_config(JNIEnv *, jobject, struct user_config *);
static int init_rdma_context(JNIEnv *, struct rdma_context *, struct user_config *);
static int destroy(JNIEnv *, struct rdma_context *, struct user_config *);

static int qp_change_state_init(JNIEnv *, struct ibv_qp *, struct user_config *);
static int qp_change_state_rtr (JNIEnv *, struct rdma_context *, struct user_config *);
static int qp_change_state_rts (JNIEnv *, struct rdma_context *, struct user_config *);

static int set_local_ib_connection(JNIEnv *, struct rdma_context *, struct user_config *);
static int print_ib_connection(char *, struct ib_connection *);

static int rdma_write(JNIEnv *, struct rdma_context *);

static int tcp_server_listen(struct user_config *);
static int tcp_client_connect(struct user_config *);
static int tcp_exch_ib_connection_info(struct rdma_context *);

static int bpill(JNIEnv *, const char *);
static void throwException(JNIEnv *, const char *cls_name, const char *msg);

static const JNINativeMethod methods[] = {
	{     "rdmaContextInit", "(Lac/ncic/syssw/azq/JniExamples/RdmaUserConfig;)Ljava/nio/ByteBuffer;", (void *)rdmaContextInit     },
	{ "rdmaResourceRelease", "()V",                                                                   (void *)rdmaResourceRelease },
	{           "rdmaWrite", "()V",                                                                   (void *)rdmaWrite           },
	{            "rdmaRead", "()V",                                                                   (void *)rdmaRead            },
};

static struct rdma_context rctx = { 0 };
static struct user_config  ucfg = { 0 };

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

//	IBV_NZ(env, ibv_fork_init(), "ibv_fork_init failed!");

	return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *jvm, void *reserved)
{
}

JNIEXPORT jobject JNICALL rdmaContextInit(JNIEnv *env, jobject this, jobject userConfig)
{
	if (!userConfig) {
		throwException(env, "Ljava/lang/IllegalArgumentException;", "rdmaContextInit: NULL param!");
		return NULL;
	}

	srand48(getpid() * time(NULL));

	CHK_NZN(init_user_config(env, userConfig, &ucfg));

	CHK_NZN(init_rdma_context(env, &rctx, &ucfg));

	if (ucfg.servername) rctx.sockfd = tcp_client_connect(&ucfg);
	else rctx.sockfd = tcp_server_listen(&ucfg);
	tcp_exch_ib_connection_info(&rctx);
	print_ib_connection("local  connection", &rctx.local_connection);
	print_ib_connection("remote connection", rctx.remote_connection);

	if (ucfg.servername) qp_change_state_rtr(env, &rctx, &ucfg);
	else qp_change_state_rts(env, &rctx, &ucfg);

	close(rctx.sockfd);

	if (ucfg.servername)
		return (*env)->NewDirectByteBuffer(env, rctx.buf + rctx.size, rctx.size);
	else
		return (*env)->NewDirectByteBuffer(env, rctx.buf, rctx.size);
}

JNIEXPORT void JNICALL rdmaResourceRelease(JNIEnv *env, jobject this)
{
	if (destroy(env, &rctx, &ucfg))
//		exit(EXIT_FAILURE);
		;
}

JNIEXPORT void JNICALL rdmaWrite(JNIEnv *env, jobject this)
{
	if (rdma_write(env, &rctx))
//		exit(EXIT_FAILURE);
		;
}

JNIEXPORT void JNICALL rdmaRead(JNIEnv *env, jobject this)
{
}

// ----------------------------------------------------------------------------

static int init_user_config(JNIEnv *env, jobject userConfig, struct user_config *ucfg)
{
	jclass userConfigCls;
	jmethodID userConfigMethodId;
	jstring serverName;

	CHK_ZI(userConfigCls = (*env)->GetObjectClass(env, userConfig));

	CHK_ZI(userConfigMethodId = (*env)->GetMethodID(env, userConfigCls, "getServerName", "()Ljava/lang/String;"));
	serverName = (jstring)(*env)->CallObjectMethod(env, userConfig, userConfigMethodId);
	if (serverName) {
		ucfg->servername = (char *)(*env)->GetStringUTFChars(env, serverName, NULL);    // instance pinned, copy allocated: need be freed.
			// here's buggy: if GetStringUTFChars returns NULL, the program would consider herself as the server.
	}

	CHK_ZI(userConfigMethodId = (*env)->GetMethodID(env, userConfigCls, "getSocketPort", "()I"));
	ucfg->sock_port = (*env)->CallIntMethod(env, userConfig, userConfigMethodId);

	ucfg->ib_dev  = 0;    // note: pick your properly working IB device
	ucfg->ib_port = 1;    //       and port.

	CHK_ZI(userConfigMethodId = (*env)->GetMethodID(env, userConfigCls, "getBufferSize", "()I"));
	ucfg->size = (*env)->CallIntMethod(env, userConfig, userConfigMethodId);

	ucfg->tx_depth = 64;

	(*env)->DeleteLocalRef(env, userConfigCls);
	(*env)->ReleaseStringUTFChars(env, serverName, NULL);    // not sure whether or not this would work.

	if ((*env)->ExceptionOccurred(env)) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}

	return 0;
}

static int init_rdma_context(JNIEnv *env, struct rdma_context *rctx, struct user_config *ucfg)
{
	struct ibv_device **dev_list;
	struct ibv_device_attr dev_attr;
	struct ibv_port_attr port_attr;
	struct ibv_qp_init_attr qp_init_attr = { 0 };
	struct ibv_qp_attr qp_attr = { 0 };

	int i, num_devices;

	// check local IB devices.
	IBV_ZI(env, dev_list = ibv_get_device_list(&num_devices), "ibv_get_device_list failed!");
	printf("found %d devices.\n", num_devices);
	for (i = 0; i < num_devices; i++) {
		printf("\t%d: %s 0x%" PRIx64 "\n", i, ibv_get_device_name(dev_list[i]), ibv_get_device_guid(dev_list[i]));
	}

	// open IB device and check port.
	printf("using device %d.\n", ucfg->ib_dev);
	rctx->dev = dev_list[ucfg->ib_dev];
	IBV_ZI(env, rctx->dev_ctx = ibv_open_device(rctx->dev), "ibv_open_device failed!");
	IBV_NZI(env, ibv_query_device(rctx->dev_ctx, &dev_attr), "ibv_query_device failed!");
	printf("device has %d port(s).\n", dev_attr.phys_port_cnt);
	printf("\tusing port %d.\n", ucfg->ib_port);
	IBV_NZI(env, ibv_query_port(rctx->dev_ctx, ucfg->ib_port, &port_attr), "ibv_query_port failed!");
	if (port_attr.state != IBV_PORT_ACTIVE) {
		throwException(env, "Lac/ncic/syssw/azq/JniExamples/RdmaException;", "IB port down!");
		return -1;
	} else {
		printf("\tport %d status: %s.\n", ucfg->ib_port, ibv_port_state_str(port_attr.state));
	}

	// allocate a protection domain.
	IBV_ZI(env, rctx->pd = ibv_alloc_pd(rctx->dev_ctx), "ibv_alloc_pd failed!");

	// allocate and prepare the RDMA buffer.
	rctx->size = ucfg->size;
	IBV_NZI(env,
	        posix_memalign(&rctx->buf, sysconf(_SC_PAGESIZE), rctx->size*2),
	        "could not allocate buffer.");
	memset(rctx->buf, 0, rctx->size*2);

	// register the buffer and assoicate with the allocated protection domain.
	IBV_ZI(env,
	       rctx->mr = ibv_reg_mr(rctx->pd, rctx->buf, rctx->size*2, IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE),
	       "ibv_reg_mr failed! do you have root access?");

	// set up S/R completion queue and CQE notification receiving.
	IBV_ZI(env, rctx->ch = ibv_create_comp_channel(rctx->dev_ctx), "ibv_create_comp_channel failed!");
	IBV_ZI(env, rctx->rcq = ibv_create_cq(rctx->dev_ctx, 1, NULL, NULL, 0), "ibv_create_cq failed!");
	IBV_ZI(env, rctx->scq = ibv_create_cq(rctx->dev_ctx, ucfg->tx_depth, rctx, rctx->ch, 0), "ibv_create_cq failed!");

	// create the queue pair.
	qp_init_attr.send_cq = rctx->scq;
	qp_init_attr.recv_cq = rctx->rcq;
	qp_init_attr.qp_type = IBV_QPT_RC;
	qp_init_attr.cap.max_send_wr     = ucfg->tx_depth;
	qp_init_attr.cap.max_recv_wr     = 1;
	qp_init_attr.cap.max_send_sge    = 1;
	qp_init_attr.cap.max_recv_sge    = 1;
	qp_init_attr.cap.max_inline_data = 0;
	IBV_ZI(env, rctx->qp = ibv_create_qp(rctx->pd, &qp_init_attr), "ibv_create_qp failed!");

	// initialize QP and set status to INIT.
	qp_attr.qp_state        = IBV_QPS_INIT;
	qp_attr.pkey_index      = 0;
	qp_attr.port_num        = ucfg->ib_port;
	qp_attr.qp_access_flags = IBV_ACCESS_REMOTE_WRITE;
	IBV_NZI(env,
	        ibv_modify_qp(rctx->qp, &qp_attr,
	                      IBV_QP_STATE      |
	                      IBV_QP_PKEY_INDEX |
	                      IBV_QP_PORT       |
	                      IBV_QP_ACCESS_FLAGS),
	        "ibv_modify_qp failed!");

	// record local IB info to exchange with peer.
	rctx->local_connection.lid   = port_attr.lid;
	rctx->local_connection.qpn   = rctx->qp->qp_num;
	rctx->local_connection.psn   = lrand48() & 0xffffff;
	rctx->local_connection.rkey  = rctx->mr->rkey;
	rctx->local_connection.vaddr = (uintptr_t)rctx->buf + rctx->size;

	return 0;
}

static int destroy(JNIEnv *env, struct rdma_context *rctx, struct user_config *ucfg)
{
	if (ucfg->servername) (*env)->ReleaseStringUTFChars(env, NULL, ucfg->servername);

	if (rctx->qp) IBV_NZI(env, ibv_destroy_qp(rctx->qp), "ibv_destroy_qp failed!");
	if (rctx->scq) IBV_NZI(env, ibv_destroy_cq(rctx->scq), "ibv_destroy_cq (scp) failed!");
	if (rctx->rcq) IBV_NZI(env, ibv_destroy_cq(rctx->rcq), "ibv_destroy_cq (rcp) failed!");
	if (rctx->ch) IBV_NZI(env, ibv_destroy_comp_channel(rctx->ch), "ibv_destroy_comp_channel failed!");
	if (rctx->mr) IBV_NZI(env, ibv_dereg_mr(rctx->mr), "ibv_dereg_mr failed!");
	if (rctx->pd) IBV_NZI(env, ibv_dealloc_pd(rctx->pd), "ibv_dealloc_pd failed!");

	if (rctx->buf) free(rctx->buf);

//	printf("resource released.\n");

	return 0;
}

static int qp_change_state_init(JNIEnv *env, struct ibv_qp *qp, struct user_config *ucfg)
{
/*	struct ibv_qp_attr qp_attr = { 0 };

	qp_attr.qp_state        = IBV_QPS_INIT;
	qp_attr.pkey_index      = 0;
	qp_attr.port_num        = ucfg->ib_port;
	qp_attr.qp_access_flags = IBV_ACCESS_REMOTE_WRITE;

	IBV_NZI(env,
	        ibv_modify_qp(qp, &qp_attr,
	                      IBV_QP_STATE      |
	                      IBV_QP_PKEY_INDEX |
	                      IBV_QP_PORT       |
	                      IBV_QP_ACCESS_FLAGS),
	        "ibv_modify_qp failed!");

	return 0;*/
}

static int qp_change_state_rtr(JNIEnv *env, struct rdma_context *rctx, struct user_config *ucfg)
{
	struct ibv_qp_attr qp_attr = { 0 };

	qp_attr.qp_state              = IBV_QPS_RTR;
	qp_attr.path_mtu              = IBV_MTU_2048;
	qp_attr.dest_qp_num           = rctx->remote_connection->qpn;
	qp_attr.rq_psn                = rctx->remote_connection->psn;
	qp_attr.max_dest_rd_atomic    = 1;
	qp_attr.min_rnr_timer         = 12;
	qp_attr.ah_attr.is_global     = 0;
	qp_attr.ah_attr.dlid          = rctx->remote_connection->lid;
	qp_attr.ah_attr.sl            = 1;
	qp_attr.ah_attr.src_path_bits = 0;
	qp_attr.ah_attr.port_num      = ucfg->ib_port;

	IBV_NZI(env,
	        ibv_modify_qp(rctx->qp, &qp_attr,
	                      IBV_QP_STATE              |
	                      IBV_QP_AV                 |
	                      IBV_QP_PATH_MTU           |
	                      IBV_QP_DEST_QPN           |
	                      IBV_QP_RQ_PSN             |
	                      IBV_QP_MAX_DEST_RD_ATOMIC |
	                      IBV_QP_MIN_RNR_TIMER),
                "could not modify QP to RTR state!");

	return 0;
}

static int qp_change_state_rts(JNIEnv *env, struct rdma_context *rctx, struct user_config *ucfg)
{
	struct ibv_qp_attr qp_attr = { 0 };

	CHK_NI(qp_change_state_rtr(env, rctx, ucfg));

	qp_attr.qp_state      = IBV_QPS_RTS;
	qp_attr.timeout       = 14;
	qp_attr.retry_cnt     = 7;
	qp_attr.rnr_retry     = 7;
	qp_attr.sq_psn        = rctx->local_connection.psn;
	qp_attr.max_rd_atomic = 1;

	IBV_NZI(env,
	        ibv_modify_qp(rctx->qp, &qp_attr,
	                      IBV_QP_STATE     |
	                      IBV_QP_TIMEOUT   |
	                      IBV_QP_RETRY_CNT |
	                      IBV_QP_RNR_RETRY |
	                      IBV_QP_SQ_PSN    |
	                      IBV_QP_MAX_QP_RD_ATOMIC),
	        "could not modify QP to RTS state!");

	return 0;
}

static int set_local_ib_connection(JNIEnv *env, struct rdma_context *rctx, struct user_config *ucfg)
{
/*	struct ibv_port_attr attr;

	IBV_NZI(env,
	        ibv_query_port(rctx->dev_ctx, ucfg->ib_port, &attr),
	        "ibv_query_port failed!");

	rctx->local_connection.lid   = attr.lid;
	rctx->local_connection.qpn   = rctx->qp->qp_num;
	rctx->local_connection.psn   = lrand48() & 0xffffff;
	rctx->local_connection.rkey  = rctx->mr->rkey;
	rctx->local_connection.vaddr = (uintptr_t)rctx->buf + rctx->size;

	return 0;*/
}

static int print_ib_connection(char *conn_name, struct ib_connection *conn)
{
	printf("%s: LID %#04x, QPN %#06x, PSN %#06x, RKey %#08x, VAddr %#016Lx\n",
	       conn_name, conn->lid, conn->qpn, conn->psn, conn->rkey, conn->vaddr);
	return 0;
}

static int rdma_write(JNIEnv *env, struct rdma_context *rctx)
{
	struct ibv_send_wr *bad_wr;

	rctx->sge_list.addr   = (uintptr_t)rctx->buf;
   	rctx->sge_list.length = rctx->size;
   	rctx->sge_list.lkey   = rctx->mr->lkey;

  	rctx->wr.wr.rdma.remote_addr = rctx->remote_connection->vaddr;
	rctx->wr.wr.rdma.rkey        = rctx->remote_connection->rkey;
	rctx->wr.wr_id               = 3;
	rctx->wr.sg_list             = &rctx->sge_list;
	rctx->wr.num_sge             = 1;
	rctx->wr.opcode              = IBV_WR_RDMA_WRITE;
	rctx->wr.send_flags          = IBV_SEND_SIGNALED;
	rctx->wr.next                = NULL;

	IBV_NZI(env, ibv_post_send(rctx->qp, &rctx->wr, &bad_wr), "ibv_post_send failed, this is bad mkay!");

	return 0;
}

static int tcp_server_listen(struct user_config *ucfg)
{
	char *port;
	int n, connfd;
	int sockfd = -1;
	struct sockaddr_in sin;
	struct addrinfo *res, *t;
	struct addrinfo hints = {
		.ai_flags    = AI_PASSIVE,
		.ai_family   = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM };

	CHK_NI(asprintf(&port, "%d", ucfg->sock_port));
	CHK_NZI(n = getaddrinfo(NULL, port, &hints, &res));
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof n);
	bind(sockfd, res->ai_addr, res->ai_addrlen);
	printf("server: waiting for client to connect ...\n");
	listen(sockfd, 1);
	connfd = accept(sockfd, NULL, 0), "server accept failed.";

	free(port);
	freeaddrinfo(res);

	return connfd;
}

static int tcp_client_connect(struct user_config *ucfg)
{
	char *port;
	int sockfd = -1;
	struct sockaddr_in sin;
	struct addrinfo *res, *t;
	struct addrinfo hints = {
		.ai_family   = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM
	};

	CHK_NI(asprintf(&port, "%d", ucfg->sock_port));
	CHK_NZI(getaddrinfo(ucfg->servername, port, &hints, &res));

	printf("client: connecting to server ...\n");
	for (t = res; t; t = t->ai_next) {
		sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);
		connect(sockfd, t->ai_addr, t->ai_addrlen);
	}

	free(port);
	freeaddrinfo(res);

	return sockfd;
}

static int tcp_exch_ib_connection_info(struct rdma_context *rctx)
{
	char msg[sizeof "0000:000000:000000:00000000:0000000000000000"];
	int parsed;
	struct ib_connection *local = &rctx->local_connection;

	sprintf(msg, "%04x:%06x:%06x:%08x:%016Lx",
	              local->lid, local->qpn, local->psn, local->rkey, local->vaddr);

	if (write(rctx->sockfd, msg, sizeof msg) != sizeof msg) {
		perror("failed to send connection_details to peer.");
		return -1;
	}

	if (read(rctx->sockfd, msg, sizeof msg) != sizeof msg) {
		perror("failed to receive connection_details from peer.");
		return -1;
	}

	if (!rctx->remote_connection) free(rctx->remote_connection);

	rctx->remote_connection = malloc(sizeof(struct ib_connection));

	struct ib_connection *remote = rctx->remote_connection;

	parsed = sscanf(msg, "%x:%x:%x:%x:%Lx",
	                      &remote->lid, &remote->qpn, &remote->psn, &remote->rkey, &remote->vaddr);

	if (parsed != 5) {
		fprintf(stderr, "failed to parse message from peer.");
		free(rctx->remote_connection);
	}

	return 0;
}

static int bpill(JNIEnv *env, const char *msg)
{
	fprintf(stderr, "native error: %s - %s\n", strerror(errno), msg);
//	exit(EXIT_FAILURE);
	throwException(env, "Lac/ncic/syssw/azq/JniExamples/RdmaException;", msg);
	return -1;
}

static void throwException(JNIEnv *env, const char *cls_name, const char *msg)
{
	jclass cls = (*env)->FindClass(env, cls_name);
	if (cls) (*env)->ThrowNew(env, cls, msg);
	(*env)->DeleteLocalRef(env, cls);
}

