//
// Created by Lorenzo on 10/12/25.
//

#ifndef DISCRETECKKS_CAR_UTILS_H
#define DISCRETECKKS_CAR_UTILS_H


#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <random>
#include <fstream>
#include <sstream>
#include "iostream"
#include <cstdint>


using namespace std;
using namespace std::chrono;

static std::vector<int> rot_256_bits = {512, 2048, 8192, 1024, 4100, 4096, 16384, 1, 8, 8200, 2, 4, 12, 8204, 21520, 2576, 16, 2580, 1048, 24, 21532, 28, 32, -992, -480, 1060, 2600, 2604, 20528, 2096, 2100, 56, 20540, 64, -1472, -4032, -1984, 68, 21576, 2120, 2124, 17488, 592, 21588, 596, 17500, 20584, 616, 620, 16496, 112, 20596, 116, 16508, 128, -6016, -16256, -8064, 17544, 136, 140, 5264, 17556, 5276, -352, 16552, 4272, 16564, 4284, 5320, 1232, 5332, 1244, 4328, 240, 4340, 252, 256, -24320, 1288, -240, 1300, 21280, 10528, 10532, 296, 21292, 308, 10552, 10556, 20288, 10048, 10052, 20300, 21336, 10072, 10076, 17248, 8544, 21348, 8548, 17260, 20344, 8568, 8572, 16256, 8064, 20356, 8068, 16268, 10640, -112, 10644, 17304, 8088, 8092, 5024, 2464, 17316, 2468, 10664, 5036, 10668, 10160, -80, 10164, 16312, 2488, 2492, 4032, 1984, 16324, 1988, 10184, -56, 4044, 10188, 8656, 8660, 5080, 2008, 2012, 992, 480, 5092, 484, 504, 8680, -24, 1004, 8684, 8176, -16, 8180, -12, -2, 4088, 508, -1};
static std::vector<int> rot_128_bits = {512, 2048, 8192, 1024, 4100, 4096, 1, 2, 8, 16384, 4, 12, 2576, 16, 2580, 1048, 24, 28, 32, -992, -480, 1060, 2600, 2604, 2096, 2100, 56, 64, -1472, -4032, -1984, 68, 2120, 2124, 592, 596, 4088, 616, 620, 112, 116, 128, -6016, 136, 140, 5264, 5276, -352, 4272, 4284, 5320, 1232, 5332, 1244, 4328, 240, 4340, 252, 256, 1288, -240, 1300, 296, 308, -112, 5024, 2464, 2468, 5036, -80, 2488, 2492, 4032, 1984, 1988, -56, 4044, 5080, 2008, 2012, 992, 480, 5092, 484, -24, 1004, -16, -12, -1, 504, 508, -2};
static std::vector<int> rot_64_bits = {128, 512, 2048, 8192, 256, 1024, 4096, 16384, 1288, 8, 136, 1, 12, 140, 2, 16, -240, -112, 1300, 4, 1048, 24, 28, 32, -352, -992, -480, 1060, 296, -80, 308, 56, 64, -1472, 68, -56, 1232, 592, 596, 1244, 992, 480, 484, 616, -24, 1004, 508, 620, 240, 112, -16, 116, -12, -2, 504, 252, -1};
static std::vector<int> rot_32_bits = {128, 512, 2048, 8192, 256, 1024, 4096, 16384, 8, 136, 1, 2, 12, 140, 16, -240, -112, 4, 24, 28, 32, -352, 296, -80, 308, 56, 64, 68, -56, -24, 240, 112, -16, 116, -12, -2, 252, -1};
static std::vector<int> rot_16_bits = {128, 512, 2048, 8192, 256, 1024, 4096, 16384, 8, 1, 2, 4, 12, 16, 24, 28, 32, -80, 56, 64, 68, -56, -24, -16, -12, -1, -2};
static std::vector<int> rot_8_bits = {128, 512, 2048, 8192, 256, 1024, 4096, 16384, 8, 1, 2, 4, 12, 16, 24, 28, 32, -80, 56, 64, 68, -56, -24, -16, -12, -1, -2};

static inline void print_duration(chrono::time_point<steady_clock, nanoseconds> start, const string &title) {
    auto ms = duration_cast<milliseconds>(steady_clock::now() - start);

    auto secs = duration_cast<seconds>(ms);
    ms -= duration_cast<milliseconds>(secs);
    auto mins = duration_cast<minutes>(secs);
    secs -= duration_cast<seconds>(mins);

    if (mins.count() < 1) {
        cout << title << secs.count() << ":" << ms.count() << " sec ⌛" << endl;
    } else {
        cout << title << mins.count() << "." << secs.count() << ":" << ms.count() << " ⌛"<< endl;
    }
}

static inline void print_duration_amortized(chrono::time_point<steady_clock, nanoseconds> start, const string &title, int quantity) {
    auto total = steady_clock::now() - start;

    // LATENCY
    auto ms = duration_cast<milliseconds>(total);
    auto secs = duration_cast<seconds>(ms);
    ms -= duration_cast<milliseconds>(secs);
    auto mins = duration_cast<minutes>(secs);
    secs -= duration_cast<seconds>(mins);

    if (mins.count() > 0) {
        cout << "⌛(Latency)   " << title
             << mins.count() << ":"
             << secs.count() << "."
             << ms.count() << "s" << endl;
    } else {
        cout << "⌛(Latency)   " << title
             << secs.count() << "."
             << ms.count() << "s" << endl;
    }

    // AMORTIZED (divisione sulla durata totale)
    auto amortized = total / quantity;

    ms = duration_cast<milliseconds>(amortized);
    secs = duration_cast<seconds>(ms);
    ms -= duration_cast<milliseconds>(secs);
    mins = duration_cast<minutes>(secs);
    secs -= duration_cast<seconds>(mins);

    if (mins.count() > 0) {
        cout << "⌛(Amortized) " << title
             << mins.count() << ":"
             << secs.count() << "."
             << ms.count() << "s" << endl;
    } else {
        cout << "⌛(Amortized) " << title
             << secs.count() << "."
             << ms.count() << "s" << endl;
    }
}

static inline std::vector<int> intToBitsLSB(uint128_t value, int digits) {
    std::vector<int> bits;

    for (int i = 0; i < digits; ++i) {
        bits.push_back((value >> i) & 1);
    }

    return bits;  // bit 0 first
}

static inline std::vector<int> intToBitsLSBprint(uint128_t value, int digits) {
    std::vector<int> bits;

    for (int i = 0; i < digits; ++i) {
        bits.push_back((value >> i) & 1);
    }

    return bits;  // bit 0 first
}

static inline uint128_t BitsLSBtoInt(const std::vector<double>& bits, int digits) {
    uint128_t value = 0;

    for (size_t i = 0; i < bits.size() && i < (size_t)digits; ++i) {
        if (bits[i] >= 0.5) {
            value |= (uint128_t(1) << i);  // shift sicuro fino a 128 bit
        }
    }

    return value;
}

static inline uint128_t BitsLSBtoInt(const std::vector<uint128_t>& bits, int digits) {
    uint128_t value = 0;

    for (size_t i = 0; i < bits.size() && i < (size_t) digits; ++i) {
        if (bits[i] != 0) {
            value |= (uint128_t(1) << i);
        }
    }

    return value;
}

static inline uint64_t bits_to_int64(const std::vector<double>& bits, int digits) {
    uint64_t value = 0;

    for (size_t i = 0; i < bits.size() && i < (size_t) digits; ++i) {
        if (bits[i] >= 0.5) {
            value |= (uint64_t(1) << i);
        }
    }

    return value;
}

static inline uint128_t bits_to_int128(const std::vector<double>& bits, int digits) {
    uint128_t value = 0;

    for (size_t i = 0; i < bits.size() && i < (size_t) digits; ++i) {
        if (bits[i] >= 0.5) {
            value |= (uint128_t(1) << i);
        }
    }

    return value;
}

static inline std::string uint128_to_string(uint128_t value) {
    if (value == 0) return "0";
    std::string result;
    while (value > 0) {
        result = char('0' + value % 10) + result;
        value /= 10;
    }
    return result;
}

static inline std::string uint128_to_string(vector<uint128_t> values) {
    std::string s = "[ ";
    for (long unsigned int i = 0; i < values.size(); i++) {
        s += uint128_to_string(values[i]);
        if (i != values.size() - 1) {
            s += ", ";
        }
    }
    s += " ]";

    return s;
}

static inline void append_zeros(std::vector<int>& v, int quantity)
{
    v.insert(v.end(), quantity, 0);
}

static inline uint128_t random_uint(std::mt19937_64 &gen, unsigned digits) {
    if (digits <= 0 || digits > 128) {
        throw std::invalid_argument("digits must be between 1 and 128");
    }

    uint64_t high = gen();
    uint64_t low  = gen();
    uint128_t result = (uint128_t(high) << 64) | uint128_t(low);

    // Applichiamo il mask per tenere solo i primi 'digits' bit
    if (digits < 128) {
        result &= (uint128_t(1) << digits) - 1;
    }
    return result;
}

static std::mt19937_64 gen(std::random_device{}());

static inline uint16_t r8()  { return static_cast<uint16_t>(gen() % 256); }
static inline uint16_t r16() { return static_cast<uint16_t>(gen()); }
static inline uint32_t r32() { return static_cast<uint32_t>(gen()); }
static inline uint64_t r64() { return gen(); }
static inline uint128_t r128() { return ( static_cast<uint128_t>(gen()) << 64) | gen();}

static inline uint128_t random_number(int bits) {
    if (bits == 8) return r8();
    if (bits == 16) return r16();
    if (bits == 32) return r32();
    if (bits == 64) return r64();
    if (bits == 128) return r128();
    else return r128();
}

static inline uint128_t random128()
{
    std::random_device rd;           // non-deterministic source
    std::mt19937_64 gen(rd());        // 64-bit generator

    uint128_t high = static_cast<uint128_t>(gen());
    uint128_t low  = static_cast<uint128_t>(gen());

    return (high << 64) | low;
}

static inline vector<double> read_vector_file(std::string filename) {
    std::ifstream infile(filename);

    if (!infile) {
        std::cerr << "Error opening file: " << filename << std::endl;
    }

    std::vector<double> numbers;
    std::string line;

    // Read the entire file line by line
    while (std::getline(infile, line)) {
        std::stringstream ss(line);
        std::string value;

        // Split line by commas
        while (std::getline(ss, value, ',')) {
            try {
                numbers.push_back(std::stod(value)); // convert string to double
            } catch (const std::invalid_argument& e) {
                std::cerr << "Invalid number: " << value << std::endl;
            }
        }
    }

    infile.close();

    return numbers;
}

inline unsigned bit_width(unsigned x) {
    unsigned w = 0;
    while (x) {
        x >>= 1;
        ++w;
    }
    return w;
}

static inline std::vector<double> mod256(const std::vector<double>& v) {
    std::vector<double> out;
    out.reserve(v.size());

    for (double x : v) {
        x = std::fmod(x, 256.0);
        if (x < -128.0)
            x += 256.0;
        else if (x >= 128.0)
            x -= 256.0;
        out.push_back(x);
    }
    return out;
}

static inline std::string to_string_uint128(__uint128_t v) {
    if (v == 0)
        return "0";

    std::string s;
    while (v > 0) {
        s.push_back((v % 10) + 48);
        v /= 10;
    }

    std::reverse(s.begin(), s.end());
    return s;
}

static inline std::string to_string_uint128(vector<uint128_t> v) {
    std::string s = "";
    for (size_t i = 0; i < v.size(); i++) {
        s += to_string_uint128(v[i]);
        if (i < v.size() - 1) {
            s += ", ";
        }
    }
    return s;
}

static inline int closest_pow2(int n) {
    if (n == 0) return 1;
    double log_val = std::log2(n);
    int exp = static_cast<int>(std::round(log_val));
    return 1 << exp;
}

static inline std::vector<uint128_t> ptxt_to_vec(vector<double> vec, int slots, int bits, bool doublebits = false) {
    vector<uint128_t> result;

    for (auto i = 0; i < slots; i++) {
        int a = (closest_pow2(bits) * closest_pow2(bits) / 2) * i;
        int b = a + bits;
        if (doublebits) b += bits; //The case of multiplications where the number of bits gets doubled

        vector<double> slice(vec.begin() + a, vec.begin() + b);

        if (doublebits) {
            result.push_back(bits_to_int128(slice, 2 * bits));
        } else {
            result.push_back(bits_to_int128(slice, bits));
        }
    }

    return result;
}


static inline std::vector<uint128_t> add_simd(vector<uint128_t> a, vector<uint128_t> b) {
    vector<uint128_t> result;

    for (size_t i = 0; i < a.size(); i++) {
        result.push_back(a[i] + b[i]);
    }

    return result;
}

static inline std::vector<uint128_t> sub_simd(vector<uint128_t> a, vector<uint128_t> b, int bits) {
    vector<uint128_t> result;

    for (size_t i = 0; i < a.size(); i++) {
        result.push_back((a[i] - b[i]) & (((uint128_t)1 << bits) - 1));
    }

    return result;
}

static inline std::vector<int> comp_simd(vector<uint128_t> a, vector<uint128_t> b) {
    vector<int> result;

    for (size_t i = 0; i < a.size(); i++) {
        if (a[i] >= b[i])
            result.push_back(1);
        else
            result.push_back(0);
    }

    return result;
}

static inline std::vector<int> eq_simd(vector<uint128_t> a, vector<uint128_t> b) {
    vector<int> result;

    for (size_t i = 0; i < a.size(); i++) {
        if (a[i] == b[i])
            result.push_back(1);
        else
            result.push_back(0);
    }

    return result;
}

static inline std::vector<uint128_t> shift_simd(vector<uint128_t> vec, int shift_index) {
    vector<uint128_t> result;

    for (size_t i = 0; i < vec.size(); i++) {
        result.push_back(vec[i] * 1 << shift_index);
    }

    return result;
}

static inline std::vector<double> last_bits(vector<double> vec, int slots, int bits) {
    vector<double> result;

    for (int i = 0; i < slots; i++) {
        int a = (closest_pow2(bits) * closest_pow2(bits) / 2) * i;
        int b = a + bits;

        result.push_back(round(vec[b]) + 0.0);
    }

    return result;
}

static inline std::vector<double> first_bits(vector<double> vec, int slots, int bits) {
    vector<double> result;

    for (int i = 0; i < slots; i++) {
        int a = (closest_pow2(bits) * closest_pow2(bits) / 2) * i;


        result.push_back(round(vec[a] + 0.0));
    }

    return result;
}

static inline std::vector<uint128_t> mul_simd(vector<uint128_t> a, vector<uint128_t> b, uint128_t bits) {
    vector<uint128_t> result;

    for (size_t i = 0; i < a.size(); i++) {
        if (bits == 128) {
            result.push_back(a[i] * b[i]);
        } else {
            result.push_back((a[i] * b[i]) & ((uint128_t(1) << bits) - 1));
        }
    }

    return result;
}

static inline std::vector<uint128_t> div_simd(vector<uint128_t> a, vector<uint128_t> b) {
    vector<uint128_t> result;

    for (size_t i = 0; i < a.size(); i++) {
        result.push_back(a[i] / b[i]);
    }

    return result;
}

static inline std::vector<uint128_t> div_simd(vector<uint128_t> a, uint128_t b) {
    vector<uint128_t> result;

    for (size_t i = 0; i < a.size(); i++) {
        result.push_back(a[i] / b);
    }

    return result;
}

static inline uint128_t isqrt(uint128_t n) {
    if (n == 0) return 0;
    uint128_t x = n;
    uint128_t y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    return x;
}

static inline std::vector<uint128_t> sqrt_simd(vector<uint128_t> a) {
    vector<uint128_t> result;

    for (size_t i = 0; i < a.size(); i++) {
        result.push_back(isqrt(a[i]));
    }

    return result;
}

//https://github.com/meichlseder/pyascon, but converted to C++
static inline void ascon_permutation(uint64_t S[5], int rounds) {
    auto rotr64 = [](uint64_t val, int r) -> uint64_t {
        return (val >> r) | ((val << (64 - r)));
    };


    for (int r = 12 - rounds; r < 12; r++) {
        // --- add round constants ---
        S[2] ^= (0xf0 - r * 0x10 + r * 0x1);

        // --- substitution layer ---
        S[0] ^= S[4];
        S[4] ^= S[3];
        S[2] ^= S[1];

        uint64_t T[5];
        for (int i = 0; i < 5; i++)
            T[i] = (~S[i]) & S[(i + 1) % 5];

        for (int i = 0; i < 5; i++)
            S[i] ^= T[(i + 1) % 5];

        S[1] ^= S[0];
        S[0] ^= S[4];
        S[3] ^= S[2];
        S[2] ^= 0xFFFFFFFFFFFFFFFFULL;

        // --- linear diffusion layer ---
        S[0] ^= rotr64(S[0], 19) ^ rotr64(S[0], 28);
        S[1] ^= rotr64(S[1], 61) ^ rotr64(S[1], 39);
        S[2] ^= rotr64(S[2],  1) ^ rotr64(S[2],  6);
        S[3] ^= rotr64(S[3], 10) ^ rotr64(S[3], 17);
        S[4] ^= rotr64(S[4],  7) ^ rotr64(S[4], 41);
    }
}





static inline vector<int> reciprocal(uint128_t b, int bits) {
    std::string cmd = "python3 ../src/Reciprocal.py " + to_string(bits) + " " + to_string_uint128(b);

    FILE* pipe = popen(cmd.c_str(), "r");

    if (!pipe) return vector<int>();

    char buffer[1024];
    std::string output;

    while (fgets(buffer, sizeof(buffer), pipe))
    output += buffer;

    pclose(pipe);


    // Parse [1, 0, 1, 0, ...]
    std::vector<int> bitsvector;

    for (char c : output) {
        if (c == '0' || c == '1')
            bitsvector.push_back(c - '0');
    }

    return bitsvector;
}




#endif //DISCRETECKKS_CAR_UTILS_H
