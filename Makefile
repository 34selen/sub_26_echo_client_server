all: echo-client echo-server

echo-client: echo-client.cpp
	g++ -std=c++11 -pthread -o echo-client echo-client.cpp

echo-server: echo-server.cpp
	g++ -std=c++11 -pthread -o echo-server echo-server.cpp

clean:
	rm -f *.o echo-client echo-server