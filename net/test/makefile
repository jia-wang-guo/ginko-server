DEBUG = 0
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2
endif

all: http_test parse_line_test


http_test: http_test.cpp ../http.cpp ../../db/sql.cpp
	g++ $(CXXFLAGS) $^ -o $@ -lpthread -lmysqlclient

parse_line_test: parse_line_test.cpp
	g++ $(CXXFLAGS) $^ -o $@

clean:
	rm -r http_test parse_line_test