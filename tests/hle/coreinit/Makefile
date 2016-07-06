TARGETS := alarm coroutine

GROUP := $(notdir $(CURDIR))

all:
	@for dir in $(TARGETS); do \
		echo; \
		echo Entering Directory $$dir; \
		$(MAKE) --no-print-directory -f $(TEST_ROOT)/common/Makefile.tests -C $$dir; \
		echo Leaving Directory $$dir; \
	done

clean:
	@for dir in $(TARGETS); do \
		echo Cleaning $$dir; \
		$(MAKE) --no-print-directory -f $(TEST_ROOT)/common/Makefile.tests -C $$dir clean; \
	done

install:
	@for dir in $(TARGETS); do \
		echo Installing $$dir; \
		cp $$dir/*.rpx $(TEST_ROOT)/bin; \
	done

.PHONY: all install clean
