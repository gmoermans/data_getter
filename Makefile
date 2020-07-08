OBJS	= base64.o mqtt.o tools.o main.o
SOURCE	= base64.cpp mqtt.cpp tools.cpp main.cpp
HEADER	= base64.h mqtt.h tools.h influxdb.hpp
OUT	= data_getter
CC	 = g++
FLAGS	 = -g3 -c -Wall -std=c++17
LFLAGS	 = -lpqxx -lmosquittopp -ljsoncpp -lpthread -lpthread

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

influxdb.o: influxdb.hpp
	$(CC) $(FLAGS) influxdb.hpp

base64.o: base64.cpp
	$(CC) $(FLAGS) base64.cpp

mqtt.o: mqtt.cpp
	$(CC) $(FLAGS) mqtt.cpp

tools.o: tools.cpp
	$(CC) $(FLAGS) tools.cpp

main.o: main.cpp
	$(CC) $(FLAGS) main.cpp


clean:
	rm -f $(OBJS) $(OUT)

debug: $(OUT)
	valgrind $(OUT)

valgrind: $(OUT)
	valgrind $(OUT)

valgrind_leakcheck: $(OUT)
	valgrind --leak-check=full $(OUT)

valgrind_extreme: $(OUT)
	valgrind --leak-check=full --show-leak-kinds=all --leak-resolution=high --track-origins=yes --vgdb=yes $(OUT)
