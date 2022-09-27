CXX=g++
COMPILER_PREFIX=
FLAGS=-L$(OUTPUTDIR) -std=c++17 -lpthread -lwiringPi -lboost_system -ldl -lssl -latomic -lcrypto 
DEFINES=

ifdef SSL
DEFINES+=-DCROW_ENABLE_SSL
endif

ifndef RELEASE
DEBUG=-g3
$(shell mkdir -p intermediates/debug)
OUTPUTDIR ?=$(shell pwd)/intermediates/debug
FINALOUTPUT=build/debug
else
DEBUG=-O3
$(shell mkdir -p intermediates/release)
OUTPUTDIR ?=$(shell pwd)/intermediates/release
FINALOUTPUT=build/release
endif

export DEFINES
export FLAGS
export DEBUG
export OUTPUTDIR
export COMPILER_PREFIX

.PHONY:all
all:libwiringpi sqlite sha thermostat
	mkdir -p $(FINALOUTPUT)
	$(COMPILER_PREFIX)g++ -o $(FINALOUTPUT)/site $(shell ls $(OUTPUTDIR)/*.o) $(FLAGS)
	cp -r build/additional_files/* $(FINALOUTPUT)

.PHONY:sha
sha:
	$(MAKE) -C dependencies/includes/sha/

.PHONY:sqlite
sqlite:
	$(MAKE) -C dependencies/includes/sqlite/ 

.PHONY:libwiringpi
libwiringpi:
ifeq '$(shell ldconfig -p | grep libwiringPi)' ''
	$(MAKE) -C dependencies/includes/WiringPi/wiringPi/
	cp dependencies/includes/WiringPi/wiringPi/libwiringPi.so.* $(OUTPUTDIR)/libwiringPi.so
endif

.PHONY:thermostat
thermostat:
	$(MAKE) -C src/ all

.PHONY:clean
clean:
	rm -f intermediates/debug/*
	rm -f intermediates/release/*
	$(MAKE) -C dependencies/includes/WiringPi/wiringPi/ clean