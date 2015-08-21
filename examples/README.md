prototypes, tests and idea demonstrations: tried and true.

* IBV programming demos: primarily `src/main/c/ibv_***`s.

* JNI-RDMA quick proven: `src/main/c/libjnirdma` and `src/main/java/.../JniRdma`,
  the C IBV codes are organized into a couple of Java native methods.

  users are not required to have a deep knowledge of verbs programming, they
  just `rdmaSetUp` to get a RDMA registered `ByteBuffer`, and invoke `RdmaWrite`
  to perform RDMA operations when data in the `ByteBuffer` is ready. When done,
  call `RdmaFree` to release native resources.

* JNI-Verbs case: `src/main/c/libjniverbs` and `src/main/java/.../JniVerbs`, a
  "struct-to-class, function-to-method" peer-to-peer wrapper of native verbs.

  this takes time to code, but is much more flexible, and good understanding of
  RDMA is required.

