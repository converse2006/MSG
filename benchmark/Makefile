CC	:= gcc
AS	:= $(CROSS)as
LD	:= $(CROSS)ld
INCLUDE := -I../msg-source/include -I../msg-source/x86/include
CFLAGS	:= -Wall -lrt -O3 -lm -DPRINTx -DERROR_CHECKx
LDFLAGS	:= -L../msg-source/x86/lib
LIB	:= -lmsg -lpthread
MAIN	:= ping-pong

include make.defs

all:
	$(CC) -fno-stack-protector $(CFLAGS) $(INCLUDE) $(LDFLAGS) $(MAIN).c -o $(MAIN) $(LIB)
clean:
	rm *.o $(MAIN)

