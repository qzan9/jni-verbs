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
	int i, num_devices;

//	TEST_NZ(ibv_fork_init(), "fork_init failed.");

	TEST_Z(dev_list = ibv_get_device_list(&num_devices), "no IB-device available. get_device_list returned NULL.");

	printf("get_device_list found %d devices.\n", num_devices);
	for (i = 0; i < num_devices; i++)
		printf("%d: %s %" PRIx64 "\n", i, ibv_get_device_name(dev_list[i]), ibv_get_device_guid(dev_list[i]));
//		printf("%d: %s %" PRIu64 "\n", i, ibv_get_device_name(dev_list[i]), ibv_get_device_guid(dev_list[i]));

	ibv_free_device_list(dev_list);

	return 0;
}

static int die(const char *reason)
{
	fprintf(stderr, "error: %s - %s.\n ", strerror(errno), reason);
	exit(EXIT_FAILURE);
	return -1;
}

