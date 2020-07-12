DEBUG = 1
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2
endif

http_test: http_test.cpp ../http.cpp ../../db/sql.cpp
	g++ $(CXXFLAGS) $^ -o $@ -lpthread -lmysqlclient

clean:
	rm -r http_test