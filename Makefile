OBJS := test/timing.o test/cl.o
CUDALIBS = /usr/local/cuda
CFLAGS = -O2 -I$(CUDALIBS)/include -iquote $(CURDIR) -c -g -Wall
LDFLAGS := -O2 -L$(CUDALIBS)/lib/x86_64 -lOpenCL -lm

OBJS_clTree = kma.o pma.o test/tb_clTree.o clArrayList.o
OBJS_kma = kma.o test/tb_kma.o
OBJS_clArrayList = pma.o kma.o clArrayList.o test/tb_clArrayList.o
OBJS_clQueue = clQueue.o test/tb_clQueue.o
OBJS_clIndexedQueue = clIndexedQueue.o test/tb_clIndexedQueue.o

clTree: $(OBJS) $(OBJS_clTree)
	gcc $(LDFLAGS) -o $@ $(OBJS) $(OBJS_$@)

kma: $(OBJS) $(OBJS_kma)
	gcc $(LDFLAGS) -o $@ $(OBJS) $(OBJS_$@)

clArrayList: $(OBJS) $(OBJS_clArrayList)
	gcc $(LDFLAGS) -o $@ $(OBJS) $(OBJS_$@)
	
clQueue: $(OBJS) $(OBJS_clQueue)
	gcc $(LDFLAGS) -o $@ $(OBJS) $(OBJS_$@)

clIndexedQueue: $(OBJS) $(OBJS_clIndexedQueue)
	gcc $(LDFLAGS) -o $@ $(OBJS) $(OBJS_$@)

%.o: %.c
	gcc $(CFLAGS) -o $@ $<

clean:
	rm -rf *.o test/*.o
