#ifndef FLEXIBLE_INTS_CKKS_CKKSCONTROLLER_H
#define FLEXIBLE_INTS_CKKS_CKKSCONTROLLER_H

#include "openfhe.h"
#include "ciphertext-ser.h"
#include "scheme/ckksrns/ckksrns-ser.h"
#include "ciphertext-ser.h"
#include "cryptocontext-ser.h"
#include "key/key-ser.h"
#include <utility>
#include "Utils.h"

using namespace lbcrypto;
using namespace std;
using namespace std::chrono;

using Ptxt = Plaintext;
using Ctxt = Ciphertext<DCRTPoly>;

class CKKSController {
    CryptoContext<DCRTPoly> context;

public:
    CKKSController() {}

    void generate_context_for_bootstrapping(int ring, int levels);
    void generate_bootstrapping();
    void generate_rotation_key(int index);
    void generate_rotation_keys(vector<int> indexes);
    void generate_rotation_keys_inverse(vector<int> indexes);
    void generate_rotations_for_additions(int bits);
    void generate_rotations_for_multiplications(int bits);
    void generate_rotations_for_bit_length(int bits);

    Ptxt encode(const vector<double>& vec, int lvl = 0);
    Ptxt encode(const vector<int>& vec, int lvl = 0);
    Ptxt encode(double value, int lvl = 0);
    Ptxt encode_multi_int(vector<uint128_t> val, int bits, int lvl = 0);

    Ctxt encrypt(const vector<double>& vec);
    Ctxt encrypt(const vector<int>& vec);
    Ctxt encrypt(const vector<double>& vec, int lvl);
    Ctxt encrypt(const vector<int>& vec, int lvl);
    Ctxt encrypt_single_int(uint128_t val, int bits, int lvl = 0);
    Ctxt encrypt_multi_int(vector<uint128_t> val, int bits, int lvl = 0);
    Ctxt encrypt_multi_int_nonpowtwo(vector<uint128_t> val, int bits, int maxslots, int lvl = 0);


    Ctxt encrypt(const Ptxt &p);

    vector<double> decode(const Ptxt& p);
    Ptxt decrypt(const Ctxt& c);

    Ctxt add(const Ctxt& c1, const Ctxt& c2);
    Ctxt add(const Ctxt& c, const Ptxt& p);
    Ctxt add(const Ctxt& c, double d);
    Ctxt add_tree(vector<Ctxt> v);

    Ctxt sub(const Ctxt& c1, const Ctxt& c2);
    Ctxt sub(const Ctxt& c, const Ptxt& p);
    Ctxt sub(const Ptxt& p, const Ctxt& c);
    Ctxt sub(double d, const Ctxt& c);

    Ctxt mult(const Ctxt& c1, const Ctxt& c2);
    Ctxt mult(const Ctxt& c, const Ptxt& p);
    Ctxt mult(const Ctxt& c, vector<double> p);
    Ctxt mult(const Ctxt& c, vector<int> p);
    Ctxt mult(const Ctxt& c, double d);
    Ctxt square(const Ctxt& c);

    Ctxt rot(const Ctxt& c, int rotIndex);
    Ctxt rot_fast(const Ctxt& c, int rotIndex, shared_ptr<vector<DCRTPoly>> precomputations);
    vector<int> rot(const vector<int>& vec, int rotIndex);
    Ctxt rot_integers(const Ctxt& c, int bits, int index);

    Ctxt clean_and_reduce(const Ctxt& c);
    Ctxt clean(const Ctxt& c);
    Ctxt reduce(const Ctxt& c);
    Ctxt mod2shallow(const Ctxt& c);
    Ctxt bintodec(const Ctxt& c, int repetitions);
    pair<Ctxt, Ctxt> csa3(const Ctxt& a, const Ctxt& b, const Ctxt& c, bool clean_vals = false);
    Ctxt majoritybit(const Ctxt& a, const Ctxt& b, const Ctxt& c);
    Ctxt csa4(const Ctxt& a, const Ctxt& b, const Ctxt& c, const Ctxt& d, int bits);
    Ctxt multiplier4bits(const Ctxt& a, const Ctxt& b, int repetitions);
    Ctxt process_array(const Ctxt& c, const Ctxt& c_processed, const std::vector<std::pair<int,int>>& mask_roll_pairs, int mask_size, int rep, shared_ptr<vector<DCRTPoly>> rot_precomputations);

    // Old implementation
    Ctxt binary_mult(const Ctxt &a, const Ctxt &b, int bits, int repetitions);

    Ctxt binary_or(const Ctxt& a, const Ctxt& b);
    Ctxt binary_and(const Ctxt& a, const Ctxt& b);

    Ctxt add_integer(const Ctxt& a, const Ctxt& b, int bits, bool clean_first = false);
    Ctxt sub_integer(const Ctxt &a, const Ctxt &b, int bits, bool clean_first = false);

    Ctxt mul_integer(const Ctxt &a, const Ctxt &b, int bits, int bits_original, int repetitions, int repetitions_original, bool overflow);
    Ctxt shf_integer(const Ctxt& a, int shift, int bits);

    // Advanced arithmetic operations
    Ctxt bit_length(const Ctxt& a, int bits);
    Ctxt inverse_bit_length(const Ctxt& a, int bits, int zslots);
    Ctxt blind_rotation(const Ctxt& a, const Ctxt &index, int bits, int zslots, int stride = 0);
    Ctxt div_integer(const Ctxt& numerator, const Ctxt& denominator, int bits, int zslots);
    Ctxt div_integer(const Ctxt& numerator, const Ptxt& denonimator, int denbitlength, int lastbiton, int bits, int zslots);
    Ctxt div_integer(const Ctxt& numerator, uint128_t denonimator, int bits, int zslots);

    Ctxt square_root_integer(const Ctxt& c, int bits, int zslots);


    //Hashing
    void ascon_permutation(Ctxt& state, int zslots);
    Ctxt mask_zero_slot(const Ctxt& c, int bits, int slot);
    Ctxt mask_zero_slots(const Ctxt& c, int bits, vector<int> slots);
    Ctxt mask_one_slot(const Ctxt& c, int bits, int slot);
    Ctxt mask_one_slots(const Ctxt& c, int bits, vector<int> slots);
    Ctxt local_rot_masked(const Ctxt& c, int bits, int slot, int index_rot);
    Ctxt correct_local_rot(const Ctxt& c, int bits, int zslots);


    Ctxt bootstrap(const Ctxt& c);
    Ctxt binboot(const Ctxt& c);

    Ctxt chebyshev(const Ctxt& c, vector<double> coeffs, int a, int b);
    Ctxt chebyshev_batch(const Ctxt& c, vector<vector<double>> coeffs, int a, int b);
    Ctxt chebyshev_batch_rep(const Ctxt& c, vector<vector<double>> coeffs, int a, int b, int repetitions);


    void print(const Ctxt& c);
    void print(const Ctxt& c, int slots);
    string print_ints(const Ctxt& c, int bits, int slots, bool doublebits = false);
    void print_moduli_chain(const DCRTPoly& poly);

    CryptoContext<DCRTPoly> get_context();

private:
    KeyPair<DCRTPoly> key_pair; // Key pair for the cryptosystem
    uint32_t slots;
    int depth;

};

#endif //FLEXIBLE_INTS_CKKS_CKKSCONTROLLER_H
