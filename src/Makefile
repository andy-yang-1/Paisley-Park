INCLUDE_PATH = -I/usr/share/liburing/src/include
LIBRARY_PATH = -L/usr/lib/x86_64-linux-gnu
LIB = -luring -lrt -laio

build:	
	clang io_call.c -o io_call $(INCLUDE_PATH) $(LIBRARY_PATH) $(LIB)

clean:
	rm -rf io_call
	rm -rf pid_file
	rm -rf test_file
