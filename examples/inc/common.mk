# global directory layout
ROOT         := ../../../..
BIN          := $(ROOT)/../bin
INC          := $(ROOT)/inc
LIB          := $(ROOT)/lib
SRC          := $(ROOT)/src/main/c

# project directories and files
PROJECTDIR   := .
OBJDIR       := $(PROJECTDIR)/obj
OUTPUTDIR    := $(BIN)/examples

OBJFILES     := $(patsubst %.c,$(OBJDIR)/%.o,$(notdir $(CFILES))) \
                $(patsubst %.cpp,$(OBJDIR)/%.o,$(notdir $(CCFILES)))
TARGETFILE   := $(OUTPUTDIR)/$(PROJECT)

# Java path
JAVA_HOME    := /opt/jdk7

# compiler setup
AMD64 = $(shell uname -m | grep 64)
ifeq "$(strip $(AMD64))" ""
    ARCH     := -m32
else
    ARCH     := -m64
endif

ifeq ($(dbg),1)
    DEBUG    := -g -D_DEBUG
    CONFIG   := debug
else
    DEBUG    := -O2 -fno-strict-aliasing
    CONFIG   := release
endif

INCLUDES     := -I$(PROJECTDIR) -I$(INC)
ifeq ($(jni),1)
    INCLUDES += -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
endif

LIBRARIES    := -L$(LIB) -L$(OUTPUTDIR)
ifeq ($(rdma),1)
   LIBRARIES += -libverbs
endif

CC           := gcc
CFLAGS       := -fPIC -std=c99 $(ARCH) $(DEBUG) $(INCLUDES)
CXX          := g++
CXXFLAGS     := -fPIC $(ARCH) $(DEBUG) $(INCLUDES)

LINK         := g++
LDFLAGS      := -fPIC -Wl,-rpath -Wl,\$$ORIGIN
ifeq ($(shared),1)
     LDFLAGS += -shared
  TARGETFILE := $(addsuffix .so,$(TARGETFILE))
endif

# misc.
ifeq ($(quiet), 1)
    QUIET    := @
else
    QUIET    :=
endif

# build rules
.PHONY: all
all: build

.PHONY: build
build: $(TARGETFILE)

$(OBJDIR)/%.o: %.c $(DEPFILES)
	$(QUIET)$(CC) $(CFLAGS) -o $@ -c $<

$(OBJDIR)/%.o: %.cpp $(DEPFILES)
	$(QUIET)$(CXX) $(CXXFLAGS) -o $@ -c $<

$(TARGETFILE): $(OBJFILES) | $(OUTPUTDIR)
	$(QUIET)$(LINK) $(LDFLAGS) -o $@ $+ $(LIBRARIES)

$(OUTPUTDIR):
	mkdir -p $(OUTPUTDIR)

$(OBJFILES): | $(OBJDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

.PHONY: run
run: build
	$(QUIET)$(TARGETFILE)

.PHONY: tidy
tidy:
	$(QUIET)find . | egrep "#" | xargs rm -f
	$(QUIET)find . | egrep "\~" | xargs rm -f

.PHONY: clean
clean: tidy
	$(QUIET)rm -f $(OBJFILES)
	$(QUIET)rm -f $(TARGETFILE)

.PHONY: clobber
clobber: clean
	$(QUIET)rm -rf $(OBJDIR)
