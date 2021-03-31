CC= gcc
GCCFLAGS= -std=c99 -Wall -g -fsanitize=address
GCCLIBS= -lrt

SOURCES= $(wildcard *.c)
BINS=$(SOURCES:.c=.out)

all: $(BINS)
	for file in *.out; do mv $${file} $${file%.*} ;done 

%.out: %.c
	$(CC) $(GCCFLAGS) $^ -o $@ $(GCCLIBS)

clean:
	rm -rf slave master vision output.txt

cleanTest:
	rm -rf report.tasks PVS-Studio.log strace_out cppoutput.txt *.valgrind

test: clean 
	pvs-studio-analyzer trace -- make  > /dev/null
	pvs-studio-analyzer analyze > /dev/null 2> /dev/null
	plog-converter -a '64:1,2,3;GA:1,2,3;OP:1,2,3' -t tasklist -o report.tasks PVS-Studio.log  > /dev/null
	valgrind --leak-check=full -v ./master files/*  2> master.valgrind; valgrind --leak-check=full -v ./vision  2> vision.valgrind; cppcheck --quiet --enable=all --force --inconclusive . 2> cppoutput.txt

.PHONY: all clean test cleanTest