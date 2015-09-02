prototypes, tests and idea demonstrations: tried and true.

* IBV programming demos: `src/main/c/ibv_***`.

* JNI-RDMA quick proven: `src/main/c/libjnirdma` and `src/main/java/.../JniRdma`.

  the native IB verbs are organized into a couple of Java native methods.

  users are not required to have a deep knowledge of verbs programming, they
  just `rdmaInit()` to get a RDMA registered `ByteBuffer`, and invoke
  `rdmaWrite()` to perform RDMA operations when data in the `ByteBuffer` is
  ready. after done, call `rdmaFree()` to release native IBV resources.

* JNI-Verbs case: `src/main/c/libjniverbs` and `src/main/java/.../JniVerbs`.

  a bunch of "struct-to-class, function-to-method" peer-to-peer wrappers of
  native verbs.

  this takes time to code, but is much more flexible, and good understanding
  of RDMA is required.

use `make` (`Makefile`) to build native codes; remember to edit `inc/common.mk`
to set up your `JAVA_HOME`.

use `mvn` (`pom.xml`) to compile Java codes and generate JAR packages, or
import the codes into your IDE (Eclipse, Idea) to build.

to execute by command line `java`, use property `-Djava.library.path` to
specify `path/to/lib***.so`.

