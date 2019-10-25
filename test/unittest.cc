#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <cstring>
#include <array>
#include "../afl/config.h"
#include "../afl/debug.h"

/* Helper functions */

void gen_map(const char* type, int location, int value) {
    size_t size;
    const char* filename;
    uint16_t *__gfz_map_area;

    if (!strcmp(type, "num")) {
    
        size = GFZ_NUM_MAP_SIZE;
        filename = "./gfz_num";

        __gfz_map_area = (uint16_t*) calloc(size, 1);
    
    } else if (!strcmp(type, "ptr")) {
    
        size = GFZ_PTR_MAP_SIZE;
        filename = "./gfz_ptr";

        __gfz_map_area = (uint16_t*) calloc(size, 1);
        
    
    }

    int i = 0;
    for (i = 0; i < size / 2; ++i) {
        __gfz_map_area[i] = GFZ_KEEP_ORIGINAL;
    }

    __gfz_map_area[location] = value;

    FILE *map_file;

    if (!(map_file = fopen(filename, "wb")))
        PFATAL("Error! Could not open file %s", filename);

    if (fwrite(__gfz_map_area, size, 1, map_file) <= 0)
        PFATAL("Error! Could not write data to %s", filename);

    free(__gfz_map_area);
}

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

/* Numeric */

void keep_original() {
    gen_map("num", 0, GFZ_KEEP_ORIGINAL);
    std::string out = exec("./test 1 2 3");
    std::string expected = "1 2 3 3 4 5 -1 3 0"; // default output

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_KEEP_ORIGINAL: ok");
}

void plus_one() {
    gen_map("num", 4, GFZ_KEEP_ORIGINAL | GFZ_PLUS_ONE);
    std::string out = exec("./test 1 2 3");
    std::string expected = "1 2 3 4 4 5 -1 3 0"; // 3 + 1 = 4

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_PLUS_ONE: ok");
}

void minus_one() {
    gen_map("num", 5, GFZ_KEEP_ORIGINAL | GFZ_MINUS_ONE);
    std::string out = exec("./test 1 2 3");
    std::string expected = "1 2 3 3 3 5 -1 3 0"; // 4 - 1 = 3

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_MINUS_ONE: ok");
}

void interesting() {
    gen_map("num", 6, GFZ_KEEP_ORIGINAL | GFZ_INTERESTING_2);
    std::string out = exec("./test 1 2 3");
    std::string expected = "1 2 3 3 4 2053 -1 3 0"; // 5 + 2048 = 2053

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_INTERESTING_2: ok");
}

void plus_max() {
    gen_map("num", 7, GFZ_PLUS_MAX);
    std::string out = exec("./test 1 2 3");
    std::string expected = "1 2 3 3 4 5 2147483647 3 0"; // 2147483647 is the max positive signed 32 bit integer

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_PLUS_MAX: ok");
}

void plus_random() {
    gen_map("num", 8, GFZ_PLUS_RAND);
    std::string out = exec("./test 1 2 3");

    // "fake test": output is not checked because it is random,
    // but we still want to run the mutation to check for crashes and stuff
    OKF("GFZ_PLUS_RAND: ok");
}

/* Floats */

void keep_original_float() {
    gen_map("num", 0, GFZ_KEEP_ORIGINAL);
    std::string out = exec("./test_float 1 2 3");
    std::string expected = "1.000000 2.000000 3.000000 3.000000 4.000000 5.000000 -1.000000 3.000000 0.666667"; // default output

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_KEEP_ORIGINAL (float): ok");
}

void plus_one_float() {
    gen_map("num", 4, GFZ_KEEP_ORIGINAL | GFZ_PLUS_ONE);
    std::string out = exec("./test_float 1 2 3");
    std::string expected = "1.000000 2.000000 3.000000 4.000000 4.000000 5.000000 -1.000000 3.000000 0.666667"; // 3 + 1 = 4

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_PLUS_ONE (float): ok");
}

void minus_one_float() {
    gen_map("num", 5, GFZ_KEEP_ORIGINAL | GFZ_MINUS_ONE);
    std::string out = exec("./test_float 1 2 3");
    std::string expected = "1.000000 2.000000 3.000000 3.000000 3.000000 5.000000 -1.000000 3.000000 0.666667"; // 4 - 1 = 3

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_MINUS_ONE (float): ok");
}

void interesting_float() {
    gen_map("num", 6, GFZ_KEEP_ORIGINAL | GFZ_INTERESTING_3);
    std::string out = exec("./test_float 1 2 3");
    std::string expected = "1.000000 2.000000 3.000000 3.000000 4.000000 9.000000 -1.000000 3.000000 0.666667"; // 5 + 4 = 9

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_INTERESTING_3 (float): ok");
}

void plus_max_float() {
    gen_map("num", 7, GFZ_PLUS_MAX);
    std::string out = exec("./test_float 1 2 3");
    std::string expected = "1.000000 2.000000 3.000000 3.000000 4.000000 5.000000 340282346638528859811704183484516925440.000000 3.000000 0.666667"; // default output

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_PLUS_MAX (float): ok");
}

void plus_random_float() {
    gen_map("num", 8, GFZ_PLUS_RAND);
    std::string out = exec("./test_float 1 2 3");

    // "fake test": output is not checked because it is random,
    // but we still want to run the mutation to check for crashes and stuff
    OKF("GFZ_PLUS_RAND (float): ok");
}

/* Pointers */

void keep_original_ptr() {
    gen_map("ptr", 0, 1);
    std::string out = exec("./test");

    std::string expected = "argc != 4"; // default output

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_KEEP_ORIGINAL (ptr): ok");
}

void bitflip_1() {
    gen_map("ptr", 0, 450);
    std::string out = exec("./test");

    std::string expected = "`rgc != 4"; // 'a' is 01100001, flipping one LSB: 01100000, which is '`'

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_BITFLIP (1): ok");
}

void bitflip_2() {
    gen_map("ptr", 0, 386);
    std::string out = exec("./test");

    std::string expected = "brgc != 4"; // 'a' is 01100001, flipping two LSBs: 01100010, which is 'b'

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_BITFLIP (2): ok");
}

void bitflip_1_offset() {
    gen_map("ptr", 0, 1986);
    std::string out = exec("./test");

    std::string expected = "argb != 4"; // 'c' is 01100011, flipping one LSB: 01100010, which is 'b'

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_BITFLIP (1, offset 3): ok");
}

void bitflip_2_offset() {
    gen_map("ptr", 0, 4482);
    std::string out = exec("./test");

    std::string expected = "argc != 7"; // '4' is 00110100, flipping two LSBs: 00110111, which is '7'

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_BITFLIP (2, offset 8): ok");
}

void byteflip_1() {
    gen_map("ptr", 0, 196);
    std::string out = exec("./test");

    std::string expected = "\x9ergc != 4"; // 'a' is 01100001, flipping byte: 10011110, which is '\x9e'

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_BYTEFLIP (1): ok");
}

void byteflip_2() {
    gen_map("ptr", 0, 132);
    std::string out = exec("./test");

    std::string expected = "\x9e\x8dgc != 4"; // 'ar' is 01100001 01110010, flipping bytes: 10011110 10001101, which is '\x9e\x8d'

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_BYTEFLIP (2): ok");
}

void byteflip_1_offset() {
    gen_map("ptr", 0, 4292);
    std::string out = exec("./test");

    std::string expected = "argc != \xcb"; // ' ' is 00110100, flipping byte: 11001011, which is '\xcb'

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_BYTEFLIP (1, offset 7): ok");
}

void byteflip_2_offset() {
    gen_map("ptr", 0, 2564);
    std::string out = exec("./test");

    std::string expected = "argc \xde\xc2\xdf\xcb"; // '!= 4' is 00100001 00111101 00100000 00110100, flipping bytes: 11011110 11000010 11011111 11001011, which is '\xde\xc2\xdf\xcb'

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_BYTEFLIP (4, offset 5): ok");
}

void arith() {
    gen_map("ptr", 0, 200);
    std::string out = exec("./test");

    std::string expected = "ergc != 4"; // 'a' is 01100001, 'a' + 4 is 'e'

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_ARITH (3): ok");
}

void interesting_ptr() {
    // Not implemented
}

void custom() {
    gen_map("ptr", 0, 32);
    std::string out = exec("./test");

    std::string expected = "AAAAAAAAA"; // default custom buffer is all 'A's

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("GFZ_CUSTOM_BUF: ok");
}

/* Branches */

void branch() {

    uint8_t *__gfz_map_area = (uint8_t*) calloc(GFZ_BNC_MAP_SIZE, 1);

    int i = 0;
    for (i = 0; i < GFZ_BNC_MAP_SIZE; ++i) {
        __gfz_map_area[i] = GFZ_KEEP_ORIGINAL;
    }

    __gfz_map_area[0] = 2;

    FILE *map_file;

    if (!(map_file = fopen("./gfz_bnc", "wb")))
        PFATAL("Error! Could not open file %s", "./gfz_bnc");

    if (fwrite(__gfz_map_area, GFZ_BNC_MAP_SIZE, 1, map_file) <= 0)
        PFATAL("Error! Could not write data to %s", "./gfz_bnc");

    free(__gfz_map_area);

    remove("./gfz_ptr");

    std::string out = exec("./test 1 2 3");
    std::string expected = "argc != 4"; // argc check is inverted

    if (out.compare(expected))
        FATAL("Test failed! (expected: \"%s\", got: \"%s\")", expected.c_str(), out.c_str());
    else
        OKF("branch: ok");

}

/* Main */

int main() {
    OKF("Testing the instrumentation...");

    /* Numeric */

    keep_original();
    plus_one();
    minus_one();
    interesting();
    plus_max();
    plus_random();

    /* Floats */

    keep_original_float();
    plus_one_float();
    minus_one_float();
    interesting_float();
    plus_max_float();
    plus_random_float();

    /* Pointers */

    keep_original_ptr();
    bitflip_1();
    bitflip_2();
    bitflip_1_offset();
    bitflip_2_offset();
    byteflip_1();
    byteflip_2();
    byteflip_1_offset();
    byteflip_2_offset();
    arith();
    interesting_ptr();
    custom();

    /* Branches */

    branch();
}