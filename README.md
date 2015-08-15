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
    # lsmod | grep -i ib
    ib_addr                 6345  2 rdma_ucm,rdma_cm
    ib_ipoib               80819  0
    ib_cm                  36612  2 rdma_cm,ib_ipoib
    ib_uverbs              36354  9 rdma_ucm
    ib_umad                11800  6
    ib_qib                395717  0
    mlx5_ib                93490  0
    mlx5_core              77850  1 mlx5_ib
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
```

verify that `ib_uverbs` and low-level HW driver (in this case `mlx4_core`) are
loaded.

under RHEL/CentOS 6.x, one lazy way to install OFED packages

```
    # yum update
    # yum groupinstall "Infiniband Support"
    # chkconfig rdma on
    # chkconfig opensm on
    # shutdown -r now
```

with the HCA driver and mid-layer core already loaded into the kernel, the
"must-install" user-space packages are `libmlx4`/`libmlx5`/`libmthca` (for MLX
hardware) and `libibverbs`.

run `ibv_devices` to show the available RDMA devices in the local machine.

```
    # ibv_devices
        device                 node GUID
        ------              ----------------
        mlx4_0              0002c9030007d550
        mthca0              0002c90200282d08
```

run `ibv_devinfo` to open a device and query for its attributes. this verifies
that the user and kernel part of RDMA stack can work together. make sure that
at least one port is `PORT_ACTIVE`.

```
    # ibv_devinfo -d mlx4_0
    hca_id: mlx4_0
            transport:                      InfiniBand (0)
            fw_ver:                         2.7.000
            node_guid:                      0002:c903:0007:d550
            sys_image_guid:                 0002:c903:0007:d553
            vendor_id:                      0x02c9
            vendor_part_id:                 26428
            hw_ver:                         0xB0
            board_id:                       MT_0D80120009
            phys_port_cnt:                  2
                    port:   1
                            state:                  PORT_ACTIVE (4)
                            max_mtu:                4096 (5)
                            active_mtu:             4096 (5)
                            sm_lid:                 4
                            port_lid:               17
                            port_lmc:               0x00
                            link_layer:             InfiniBand

                    port:   2
                            state:                  PORT_DOWN (1)
                            max_mtu:                4096 (5)
                            active_mtu:             4096 (5)
                            sm_lid:                 0
                            port_lid:               0
                            port_lmc:               0x00
                            link_layer:             InfiniBand
```

