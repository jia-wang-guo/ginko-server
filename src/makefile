DEBUG = 0
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2

endif

all:ginko

ginko:main.cpp ginko.cpp \
        ../os/thread.cpp \
		../os/timer.cpp  \
		../net/http.cpp \
		../log/log.cpp   \
		../db/sql.cpp   
	$(CXX) $(CXXFLAGS) $^ -o $@  -lpthread -lmysqlclient

clean:
	rm -r ginko