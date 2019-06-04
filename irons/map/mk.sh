rm /dev/shm/gfzidfile
make -C ../../ clean
make -C ../../
rm /dev/shm/gfzidfile
GFZ_WHITE_LIST=1 ../../afl/bin/gfz-clang-fast -I. map_binary.c
