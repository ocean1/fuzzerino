git clone https://github.com/glennrp/libpng.git
mv libpng libpng-cov
cd libpng-cov
./configure CFLAGS='-fprofile-arcs -ftest-coverage' CXXFLAGS='-fprofile-arcs -ftest-coverage'
make -j4
cd contrib/libtests
gcc readpng.c ../../.libs/libpng16.a -lm -lz -fprofile-arcs -ftest-coverage -o readpng