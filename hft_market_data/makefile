CC = g++
CFLAGS = -O3 $(shell pkg-config --cflags libdpdk)
LDFLAGS = $(shell pkg-config --libs libdpdk)

APP = hft_market_data

all: $(APP)

$(APP): main.cpp
	$(CC) $(CFLAGS) main.cpp -o $(APP) $(LDFLAGS)

clean:
	rm -f $(APP)
