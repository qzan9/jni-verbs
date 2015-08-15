#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include <infiniband/verbs.h>

#define TEST_NZ(x,y) do { if ( (x) ) die(y); } while (0)    /* if x is NON-ZERO, error is printed */
#define TEST_Z(x,y)  do { if (!(x) ) die(y); } while (0)    /* if x is ZERO,     error is printed */
#define TEST_N(x,y)  do { if ((x)<0) die(y); } while (0)    /* if x is NEGATIVE, error is printed */

static int die(const char *reason);

int main(int argc, char *argv[])
{
	struct ibv_device **dev_list;
	struct ibv_context *ctx;
	struct ibv_device_attr device_attr;
	struct ibv_port_attr port_attr;
	int i, j, num_devices;

	TEST_NZ(ibv_fork_init(), "fork_init failed");

	TEST_Z(dev_list = ibv_get_device_list(&num_devices), "no IB-device available. get_device_list returned NULL");
	printf("get_device_list found %d devices.\n", num_devices);

	for (i = 0; i < num_devices; i++) {
		printf("\n%d: %s 0x%" PRIx64 "\n", i, ibv_get_device_name(dev_list[i]), ibv_get_device_guid(dev_list[i]));

		TEST_Z(ctx = ibv_open_device(dev_list[i]), "cannot create context. open_device returned NULL");

		TEST_NZ(ibv_query_device(ctx, &device_attr), "query_device failed");
		printf("\tfirmware version: %s\n", device_attr.fw_ver);
		printf("\tnode GUID: 0x%" PRIx64 "\n", device_attr.node_guid);
		printf("\tvendor ID: 0x%" PRIx32 "\n", device_attr.vendor_id);
		printf("\tnumber of physical ports: %" PRIu8 "\n", device_attr.phys_port_cnt);

		for (j = 1; j <= device_attr.phys_port_cnt; j++) {
			TEST_NZ(ibv_query_port(ctx, j, &port_attr), "query_port failed");
			printf("\t\tport %d\n", j);
			printf("\t\t\tstate active: %s (%d)\n", (port_attr.state == IBV_PORT_ACTIVE) ? "YES" : "NO", port_attr.state);
			printf("\t\t\tlocal ID: 0x%" PRIx16 "\n", port_attr.lid);
			printf("\t\t\tSM local ID: 0x%" PRIx16 "\n", port_attr.sm_lid);
		}

		TEST_NZ(ibv_close_device(ctx), "close_device failed.");
	}

	ibv_free_device_list(dev_list);

	return 0;
}

static int die(const char *reason)
{
	fprintf(stderr, "error: %s - %s.\n ", strerror(errno), reason);
	exit(EXIT_FAILURE);
	return -1;
}

