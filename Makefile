CXX = g++
CXXFLAGS = -std=c++17 -Iinclude -I.
LDFLAGS = -lssl -lcrypto -pthread

SRCS = main.cpp auth/auth.cpp utils/dotenv.cpp clients/kicksdb_client.cpp
OBJS = $(SRCS:.cpp=.o)

TARGET = main

all: $(TARGET)



main.o: $(SRCS)
	$(CXX) $(CXXFLAGS) main.cpp -o $@ $(LDFLAGS)



auth.o: auth/auth.cpp utils/dotenv.hpp include/cpp-httplib/httplib.h include/nlohmann/json.hpp
	$(CXX) $(CXXFLAGS) -c auth/auth.cpp -o auth.o 



auth_dbug: auth/auth.cpp utils/dotenv.hpp include/cpp-httplib/httplib.h include/nlohmann/json.hpp
	$(CXX) $(CXXFLAGS) -g auth/auth.cpp -o $@ $(LDFLAGS) -g

kicksdb_dbug: clients/kicksdb_client.cpp clients/kicksdb_client.hpp utils/dotenv.hpp include/cpp-httplib/httplib.h include/nlohmann/json.hpp
	$(CXX) $(CXXFLAGS) -g clients/kicksdb_client.cpp utils/dotenv.cpp -o $@ $(LDFLAGS) -g
	


dotenv.o: utils/dotenv.cpp
	$(CXX) $(CXXFLAGS) -c utils/dotenv.cpp -o dotenv.o 



clean:
	rm -f $(OBJS) $(TARGET) auth_dbug
