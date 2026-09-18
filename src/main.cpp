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
bool input_mode = false;
bool mev = false;
bool ascon = false;
bool noise_estimate = false;

bool noise_itob = false;
bool noise_btoi_itob = false;

void read_arguments(int argc, char* argv[]);
void random_operations(int bits);
void random_operations_batched(int bits);

void experiment_division(int bits);
void experiment_squareroot(int bits);
void experiment_hash_ascon();
void experiment_mev();
void experiment_noise_estimate();

void experiment_ItoB();
void experiment_BtoI_ItoB();

int main(int argc, char* argv[]) {
    read_arguments(argc, argv);

    // Con 13 levels 256-bits
    cc.generate_context_for_bootstrapping(1 << ring_size, 14);
    cc.generate_rotations_for_additions(wordsize * 2);
    cc.generate_rotations_for_multiplications(wordsize);
    cc.generate_rotations_for_bit_length(wordsize);
    cc.generate_precomputations_for_multiplications(wordsize, cc.get_context()->GetRingDimension());

    if (mev) {
        experiment_mev();
        exit(0);
    }

    if (ascon) {
        experiment_hash_ascon();
        exit(0);
    }

    if (noise_estimate) {
        experiment_noise_estimate();
        exit(0);
    }

    if (noise_itob) {
        experiment_ItoB();
        exit(0);
    }

    if (noise_btoi_itob) {
        experiment_BtoI_ItoB();
        exit(0);
    }


    /*
     * Experiments
     */
    //experiment_hash_ascon();
    //experiment_division(wordsize);
    //experiment_mev();

    /*
     * End experiments
     */

    if (test) {
        cout << "Keygen works, you are good to go to use the program :)" << endl;
        return 0;
    }

    if (input_mode) {
        random_operations(0);
    } else {
        random_operations_batched(wordsize);
    }
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

void experiment_noise_estimate() {
    vector<uint128_t> a;

    int bits = wordsize;

    for (uint32_t i = 0; i < cc.get_context()->GetRingDimension() / (bits * bits); i++) {
        a.push_back(0);
    }

    Ctxt c = cc.encrypt_multi_int(a, bits, 11);

    int zslots = cc.get_context()->GetRingDimension() / (bits * bits);

    Ctxt result = cc.binboot(cc.add_integer(c, c, bits, zslots));

    vector<double> result_vector = cc.decode(cc.decrypt(result));
    for (double & i : result_vector) {
        i = abs(i);
    }

    double average = accumulate(result_vector.begin(), result_vector.end(), 0.0)
                     / result_vector.size();

    average = -log2(average); //Error to precision bits
    double minimum = -log2(*max_element(result_vector.begin(), result_vector.end())); //We invert minimum and maximum as max error => min precision
    double maximum = -log2(*min_element(result_vector.begin(), result_vector.end()));

    cout << "Precision bits in addition (" << bits << " bits)" << endl;
    cout << "Average : " << average << endl;
    cout << "Minimum : " << minimum << endl;
    cout << "Maximum : " << maximum << endl << "*****" << endl;

    result = cc.eq_integer(c, c, bits, zslots);

    result_vector = cc.decode(cc.decrypt(result));

    //Slots in (relative) position 0 are equal to 1, let's correct them
    for (int i = 0; i < zslots; i++) result_vector[i * (bits * bits) / 2] -= 1;

    for (double & i : result_vector) {
        i = abs(i);
    }

    average = accumulate(result_vector.begin(), result_vector.end(), 0.0)
              / result_vector.size();

    average = -log2(average); //Error to precision bits
    minimum = -log2(*max_element(result_vector.begin(), result_vector.end())); //We invert minimum and maximum as max error => min precision
    maximum = -log2(*min_element(result_vector.begin(), result_vector.end()));

    cout << "Precision bits in equality (" << bits << " bits)" << endl;
    cout << "Average : " << average << endl;
    cout << "Minimum : " << minimum << endl;
    cout << "Maximum: " << maximum << endl << "*****" << endl;

    result = cc.mul_integer(c, c, bits, bits, zslots, zslots, true);

    result_vector = cc.decode(cc.decrypt(result));
    for (double & i : result_vector) {
        i = abs(i);
    }

    average = accumulate(result_vector.begin(), result_vector.end(), 0.0)
                     / result_vector.size();

    average = -log2(average); //Error to precision bits
    minimum = -log2(*max_element(result_vector.begin(), result_vector.end())); //We invert minimum and maximum as max error => min precision
    maximum = -log2(*min_element(result_vector.begin(), result_vector.end()));

    cout << "Precision bits in multiplication (" << bits << " bits)" << endl;
    cout << "Average : " << average << endl;
    cout << "Minimum : " << minimum << endl;
    cout << "Maximum: " << maximum << endl << "*****" << endl;

    result = cc.square_root_integer(c, bits, zslots);

    result_vector = cc.decode(cc.decrypt(result));
    for (double & i : result_vector) {
        i = abs(i);
    }

    average = accumulate(result_vector.begin(), result_vector.end(), 0.0)
              / result_vector.size();

    average = -log2(average); //Error to precision bits
    minimum = -log2(*max_element(result_vector.begin(), result_vector.end())); //We invert minimum and maximum as max error => min precision
    maximum = -log2(*min_element(result_vector.begin(), result_vector.end()));

    cout << "Precision bits in square root (" << bits << " bits)" << endl;
    cout << "Average : " << average << endl;
    cout << "Minimum : " << minimum << endl;
    cout << "Maximum : " << maximum << endl << "*****" << endl;


    result = cc.div_integer(c, c, bits, zslots);

    result_vector = cc.decode(cc.decrypt(result));
    for (double & i : result_vector) {
        i = abs(i);
    }

    average = accumulate(result_vector.begin(), result_vector.end(), 0.0)
              / result_vector.size();

    average = -log2(average); //Error to precision bits
    minimum = -log2(*max_element(result_vector.begin(), result_vector.end())); //We invert minimum and maximum as max error => min precision
    maximum = -log2(*min_element(result_vector.begin(), result_vector.end()));

    cout << "Precision bits in division (" << bits << " bits)" << endl;
    cout << "Average : " << average << endl;
    cout << "Minimum : " << minimum << endl;
    cout << "Maximum : " << maximum << endl << "*****" << endl;



    exit(0);
}

void experiment_noise_conversion() {
    vector<uint128_t> v;

    int zslots = cc.get_context()->GetRingDimension() / (wordsize * wordsize);

    for (int i = 0; i < zslots; i++ ) {
        /*
        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 225);
        int n = dist(rng);
        */
        v.push_back(5);
    }

    Ctxt zckks = cc.encrypt_multi_int(v, wordsize, startinglevel);

    vector<double> mask3;

    for (uint32_t i = 0; i < cc.get_context()->GetRingDimension() / 2; i++) {
        mask3.push_back(0.01);
    }

    zckks = cc.add(zckks, cc.encode(mask3, zckks->GetLevel()));


    cc.print(zckks, 128);

    vector<double> mask;

    for (int i = 0; i < zslots; i++) {
        mask.insert(mask.end(), {1, 2, 4, 8, 0, 0, 0, 0});
    }

    Ctxt rckks = cc.mult(zckks, mask);
    rckks = cc.add(rckks, cc.rot(rckks, 1));
    rckks = cc.add(rckks, cc.rot(rckks, 2));

    vector<double> mask2;

    for (int i = 0; i < zslots; i++) {
        mask2.insert(mask2.end(), {1/225.0, 0, 0, 0, 0, 0, 0, 0});
    }

    rckks = cc.mult(rckks, mask2);
    rckks = cc.add(rckks, cc.rot(rckks, -1));
    rckks = cc.add(rckks, cc.rot(rckks, -2));
    rckks = cc.add(rckks, cc.rot(rckks, -4));

    cc.print(rckks, 128);

    vector<vector<double>> coeffs;
    coeffs.push_back(read_vector_file("../coeffs/p1-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p2-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p3-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p4-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p5-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p6-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p7-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p8-norm-369.txt"));

    Ctxt resultpoly = cc.get_context()->EvalChebyshevSeriesPSBatchRepeated(rckks, coeffs, -1, 1, zslots);

    resultpoly = cc.binboot(resultpoly);

    cc.print(resultpoly, 128);

    cout << setprecision(20) << cc.decrypt(resultpoly)->GetRealPackedValue()[0] << endl;
}

void experiment_BtoI_ItoB() {
    vector<int> v;

    wordsize = 8;

    int zslots = cc.get_context()->GetRingDimension() / 2;

    for (int i = 0; i < zslots; i++ ) {
        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 1);
        int n = dist(rng);
        v.push_back(n);
    }

    Ctxt bckks = cc.encrypt(v, startinglevel);




    vector<double> initialNoise;
    for (int i = 0; i < cc.get_context()->GetRingDimension() / 2; i++) {
        std::random_device rd;
        std::mt19937 gen(rd());
        //By enlarging Delta we can make the noise larger, but we do not really care at this stage
        std::uniform_real_distribution<double> dist(-0.00001, 0.00001);

        double x = dist(gen);
        initialNoise.push_back(x);
    }

    bckks = cc.add(bckks, cc.encode(initialNoise));

    /*
     * B-to-I
     */
    vector<double> mask;

    for (int i = 0; i < zslots / 8; i++) {
        //Turning off 32 as 256-32=224, which is roughly the same interval as the one we use ([0, 225])
        mask.insert(mask.end(), {1, 2, 4, 8, 16, 0, 64, 128});
    }

    cc.print(bckks, 2048);

    Ctxt ickks = cc.mult(bckks, mask);

    cc.print(ickks, 2048);

    ickks = cc.add(ickks, cc.rot(ickks, 1));
    ickks = cc.add(ickks, cc.rot(ickks, 2));
    ickks = cc.add(ickks, cc.rot(ickks, 4));



    vector<double> mask2;



    for (int i = 0; i < zslots / 8; i++) {
        mask2.insert(mask2.end(), {2/225.0, 0, 0, 0, 0, 0, 0, 0});
    }

    ickks = cc.mult(ickks, mask2);

    cc.print(ickks, 2048);

    vector<double> mask3;
    for (int i = 0; i < zslots / 8; i++) {
        mask3.insert(mask3.end(), {-1, 0, 0, 0, 0, 0, 0, 0});
    }

    ickks = cc.add(ickks, cc.encode(mask3));

    cout << "PRIOR Input to polys: " << endl;
    cc.print(ickks, 2048);

    ickks = cc.add(ickks, cc.rot(ickks, -1));
    ickks = cc.add(ickks, cc.rot(ickks, -2));
    ickks = cc.add(ickks, cc.rot(ickks, -4));

    cout << "Input to polys: " << endl;
    cc.print(ickks, 2048);


    //I-to-B

    vector<vector<double>> coeffs;
    coeffs.push_back(read_vector_file("../coeffs/p1-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p2-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p3-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p4-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p5-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p6-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p7-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p8-norm-369.txt"));

    Ctxt resultpoly = cc.get_context()->EvalChebyshevSeriesPSBatchRepeated(ickks, coeffs, -1, 1,  cc.get_context()->GetRingDimension() / 16);

    resultpoly = cc.binboot(resultpoly);
    resultpoly = cc.clean(resultpoly);

    //cc.print(resultpoly, 128);

    vector<double> res = cc.decrypt(resultpoly)->GetRealPackedValue();
    vector<double> realres;
    for (auto i = 0; i < res.size(); i++) {
        if (res[i] > 0.5) realres.push_back(1); else realres.push_back(0);
    }

    cout << res << endl;

    double inf_norm = 0.0;
    for (size_t i = 0; i < res.size(); ++i) {
        inf_norm = std::max(inf_norm, std::abs(res[i] - realres[i]));
    }

    vector<double> initialres = cc.decrypt(bckks)->GetRealPackedValue();
    vector<double> initialrealres;
    for (auto i = 0; i < res.size(); i++) {
        if (initialres[i] > 0.5) initialrealres.push_back(1); else initialrealres.push_back(0);
    }

    double original_inf_norm = 0.0;
    for (size_t i = 0; i < res.size(); ++i) {
        original_inf_norm = std::max(original_inf_norm, std::abs(initialrealres[i] - initialres[i]));
    }

    cout << "Infinity norm from " << original_inf_norm << " to " << inf_norm << endl;
}

void experiment_ItoB() {
    vector<int> v;

    wordsize = 8;

    int zslots = cc.get_context()->GetRingDimension() / (2 * wordsize);

    for (int i = 0; i < zslots; i++ ) {
        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 225);
        int n = dist(rng);
        v.push_back(n);
    }

    vector<double> toBeEncoded;

    for (size_t i = 0; i < v.size(); i++) {
        double val = 2 * (v[i] / 225.0) - 1;
        for (int j = 0; j < 8; j++)
        toBeEncoded.push_back(val);
    }

    cout << toBeEncoded << endl;

    Ctxt zckks = cc.encrypt(toBeEncoded, startinglevel);

    vector<double> initialNoise;
    for (int i = 0; i < cc.get_context()->GetRingDimension() / 2; i++) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dist(-0.01, 0.01);

        double x = dist(gen);
        initialNoise.push_back(x);
    }

    //zckks = cc.add(zckks, cc.encode(initialNoise));

    //cc.print(zckks, 128);

    vector<vector<double>> coeffs;
    coeffs.push_back(read_vector_file("../coeffs/p1-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p2-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p3-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p4-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p5-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p6-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p7-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p8-norm-369.txt"));

    Ctxt resultpoly = cc.get_context()->EvalChebyshevSeriesPSBatchRepeated(zckks, coeffs, -1, 1,  cc.get_context()->GetRingDimension() / 16);

    //resultpoly = cc.binboot(resultpoly);

    //cc.print(resultpoly, 128);

    vector<double> res = cc.decrypt(resultpoly)->GetRealPackedValue();
    vector<double> realres;
    for (auto i = 0; i < res.size(); i++) {
        if (res[i] > 0.5) realres.push_back(1); else realres.push_back(0);
    }

    cout << res << endl;

    double inf_norm = 0.0;
    for (size_t i = 0; i < res.size(); ++i) {
        inf_norm = std::max(inf_norm, std::abs(res[i] - realres[i]));
    }

    double max_val = -std::numeric_limits<double>::infinity();
    for (double x : initialNoise)
        if (x > max_val) max_val = x;

    cout << "Infinity norm from " << max_val << " to " << inf_norm << endl;
}

void random_operations_batched(int bits) {
    int slots = cc.get_context()->GetRingDimension() / (bits * bits);

    vector<uint128_t> a;
    vector<uint128_t> b;

    Logger log(verbose);

    log.yellow_bold(1) << endl << "Running batched " << bits << "-bits operations experiment!" << endl;

    for (int i = 0; i < slots; i++) {
        a.push_back(random_number(bits));
        b.push_back(random_number(bits));
    }

    log(1) << "a: " << to_string_uint128(a) << endl << "b: " << to_string_uint128(b) << endl << endl;

    Ctxt c1 = cc.encrypt_multi_int(a, bits, startinglevel);
    Ctxt c2 = cc.encrypt_multi_int(b, bits, startinglevel);

    auto time = steady_clock::now();

    Ctxt csum = cc.binboot(cc.add_integer(c1, c2, bits));
    log.info(1) << "Addition (a + b)" << endl;
    log(2) << "Expected: " << to_string_uint128(add_simd(a, b)) << endl;
    log(2) << "Obtained: " << cc.print_ints(csum, bits + 1, slots) << endl;
    if (verbose >= 3) print_duration(time, "Addition took: ");
    log(1) << "-----" << endl;

    time = steady_clock::now();

    Ctxt csub = cc.binboot(cc.sub_integer(c1, c2, bits));
    log.info(1) << "Subtraction (a - b)" << endl;
    log(2) << "Expected: " << to_string_uint128(sub_simd(a, b, bits)) << endl;
    log(2) << "Obtained: " << cc.print_ints(csub, bits, slots) << endl;
    if (verbose >= 3) print_duration(time, "Subtraction took: ");
    log(1) << "-----" << endl;

    time = steady_clock::now();

    Ctxt ccomp = cc.binboot(cc.sub_integer(c1, c2, bits));
    log.info(1) << "Comparison (a ≥ b)" << endl;
    log(2) << "Expected: " << comp_simd(a, b) << endl;
    log(2) << "Obtained: " << last_bits(cc.decode(cc.decrypt(ccomp)), slots, bits) << endl;
    if (verbose >= 3) print_duration(time, "Comparison took: ");
    log(1) << "-----" << endl;

    time = steady_clock::now();

    Ctxt ceq = cc.eq_integer(c1, c2, bits, slots);
    log.info(1) << "Equality (a = b)" << endl;
    log(2) << "Expected: " << eq_simd(a, b) << endl;
    log(2) << "Obtained: " << first_bits(cc.decode(cc.decrypt(ceq)), slots, bits) << endl;
    if (verbose >= 3) print_duration(time, "Equality took: ");
    log(1) << "-----" << endl;

    time = steady_clock::now();

    Ctxt cmultmod = cc.mul_integer(c1, c2, bits, bits, slots, slots, false);

    log.info(1) << "Multiplication (a * b) % 2^n" << endl;
    log(2) << "Expected: " << to_string_uint128(mul_simd(a, b, bits)) << endl;
    log(2) << "Obtained: " << cc.print_ints(cmultmod, bits, slots) << endl;
    if (verbose >= 3) print_duration(time, "Multiplication took: ");
    log(1) << "-----" << endl;

    time = steady_clock::now();

    Ctxt cmult = cc.mul_integer(c1, c2, bits, bits, slots, slots, true);

    if (bits > 64) {
        log.info(1) << "Multiplication with overflow (a * b), Warning: results will be inaccurate as we only have access to uint128_t :-(" << endl;
    } else {
        log.info(1) << "Multiplication with overflow (a * b)" << endl;
    }
    log(2) << "Expected: " << to_string_uint128(mul_simd(a, b, 2 * bits)) << endl;
    log(2) << "Obtained: " << cc.print_ints(cmult, bits, slots, true) << endl;

    if (verbose >= 3) print_duration(time, "Multiplication took: ");
    log(1) << "-----" << endl;

    time = steady_clock::now();

    Ctxt cshift = cc.rot(cmult, -2);
    log.info(1) << "Logical shift (a * b << 2)" << endl;
    log(2) << "Expected: " << to_string_uint128(shift_simd(mul_simd(a, b, bits), 2)) << endl;
    log(2) << "Obtained: " << cc.print_ints(cshift, bits + 2, slots) << endl;
    if (verbose >= 3) print_duration(time, "Logical shift took: ");
    log(1) << "-----" << endl;

    time = steady_clock::now();


    Ctxt cdiv = cc.div_integer(c1, c2, bits, slots);
    log.info(1) << "Quotient (a / b)" << endl;
    log(2) << "Expected: " << to_string_uint128(div_simd(a, b)) << endl;
    log(2) << "Obtained: " << cc.print_ints(cdiv, bits, slots) << endl;
    if (verbose >= 3) print_duration(time, "Quotient took: ");
    log(1) << "-----" << endl;


    time = steady_clock::now();

    Ctxt csqrt = cc.square_root_integer(c1, bits, slots);
    log.info(1) << "Square root (sqrt(a))" << endl;
    log(2) << "Expected: " << to_string_uint128(sqrt_simd(a)) << endl;
    log(2) << "Obtained: " << cc.print_ints(csqrt, bits, slots) << endl;
    if (verbose >= 3) print_duration(time, "Square root took: ");
    log(1) << endl;
}

void random_operations(int bits) {
    //uint128_t a = random_number(bits);
    //uint128_t b = random_number(bits);

    cout << "Insert the desired number of bits (8, 16, 32, 64, 128, 256): " << endl;
    cin >> bits;

    if (bits != 8 && bits != 16 && bits != 32 && bits != 64 && bits != 128 && bits != 256) {
        cerr << "The amount of bits (" << wordsize << ") is not supported. Pick one out of (8, 16, 32, 64, 128, 256)" << endl;
        return;
    }

    string a_str, b_str;
    uint128_t a = 0, b = 0;

    cout << "Insert A: ";
    cin >> a_str;
    for (char c : a_str) {
        if (c >= '0' && c <= '9') {
            a = a * 10 + (c - '0');
        }
    }

    cout << "Insert B: ";
    cin >> b_str;
    for (char c : b_str) {
        if (c >= '0' && c <= '9') {
            b = b * 10 + (c - '0');
        }
    }

    Logger log(verbose);

    log(1) << endl
           << "Running single " << bits << "-bits operations experiment!"
           << endl;

    log(1) << "a: " << to_string_uint128(a) << ", b: " << to_string_uint128(b) << endl << endl;

    Ctxt c1 = cc.encrypt_single_int(a, bits, startinglevel);
    Ctxt c2 = cc.encrypt_single_int(b, bits, startinglevel);

    auto time = steady_clock::now();

    Ctxt csum = cc.binboot(cc.add_integer(c1, c2, bits));
    log(1) << "Addition (a + b)" << endl;
    log(2) << "Expected: " << to_string_uint128(a + b) << endl;
    log(2) << "Obtained: " << to_string_uint128(bits_to_int128(cc.decode(cc.decrypt(csum)), bits + 1)) << endl;
    if (verbose >= 1) print_duration(time, "Addition took: ");
    log(1) << "-----" << endl;

    Ctxt csub = cc.binboot(cc.sub_integer(c1, c2, bits));
    log(1) << "Subtraction (a - b)" << endl;
    log(2) << "Expected: " << to_string_uint128(a - b) << endl;
    log(2) << "Obtained: " << to_string_uint128(bits_to_int128(cc.decode(cc.decrypt(csub)), bits)) << endl;
    if (verbose >= 1) print_duration(time, "Addition took: ");
    log(1) << "-----" << endl;

    time = steady_clock::now();

    Ctxt ccomp = cc.binboot(cc.sub_integer(c1, c2, bits));
    log(1) << "Comparison (a ≥ b)" << endl;
    log(2) << "Expected: " << (a >= b) << endl;
    log(2) << "Obtained: " << cc.decode(cc.decrypt(ccomp))[bits] << endl;
    if (verbose >= 1) print_duration(time, "Comparison took: ");
    log(1) << "-----" << endl;

    time = steady_clock::now();

    Ctxt ceq = cc.eq_integer(c1, c2, bits, 1);
    log.info(1) << "Equality (a = b)" << endl;
    log(2) << "Expected: " << (a == b) << endl;
    log(2) << "Obtained: " << cc.decode(cc.decrypt(ceq))[0] << endl;
    if (verbose >= 3) print_duration(time, "Equality took: ");
    log(1) << "-----" << endl;

    time = steady_clock::now();

    Ctxt cmultmod = cc.mul_integer(c1, c2, bits, bits, 1, 1, false);

    log(1) << "Multiplication (a * b) % 2^n" << endl;
    log(2) << "Expected (" << bits << " bits): " << to_string_uint128((a * b) & ((uint128_t(1) << bits) - 1)) << endl;
    log(2) << "Obtained (" << bits << " bits): " << to_string_uint128(bits_to_int128(cc.decode(cc.decrypt(cmultmod)), bits)) << endl;
    if (verbose >= 1) print_duration(time, "Multiplication took: ");
    log(1) << "-----" << endl;

    time = steady_clock::now();

    Ctxt cmult = cc.mul_integer(c1, c2, bits, bits, 1, 1, true);

    if (bits > 64) {
        log(1) << "Multiplication with overflow (a * b), Warning: results will be inaccurate as we only have access to uint128_t :-(" << endl;
    } else {
        log(1) << "Multiplication with overflow (a * b)" << endl;
    }
    log(2) << "Expected (" << bits * 2 << " bits): " << to_string_uint128((uint128_t)(a * b)) << endl;
    log(2) << "Obtained (" << bits * 2 << " bits): " << to_string_uint128(bits_to_int128(cc.decode(cc.decrypt(cmult)), bits * 2)) << endl;
    if (verbose >= 1) print_duration(time, "Multiplication took: ");
    log(1) << "-----" << endl;

    time = steady_clock::now();

    Ctxt cshift = cc.rot(cmultmod, -2);

    log(1) << "Logical shift (a * b << 2)" << endl;
    log(2) << "Expected: " << to_string_uint128(((uint128_t)(a * b) << 2) & ((uint128_t(1) << bits) - 1)) << endl;
    log(2) << "Obtained: " << to_string_uint128(bits_to_int128(cc.decode(cc.decrypt(cshift)), bits + 2)) << endl;
    if (verbose >= 1) print_duration(time, "Logical shift took: ");
    log(1) << "-----" << endl;

    time = steady_clock::now();


    Ctxt cdiv = cc.div_integer(c1, c2, bits, 1);

    log(1) << "Quotient (a/b)" << endl;
    log(2) << "Expected: " << to_string_uint128(a / b) << endl;
    log(2) << "Obtained: " << to_string_uint128(bits_to_int128(cc.decode(cc.decrypt(cdiv)), bits)) << endl;
    if (verbose >= 1) print_duration(time, "Quotient took: ");
    log(1) << "-----" << endl;


    time = steady_clock::now();

    Ctxt csqrt = cc.square_root_integer(c1, bits, 1);

    log(1) << "Square root (sqrt(a))" << endl;
    log(2) << "Expected: " << to_string_uint128(sqrt(a)) << endl;
    log(2) << "Obtained: " << to_string_uint128(bits_to_int128(cc.decode(cc.decrypt(csqrt)), bits)) << endl;
    if (verbose >= 1) print_duration(time, "Square root took: ");




    log(1) << endl << endl;
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
        if (arg == "--test") {
            cout << "The program has been compiled and linked successfully, now checking if keygen works..." << endl;
            test = true;
        }
        if (arg == "--mev") {
            mev = true;
        }
        if (arg == "--hash") {
            ascon = true;
        }
        if (arg == "--noise") {
            noise_estimate = true;
        }
        if (arg == "--input") {
            input_mode = true;
        }
        if (arg == "--itob") {
            noise_itob = true;
        }
        if (arg == "--btoi-itob") {
            noise_btoi_itob = true;
        }
    }
}