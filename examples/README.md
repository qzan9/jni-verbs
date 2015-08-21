prototypes, tests and idea demonstrations: tried and true.

* IBV programming demos: `src/main/c/ibv_***`.

* JNI-RDMA quick proven: `src/main/c/libjnirdma` and `src/main/java/.../JniRdma`,
  the native IB verbs are organized into a couple of Java native methods.

  developers are not required to have a deep knowledge of verbs programming,
  they just `rdmaSetUp()` to get a RDMA registered `ByteBuffer`, and invoke
  `RdmaWrite()` to perform RDMA operations when data in the `ByteBuffer` is
  ready. after done, call `RdmaFree()` to release native IBV resources.

* JNI-Verbs case: `src/main/c/libjniverbs` and `src/main/java/.../JniVerbs`, a
  bunch of "struct-to-class, function-to-method" peer-to-peer wrappers of
  native verbs.

  this takes time to code, but is much more flexible, and good understanding of
  RDMA is required.

use `make` to build native codes; note that edit `inc/common.mk` to set up your
`JAVA_HOME`.

since `pom.xml` is provided, you can use `mvn` to build Java codes, or import
into your IDE (Eclipse, Idea) to compile.

to execute using command line `java`, remember to add `path/to/lib***.so` to
`-Djava.library.path`.

