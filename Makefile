CXX=g++

WIRINGPI=dependencies/includes/WiringPi/wiringPi/
SHA256=dependencies/includes/sha/
FLAGS=-Ldependencies/libs/ -lsha256 -lpthread -lwiringPi -Idependencies/includes/ -I$(WIRINGPI)
CROSSCOMPILE=NO

CURRENT_DIR=$(shell pwd)
OUTPUT=build
FILES=cookie.cpp controller.cpp display.cpp file.cpp site.cpp relay.cpp app.cpp main.cpp
OJBFILES=$(FILES:.cpp=.o)

all: setup sha256 wiringPi main

setup:
	mkdir -p $(OUTPUT)

main: $(OJBFILES)
	$(CXX) -o $(OUTPUT)/site $(OJBFILES) $(FLAGS)

%.o: %.cpp
	$(CXX) -c $< $(FLAGS)

wiringPi:
	$(MAKE) -C $(WIRINGPI)
	mv $(WIRINGPI)*.so.* dependencies/libs/libwiringPi.so
	cp dependencies/libs/libwiringPi.so ./build/

sha256:
	$(MAKE) -C $(SHA256)
	mv $(SHA256)/libsha256.a dependencies/libs/

clean:
	$(MAKE) -C $(WIRINGPI) clean
	$(MAKE) -C $(SHA256) clean

	rm *.o
