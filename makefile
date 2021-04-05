CC= gcc
GCCFLAGS= -std=c99 -Wall -g -D_XOPEN_SOURCE=700
# -fsanitize=address 
GCCLIBS= -lrt -lpthread
EXEC = Master Slave Vision

LIB_O = shm_lib.o sem_lib.o
LIB_H = shm_lib.h sem_lib.h
LIB_C = shm_lib.c sem_lib.c

SLAVE_O = slave.o
SLAVE_C = slave.c

MASTER_O = master.o
MASTER_C = master.c

VISION_O = vision.o
VISION_C = vision.c


SOURCES= $(wildcard *.c)
BINS=$(SOURCES:.c=.out)

all: $(EXEC)

# all: $(BINS)
# for file in *.out; do mv $${file} $${file%.*} ;done 
# %.out: %.c
#	$(CC) $(GCCFLAGS) $^ -o $@ $(GCCLIBS)

Slave: $(SLAVE_O) $(LIB_O) $(LIB_H)
	$(CC) $(GCCFLAGS) $(LIB_O) $(SLAVE_O) -o Slave $(GCCLIBS) $(LIB_H)


Master: $(MASTER_O) $(LIB_O) $(LIB_H)
	$(CC) $(GCCFLAGS) $(LIB_O) $(MASTER_O) -o Master $(GCCLIBS) $(LIB_H) 

Vision: $(VISION_O) $(LIB_O) $(LIB_H)
	$(CC) $(GCCFLAGS) $(LIB_O) $(VISION_O) -o Vision $(GCCLIBS) $(LIB_H) 


$(LIB_O): $(LIB_C) $(LIB_H)
	$(CC) $(GCCFLAGS) -c $(LIB_C) $(GCCLIBS)	

$(MASTER_O): $(MASTER_C)
	$(CC) $(GCCFLAGS) -c $(MASTER_C) $(GCCLIBS)

$(VISION_O): $(VISION_C)
	$(CC) $(GCCFLAGS) -c $(VISION_C) $(GCCLIBS)

$(SLAVE_O): $(SLAVE_C) $(SLAVE_H)
	$(CC) $(GCCFLAGS) -c $(SLAVE_C) $(GCCLIBS)	

clean:
	rm -rf $(LIB_O) $(MASTER_O) $(SLAVE_O) $(VISION_O) $(EXEC) results.txt

cleanTest:
	rm -rf report.tasks PVS-Studio.log strace_out cppoutput.txt *.valgrind

test: clean 
	pvs-studio-analyzer trace -- make
	pvs-studio-analyzer analyze
	plog-converter -a '64:1,2,3;GA:1,2,3;OP:1,2,3' -t tasklist -o report.tasks PVS-Studio.log  > /dev/null
	valgrind --leak-check=full -v ./Master files/*  2> Master.valgrind; valgrind --leak-check=full -v ./Master files/* > ./Vision  2> Vision.valgrind; cppcheck --quiet --enable=all --force --inconclusive . 2> cppoutput.txt
# valgrind --leak-check=full -v ./Master files/* > ./Vision 2> Master.valgrind
.PHONY: all clean test cleanTest