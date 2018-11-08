################################################################################
#                                                                              #
#                           Copyright 2018 C. P. Tam                           #
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

NAME = argparse

_SRCS = $(NAME) test
SRCS = $(addsuffix .c ,$(_SRCS))
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

LIB = lib$(NAME).a
TEST_BIN = $(NAME)_test

all: $(LIB)

$(LIB): $(NAME).o
	$(AR) rvs $@ $^

$(TEST_BIN): test.o $(LIB)
	$(CC) $(CCLDFLAGS) -L. -o $@ $< -l$(NAME)

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(LIB) $(TEST_BIN) $(OBJS) $(DEPS)

.PHONY: all clean

-include $(DEPS)
