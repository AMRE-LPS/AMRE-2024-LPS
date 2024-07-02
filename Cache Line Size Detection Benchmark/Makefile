# Makefile

# Directories and file names

TARGETS	:=	cache_linesize_benchmark 

#Default target
all: $(TARGETS)

cache_linesize_benchmark: cache_linesize_benchmark.cpp 
	g++ -lpthread -std=c++17 cache_linesize_benchmark.cpp -o cache_linesize_benchmark
run: 
	./cache_linesize_benchmark
clean:
	rm -rf $(OUTPUT_BINARY)
