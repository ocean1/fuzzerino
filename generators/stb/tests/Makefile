INCLUDES = -I..
CC = ../../../afl/bin/gfz-clang-fast
CFLAGS = -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -DSTB_DIVIDE_TEST
CPPFLAGS = -Wno-write-strings -DSTB_DIVIDE_TEST

#CFLAGS += -O -fsanitize=address

all: cleangfz
	$(CC) $(INCLUDES) $(CFLAGS) -DIWT_TEST image_write_test.c -lm -o image_write_test

cleangfz:
	rm -rf /dev/shm/gfzidfile

clean:
	rm -rf image_write_test
	rm -rf ../out
	rm -rf ../gfz.log
	rm -rf ../gfz_plot_data