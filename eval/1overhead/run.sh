cd map_binary
echo "(map_binary) no instrumentation:" `python -m timeit -s 'import os' 'os.system("./map_binary 1 2 3 > /dev/null 2>&1")' | awk '{print $6 " " $7}'`
echo "(map_binary) instrumented, no mutations:" `python -m timeit -s 'import os' 'os.system("./map_binary_gfz 1 2 3 > /dev/null 2>&1")' | awk '{print $6 " " $7}'`
cp maps/num ./gfz_num
cp maps/ptr ./gfz_ptr
echo "(map_binary) instrumented, mutations:" `python -m timeit -s 'import os' 'os.system("./map_binary_gfz 1 2 3 > /dev/null 2>&1")' | awk '{print $6 " " $7}'`
rm -rf gfz*
cd ..
echo
cd t1
echo "(t1) no instrumentation:" `python -m timeit -s 'import os' 'os.system("./gtest > /dev/null 2>&1")' | awk '{print $6 " " $7}'`
echo "(t2) instrumented, no mutations:" `python -m timeit -s 'import os' 'os.system("./gtest_gfz > /dev/null 2>&1")' | awk '{print $6 " " $7}'`
cp maps/num ./gfz_num
echo "(t3) instrumented, mutations:" `python -m timeit -s 'import os' 'os.system("./gtest_gfz > /dev/null 2>&1")' | awk '{print $6 " " $7}'`
rm -rf gfz*
cd ..
echo
cd t2
echo "(t2) no instrumentation:" `python -m timeit -s 'import os' 'os.system("./gtest > /dev/null 2>&1")' | awk '{print $6 " " $7}'`
echo "(t2) instrumented, no mutations:" `python -m timeit -s 'import os' 'os.system("./gtest_gfz > /dev/null 2>&1")' | awk '{print $6 " " $7}'`
cp maps/num ./gfz_num
cp maps/ptr ./gfz_ptr
echo "(t3) instrumented, mutations:" `python -m timeit -s 'import os' 'os.system("./gtest_gfz > /dev/null 2>&1")' | awk '{print $6 " " $7}'`
rm -rf gfz*
cd ..
echo
cd gif2apng
echo "(gif2apng) no instrumentation:" `python -m timeit -s 'import os' 'os.system("./gif2apng giftest > /dev/null 2>&1")' | awk '{print $6 " " $7}'`
echo "(gif2apng) instrumented, no mutations:" `python -m timeit -s 'import os' 'os.system("./gif2apng_gfz giftest > /dev/null 2>&1")' | awk '{print $6 " " $7}'`
cp maps/num ./gfz_num
cp maps/ptr ./gfz_ptr
echo "(gif2apng) instrumented, mutations:" `python -m timeit -s 'import os' 'os.system("./gif2apng_gfz giftest > /dev/null 2>&1")' | awk '{print $6 " " $7}'`
rm -rf gfz*
cd ..
echo
cd miniz
echo "(miniz) no instrumentation:" `python -m timeit -s 'import os' 'os.system("./ex6 16 16 > /dev/null 2>&1")' | awk '{print $6 " " $7}'`
echo "(miniz) instrumented, no mutations:" `python -m timeit -s 'import os' 'os.system("./ex6_gfz 16 16 > /dev/null 2>&1")' | awk '{print $6 " " $7}'`
cp maps/num ./gfz_num
cp maps/ptr ./gfz_ptr
echo "(miniz) instrumented, mutations:" `python -m timeit -s 'import os' 'os.system("./ex6_gfz 16 16 > /dev/null 2>&1")' | awk '{print $6 " " $7}'`
rm -rf gfz*