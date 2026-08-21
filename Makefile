# Re.Sell build
#
# Needs OpenSSL headers for the HTTPS client:
#   sudo apt install libssl-dev

CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Iinclude -I.
LDFLAGS  := -lssl -lcrypto -pthread

BIN      := main
SRCS     := main.cpp \
            auth/auth.cpp \
            utils/dotenv.cpp \
            clients/kicksdb_client.cpp \
            clients/kicksdb_parse.cpp \
            database/dbclient.cpp \
            plot/plot.cpp \
            models/hmc.cpp \
            models/utils.cpp
OBJS     := $(SRCS:.cpp=.o)

# Offline half of the project: no OpenSSL, no network
OFFLINE_SRCS := database/dbclient.cpp plot/plot.cpp models/hmc.cpp models/utils.cpp

.PHONY: all clean offline hmc-demo test

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile-check the parts that do not need OpenSSL installed
offline:
	$(CXX) $(CXXFLAGS) -fsyntax-only $(OFFLINE_SRCS)
	@echo "offline sources OK"

# Test suite. Deliberately free of OpenSSL/network, so it runs anywhere:
# the parsing layer lives in clients/kicksdb_parse.cpp, apart from the HTTP client.
TEST_SRCS := tests/main.cpp \
             tests/test_database.cpp \
             tests/test_hmc.cpp \
             tests/test_plot.cpp \
             tests/test_parse.cpp
TEST_DEPS := database/dbclient.cpp \
             plot/plot.cpp \
             models/hmc.cpp \
             models/utils.cpp \
             clients/kicksdb_parse.cpp

test: run_tests
	./run_tests $(FILTER)

run_tests: $(TEST_SRCS) $(TEST_DEPS)
	$(CXX) $(CXXFLAGS) -Itests $(TEST_SRCS) $(TEST_DEPS) -o run_tests

# Standalone sampler check against a known distribution
hmc-demo:
	$(CXX) $(CXXFLAGS) -DHMC_DEMO models/hmc.cpp models/utils.cpp -o hmc-demo
	./hmc-demo

clean:
	rm -f $(OBJS) $(BIN) hmc-demo run_tests models/a.out
