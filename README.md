# JNI Verbs #

a JNI wrapper of native IB verbs API.

InfiniBand refers to two distinctly different things:

* a physical link-layer protocol for InfiniBand networks.

* a higher level programming API called the InfiniBand Verbs API, which is an
  implementation of RDMA technology.

# Environment Setup #

## OFED ##

check the kernel modules

```
    # lsmod | grep -i ib_
    ib_addr                 6345  2 rdma_ucm,rdma_cm
    ib_ipoib               80819  0
    ib_cm                  36612  2 rdma_cm,ib_ipoib
    ib_uverbs              36354  9 rdma_ucm
    ib_umad                11800  6
    ib_qib                395717  0
    ib_sa                  23952  5 rdma_ucm,rdma_cm,ib_ipoib,ib_cm,mlx4_ib
    ib_mthca              134305  1
    ib_mad                 39175  6 ib_cm,ib_umad,ib_qib,mlx4_ib,ib_sa,ib_mthca
    ib_core                74153  17 rdma_ucm,rdma_cm,iw_cm,ib_ipoib,ib_cm,ib_uverbs,ib_umad,ocrdma,iw_nes,iw_cxgb4,iw_cxgb3,ib_qib,mlx5_ib,mlx4_ib,ib_sa,ib_mthca,ib_mad

    # lsmod | grep -i rdma
    rdma_ucm               16141  0
    rdma_cm                36810  1 rdma_ucm
    iw_cm                   8867  1 rdma_cm
```

under RHEL/CentOS 6.x, one lazy way to install OFED packages

```
    # yum update
    # yum groupinstall "Infiniband Support"
    # chkconfig rdma on
    # chkconfig opensm on
    # shutdown -r now
```

(for Mellanox hardware) with the HCA driver and mid-layer core alread loaded
into the kernel, the "must-install" user-space packages are
`libmlx4`/`libmlx5`/`libmthca` and `libibverbs`. additional configurations may
need to be set up in `/etc/rdma/mlx4.conf` or `/etc/rdma/mlx5.conf` and
`/etc/modprobe.d/mlx4.conf` or `/etc/modprobe.d/mlx5.conf`.

