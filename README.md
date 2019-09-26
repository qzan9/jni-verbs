# JNI Verbs #

Verbs API for Java: take advantage of high-performance IB/RDMA through JNI.

InfiniBand refers to two distinctly different things:

* a physical link-layer protocol for InfiniBand networks, and

* a higher level programming API called the InfiniBand Verbs API, which is an implementation of RDMA technology.

## How to make Java RDMA-enabled? ##

* full JVM integration of `libibverbs` and `librdmacm`: modifications to JVM specification and implementation required.

* use JNI to attach to `libibverbs` and `librdmacm`

  - "struct-to-class, function-to-method" one-to-one mapping and wrapping: expose whole RDMA concepts (and native C stuff) to Java.

  - customize and simplify Java/JNI RDMA APIs for specific requirements and scenarios: easy to use and hide a lot native low-level details but less flexible.

* re-write user space OFED mid layer using purely Java.

  implement OFED I/O protocol and talk directly to `/dev/ib_verbs`: open the device file and communicate with kernel via RDMA ABI totally in Java.

## Environment Setup ##

### OFED RDMA Stack ###

check the kernel modules

    # lsmod | grep -i ib
    ib_addr                 6345  2 rdma_ucm,rdma_cm
    ib_ipoib               80819  0
    ib_cm                  36612  2 rdma_cm,ib_ipoib
    ib_uverbs              36354  9 rdma_ucm
    ib_umad                11800  6
    ib_qib                395717  0
    mlx4_ib               129782  1
    ib_sa                  23952  5 rdma_ucm,rdma_cm,ib_ipoib,ib_cm,mlx4_ib
    mlx4_core             213006  2 mlx4_en,mlx4_ib
    ib_mthca              134305  1
    ib_mad                 39175  6 ib_cm,ib_umad,ib_qib,mlx4_ib,ib_sa,ib_mthca
    ib_core                74153  17 rdma_ucm,rdma_cm,iw_cm,ib_ipoib,ib_cm,ib_uverbs,ib_umad,ocrdma,iw_nes,iw_cxgb4,iw_cxgb3,ib_qib,mlx5_ib,mlx4_ib,ib_sa,ib_mthca,ib_mad
    
    # lsmod | grep -i rdma
    rdma_ucm               16141  0
    rdma_cm                36810  1 rdma_ucm
    iw_cm                   8867  1 rdma_cm

verify that low-level HW driver (in this case `mlx4_core` and `mlx4_ib`) and mid-layer core (`ib_core`, `ib_uverbs`, etc.) are loaded. you can customize RDMA modules in kernel's build menu `Device Drivers` -> `InfiniBand support`.

under RHEL/CentOS 6.x, one lazy way to install OFED packages

    # yum update
    # yum groupinstall "Infiniband Support"
    # /etc/init.d/rdma start
    # /etc/init.d/opensmd start
    # chkconfig rdma on
    # chkconfig opensmd on
    # shutdown -r now

here are the packages provided by the "Infiniband Support"

    Group: Infiniband Support
     Description: Software designed for supporting clustering and grid connectivity using RDMA-based InfiniBand and iWARP fabrics.
     Mandatory Packages:
       libibcm
       libibverbs
       libibverbs-utils
       librdmacm
       librdmacm-utils
       rdma
     Default Packages:
       dapl
       ibacm
       ibsim
       ibutils
       libcxgb3
       libehca
       libibmad
       libibumad
       libipathverbs
       libmlx4
       libmlx5
       libmthca
       libnes
       rds-tools
     Optional Packages:
       compat-dapl
       glusterfs-rdma
       infiniband-diags
       libibcommon
       libocrdma
       mstflint
       opensm
       perftest
       qperf
       srptools

the **"must-install"** user-space packages include userspace driver (for Mellanox, it's `libmlx4` or `libmlx5` or `libmthca`) and verbs library (`libibverbs`). for development, `libibverbs-devel` should be installed.

other installation options include downloading OFED package tarball from [openfabrics.org][1] or using vendor-specific OFED distribution (such as MLNX-OFED).

to verify local RDMA device and query device attributes, use `ibv_devinfo` (contained in package `libibverbs-utils`), e.g.,

    $ ibv_devinfo 
    hca_id: mlx4_0
            transport:                      InfiniBand (0)
            fw_ver:                         2.34.5000
            node_guid:                      7cfe:9003:0099:3000
            sys_image_guid:                 7cfe:9003:0099:3003
            vendor_id:                      0x02c9
            vendor_part_id:                 4099
            hw_ver:                         0x1
            board_id:                       MT_1100120019
            phys_port_cnt:                  1
                    port:   1
                            state:                  PORT_ACTIVE (4)
                            max_mtu:                4096 (5)
                            active_mtu:             4096 (5)
                            sm_lid:                 2
                            port_lid:               1
                            port_lmc:               0x00
                            link_layer:             InfiniBand

to check the basic IB connection info, use `ibstat` or `ibstatus` (contained in package `infiniband-diags`), e.g.,

    $ ibstatus
    Infiniband device 'mlx4_0' port 1 status:
            default gid:     fe80:0000:0000:0000:7cfe:9003:0099:3001
            base lid:        0x1
            sm lid:          0x2
            state:           4: ACTIVE
            phys state:      5: LinkUp
            rate:            40 Gb/sec (4X QDR)
            link_layer:      InfiniBand

### JVM/JDK ###

Oracle/Sun HotSpot JVM 1.7 is recommended.

to configure Java environemnt, download the JDK tarball (from Oracle) and extract (to your home directory or some other locations) and set-up (`$JAVA_HOME`, `PATH`, etc.), or simply (under Ubuntu 14.04)

    $ sudo apt-add-repository ppa:webupd8team/java
    $ sudo apt-get update
    $ sudo apt-get install oracle-java7-installer
    $ sudo apt-get install oracle-java7-set-default

note that at least `JNI_VERSION_1_6` is required.

you can use Maven to compile the codes and build the jar; Eclipse and Idea would also be able to do the job.

## Current Implementations ##

current codes are primarily prototypes, tests or idea demonstrations (tried and true):

* IBV demos: `src/main/c/ibv_***`.

* JNI-RDMA quick proven: `src/main/c/libjnirdma` and `src/main/java/.../JniRdma`.

  the native IB verbs are organized into a couple of Java native methods.

  users are not required to have a deep knowledge of verbs programming, they just `rdmaInit()` to get a RDMA registered `ByteBuffer`, and invoke `rdmaWrite()` to perform RDMA operations when data in the `ByteBuffer` is ready. after done, call `rdmaFree()` to release native IBV resources.

  `src/mai/java/.../RunJniRdma` provides a set of micro-benchmarks for analyzing JNI/IB characteristics.

* JNI-Verbs case: `src/main/c/libjniverbs` and `src/main/java/.../JniVerbs`.

  a bunch of "struct-to-class, function-to-method" peer-to-peer wrappers of native verbs.

  this takes time to code, but is much more flexible, and a good understanding of RDMA is required.

to build the codes:

* use `make` (`Makefile`) to build native codes; remember to edit `inc/common.mk` to set up your own `JAVA_HOME`.

* use `mvn` (`pom.xml`) to compile Java codes and generate JAR packages, or import the codes into your IDE (Eclipse, Idea) to build.

* to execute by command line `java`, use property `-Djava.library.path` to specify `path/to/lib***.so`.

[1]: https://www.openfabrics.org

