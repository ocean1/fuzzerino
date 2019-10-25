all:
	rm -rf /dev/shm/gfzidfile
	../afl/bin/gfz-clang-fast test.c -o test
	rm -rf /dev/shm/gfzidfile
	../afl/bin/gfz-clang-fast test_float.c -o test_float
	$(CXX) unittest.cc -o unittest
	./unittest

clean:
	rm -f test test_float unittest locations.log gfz_num gfz_ptr gfz_bnc