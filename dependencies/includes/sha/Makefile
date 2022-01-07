CXX=g++



sha256.a:sha256.o
	ar rcs libsha256.a sha256.o

sha256.o:sha256.cpp
	$(CXX) -c -o sha256.o sha256.cpp -O3
clean:
	rm *.o