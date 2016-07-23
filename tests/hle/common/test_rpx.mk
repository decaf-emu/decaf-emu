ifeq ($(strip $(WUT_ROOT)),)
$(error "Please ensure WUT_ROOT is in your environment.")
endif

ifeq ($(shell uname -o),Cygwin)
WUT_ROOT := $(shell cygpath -w ${WUT_ROOT})
BUILDDIR := $(shell cygpath -w ${CURDIR})
else
BUILDDIR := $(CURDIR)
endif

include $(WUT_ROOT)/rules/rpl.mk

LIBS     := -lcoreinit
CFLAGS   += -O2 -Wall -std=c11
CXXFLAGS += -O2 -Wall

# Cancel the elf rule from WUT_ROOT
%.elf: $(OFILES)

%.elf: %.o
	@echo "[LD]  $(notdir $@)"
	@$(LD) $^ $(LIBPATHS) $(LIBS) $(LDFLAGS) -o $@

CFILES   := $(wildcard *.c)
SFILES   := $(wildcard *.S)
OFILES   := $(CFILES:.c=.o) $(SFILES:.S=.o)
OUTELF   := $(OFILES:.o=.elf)
OUTRPX   := $(OFILES:.o=.rpx)

all: $(OFILES) $(OUTELF) $(OUTRPX)

clean:
	@echo $(GROUP)
	@echo "[RM] $(notdir $(OUTELF))"
	@rm -f $(OFILES) $(OUTELF) $(OUTRPX)
