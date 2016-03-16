## Makefile

.PHONY: clean realclean

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    CC=g++
endif
ifeq ($(UNAME_S),Darwin)
    CC=clang++
endif

OUTNAME_BIN=simserv
BUILDDIR_BIN=../../bin
OBJDIR=../../obj

INCDIR=../core
COREDIR=$(INCDIR)/sibyl
COREDIR_HDRS=$(INCDIR)/sibyl

SRCDIR=./
SRCDIR_HDRS=./

LIBS=
LIBDIR=
LDFLAGS=

CPPFLAGS=-Wall -std=c++11
OPTFLAGS=-m64 -Ofast -flto -march=native -funroll-loops

#########################################################################################

INCLUDES+=$(patsubst %,-I%,$(INCDIR))
LDFLAGS+=$(patsubst %,-L%,$(LIBDIR))

CPPFLAGS+=$(OPTFLAGS)
LDFLAGS+=$(OPTFLAGS)

# COREDIR files
HDRS=$(wildcard $(COREDIR_HDRS)/**/**/*.h)
HDRS=$(wildcard $(COREDIR_HDRS)/**/*.h)
HDRS+=$(wildcard $(COREDIR_HDRS)/*.h)
HDRS+=$(wildcard $(COREDIR_HDRS)/**/**/*.hxx)
HDRS+=$(wildcard $(COREDIR_HDRS)/**/*.hxx)
HDRS+=$(wildcard $(COREDIR_HDRS)/*.hxx)
OBJS+=$(patsubst $(COREDIR)/%.cc,$(OBJDIR)/%.o, $(wildcard $(COREDIR)/**/**/*.cc))
OBJS+=$(patsubst $(COREDIR)/%.cc,$(OBJDIR)/%.o, $(wildcard $(COREDIR)/**/*.cc))
OBJS+=$(patsubst $(COREDIR)/%.cc,$(OBJDIR)/%.o, $(wildcard $(COREDIR)/*.cc))

# SRCDIR files
HDRS=$(wildcard $(SRCDIR_HDRS)/**/**/*.h)
HDRS=$(wildcard $(SRCDIR_HDRS)/**/*.h)
HDRS+=$(wildcard $(SRCDIR_HDRS)/*.h)
HDRS+=$(wildcard $(SRCDIR_HDRS)/**/**/*.hxx)
HDRS+=$(wildcard $(SRCDIR_HDRS)/**/*.hxx)
HDRS+=$(wildcard $(SRCDIR_HDRS)/*.hxx)
OBJS+=$(patsubst $(SRCDIR)/%.cc,$(OBJDIR)/%.o, $(wildcard $(SRCDIR)/**/**/*.cc))
OBJS+=$(patsubst $(SRCDIR)/%.cc,$(OBJDIR)/%.o, $(wildcard $(SRCDIR)/**/*.cc))
OBJS+=$(patsubst $(SRCDIR)/%.cc,$(OBJDIR)/%.o, $(wildcard $(SRCDIR)/*.cc))

TARGET_BIN=$(BUILDDIR_BIN)/$(OUTNAME_BIN)


all:$(TARGET_BIN)

$(TARGET_BIN):$(OBJS) 
	@mkdir -p $(@D)
	$(CC) -o $(TARGET_BIN)    $(LDFLAGS) $(OBJS) $(LIBS)

# dependencies
$(OBJDIR)/%.o:$(COREDIR)/%.cc $(HDRS)
	@mkdir -p $(@D)
	$(CC) -o $@    $(CPPFLAGS) $(INCLUDES) -c $<

$(OBJDIR)/%.o:$(SRCDIR)/%.cc $(HDRS)
	@mkdir -p $(@D)
	$(CC) -o $@    $(CPPFLAGS) $(INCLUDES) -c $<

## other options
clean:
	rm -rf $(OBJS)

realclean:
	rm -rf $(OBJDIR) $(TARGET_BIN) 

