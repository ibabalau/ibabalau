CC = gcc
CFLAGS = -g -fPIC -Wall

.PHONY: build
build: libscheduler.so

libscheduler.so: libscheduler.o prio_queue.o
	$(CC) -shared -o $@ $^

libscheduler.o: libscheduler.c
	$(CC) $(CFLAGS) -o $@ -c $< -lpthread

prio_queue.o: prio_queue.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	-rm -f prio_queue.o libscheduler.o libscheduler.so
