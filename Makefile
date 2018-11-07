################################################################################
#                                                                              #
#                        Copyright 2018 TAM, Chun Pang                         #
#                                                                              #
#       The argparse project is covered by the terms of the MIT License.       #
#       See the file "LICENSE" for details.                                    #
#                                                                              #
################################################################################

CC = gcc
CCLD = $(CC)
AR = ar

CFLAGS = -MMD -Wall -Wextra -g $(EXTRA_CFLAGS)
CCLDFLAGS = $(CFLAGS)

_SRCS = argparse test
SRCS = $(addsuffix .c ,$(_SRCS))
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

LIB = libargparse.a
TEST_BIN = argparse_test

all: $(LIB)

$(LIB): argparse.o
	$(AR) rvs $@ $^

$(TEST_BIN): $(OBJS)
	$(CC) $(CCLDFLAGS) -o $@ $^

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(LIB) $(TEST_BIN) $(OBJS) $(DEPS)

.PHONY: all clean

-include $(DEPS)
