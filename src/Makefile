
WARN = -Wall -g -O0
LIBS = $(shell pkg-config --cflags --libs libgphoto2 gtk+-2.0 libglade-2.0)
FLAGS = -std=c99 $(LIBS) $(WARN)

SOURCES = $(wildcard *.c)

OBJECTS = $(SOURCES:%.c=%.o)

all: capa

capa: $(OBJECTS)
	gcc $(FLAGS) -o $@ $^

%.o: %.c
	gcc $(FLAGS) -o $@ -c $<

clean:
	rm -f *.o capa *~ *.bak
