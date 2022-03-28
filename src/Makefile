CXX=g++

INCLUDES=-I../dependencies/includes/ -I../dependencies/includes/Crow/include/ -I../dependencies/includes/WiringPi/wiringPi/

FILES=app.cpp controller.cpp cookie.cpp display.cpp file.cpp main.cpp relay.cpp site.cpp
OJBFILES=$(addprefix $(OUTPUTDIR)/, $(FILES:.cpp=.o))



.PHONY:all
all:$(OJBFILES)

$(OUTPUTDIR)/%.o: %.cpp
	$(COMPILER_PREFIX)$(CXX) -c -o $@ $< $(INCLUDES) $(DEBUG)

app.cpp:app.h
controller.cpp:controller.h json_custom.h database.h
cookie.cpp:cookie.h
display.cpp:display.h
file.cpp:file.h
main.cpp:app.h
relay.cpp:relay.h
site.cpp:site.h