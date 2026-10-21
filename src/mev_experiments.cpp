// The MEV back-run experiments (Uniswap v2/v3 back-run amounts, the bit decomposition, Ascon), kept apart from
// main.cpp so the library's own demo stays free of them. Built as the AdvancedFHE_MEV executable.
#include <iostream>
#include "openfhe.h"
#include "CKKSController.h"
#include "chrono"
#include <functional>
#include "Utils.h"
#include "Logger.h"

using namespace lbcrypto;
using namespace std;
using namespace chrono;

CKKSController cc;
int ring_size = 12;
int verbose = 3;
int wordsize = 32;

int startinglevel = 12;

bool test = false;
bool mev = false;
bool uniswapv3 = false;
bool decompose = false;
bool ascon = false;
bool division = false;
bool squareroot = false;

void read_arguments(int argc, char* argv[]);

void experiment_division(int bits);
void experiment_squareroot(int bits);
void experiment_hash_ascon();
void experiment_mev();
void experiment_uniswap_v3();
void experiment_decompose();

static steady_clock::time_point t_start = steady_clock::now();
static double setup_s = 0;

// Where the time goes, for comparison with the GPU port (which loads LUTs and encodes
// its plaintexts once, ahead of time). Printed at exit, parsed by run_benchmarks.sh.
static void print_profile() {
    auto secs = [](uint64_t ns) { return ns / 1e9; };
    fprintf(stderr, "[profile] total: %.3f s\n", duration<double>(steady_clock::now() - t_start).count());
    fprintf(stderr, "[profile] setup (context + keys + precomputations): %.3f s\n", setup_s);
    fprintf(stderr, "[profile] LUT file reads: %.3f s (%llu)\n", secs(lut_read_ns()), (unsigned long long)lut_read_files());
#ifdef OPENFHE_PROFILE_TIMERS
    const char* names[] = {"plaintext encode (all callers)", "Chebyshev PSBatch (incl. its encodes)", "StC-first bootstrap"};
    for (int i = 0; i < PROF_NSLOTS; i++)
        fprintf(stderr, "[profile] %s: %.3f s (%llu)\n", names[i], secs(ProfileCounters()[i].ns), (unsigned long long)ProfileCounters()[i].calls);
#endif
}

int main(int argc, char* argv[]) {
    read_arguments(argc, argv);
    atexit(print_profile);

    // Con 13 levels 256-bits
    cc.generate_context_for_bootstrapping(1 << ring_size, 14);
    cc.generate_rotations_for_additions(wordsize * 2);
    cc.generate_rotations_for_multiplications(wordsize);
    cc.generate_rotations_for_bit_length(wordsize);
    cc.generate_precomputations_for_multiplications(wordsize, cc.get_context()->GetRingDimension());
    setup_s = duration<double>(steady_clock::now() - t_start).count();

    if (mev) experiment_mev();
    else if (ascon) experiment_hash_ascon();
    else if (uniswapv3) experiment_uniswap_v3();
    else if (decompose) experiment_decompose();
    else if (division) experiment_division(wordsize);
    else if (squareroot) experiment_squareroot(wordsize);
    else if (test) cout << "Keygen works, you are good to go to use the program :)" << endl;
    else {
        cerr << "Pick an experiment: --uniswapv3, --mev, --decompose, --hash, --division or --sqrt" << endl;
        return 1;
    }
    return 0;
}

void experiment_hash_ascon() {
    int bits = 64;

    cc.generate_rotation_key(bits * bits / 2);
    cc.generate_rotation_key(5 * bits * bits / 2);
    cc.generate_rotation_key(4 * bits * bits / 2);
    cc.generate_rotation_key((cc.get_context()->GetRingDimension() / (bits * bits) - 5) * bits * bits / 2);
    cc.generate_rotation_keys({19, 28, 61, 39, 1, 6, 10, 17, 7, 41});

    int a = 12;
    int b = 12;
    uint64_t rate = 8;
    int taglen = 256;

    /*
     * Offline part
     */

    uint64_t iv[40] = {2, 0, (uint8_t)((b<<4)+a), (uint8_t)(taglen&0xFF), (uint8_t)(taglen>>8), rate, 0, 0,
                      0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};

    uint64_t S[5];
    for(int w = 0; w < 5; w++){
        S[w] = 0;
        for(int i = 0; i < 8; i++)
            S[w] |= (uint64_t)iv[8*w+i] << (i*8);
    }

    ascon_permutation(S, 12);

    vector<uint128_t> S_vector;
    S_vector.push_back(S[0]);
    S_vector.push_back(S[1]);
    S_vector.push_back(S[2]);
    S_vector.push_back(S[3]);
    S_vector.push_back(S[4]);

    for (uint32_t i = 0; i < cc.get_context()->GetRingDimension() / (bits * bits) - 5; i++) {
        S_vector.push_back(0);
    }

    Ctxt Sctxt = cc.encrypt_multi_int(S_vector, bits, startinglevel);

    /*
     * Online phase
     */

    std::string message = "67";
    int msg_len = message.size();

    // m_padding
    std::vector<uint8_t> m_padding(rate - (msg_len % rate), 0x00);
    m_padding[0] = 0x01;

    // m_padded
    std::vector<uint8_t> m_padded(message.begin(), message.end());
    m_padded.insert(m_padded.end(), m_padding.begin(), m_padding.end());

    // bytes_to_int (little-endian)
    uint64_t m_int = 0;
    for (uint32_t i = 0; i < m_padded.size(); i++)
        m_int |= (uint64_t)m_padded[i] << (i * 8);

    cout << m_int << endl;

    /*
     * Assuming m_int to occupy 8 bytes
     */

    /*
     * CLEAR VERSION
     */
    S[0] ^= m_int;
    ascon_permutation(S, 12);


    /*
     * FHE VERSION
     */
    vector<uint128_t> M_vector;
    M_vector.push_back(m_int);

    for (uint32_t i = 0; i < cc.get_context()->GetRingDimension() / (bits * bits) - 1; i++) M_vector.push_back(0);

    Ctxt Mctxt = cc.encrypt_multi_int(M_vector, bits, startinglevel);

    // XOR
    Sctxt = cc.square(cc.sub(Sctxt, Mctxt));

    //Sctxt = cc.binboot(Sctxt);

    cc.ascon_permutation(Sctxt, cc.get_context()->GetRingDimension() / (bits * bits));

    cout << "Obtained : " << cc.print_ints(Sctxt, bits, 5) << endl;
    cout << "Expected : " << S[0] << ",  " << S[1] << ", " << S[2] << ", " << S[3] << ", " << S[4] << endl;

}

void experiment_mev() {
    int bits = 128;

    vector<uint128_t> X;
    X.push_back(15187039806);

    vector<uint128_t> Y;
    Y.push_back(11870329);

    vector<uint128_t> ext_price;
    ext_price.push_back(1284000);

    vector<uint128_t> g;
    g.push_back(997);

    for (uint32_t i = 0; i < cc.get_context()->GetRingDimension() / (bits * bits) - 1; i++) {
        //Filling the rest of slots with zeroes
        X.push_back(0);
        Y.push_back(0);
        ext_price.push_back(0);
        g.push_back(0);
    }

    Ctxt X_ciph = cc.encrypt_multi_int(X, bits, 11);
    Ctxt Y_ciph = cc.encrypt_multi_int(Y, bits, 11);
    Ctxt ext_price_ciph = cc.encrypt_multi_int(ext_price, bits, 11);
    Ctxt g_ciph = cc.encrypt_multi_int(g, bits, 11);

    cout << "X: " << cc.print_ints(X_ciph, bits, 1) << endl <<
            "Y: " << cc.print_ints(Y_ciph, bits, 1) << endl <<
            "ext_price: " << cc.print_ints(ext_price_ciph, bits, 1) << endl <<
            "g: " << cc.print_ints(g_ciph, bits, 1) << endl;


    Ctxt term1 = cc.mul_integer(X_ciph, Y_ciph, bits, bits, 1, 1, false);
    Ctxt term2 = cc.mul_integer(ext_price_ciph, g_ciph, bits, bits, 1, 1, false);

    cout << "[X * Y]: " << cc.print_ints(term1, bits, 1) << ", [ext * g]: " << cc.print_ints(term2, bits, 1) << endl;

    Ctxt total = cc.mul_integer(term1, term2, bits, bits, 1, 1, false);

    cout << "[X * Y * ext * g]: " << cc.print_ints(total, bits, 1) << endl;
    cout << "Now the long one: computing the square root" << endl;

    Ctxt squareroot = cc.square_root_integer(total, bits, 1);

    cout << "[sqrt(X * Y * ext * g)]: " << cc.print_ints(squareroot, bits, 1) << endl;

    squareroot = cc.binboot(cc.sub_integer(squareroot, X_ciph, bits));

    cout << "[sqrt(X * Y * ext * g) - X]: " << cc.print_ints(squareroot, bits, 1) << endl;

    Ctxt result = cc.div_integer(squareroot, 997, bits, 1);

    cout << "[(sqrt(X * Y * g * ext) - X) / g]: " << cc.print_ints(result, bits, 1) << endl;
}

void experiment_decompose() {
    vector<int> random_inputs;

    std::random_device rd;
    std::mt19937 randgen(rd());

    std::uniform_int_distribution<int> dist(0, 255);

    int zslots = cc.get_context()->GetRingDimension() / (2 * 8);

    for (int i = 0; i < zslots; i++) {
        int val = dist(randgen);
        random_inputs.push_back(val);
        random_inputs.push_back(val);
        random_inputs.push_back(val);
        random_inputs.push_back(val);
        random_inputs.push_back(val);
        random_inputs.push_back(val);
        random_inputs.push_back(val);
        random_inputs.push_back(val);
    }

    Ctxt ciphertext = cc.encrypt(random_inputs);

    cout << "Preview of the first 100 elements: " << endl;
    cc.print(ciphertext, 100);

    /*
     * Decomposing via Chebyshev
     */
    vector<vector<double>> coeffs;
    coeffs.push_back(read_vector_file("../coeffs/Decompose8bits/p1-451.txt"));
    coeffs.push_back(read_vector_file("../coeffs/Decompose8bits/p2-451.txt"));
    coeffs.push_back(read_vector_file("../coeffs/Decompose8bits/p3-451.txt"));
    coeffs.push_back(read_vector_file("../coeffs/Decompose8bits/p4-451.txt"));
    coeffs.push_back(read_vector_file("../coeffs/Decompose8bits/p5-451.txt"));
    coeffs.push_back(read_vector_file("../coeffs/Decompose8bits/p6-451.txt"));
    coeffs.push_back(read_vector_file("../coeffs/Decompose8bits/p7-451.txt"));
    coeffs.push_back(read_vector_file("../coeffs/Decompose8bits/p8-451.txt"));

    auto time = steady_clock::now();

    Ctxt resultpoly = cc.get_context()->EvalChebyshevSeriesPSBatchRepeated(ciphertext, coeffs, 0, 255, zslots);
    resultpoly = cc.binboot(resultpoly);

    print_duration(time, "Decomposing + bootstrapping took: ");

    cout << "Result, decomposed: ";
    cc.print(resultpoly, 100);
    cout << endl;
}

void experiment_uniswap_v3() {
    /*
     * The code is poorly optimized, but the overall complexity is given by the following operations:
     *
     * 1x 128-bit multiplication
     * 1x 128-bit division as multiplication (N.b. in the code the division has been split in two as it wasn't working... will fix it)
     * 1x 64-bit addition
     * 1x 128-bit division
     * 1x 128-bit subtraction
     * 1x 128-bit multiplication
     * 1x 128-bit division as multiplication
     *
     * Using data from [Rov26], on a NVIDIA L40S this would theoretically require
     * 622ms + 622 ms + 57ms + 6.7s + 57ms + 622ms + 622ms = 9.7 seconds roughly
     *
     * On the same hardware, the same operations in TFHE would cost
     * 2073ms + 2073ms + 146ms + 36.8s + 146ms + 2073ms + 2037ms = 45.3 seconds roughly
     *
     * This would correpsond to a 4.6x improvement simply by changing the scheme
     *
     * (Notice that there is no difference between ciphertext/ciphertext and ciphertext/plaintext operations)
     */
    int bits = 128;

    vector<uint128_t> g_num_inv_L_fx;
    g_num_inv_L_fx.push_back(43315879362536242);

    vector<uint128_t> user_amount;
    user_amount.push_back(3750000000000000000);

    vector<uint128_t> g_den_Y_prec;
    g_den_Y_prec.push_back(static_cast<uint128_t>(1000000000000000000ULL) * 1000); // This is 1000000000000000000000

    vector<uint128_t> inv_sqrt_P0_fx;
    inv_sqrt_P0_fx.push_back(515557995982445430);

    // The precomputed numerator scaled by 2^32
    vector<uint128_t> numerator;
    numerator.push_back((static_cast<uint128_t>(1823591698684026ULL) << 64)
                          | 9043734471353827328ULL);

    vector<uint128_t> m_fx;
    m_fx.push_back((static_cast<uint128_t>(15188443ULL) << 64)
                   | 3405228930222675476ULL);

    vector<uint128_t> g_den;
    g_den.push_back(1000);

    vector<uint128_t> g_num;
    g_num.push_back(997);



    for (uint32_t i = 0; i < cc.get_context()->GetRingDimension() / (bits * bits) - 1; i++) {
        //Filling the rest of slots with zeroes
        g_num_inv_L_fx.push_back(0);
        user_amount.push_back(0);
        g_den_Y_prec.push_back(0);
        inv_sqrt_P0_fx.push_back(0);
        numerator.push_back(0);
        m_fx.push_back(0);
        g_den.push_back(0);
    }


    Ctxt g_num_inv_L_fx_ciph = cc.encrypt_multi_int(g_num_inv_L_fx, 128, 10);
    Ctxt user_amount_ciph = cc.encrypt_multi_int(user_amount, 128, 10);
    Ctxt g_den_Y_prec_ciph = cc.encrypt_multi_int(g_den_Y_prec, 128, 10);
    Ctxt inv_sqrt_P0_fx_ciph = cc.encrypt_multi_int(inv_sqrt_P0_fx, 128, 10);
    Ctxt numerator_ciph = cc.encrypt_multi_int(numerator, 128, 10);
    Ctxt m_fx_ciph = cc.encrypt_multi_int(m_fx, 128, 10);
    Ctxt g_den_ciph = cc.encrypt_multi_int(g_den, 128, 10);


    cout << "g_num_inv_L_fx_ciph: " << cc.print_ints(g_num_inv_L_fx_ciph, 128, 1) << endl <<
         "user_amount: " << cc.print_ints(user_amount_ciph, 128, 1) << endl <<
         "g_den_Y_prec: " << cc.print_ints(g_den_Y_prec_ciph, 128, 1) << endl <<
         "inv_sqrt_P0_fx: " << cc.print_ints(inv_sqrt_P0_fx_ciph, 128, 1) << endl <<
         "numerator: " << cc.print_ints(numerator_ciph, 128, 1) << endl;

    cout << "*****" << endl;
    // This is correct
    Ctxt term2_fx = cc.mul_integer(user_amount_ciph, g_num_inv_L_fx_ciph, 128, 128, 1, 1, false);

    // 1000000000000000000000 = 2^21 \cdot 5^21
    cc.generate_rotation_key(21);
    term2_fx = cc.rot(term2_fx, 21);

    // 5^21 = 476837158203125 = 9765625 * 48828125
    //term2_fx = cc.div_integer(term2_fx, 9765625, 128, 1);
    //term2_fx = cc.div_integer(term2_fx, 48828125, 128, 1);
    vector<int> reciprocal = {0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0};
    append_zeros(reciprocal, (int)(term2_fx->GetSlots() - reciprocal.size()));
    Ptxt p = cc.encode(reciprocal, term2_fx->GetLevel());
    term2_fx = cc.div_integer(term2_fx, p, 49, 1, 128, 1);



    // Appearantly the code fails if I divide directly with 476837158203125... must be an issue in the data represntation, whatever

    cout << "term2_fx: " << cc.print_ints(term2_fx, 64, 1) << endl;

    Ctxt u_fx = cc.binboot(cc.add_integer(term2_fx, inv_sqrt_P0_fx_ciph, 64));

    cout << "u_fx: " << cc.print_ints(u_fx, 64, 1) << endl;


    Ctxt X_post_fx = cc.binboot(cc.div_integer(numerator_ciph, u_fx, 128, 1));
    cc.generate_rotation_key(-32);
    X_post_fx = cc.rot(X_post_fx, -32);

    cout << "X_post_fx: " << cc.print_ints(X_post_fx, 128, 1) << endl;

    Ctxt diff_fx = cc.binboot(cc.sub_integer(m_fx_ciph, X_post_fx, 128));

    cout << "diff_fx: " << cc.print_ints(diff_fx, 128, 1) << endl;

    Ctxt amount_fx = cc.mul_integer(diff_fx, g_den_ciph, 128, 128, 1, 1, false);
    amount_fx = cc.div_integer(amount_fx, 997, 128, 1);

    cout << "Amount_fx: " << cc.print_ints(amount_fx, 128, 1) << endl;

}

void experiment_squareroot(int bits) {
    vector<uint128_t> a;

    a.push_back(random_number(bits));

    for (uint32_t i = 0; i < cc.get_context()->GetRingDimension() / (bits * bits) - 1; i++) {
        a.push_back(random_number(bits));
    }

    cout << "Numbers:   " << to_string_uint128(a) << endl;

    Ctxt c = cc.encrypt_multi_int(a, bits, 11);

    int zslots = cc.get_context()->GetRingDimension() / (bits * bits);

    Ctxt result = cc.square_root_integer(c, bits, zslots);

    cout << "Obtained: " << cc.print_ints(result, bits, zslots) << endl;
    cout << "Expected: " << to_string_uint128(sqrt_simd(a)) << endl;

    exit(0);
}

void experiment_division(int bits) {
    vector<uint128_t> a;
    vector<uint128_t> b;

    a.push_back(random_number(bits));
    b.push_back(random_number(bits/2));

    cout << "Numerator:   " << to_string_uint128(a[0]) << endl;
    cout << "Denominator: " << to_string_uint128(b[0]) << endl;

    for (uint32_t i = 0; i < cc.get_context()->GetRingDimension() / (bits * bits) - 1; i++) {
        a.push_back(random_number(bits));
        b.push_back(random_number(bits/2));
    }

    Ctxt numerator = cc.encrypt_multi_int(a, bits, startinglevel);
    Ctxt denominator = cc.encrypt_multi_int(b, bits, startinglevel);

    int zslots = cc.get_context()->GetRingDimension() / (bits * bits);

    Ctxt result = cc.div_integer(numerator, denominator, bits, zslots);

    cout << "Expected: " << to_string_uint128(div_simd(a, b)) << endl;
    cout << "Obtained: " << cc.print_ints(result, bits, zslots, false) << endl;

    exit(0);
}

void read_arguments(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--ring" && i + 1 < argc) {
            ring_size = stoi(argv[i + 1]);
            ++i;
        }
        if (arg == "--verbose" && i + 1 < argc) {
            verbose = stoi(argv[i + 1]);
            ++i;
        }
        if (arg == "--bits" && i + 1 < argc) {
            wordsize = stoi(argv[i + 1]);

            if (wordsize != 8 && wordsize != 16 && wordsize != 32 && wordsize != 64 && wordsize != 128 &&
                wordsize != 256) {
                cerr << "The amount of bits (" << wordsize
                     << ") is not supported. Pick one out of (8, 16, 32, 64, 128, 256)" << endl;
            }

            ++i;
        }
        if (arg == "--test") test = true;
        if (arg == "--mev") mev = true;
        if (arg == "--uniswapv3") uniswapv3 = true;
        if (arg == "--hash") ascon = true;
        if (arg == "--decompose") decompose = true;
        if (arg == "--division") division = true;
        if (arg == "--sqrt") squareroot = true;
    }
}
