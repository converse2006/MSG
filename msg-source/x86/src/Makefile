MPICC	:= mpicc
CC	:= $(CROSS)gcc
AS	:= $(CROSS)as
LD	:= $(CROSS)ld
INCLUDE := -I../include/ -I../../include
CFLAGS	:= -Wall -DMSG_x86 -O3 -DMSG_DEBUG -DSET_AFFINITYx -DBARRIER_OPT -DBCAST_OPT2 -DSCATTER_OPT2 -DGATHER_OPT -DSERVICE_PROCESSORx -DREDUCE_OPT
LDFLAGS	:= -L../lib
LIB	:= --static
OBJS	:= msgi_runtime.o msgi_topology.o msgi_mm.o msgi_sbp.o msgi_process.o msgi_wtime.o msgi_service_processor.o \
	   msgi_send_recv.o msgi_bcast.o msgi_barrier.o msgi_gather_scatter.o msgi_reduce.o \
	   msg_init.o msg_exit.o msg_comm_rank.o msg_comm_size.o msg_wtime.o \
	   msg_send.o msg_recv.o msg_bcast.o msg_barrier.o msg_gather.o msg_scatter.o msg_reduce.o
LINK	:= libmsg.a

VPATH	:= ../../common

all: $(LINK)

$(LINK): $(OBJS)
	   ar rcs $@ $^
	   ranlib $@
	   mv $(LINK) ../lib
%.o:%.c
	$(CC) -c $(CFLAGS) $(INCLUDE) $(LDFLAGS) $(LIB) $< -o $@

clean:
	rm -f *.o
	rm ../lib/$(LINK)
