DEBUG = 0
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g -G
else
    CXXFLAGS += -O2

endif

ginko:
	$(CXX) -o $@ $^ $(CXXFLAGS) 

clean:
	rm -r ginko