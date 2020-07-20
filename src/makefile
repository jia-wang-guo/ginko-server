DEBUG = 0
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2

endif

ginko: main.cpp  ginko.cpp  ../os/thread.cpp ../os/timer.cpp ../net/http.cpp ../db/sql.cpp -lpthread -lmysqlclient
	$(CXX) $(CXXFLAGS) $^ -o $@   

clean:
	rm -r ginko