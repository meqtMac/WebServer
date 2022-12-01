CXX=g++
src = $(wildcard ./*.cpp)
objs = $(patsubst %.cpp, %.o, $(src))
target=test
$(target):$(objs)
	$(CXX) $^ -pthread -o $@
%.o:%.cpp
	$(CXX) -c $<  -pthread -o $@

.PHONY:clean
clean:
	rm $(objs) 
