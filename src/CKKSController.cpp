#include "CKKSController.h"

void CKKSController::generate_context_for_bootstrapping(int ring, int levels) {
    CCParams<CryptoContextCKKSRNS> parameters;

    parameters.SetSecretKeyDist(lbcrypto::SPARSE_ENCAPSULATED);

    int dcrtBits = 36;
    int firstMod = 41;

    depth = levels + FHECKKSRNS::GetBootstrapDepth({3, 3}, lbcrypto::SPARSE_ENCAPSULATED);


    parameters.SetSecurityLevel(lbcrypto::HEStd_NotSet);
    parameters.SetRingDim(ring);
    parameters.SetNumLargeDigits(3);

    parameters.SetBatchSize(ring / 2);

    this->slots = ring / 2;

    ScalingTechnique rescaleTech = FLEXIBLEAUTO;

    parameters.SetScalingModSize(dcrtBits);
    parameters.SetScalingTechnique(rescaleTech);
    parameters.SetFirstModSize(firstMod);
    parameters.SetMultiplicativeDepth(depth);

    context = GenCryptoContext(parameters);
    context->Enable(PKE);
    context->Enable(KEYSWITCH);
    context->Enable(LEVELEDSHE);
    context->Enable(ADVANCEDSHE);
    context->Enable(FHE);

    key_pair = context->KeyGen();

    context->EvalMultKeyGen(key_pair.secretKey);
    print_moduli_chain(key_pair.publicKey->GetPublicElements()[0]);
    generate_bootstrapping();
}

void CKKSController::generate_bootstrapping() {
    int slots_bootstrapping = slots;

    context->EvalBootstrapSetup({3, 3}, {0, 0}, slots_bootstrapping, 0, true, true);
    context->EvalBootstrapKeyGen(key_pair.secretKey, slots_bootstrapping);
}

void CKKSController::generate_rotation_key(int index) {
    vector<int> rotations;

    rotations.push_back(index);

    context->EvalRotateKeyGen(key_pair.secretKey, rotations);
}

void CKKSController::generate_rotation_keys(vector<int> indexes) {
    context->EvalRotateKeyGen(key_pair.secretKey, indexes);
}

void CKKSController::generate_rotation_keys_inverse(vector<int> indexes) {
    vector<int> inverted;
    for (size_t i = 0; i < indexes.size(); i++) {
        inverted.push_back(-indexes[i]);
    }
    context->EvalRotateKeyGen(key_pair.secretKey, inverted);
}

void CKKSController::generate_rotations_for_additions(int bits) {
    for (int i = 1; i < bits; i *= 2)
        generate_rotation_key(-i);
}

void CKKSController::generate_rotations_for_bit_length(int bits) {
    for (int i = 1; i < bits; i *= 2)
        generate_rotation_key(i);

    for (int i = 1; i < 256; i *= 2)
        generate_rotation_key(i);

    generate_rotation_key(7);

    generate_rotation_key(1);
    generate_rotation_key(2);
    generate_rotation_key(3);
    generate_rotation_key(4);
    generate_rotation_key(5);
    generate_rotation_key(6);
    generate_rotation_key(bits - 1 - 8);
    generate_rotation_key(bits - 7);
    generate_rotation_key(-bits);
    generate_rotation_key(-bits-1);
}

void CKKSController::generate_rotations_for_multiplications(int bits) {
    if (bits == 8) {
        generate_rotation_keys_inverse(rot_8_bits);
    } else if (bits == 16) {
        generate_rotation_keys_inverse(rot_16_bits);
    } else if (bits == 32) {
        generate_rotation_keys_inverse(rot_32_bits);
    } else if (bits == 64) {
        generate_rotation_keys_inverse(rot_64_bits);
    } else if (bits == 128) {
        generate_rotation_keys_inverse(rot_128_bits);
    } else if (bits == 256) {
        generate_rotation_keys_inverse(rot_256_bits);
    } else {
        cerr << "Unsupported number of bits (" << bits << "), use 16, 32, 64, 128 or 256" << endl;
        return;
    }
}

void CKKSController::generate_precomputations_for_multiplications(int bits, int N) {
    if (bits > 8) {
        process_array_precomp({{8, 64}, {12, 80}}, bits, N);
    }

    if (bits > 16) {
        process_array_precomp({{16, 256}, {20, 272}, {24, 320}, {28, 336}}, bits, N);
    }

    if (bits > 32) {
        process_array_precomp({{32, 1024}, {36, 1040}, {40, 1088}, {44, 1104}, {48, 1280}, {52, 1296}, {56, 1344}, {60, 1360}}, bits, N);
    }

    if (bits > 64) {
        process_array_precomp({{64, 4096}, {68, 4112}, {72, 4160}, {76, 4176}, {80, 4352}, {84, 4368}, {88, 4416}, {92, 4432}, {96, 5120}, {100, 5136}, {104, 5184}, {108, 5200}, {112, 5376}, {116, 5392}, {120, 5440}, {124, 5456}}, bits, N);
    }

    if (bits > 128) {
        process_array_precomp({{128, 16384}, {132, 16400}, {136, 16448}, {140, 16464},{144, 16640}, {148, 16656}, {152, 16704}, {156, 16720},{160, 17408}, {164, 17424}, {168, 17472}, {172, 17488},{176, 17664}, {180, 17680}, {184, 17728}, {188, 17744},{192, 20480}, {196, 20496}, {200, 20544}, {204, 20560},{208, 20736}, {212, 20752}, {216, 20800}, {220, 20816},{224, 21504}, {228, 21520}, {232, 21568}, {236, 21584},{240, 21760}, {244, 21776}, {248, 21824}, {252, 21840}}, bits, N);
    }
    if (bits > 8) {
        process_array_precomp({{8, 32}, {12, 40}}, bits, N);
    }

    if (bits > 16) {
        process_array_precomp({{16, 128}, {20, 136}, {24, 160}, {28, 168}}, bits, N);
    }

    if (bits > 32) {
        process_array_precomp({{32, 512}, {36, 520}, {40, 544}, {44, 552}, {48, 640}, {52, 648}, {56, 672}, {60, 680}}, bits, N);
    }

    if (bits > 64) {
        process_array_precomp({{64, 2048}, {68, 2056}, {72, 2080}, {76, 2088}, {80, 2176}, {84, 2184}, {88, 2208}, {92, 2216}, {96, 2560}, {100, 2568}, {104, 2592}, {108, 2600}, {112, 2688}, {116, 2696}, {120, 2720}, {124, 2728}}, bits, N);
    }

    if (bits > 128) {
        process_array_precomp({{128, 8192}, {132, 8200}, {136, 8224}, {140, 8232}, {144, 8320}, {148, 8328}, {152, 8352}, {156, 8360}, {160, 8704}, {164, 8712}, {168, 8736}, {172, 8744}, {176, 8832}, {180, 8840}, {184, 8864}, {188, 8872}, {192, 10240}, {196, 10248}, {200, 10272}, {204, 10280}, {208, 10368}, {212, 10376}, {216, 10400}, {220, 10408}, {224, 10752}, {228, 10760}, {232, 10784}, {236, 10792}, {240, 10880}, {244, 10888}, {248, 10912}, {252, 10920}}, bits, N);
    }
}


Ptxt CKKSController::encode(const vector<double> &vec, int lvl) {
    Ptxt p = context->MakeCKKSPackedPlaintext(vec, 1, lvl, nullptr, vec.size());
    p->SetLength(vec.size());

    return p;
}

Ptxt CKKSController::encode(const vector<int> &vec, int lvl) {
    std::vector<std::complex<double>> complex_vec;
    complex_vec.reserve(vec.size());

    std::transform(vec.begin(), vec.end(),
                   std::back_inserter(complex_vec),
                   [](int x) { return std::complex<double>(x, 0.0); });


    Ptxt p = context->MakeCKKSPackedPlaintext(complex_vec, 1, lvl, nullptr, vec.size());
    p->SetLength(vec.size());

    return p;
}

Ptxt CKKSController::encode(double value, int lvl) {
    vector<double> repeated_value;
    for (uint32_t i = 0; i < slots; i++) repeated_value.push_back(value);

    return encode(repeated_value, lvl);
}

Ptxt CKKSController::encode_multi_int(vector<uint128_t> val, int bits, int lvl) {
    vector<int> toBeEncoded;

    for (size_t i = 0; i < val.size(); i++) {
        vector<int> vec = intToBitsLSB(val[i], bits);
        append_zeros(vec, bits * bits / 2 - bits);

        toBeEncoded.insert(toBeEncoded.end(), vec.begin(), vec.end());
    }

    return encode(toBeEncoded, lvl);
}

Ctxt CKKSController::encrypt(const vector<double> &vec) {
    return encrypt(encode(vec));
}

Ctxt CKKSController::encrypt(const vector<int> &vec) {
    return encrypt(encode(vec));
}

Ctxt CKKSController::encrypt(const vector<double> &vec, int lvl) {
    return encrypt(encode(vec, lvl));
}

Ctxt CKKSController::encrypt(const vector<int> &vec, int lvl) {
    return encrypt(encode(vec, lvl));
}

Ctxt CKKSController::encrypt(const Ptxt &p) {
    return context->Encrypt(p, key_pair.publicKey);
}

Ctxt CKKSController::encrypt_single_int(uint128_t val, int bits, int lvl) {
    vector<int> vec = intToBitsLSB(val, bits);
    append_zeros(vec, context->GetRingDimension() / 2 - bits);
    return encrypt(vec, lvl);
}

Ctxt CKKSController::encrypt_multi_int(vector<uint128_t> val, int bits, int lvl) {
    vector<int> toBeEncoded;

    for (size_t i = 0; i < val.size(); i++) {
        vector<int> vec = intToBitsLSB(val[i], bits);
        append_zeros(vec, bits * bits / 2 - bits);

        toBeEncoded.insert(toBeEncoded.end(), vec.begin(), vec.end());
    }

    return encrypt(toBeEncoded, lvl);
}

Ctxt CKKSController::encrypt_multi_int_nonpowtwo(vector<uint128_t> val, int bits, int maxslots, int lvl) {
    vector<int> toBeEncoded;

    for (size_t i = 0; i < val.size(); i++) {
        vector<int> vec = intToBitsLSB(val[i], bits);
        append_zeros(vec, closest_pow2(bits) * closest_pow2(bits) / 2 - bits);

        toBeEncoded.insert(toBeEncoded.end(), vec.begin(), vec.end());
    }

    append_zeros(toBeEncoded, maxslots - toBeEncoded.size());

    return encrypt(toBeEncoded, lvl);
}

vector<double> CKKSController::decode(const Ptxt& p) {
    return p->GetRealPackedValue();
}

Ptxt CKKSController::decrypt(const Ctxt &c) {
    Ptxt p;
    context->Decrypt(key_pair.secretKey, c, &p);

    return p;
}

Ctxt CKKSController::add(const Ctxt &a, const Ctxt &b) {
    return context->EvalAdd(a, b);
}

Ctxt CKKSController::add(const Ctxt &a, const Ptxt &b) {
    Ptxt temp(b);
    return context->EvalAdd(a, temp);
}

Ctxt CKKSController::add(const Ctxt &a, double d) {
    Ptxt temp = encode(d);
    return context->EvalAdd(a, temp);
}

Ctxt CKKSController::add_tree(vector<Ctxt> v) {
    return context->EvalAddMany(v);
}

Ctxt CKKSController::sub(const Ctxt &a, const Ctxt &b) {
    return context->EvalSub(a, b);
}

Ctxt CKKSController::sub(const Ctxt &c, const Ptxt &p) {
    Ptxt temp(p);
    return context->EvalSub(c, temp);
}

Ctxt CKKSController::sub(const Ptxt &p, const Ctxt &c) {
    Ptxt temp(p);
    return context->EvalSub(temp, c);
}

Ctxt CKKSController::sub(double d, const Ctxt &c) {
    //Ptxt temp(p);
    return context->EvalSub(d, c);
}

Ctxt CKKSController::mult(const Ctxt &c, const Ptxt& p) {
    return context->EvalMult(c, p);
}

Ctxt CKKSController::mult(const Ctxt &c, vector<double> p) {
    Ptxt ptxt = encode(p, c->GetLevel());
    return context->EvalMult(c, ptxt);
}

Ctxt CKKSController::mult(const Ctxt &c, vector<int> p) {
    Ptxt ptxt = encode(p, c->GetLevel());
    return context->EvalMult(c, ptxt);
}

Ctxt CKKSController::mult(const Ctxt &c1, const Ctxt &c2) {
    return context->EvalMult(c1, c2);
}

Ctxt CKKSController::mult(const Ctxt &c, double v) {
    return context->EvalMult(c, encode(v));
}

Ctxt CKKSController::square(const Ctxt &c) {
    return context->EvalSquare(c);
}

Ctxt CKKSController::rot(const Ctxt &c, int rotIndex) {
    if (rotIndex == 0) return c;
    return context->EvalRotate(c, rotIndex);
}
Ctxt CKKSController::rot_fast(const Ctxt &c, int rotIndex, shared_ptr<vector<lbcrypto::DCRTPoly>> precomputations) {
    if (rotIndex == 0) return c;
    return context->EvalFastRotation(c, rotIndex, context->GetCyclotomicOrder(), precomputations);
}

vector<int> CKKSController::rot(const vector<int>& vec, int rotIndex) {
    if (vec.empty()) return {};

    int n = vec.size();
    rotIndex = rotIndex % n;
    if (rotIndex == 0) return vec;

    std::vector<int> result = vec;

    if (rotIndex > 0) {
        // Positive shift → left rotation
        std::rotate(result.begin(), result.begin() + rotIndex, result.end());
    } else {
        // Negative shift → right rotation
        rotIndex = -rotIndex;
        std::rotate(result.rbegin(), result.rbegin() + rotIndex, result.rend());
    }

    return result;
}

Ctxt CKKSController::rot_integers(const Ctxt &c, int bits, int rotIndex) {
    int distance = (bits * bits) / 2;
    if (rotIndex == 0) return c;
    return context->EvalRotate(c, rotIndex * distance);
}

Ctxt CKKSController::clean_and_reduce(const Ctxt &c) {
    return context->EvalMult(context->EvalSquare(c), context->EvalSquare(context->EvalSub(c, 2)));
}

Ctxt CKKSController::clean(const Ctxt &c) {
    Ctxt sq = context->EvalSquare(c);
    Ctxt t1 = context->EvalMult(c, -2);

    return context->EvalAdd(context->EvalMult(sq, t1), context->EvalMult(sq, 3));
}

Ctxt CKKSController::mod2shallow(const Ctxt &c, double scale) {
    return context->EvalSub(context->EvalMult(c, 2), context->EvalSquare(c));
}

Ctxt CKKSController::bintodec(const Ctxt &c, int repetitions) {
    vector<double> mask;

    for (int i = 0; i < repetitions; i++) {
        mask.insert(mask.end(), {1, 2, 4, 8, 0, 0, 0, 0});
    }

    Ctxt res = mult(c, mask);
    res = add(res, rot(res, 1));
    res = add(res, rot(res, 2));

    vector<double> mask2;

    for (int i = 0; i < repetitions; i++) {
        mask2.insert(mask2.end(), {sqrt(1.0 / (225.0 / 2.0)), 0, 0, 0, 0, 0, 0, 0});
    }

    res = mult(res, mask2);
    res = add(res, rot(res, -1));
    res = add(res, rot(res, -2));
    res = add(res, rot(res, -4));

    return res;
}

std::pair<Ctxt, Ctxt> CKKSController::csa3(const Ctxt &a, const Ctxt &b, const Ctxt &c, bool clean_vals) {
    Ctxt S;

    if (clean_vals) {
        S = clean_and_reduce(add(a, b));
        S = mod2shallow(add(S, clean(c)));
    } else {
        S = mod2shallow(add(a, b));
        S = mod2shallow(add(S, c));
    }

    Ctxt C = majoritybit(a, b, c);

    return {S, C};
}

Ctxt CKKSController::multiplier4bits(const Ctxt &a, const Ctxt &b, int repetitions) {
    Ctxt result = mult(a, b);
    result = add(result, -1);

    vector<vector<double>> coeffs;

    coeffs.push_back(read_vector_file("../coeffs/p1-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p2-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p3-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p4-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p5-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p6-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p7-norm-369.txt"));
    coeffs.push_back(read_vector_file("../coeffs/p8-norm-369.txt"));

    Ctxt resultpoly = context->EvalChebyshevSeriesPSBatchRepeated(result, coeffs, -1, 1, repetitions);

    resultpoly = binboot(resultpoly);

    return resultpoly;
}

Ctxt CKKSController::majoritybit(const Ctxt &a, const Ctxt &b, const Ctxt &c) {
    Ctxt total = add(add(a, b), c);

    Ctxt sq = context->EvalSquare(total);
    Ctxt t1 = mult(total, -1.0/3.0);

    return add(add(mult(t1, sq), mult(sq, 3.0/2.0)), mult(total, -7.0/6.0));

}

Ctxt CKKSController::csa4(const Ctxt &a, const Ctxt &b, const Ctxt &c, const Ctxt &d, int bits) {
    Ctxt s1, c1;
    std::tie(s1, c1) = csa3(a, b, c, false);


    c1 = rot(c1, -1);

    Ctxt s2, c2;
    std::tie(s2, c2) = csa3(s1, c1, d, false);

    c2 = rot(c2, -1);
    Ctxt result = add_integer(s2, c2, bits, false);
    return result;
}

Ctxt CKKSController::binary_or(const Ctxt &a, const Ctxt &b) {
    // a+b - a*b
    return sub(add(a, b), mult(a, b));
}


Ctxt CKKSController::add_integer(const Ctxt &a, const Ctxt &b, int bits, bool clean_first) {
    Ctxt p;

    if (clean_first) {
        p = clean_and_reduce(add(a,b));
    } else {
        p = square(sub(a, b));
    }

    Ctxt absum = p->Clone();

    Ctxt g = mult(a, b);

    for (int i = 1; i < bits; i *= 2) {
        Ctxt p_shift = rot(p, -i);
        Ctxt g_shift = rot(g, -i);

        Ctxt pg = mult(p, g_shift);
        g = sub(add(g, pg), mult(p, g));

        if (i < bits - 1) p = mult(p, p_shift);
    }

    g = rot(g, -1);
    Ctxt s = square(sub(absum, g));

    return s;
}

Ctxt CKKSController::sub_integer(const Ctxt &a, const Ctxt &b, int bits, bool clean_first) {
    vector<double> ones;

    int s = context->GetRingDimension() / (bits * bits);

    for (int i = 0; i < s; i++) {
        for (int j = 0; j < bits + 1; j++) {
            ones.push_back(1);
        }
        for (int j = 0; j < bits * bits / 2 - bits - 1; j++) {
            ones.push_back(0);
        }
    }

    Ctxt inverted = context->EvalSub(encrypt(encode(ones, b->GetLevel())), b);
    inverted = add_integer(a, inverted, bits, clean_first);

    //Cleaning garbage (NEW)
    vector<double> mask(inverted->GetSlots());
    fill(mask.begin(), mask.end(), 0.0);
    for (int i = 0; i < s; i++) {
        int stride = bits * bits / 2;
        for (int j = 0; j < bits; j++) mask[stride * i + j] = 1;
    }

    return mult(inverted, encode(mask, inverted->GetLevel()));

}

/*
Ctxt CKKSController::binary_mult(const Ctxt &a, const Ctxt &b, int bits, int repetitions) {
    Ctxt result;

    int rep_size = bits * bits / 2;
    int dunn = bits * bits / 8;

    Ctxt a_processed, b_processed;

    if (bits > 8) {
        vector<double> mask_low(a->GetSlots(), 0);
        vector<double> mask_high(a->GetSlots(), 0);

        for (int j = 0; j < repetitions; ++j) {
            for (int i = 0; i < bits / 2; ++i) {
                mask_low[(rep_size * j) + i] = 1;
            }
        }

        for (int j = 0; j < repetitions; ++j) {
            for (int i = bits / 2; i < bits; ++i) {
                mask_high[(rep_size * j) + i] = 1;
            }
        }

        a_processed = mult(a, mask_low);
        Ctxt a_processed_high = mult(a, mask_high);

        a_processed = add(a_processed, rot(a_processed, -(dunn)));
        a_processed = add(a_processed, rot(a_processed_high, -(dunn * 2 - bits / 2)));
        a_processed = add(a_processed, rot(a_processed_high, -(dunn * 3 - bits / 2)));

        b_processed = mult(b, mask_low);
        Ctxt b_processed_high = mult(b, mask_high);
        b_processed = add(b_processed, rot(b_processed, -(dunn * 2)));
        b_processed = add(b_processed, rot(b_processed_high, -(dunn - bits / 2)));
        b_processed = add(b_processed, rot(b_processed_high, -(dunn * 3 - bits / 2)));

    } else {
        a_processed = a->Clone();
        a_processed = add(a_processed, rot(a, -8));
        a_processed = add(a_processed, rot(a, -(8 + 4)));
        a_processed = add(a_processed, rot(a, -(16 + 4)));

        b_processed = b->Clone();
        b_processed = add(b_processed, rot(b, -16));
        b_processed = add(b_processed, rot(b, -4));
        b_processed = add(b_processed, rot(b, -20));
    }

    if (bits == 8) {
        result = multiplier4bits(bintodec(a_processed, repetitions * 4),
                                    bintodec(b_processed, repetitions * 4),
                                    repetitions * 4);
    } else {
        result = binary_mult(a_processed, b_processed, bits / 2, 4 * repetitions);
    }

    int dunn2 = dunn * 2;

    vector<double> mask1(a->GetSlots(), 0.0);

    for (int j = 0; j < repetitions; ++j) {
        for (int i = 0; i < bits; ++i) {
            mask1[(j * rep_size) + i] = 1.0;
            mask1[(j * rep_size) + i + dunn2] = 1.0;
        }
    }

    vector<double> mask2(a->GetSlots(), 0.0);

    for (int j = 0; j < repetitions; ++j) {
        for (int i = 0; i < bits; ++i) {
            mask2[(j * rep_size) + rep_size / 4 + i] = 1.0;
            mask2[(j * rep_size) + rep_size / 4 + i + dunn2] = 1.0;
        }
    }

    Ctxt p1 = mult(result, mask1);
    Ctxt p2 = rot(p1, -(-rep_size / 2 + bits / 2));

    Ctxt p3, p4;

    if (bits == 8) {
        p3 = rot(mult(result, mask2), 16);
    } else {
        p3 = rot(mult(result, mask2), -(-rep_size / 4 + bits / 2));
    }

    if (bits == 8) {
        p4 = rot(p3, -12);
    } else if (bits == 16) {
        p4 = rot(mult(result, mask2), 80);
    } else if (bits == 32) {
        p4 = rot(mult(result, mask2), 352);
    } else if (bits == 64) {
        p4 = rot(mult(result, mask2), 1472);
    }

    result = csa4(p1, p2, p3, p4, bits);

    result = binboot(result);


    return result;
}
*/

void CKKSController::process_array_precomp(const std::vector<std::pair<int,int>>& mask_roll_pairs, int bits, int N) {
    int lvl = 12; //Fixed according to the params...

    int mask_size = bits * (bits / 2);

    //Assuming full reps?
    int rep = N / (bits * bits);

    for (auto [start, roll_base] : mask_roll_pairs) {
        int total_size = mask_size * rep;

        vector<int> mask(total_size, 0);

        for (int i = 0; i < rep; ++i) {
            for (int j = 0; j < 4; ++j) {
                mask[i * mask_size + start + j] = 1;
            }
        }

        int shift = roll_base - start;

        vector<int> rolled_mask = rot(mask, -shift);

        processedMasksMult.push_back(encode(rolled_mask, lvl));
    }

    vector<int> masklow(slots, 0);
    for (int j = 0; j < rep; j++) {
        masklow[0 + j * mask_size] = 1;
        masklow[1 + j * mask_size] = 1;
        masklow[2 + j * mask_size] = 1;
        masklow[3 + j * mask_size] = 1;
    }

    precompMaskLow = encode(masklow);

    vector<int> maskhigh(slots, 0);

    for (int j = 0; j < rep; j++) {
        maskhigh[4 + j * mask_size] = 1;
        maskhigh[5 + j * mask_size] = 1;
        maskhigh[6 + j * mask_size] = 1;
        maskhigh[7 + j * mask_size] = 1;
    }

    precompMaskHigh164 = encode(rot(maskhigh, -(16 - 4)));
    precompMaskHigh4 = encode(rot(maskhigh, -4));

    int min_bits = 8;
    int rep_original = N / (bits * bits);

    if (bits == 16) {
        rep_original *= 4;
    }
    if (bits == 32) {
        rep_original *= 4;
    }
    if (bits == 64) {
        rep_original *= 4;
    }
    if (bits == 128) {
        rep_original *= 4;
    }
    if (bits == 256) {
        rep_original *= 4;
    }

    while (min_bits <= bits) {
        int dunn = (min_bits * min_bits / 8) * 2;
        int rep_size = min_bits * min_bits / 2;

        vector<double> mask1(slots, 0.0);

        for (int j = 0; j < rep_original; ++j) {
            for (int i = 0; i < min_bits; ++i) {
                mask1[(j * rep_size) + i] = 1.0;
                mask1[(j * rep_size) + i + dunn] = 1.0;
            }
        }

        vector<double> mask2(slots, 0.0);

        for (int j = 0; j < rep_original; ++j) {
            for (int i = 0; i < min_bits; ++i) {
                mask2[(j * rep_size) + rep_size / 4 + i] = 1.0;
                mask2[(j * rep_size) + rep_size / 4 + i + dunn] = 1.0;
            }
        }

        precompMasksPart2a[min_bits] = encode(mask1);
        precompMasksPart2b[min_bits] = encode(mask2);

        min_bits *= 2;
        rep_original /= 4;
    }

}



Ctxt CKKSController::process_array(const Ctxt& c, const Ctxt& c_processed, const std::vector<std::pair<int,int>>& mask_roll_pairs, int mask_size, int rep, shared_ptr<vector<DCRTPoly>> rot_precomputations ) {
    Ctxt c_processed_clone = c_processed->Clone();

    for (auto [start, roll_base] : mask_roll_pairs) {
        int shift = roll_base - start;
        Ctxt rolled_ctxt = rot_fast(c, -shift, rot_precomputations);
        cout << "Rotating by " << -shift << endl;
        c_processed_clone = add(c_processed_clone, mult(rolled_ctxt, processedMasksMult[indexMap]));
        indexMap += 1;
    }

    return c_processed_clone;
}

Ctxt CKKSController::mul_integer(const Ctxt &a, const Ctxt &b, int bits, int bits_original, int repetitions, int repetitions_original, bool overflow) {
    Ctxt result;

    int rep_size = bits * bits / 2;

    Ctxt a_processed, b_processed;

    if (bits == 8) {
        shared_ptr<vector<DCRTPoly>> a_precomputations = context->EvalFastRotationPrecompute(a);
        shared_ptr<vector<DCRTPoly>> b_precomputations = context->EvalFastRotationPrecompute(b);

        int mask_size = bits_original * (bits_original / 2);

        a_processed = mult(a, precompMaskLow);


        a_processed = add(a_processed, mult(rot(a, -(16 - 4)), precompMaskHigh164));


        indexMap = 0;

        if (bits_original > 8) {
            a_processed = process_array(a, a_processed, {{8, 64}, {12, 80}}, mask_size, repetitions_original, a_precomputations);
        }

        if (bits_original > 16) {
            a_processed = process_array(a, a_processed, {{16, 256}, {20, 272}, {24, 320}, {28, 336}}, mask_size, repetitions_original, a_precomputations);
        }

        if (bits_original > 32) {
            a_processed = process_array(a, a_processed,{{32, 1024}, {36, 1040}, {40, 1088}, {44, 1104}, {48, 1280}, {52, 1296}, {56, 1344}, {60, 1360}}, mask_size, repetitions_original, a_precomputations);
        }

        if (bits_original > 64) {
            a_processed = process_array(a, a_processed, {{64, 4096}, {68, 4112}, {72, 4160}, {76, 4176}, {80, 4352}, {84, 4368}, {88, 4416}, {92, 4432}, {96, 5120}, {100, 5136}, {104, 5184}, {108, 5200}, {112, 5376}, {116, 5392}, {120, 5440}, {124, 5456}},mask_size,repetitions_original,a_precomputations);
        }

        if (bits_original > 128) {
            a_processed = process_array(a,a_processed,{{128, 16384}, {132, 16400}, {136, 16448}, {140, 16464},{144, 16640}, {148, 16656}, {152, 16704}, {156, 16720},{160, 17408}, {164, 17424}, {168, 17472}, {172, 17488},{176, 17664}, {180, 17680}, {184, 17728}, {188, 17744},{192, 20480}, {196, 20496}, {200, 20544}, {204, 20560},{208, 20736}, {212, 20752}, {216, 20800}, {220, 20816},{224, 21504}, {228, 21520}, {232, 21568}, {236, 21584},{240, 21760}, {244, 21776}, {248, 21824}, {252, 21840}},mask_size,repetitions_original,a_precomputations);
        }

        if (bits_original > 4) a_processed = add(a_processed, rot(a_processed, -8));
        if (bits_original > 8) a_processed = add(a_processed, rot(a_processed, -32));
        if (bits_original > 16) a_processed = add(a_processed, rot(a_processed, -128));
        if (bits_original > 32) a_processed = add(a_processed, rot(a_processed, -512));
        if (bits_original > 64) a_processed = add(a_processed, rot(a_processed, -2048));
        if (bits_original > 128) a_processed = add(a_processed, rot(a_processed, -8192));

        // B //

        b_processed = mult(b, precompMaskLow);
        b_processed = add(b_processed, mult(rot(b, - 4), precompMaskHigh4));

        if (bits_original > 8) {
            b_processed = process_array(b, b_processed, {{8, 32}, {12, 40}}, mask_size, repetitions_original, b_precomputations);
        }

        if (bits_original > 16) {
            b_processed = process_array(b, b_processed, {{16, 128}, {20, 136}, {24, 160}, {28, 168}}, mask_size, repetitions_original, b_precomputations);
        }

        if (bits_original > 32) {
            b_processed = process_array(b, b_processed, {{32, 512}, {36, 520}, {40, 544}, {44, 552}, {48, 640}, {52, 648}, {56, 672}, {60, 680}}, mask_size, repetitions_original, b_precomputations);
        }

        if (bits_original > 64) {
            b_processed = process_array(b, b_processed, {{64, 2048}, {68, 2056}, {72, 2080}, {76, 2088}, {80, 2176}, {84, 2184}, {88, 2208}, {92, 2216}, {96, 2560}, {100, 2568}, {104, 2592}, {108, 2600}, {112, 2688}, {116, 2696}, {120, 2720}, {124, 2728}}, mask_size, repetitions_original, b_precomputations);
        }

        if (bits_original > 128) {
            b_processed = process_array(b, b_processed, {{128, 8192}, {132, 8200}, {136, 8224}, {140, 8232}, {144, 8320}, {148, 8328}, {152, 8352}, {156, 8360}, {160, 8704}, {164, 8712}, {168, 8736}, {172, 8744}, {176, 8832}, {180, 8840}, {184, 8864}, {188, 8872}, {192, 10240}, {196, 10248}, {200, 10272}, {204, 10280}, {208, 10368}, {212, 10376}, {216, 10400}, {220, 10408}, {224, 10752}, {228, 10760}, {232, 10784}, {236, 10792}, {240, 10880}, {244, 10888}, {248, 10912}, {252, 10920}}, mask_size, repetitions_original, b_precomputations);
        }


        if (bits_original > 4) b_processed = add(b_processed, rot(b_processed, -16));
        if (bits_original > 8) b_processed = add(b_processed, rot(b_processed, -64));
        if (bits_original > 16) b_processed = add(b_processed, rot(b_processed, -256));
        if (bits_original > 32) b_processed = add(b_processed, rot(b_processed, -1024));
        if (bits_original > 64) b_processed = add(b_processed, rot(b_processed, -4096));
        if (bits_original > 128) b_processed = add(b_processed, rot(b_processed, -16384));


        //auto t = steady_clock::now();
        result = multiplier4bits(bintodec(a_processed, repetitions * 4),
                                 bintodec(b_processed, repetitions * 4), repetitions * 4);


        //print_duration(t, "4 bits multiplier took: ");
    } else {
        result = mul_integer(a, b, bits / 2, bits_original, 4 * repetitions, repetitions_original, overflow);
    }



    Ctxt p1 = mult(result, precompMasksPart2a[bits]);

    Ctxt p2 = rot(p1, -(-rep_size/2 + bits/2));

    Ctxt p3, p4;

    Ctxt resmask2 = mult(result, precompMasksPart2b[bits]);

    if (bits == 8) {
        p3 = rot(resmask2, 16);
    } else {
        p3 = rot(resmask2, -(-rep_size / 4 + bits / 2));
    }

    if (!overflow && bits == bits_original && bits != 8) {
        pair<Ctxt, Ctxt> out = csa3(p1, p2, p3);
        result = binboot(add_integer(out.first, rot(out.second, -1), bits));
    } else {
        if (bits == 8) {
            p4 = rot(p3, -12);
        } else {
            p4 = rot(resmask2, (((bits - 2) * (3 * bits - 2)) / 8));
        }

        result = binboot(csa4(p1, p2, p3, p4, bits));
    }

    return result;
}

Ctxt CKKSController::shf_integer(const Ctxt &a, int shift, int bits) {
    vector<double> mask;

    int int_slots = context->GetRingDimension() / (bits * bits);

    for (int i = 0; i < int_slots; i++) {
        for (int j = 0; j < bits; j++) {
            mask.push_back(1);
        }
        for (int j = 0; j < (bits / 2) * (bits / 2) / 2 - bits; j++) {
            mask.push_back(0);
        }
    }

    return rot(mult(a, mask), -shift);
}

Ctxt CKKSController::bit_length(const Ctxt &a, int bits) {
    //TODO write it (similar as below)
    return nullptr;
}

Ctxt CKKSController::inverse_bit_length(const Ctxt &a, int bits, int zslots) {

    int step = 1;
    Ctxt result = a->Clone();
    while (step < bits) {
        result = binary_or(result, rot(result, step));
        step *= 2;
    }

    result = binboot(result);

    // Now we sum all the ones
    for (int i = 0; i < log2(bits); i++) {
        result = add(result, rot(result, -pow(2, i)));
    }

    //The result is in the last (partial) slot
    vector<double> mask(result->GetSlots());

    int stride = bits * bits/2; //Distance between each element
    for (int i = 0; i < zslots; i++) {
        mask[(bits - 1) + i * stride] = -1.0 / (bits / 2.0);
    }
    //mask[bits - 1] = -1.0 / (bits / 2.0);
    result = mult(result, mask);

    for (int i = 0; i < zslots; i++) {
        mask[(bits - 1) + i * stride] = 1.0;
    }
    //mask[bits - 1] = 1.0;
    result = add(result, encode(mask, result->GetLevel()));

    Ctxt resultclone = result->Clone();

    result = add(result, rot(result, 1));
    result = add(result, rot(result, 2));
    result = add(result, rot(result, 4));
    result = sub(result, rot(resultclone, 7));

    result = rot(result, bits - 7);

    return result;
}

/*
 * Takes as input a 'bits' size input a, and a number of at most 7 bits (128 at most), LSB to MSB in binary, to the RIGHT
 */
Ctxt CKKSController::blind_rotation(const Ctxt &a, const Ctxt &index, int bits, int zslots, int stride) {
    Ctxt result = a->Clone();

    if (stride == 0) {
        stride = bits * bits / 2;
    }

    for (int i = 0; i < 7; i++) {
        vector<double> mask(a->GetSlots());
        for (int j = 0; j < zslots; j++) {
            mask[i + j * stride] = 1;
        }
        //mask[i] = 1;
        Ctxt current_index = mult(index, encode(mask, index->GetLevel()));
        current_index = rot(current_index, i);
        for (int j = 0; j < log2(bits); j++) {
            //Filling
            current_index = add(current_index, rot(current_index, -pow(2, j)));
        }

        //If condition
        result = add(mult(result, sub(1, current_index)), mult(rot(result, -pow(2, i)), current_index));
    }

    return result;
}

Ctxt CKKSController::div_integer(const Ctxt &num, const Ctxt &den, int bits, int zslots) {
    int LUT_BITS = 8;

    bool verbose = false;

    //This computes the \hat{x} value, already normalized in [-1, 1], ready to be given as input to the LUT
    Ctxt b = inverse_bit_length(den, bits, zslots);

    //These polynomials decompose the bit_length from integer into binary to unlock blind rotation
    vector<vector<double>> coeffs;
    coeffs.push_back(read_vector_file("../coeffs/LUTs/" + to_string(bits) + " bits/lut/p1-norm-247-LUT-DIVISION.txt"));
    coeffs.push_back(read_vector_file("../coeffs/LUTs/" + to_string(bits) + " bits/lut/p2-norm-247-LUT-DIVISION.txt"));
    coeffs.push_back(read_vector_file("../coeffs/LUTs/" + to_string(bits) + " bits/lut/p3-norm-247-LUT-DIVISION.txt"));
    coeffs.push_back(read_vector_file("../coeffs/LUTs/" + to_string(bits) + " bits/lut/p4-norm-247-LUT-DIVISION.txt"));
    coeffs.push_back(read_vector_file("../coeffs/LUTs/" + to_string(bits) + " bits/lut/p5-norm-247-LUT-DIVISION.txt"));
    coeffs.push_back(read_vector_file("../coeffs/LUTs/" + to_string(bits) + " bits/lut/p6-norm-247-LUT-DIVISION.txt"));
    coeffs.push_back(read_vector_file("../coeffs/LUTs/" + to_string(bits) + " bits/lut/p7-norm-247-LUT-DIVISION.txt"));

    for (int i = 0; i < bits - 7; i++) coeffs.push_back(read_vector_file("../coeffs/LUTs/" + to_string(bits) + " bits/lut/p1-norm-247-LUT-DIVISION.txt")); //Garbage slots

    //This is bits - den.bit_length()
    Ctxt s = context->EvalChebyshevSeriesPSBatchRepeated(b, coeffs, -1, 1, (int)(b->GetSlots() / coeffs.size()));
    s = binboot(s);

    if (verbose) print(s, bits);
    if (verbose) print(rot(s,  bits * bits/2), bits);

    if (verbose) cout << "s: " << print_ints(s, bits, 2) << endl;

    Ctxt den_norm = blind_rotation(den, s, bits, zslots); //This performs the den << (bits - s.bit_length()) step

    if (verbose) cout << "den: " << print_ints(den_norm, bits, 2) << endl;

    den_norm = binboot(den_norm);

    Ctxt den_norm_rot = rot(den_norm, bits - 1 - LUT_BITS);

    // Now the index must be in decimal to be given as input to the Chebyshev-LUT
    vector<double> mask(den_norm_rot->GetSlots());

    for (int j = 0; j < zslots; j++) {
        int stride = bits * bits / 2;
        for (int i = 0; i < LUT_BITS; i++) mask[stride * j + i] = pow(2, i);
    }

    Ctxt idx = mult(den_norm_rot, mask);

    //N.b. idx \in [0, 256]
    for (int i = 0; i < log2(LUT_BITS); i++) {
        idx = add(idx, rot(idx, pow(2, i)));
    }

    fill(mask.begin(), mask.end(), 0.0);

    for (int j = 0; j < zslots; j++) {
        int stride = bits * bits/2;
        mask[j * stride] = 1;
    }

    idx = mult(idx, mask);

    Ctxt idx_masked_clone = idx->Clone();

    for (int i = 0; i < log2(bits); i++) {
        idx = add(idx, rot(idx, -pow(2, i)));
    }

    idx = add(idx, rot(idx_masked_clone, -bits));
    idx = add(idx, rot(idx_masked_clone, -bits-1));

    //LUT output occupies bits + 2 slots, so we repeat idx 'bits + 2' times

    coeffs.clear();
    for (int i = 0; i < bits + 2; i++) coeffs.push_back(read_vector_file("../coeffs/LUTs/" + to_string(bits) + " bits/division/LUT-DIVISION-" + to_string(bits) + "-bits-" + to_string(i) + ".txt"));
    for (int i = 0; i < (bits * bits / 2) - (bits + 2); i++) coeffs.push_back(read_vector_file("../coeffs/LUTs/" + to_string(bits) + " bits/division/LUT-DIVISION-" + to_string(bits) + "-bits-0.txt")); //Garbage

    Ctxt x = get_context()->EvalChebyshevSeriesPSBatchRepeated(idx, coeffs, 0, 256, (int)(idx->GetSlots() / coeffs.size()));
    x = binboot(x);

    //x is the actual hint, let's go


    if (verbose) cout << "first hint : " << print_ints(x, bits, 2) << endl;
    if (verbose) cout << "den norm   : " << print_ints(den_norm, bits, 2) << endl;


    for (int iter = 0; iter < ceil(log2(bits / LUT_BITS)); iter++) {
        //Ctxt term = mul_integer(x, den_norm, bits * 2, bits * 2, 1, 1, true);
        Ctxt term = mul_integer(x, den_norm, bits, bits, zslots, zslots, true);

        if (verbose) cout << "Hint: " << print_ints(term, bits * 2 , 2) << endl;

        //Now we must add rot(den_norm * lastbitofx, -bits) THE BIT 128
        fill(mask.begin(), mask.end(), 0.0);
        for (int j = 0; j < zslots; j++) {
            int stride = bits * bits/2;
            mask[bits + j * stride] = 1;
        }
        Ctxt lastBit = mult(x, encode(mask, x->GetLevel()));
        for (int j = 0; j < log2(bits); j++) {
            lastBit = add(lastBit, rot(lastBit, -pow(2, j)));
        }
        lastBit = mult(lastBit, rot(den_norm, -bits));
        term = binboot(add_integer(term, lastBit, bits, false));

        if (verbose) cout << "Hint: " << print_ints(term, bits * 2, 2) << endl;

        //Now we must add rot(den_norm * lastbitofx, -bits) THE BIT 129 as x can have bits + 2 bits at most (experiment
        //observed, use b = 1 to see
        fill(mask.begin(), mask.end(), 0.0);
        for (int j = 0; j < zslots; j++) {
            int stride = bits * bits/2;
            mask[(bits + 1) + j * stride] = 1;
        }
        lastBit = mult(x, encode(mask, x->GetLevel()));

        for (int j = 0; j < log2(bits); j++) {
            lastBit = add(lastBit, rot(lastBit, -pow(2, j)));
        }
        lastBit = mult(lastBit, rot(rot(den_norm, -bits), - 1));
        term = binboot(add_integer(term, lastBit, bits, false));


        if (verbose) cout << "Hint: " << print_ints(term, bits * 2, 2) << endl;

        fill(mask.begin(), mask.end(), 0.0);
        for (int j = 0; j < zslots; j++) {
            int stride = bits * bits/2;
            for (int i = 0; i < bits * 2 + 1; i++) mask[i + j * stride] = 1;
        }
        term = sub(encode(mask, term->GetLevel()), term);


        fill(mask.begin(), mask.end(), 0.0);
        for (int j = 0; j < 1; j++) {
            int stride = bits * bits/2;
            mask[j * stride] = 1;
        }

        //The + 1 that corrects the subtraction (a - b = a + !b + 1
        term = binboot(add_integer(term, encrypt(mask, term->GetLevel()), bits * 2, false));

        term = rot(term, bits); //Removing the first 'bits' least significative

        //x = mul_integer(x, term, bits * 2, bits * 2, 1, 1, true);
        x = mul_integer(rot(x, 2), rot(term, 2), bits, bits, zslots, zslots, true);

        x = rot(x, bits);
        x = rot(x, -1);
        x = rot(x, -1);
        x = rot(x, -1);
        x = rot(x, -1);

        if (verbose) cout << "Hint: " << print_ints(x, bits * 2, 2) << endl;


    }

    if (verbose) cout << "Final hint : " << print_ints(x, bits * 2, 2) << endl;


    //TODO fixa questa e siamo a posto con anche SIMD :)

    Ctxt result = mul_integer(rot(num, 2), rot(x, 2), bits, bits, zslots, zslots, true);

    if (verbose) cout << "Result    : " << print_ints(result, bits * 2, 2) << endl;

    result = rot(result, -1);
    result = rot(result, -1);
    result = rot(result, -1);
    result = rot(result, -1);

    if (verbose) cout << "Resultspos : " << print_ints(result, bits * 2, 2) << endl;


    //Ctxt result = mul_integer(num, x, bits*2, bits*2, 1, 1, true);

    result = rot(result, bits);

    fill(mask.begin(), mask.end(), 0.0);
    for (int j = 0; j < zslots; j++) {
        int stride = bits * bits / 2;
        for (int i = 0; i < bits * 2; i++) {
            mask[i + j * stride] = 1;
        }
    }
    result = mult(result, mask);

    result = blind_rotation(result, s, bits * 2, zslots, bits * bits / 2);


    result = rot(result, bits);




    fill(mask.begin(), mask.end(), 0.0);

    for (int j = 0; j < zslots; j++) {
        int stride = bits * bits / 2;
        for (int i = 0; i < bits; i++) mask[i + j * stride] = 1;
    }

    result = mult(result, mask);

    return result;
}

Ctxt CKKSController::div_integer(const Ctxt &num, const Ptxt &den, int den_bitlength, int lastbiton, int bits, int zslots) {
    //den MUST BE den = (1 << (bits + b.bit_length())) // b
    //compute that in Py, since its plaintext, as in c++ its a bit tricky

    Ctxt x = encrypt(den);

    Ctxt result = mul_integer(num, x, bits, bits, 1, 1, true);

    // we add the last bit of the reciprocal (which is always on i guess?)
    result = binboot(add_integer(result, rot(num, -bits), bits, false));

    generate_rotation_key(bits + den_bitlength);

    result = rot(result, bits + den_bitlength);

    vector<double> mask(num->GetSlots());
    fill(mask.begin(), mask.end(), 0.0);
    for (int j = 0; j < zslots; j++) {
        int stride = bits * bits / 2;
        for (int i = 0; i < bits; i++) {
            mask[i + j * stride] = 1;
        }
    }

    result = mult(result, mask);

    return result;
}

Ctxt CKKSController::div_integer(const Ctxt &num, uint128_t den, int bits, int zslots) {
    vector<int> r = reciprocal(den, bits);
    append_zeros(r, num->GetSlots() - r.size());

    Ctxt x = encrypt(encode(r, num->GetLevel()));

    Ctxt result = mul_integer(num, x, bits, bits, 1, 1, true);

    // we add the last bit of the reciprocal (which is always on i guess?)
    result = binboot(add_integer(result, rot(num, -bits), bits, false));

    int den_bitlength = bit_width(den);

    generate_rotation_key(bits + den_bitlength);

    result = rot(result, bits + den_bitlength);

    vector<double> mask(num->GetSlots());
    fill(mask.begin(), mask.end(), 0.0);
    for (int j = 0; j < zslots; j++) {
        int stride = bits * bits / 2;
        for (int i = 0; i < bits; i++) {
            mask[i + j * stride] = 1;
        }
    }

    result = mult(result, mask);

    return result;
}

Ctxt CKKSController::square_root_integer(const Ctxt &c, int bits, int zslots) {
    int LUT_BITS = 8;

    bool verbose = false;

    //This computes the \hat{x} value, already normalized in [-1, 1], ready to be given as input to the LUT
    Ctxt hatx = inverse_bit_length(c, bits, zslots);

    //These polynomials decompose the bit_length from integer into binary to unlock blind rotation
    vector<vector<double>> coeffs;

    int deg = 247;


    coeffs.push_back(read_vector_file("../coeffs/LUTs/" + to_string(bits) + " bits/lut/p1-norm-" + to_string(deg) + "-LUT-DIVISION.txt"));
    coeffs.push_back(read_vector_file("../coeffs/LUTs/" + to_string(bits) + " bits/lut/p2-norm-" + to_string(deg) + "-LUT-DIVISION.txt"));
    coeffs.push_back(read_vector_file("../coeffs/LUTs/" + to_string(bits) + " bits/lut/p3-norm-" + to_string(deg) + "-LUT-DIVISION.txt"));
    coeffs.push_back(read_vector_file("../coeffs/LUTs/" + to_string(bits) + " bits/lut/p4-norm-" + to_string(deg) + "-LUT-DIVISION.txt"));
    coeffs.push_back(read_vector_file("../coeffs/LUTs/" + to_string(bits) + " bits/lut/p5-norm-" + to_string(deg) + "-LUT-DIVISION.txt"));
    coeffs.push_back(read_vector_file("../coeffs/LUTs/" + to_string(bits) + " bits/lut/p6-norm-" + to_string(deg) + "-LUT-DIVISION.txt"));
    coeffs.push_back(read_vector_file("../coeffs/LUTs/" + to_string(bits) + " bits/lut/p7-norm-" + to_string(deg) + "-LUT-DIVISION.txt"));

    for (int i = 0; i < bits - 7; i++) coeffs.push_back(read_vector_file("../coeffs/LUTs/" + to_string(bits) + " bits/lut/p1-norm-" + to_string(deg) + "-LUT-DIVISION.txt")); //Garbage slots

    //This is bits - c.bit_length()
    Ctxt s = context->EvalChebyshevSeriesPSBatchRepeated(hatx, coeffs, -1, 1, (int)(hatx->GetSlots() / coeffs.size()));
    s = binboot(s);

    if (verbose) cout << "s:" << endl;
    if (verbose) print(s, bits);

    hatx = blind_rotation(c, s, bits, zslots); //This performs the den << (bits - s.bit_length()) step

    if (verbose) cout << "a_hat: " << print_ints(hatx, bits, 1) << endl;

    hatx = binboot(hatx);

    Ctxt hatx_clone = hatx->Clone();

    hatx = rot(hatx, bits - 1 - LUT_BITS);


    // Now the index must be in decimal to be given as input to the Chebyshev-LUT
    vector<double> mask(hatx->GetSlots());

    for (int j = 0; j < zslots; j++) {
        int stride = bits * bits / 2;
        for (int i = 0; i < LUT_BITS; i++) mask[stride * j + i] = pow(2, i);
    }

    Ctxt idx = mult(hatx, mask);

    //N.b. idx \in [0, 256]
    for (int i = 0; i < log2(LUT_BITS); i++) {
        idx = add(idx, rot(idx, pow(2, i)));
    }

    fill(mask.begin(), mask.end(), 0.0);

    for (int j = 0; j < zslots; j++) {
        int stride = bits * bits/2;
        mask[j * stride] = 1;
    }

    idx = mult(idx, mask);

    Ctxt idx_masked_clone = idx->Clone();

    for (int i = 0; i < log2(bits); i++) {
        idx = add(idx, rot(idx, -pow(2, i)));
    }

    idx = add(idx, rot(idx_masked_clone, -bits));
    idx = add(idx, rot(idx_masked_clone, -bits-1));

    if (verbose) print(idx, bits+3);

    //Computed the index i

    coeffs.clear();

    if (bits == 64) {
        for (int i = 0; i < bits; i++) coeffs.push_back(read_vector_file(
                    "../coeffs/LUTs/" + to_string(bits) + " bits/square-root/LUT-SQUARE-ROOT-" + to_string(bits) +
                    "-BITS-" + to_string(i) + ".txt"));
        for (int i = 0; i < (bits * bits / 2) - bits; i++) coeffs.push_back(read_vector_file(
                    "../coeffs/LUTs/" + to_string(bits) + " bits/square-root/LUT-SQUARE-ROOT-" + to_string(bits) +
                    "-BITS-20.txt")); //Garbage
    } else if (bits == 128) {
        for (int i = 0; i < bits; i++) coeffs.push_back(read_vector_file(
                    "../coeffs/LUTs/" + to_string(bits) + " bits/square-root/LUT-SQUARE-ROOT-" + to_string(bits) +
                    "-BITS-" + to_string(i) + ".txt"));
        for (int i = 0; i < (bits * bits / 2) - bits; i++) coeffs.push_back(read_vector_file(
                    "../coeffs/LUTs/" + to_string(bits) + " bits/square-root/LUT-SQUARE-ROOT-" + to_string(bits) +
                    "-BITS-80.txt")); //Garbage
    } else {
        for (int i = 0; i < bits + 2; i++) coeffs.push_back(read_vector_file(
                    "../coeffs/LUTs/" + to_string(bits) + " bits/square-root/LUT-SQUARE-ROOT-" + to_string(bits) +
                    "-BITS-" + to_string(i) + ".txt"));
        for (int i = 0; i < (bits * bits / 2) - (bits + 2); i++) coeffs.push_back(read_vector_file(
                    "../coeffs/LUTs/" + to_string(bits) + " bits/square-root/LUT-SQUARE-ROOT-" + to_string(bits) +
                    "-BITS-0.txt")); //Garbage
    }

    if (bits == 64) {
        coeffs[0] = coeffs[12]; //Cause the first is degree-zero
        coeffs[1] = coeffs[13];
        coeffs[2] = coeffs[14];
        coeffs[3] = coeffs[15];
        coeffs[4] = coeffs[15];
        coeffs[5] = coeffs[15];
        coeffs[6] = coeffs[15];
        coeffs[7] = coeffs[15];
        coeffs[8] = coeffs[15];
        coeffs[9] = coeffs[15];
        coeffs[10] = coeffs[15];
    } else if (bits == 128) {
        for (int i = 0 ; i < 75; i++) coeffs[i] = coeffs[80];
        coeffs[126] = coeffs[78];
        coeffs[127] = coeffs[78];
    }

    Ctxt x = get_context()->EvalChebyshevSeriesPSBatchRepeated(idx, coeffs, 0, 256, (int)(idx->GetSlots() / coeffs.size()));

    if (bits == 16) {
        //Slots 14, 16, 17 must be empty as polynomials there are null (so PS returns something that is not correct)
        fill(mask.begin(), mask.end(), 1.0);
        for (int j = 0; j < zslots; j++) {
            int stride = bits * bits / 2;
            mask[stride * j + 14] = 0;
            mask[stride * j + 16] = 0;
            mask[stride * j + 17] = 0;
        }

        x = mult(x, encode(mask, x->GetLevel()));
    } else if (bits == 32) {
        //Slots 14, 16, 17 must be empty as polynomials there are null (so PS returns something that is not correct)
        fill(mask.begin(), mask.end(), 1.0);

        for (int j = 0; j < zslots; j++) {
            int stride = bits * bits / 2;
            mask[stride * j + 30] = 0;
            mask[stride * j + 32] = 0;
            mask[stride * j + 33] = 0;
        }
        x = mult(x, encode(mask, x->GetLevel()));
    } else if (bits == 64) {
        //Slots 14, 16, 17 must be empty as polynomials there are null (so PS returns something that is not correct)
        fill(mask.begin(), mask.end(), 1.0);

        for (int j = 0; j < zslots; j++) {
            int stride = bits * bits / 2;
            mask[stride * j + 62] = 0;
            mask[stride * j + 64] = 0;
            mask[stride * j + 65] = 0;
            mask[stride * j + 10] = 0;
            mask[stride * j + 9] = 0;
            mask[stride * j + 8] = 0;
            mask[stride * j + 7] = 0;
            mask[stride * j + 6] = 0;
            mask[stride * j + 5] = 0;
            mask[stride * j + 4] = 0;
            mask[stride * j + 3] = 0;
            mask[stride * j + 2] = 0;
            mask[stride * j + 1] = 0;
            mask[stride * j + 0] = 0;
        }

        x = mult(x, encode(mask, x->GetLevel()));
    } else if (bits == 128) {
        fill(mask.begin(), mask.end(), 1.0);

        for (int j = 0; j < zslots; j++) {
            int stride = bits * bits / 2;

            for (int i = 0; i < 75; i++) mask[stride * j + i] = 0;
            mask[stride * j + 126] = 0;
            mask[stride * j + 127] = 0; //This is always 1 i think looking at the poly
            mask[stride * j + 128] = 0;
            mask[stride * j + 129] = 0;
        }

        x = mult(x, encode(mask, x->GetLevel()));

        fill(mask.begin(), mask.end(), 0.0);
        for (int j = 0; j < zslots; j++) {
            int stride = bits * bits / 2;
            mask[stride * j + 127] = 1;
        }

        x = add(x, encode(mask, x->GetLevel()));
    }

    x = binboot(x);

    if (verbose) cout << "Output of LUT: " << print_ints(x, bits+2, zslots) << endl;


    for (int i = 0; i < ceil(log2(bits / LUT_BITS)) - 1; i++) {
        /*
         * x2 = (x * x) >> F
         */
        Ctxt x2 = mul_integer(x, x, bits, bits, zslots, zslots, true);
        x2 = rot(x2, bits);
        x2 = rot(x2, -1);
        if (verbose) cout << "x2: " << print_ints(x2, bits, 1) << endl;
        /*
         * mx2 = (m * x2) >> bits
         */
        Ctxt mx2 = mul_integer(hatx_clone, x2, bits, bits, zslots, zslots, true);
        mx2 = rot(mx2, bits);
        if (verbose) cout << "MX2 FACTORS: " << print_ints(hatx_clone, bits, 1) << ", " << print_ints(x2, bits, 1) << endl;
        if (verbose) cout << "mx2: " << print_ints(mx2, bits, 1) << endl;
        /*
         * term1 = (3 << F) - mx2
         */
        vector<uint128_t> a(zslots);
        for (uint32_t j = 0; j < a.size(); j++) a[j] = 3;
        //for (int j = 0; j < (2 * get_context()->GetRingDimension() / (bits * bits)) - 1; j++) {
        //    a.push_back(0);
        //}
        Ctxt const3f = encrypt_multi_int_nonpowtwo(a, bits, mx2->GetSlots(), mx2->GetLevel());
        const3f = rot(const3f, -bits);
        const3f = rot(const3f, 1);
        fill(mask.begin(), mask.end(), 0.0);
        for (int j = 0; j < zslots; j++) {
            int stride = bits * bits / 2;
            mask[stride * j] = 1;
        }
        const3f = add(const3f, encode(mask, const3f->GetLevel()));

        //Term1 = const3f - mxx
        //Ctxt term1 = binboot(sub_integer(const3f, mx2, bits + 1, bits * 2));

        fill(mask.begin(), mask.end(), 0.0);
        for (int j = 0; j < zslots; j++) {
            int stride = bits * bits / 2;
            for (int z = 0; z < bits * 2; z++) mask[z + j * stride] = 1;
        }

        Ctxt inverted = context->EvalSub(encrypt(encode(mask, mx2->GetLevel())), mx2);
        Ctxt term1 = binboot(add_integer(const3f, inverted, bits * 2, false));




        if (verbose) cout << "term1: " << print_ints(term1, bits * 2, 1) << endl;
        /*
         * x = (x * term1) >> (F + 1)
         */



        fill(mask.begin(), mask.end(), 0.0);
        for (int j = 0; j < zslots; j++) {
            int stride = bits * bits / 2;
            for (int z = 0; z < bits + 2; z++) {
                mask[z + stride * j] = 1;
            }
        }

        x = mult(x, encode(mask, x->GetLevel()));
        term1 = mult(term1, encode(mask, term1->GetLevel()));

        if (verbose) cout << "TERM 1 BITS" << endl;
        if (verbose) print(term1, bits + 10);


        //Test, sposto a sx di uno e li considero 128 bit, tanto poi shifto di bits....
        //x = mul_integer(x, term1, bits*2, bits*2, 1, 1, true);
        Ctxt term1_lo = term1->Clone();
        Ctxt term1_hi = term1->Clone();

        fill(mask.begin(), mask.end(), 0.0);
        for (int z = 0; z < zslots; z++) {
            int stride = bits * bits / 2;
            for (int j = 0; j < bits; j++) mask[j + z * stride] = 1;
        }
        term1_lo = mult(term1_lo, mask);

        fill(mask.begin(), mask.end(), 0.0);
        for (int j = 0; j < zslots; j++) {
            int stride = bits * bits / 2;
            mask[bits + (j * stride)] = 1;
        }
        term1_hi = rot(mult(term1_hi, mask), bits);
        for (int j = 0; j < log2(bits); j++) term1_hi = add(term1_hi, rot(term1_hi, -pow(2, j)));
        if (verbose) cout << "T1 hi mask: ";
        if (verbose) print(term1_hi, bits);

        term1_hi = mult(term1_hi, x);

        if (verbose) cout << "T1 hi: " << print_ints(term1_hi, bits * 2, 1) << endl;
        if (verbose) print(term1_hi, bits);
        if (verbose) cout << "T1 lo: " << print_ints(term1_lo, bits, 1) << endl;


        Ctxt xpartial = mul_integer(x, term1_lo, bits, bits, zslots, zslots, true);
        xpartial = rot(xpartial, bits);

        fill(mask.begin(), mask.end(), 0.0);
        for (int j = 0; j < zslots; j++) {
            int stride = bits * bits / 2;
            for (int z = 0; z < bits; z++) mask[z + j * stride] = 1;
        }
        xpartial = mult(xpartial, mask);

        if (verbose) cout << "xpartial: " << print_ints(xpartial, bits, 1) << endl;
        xpartial = add_integer(xpartial, term1_hi, bits, false);

        x = binboot(xpartial);


        if (verbose) cout << "x: " << print_ints(x, bits, 1) << endl;

    }

    Ctxt y = mul_integer(x, hatx_clone, bits, bits, zslots, zslots, true);
    y = rot(y, bits);

    //Con 193596853352420758544802764741038508842 la y è giusta a 128 bits (matcha)

    if (verbose) cout << "y: " << print_ints(y, bits, 1) << endl;

    //i can use the first bit of s as it has bits - bl, but bits = 0 mod 2 so it will get the parity of bl
    Ctxt parity = s->Clone();
    fill(mask.begin(), mask.end(), 0.0);
    for (int j = 0; j < zslots; j++) {
        int stride = bits * bits / 2;
        mask[stride * j] = 1;
    }

    parity = mult(parity, mask);
    for (int j = 0; j < log2(bits); j++) {
        parity = add(parity, rot(parity, -pow(2, j)));
    }

    if (verbose) cout << "Parity: ";
    if (verbose) print(parity, bits);

    Ctxt parityinverse = parity->Clone();
    fill(mask.begin(), mask.end(), 0.0);
    for (int z = 0; z < zslots; z++) {
        int stride = bits * bits / 2;
        for (int j = 0; j < bits; j++) {
            mask[j + z * stride] = 1;
        }
    }
    parityinverse = sub(encode(mask, parityinverse->GetLevel()), parityinverse);

    uint128_t ONE_FP = (uint128_t)1 << (bits - 1);
    uint128_t SQRT2_FP = static_cast<uint128_t>(1.4142135623730951 * (1ULL << (bits - 1)));

    if (bits == 128) {
        //Computed in Py as its messy to do that in c++
        SQRT2_FP = (static_cast<uint128_t>(0xb504f333f9de6484ULL) << 64) |
                   static_cast<uint128_t>(0x597d89b3754abe9fULL);
    }

    vector<uint128_t> a(context->GetRingDimension() / (bits * bits));
    for (uint32_t i = 0; i < a.size(); i++) a[i] = ONE_FP;
    Ctxt ONE_FP_CTXT = encrypt_multi_int(a, bits, parity->GetLevel());
    for (uint32_t i = 0; i < a.size(); i++) a[i] = SQRT2_FP;
    Ctxt SQRT2_FP_CTXT = encrypt_multi_int(a, bits, parity->GetLevel());

    Ctxt correction = add(mult(SQRT2_FP_CTXT, parity), mult(ONE_FP_CTXT, parityinverse));

    correction = binboot(correction);

    if (verbose) cout << "CORR: " << print_ints(correction, bits, 1) << endl;

    y = mul_integer(y, correction, bits, bits, zslots, zslots, true);

    if (verbose) cout << "Y final: " << print_ints(y, bits * 2, 1) << endl;
    if (verbose) print(y, bits * 2 + 2);


    for (uint32_t i = 0; i < a.size(); i++) a[i] = bits + 1;
    Ctxt finalshift = encrypt_multi_int(a, bits, y->GetLevel());

    finalshift = binboot(sub_integer(finalshift, s, bits)); //This should contain bit_length
    finalshift = rot(finalshift, 1);

    if (verbose) cout << "HA BL/2?: " << print_ints(finalshift, bits, 1) << endl;


    Ctxt yrot = blind_rotation(y, finalshift, bits * 4, zslots, bits * bits / 2);
    if (verbose) cout << "BLted?: " << print_ints(yrot, bits*4, 1) << endl;
    if (verbose) print(yrot, bits * 4);

    yrot = rot(yrot, bits);
    yrot = rot(yrot, bits);
    yrot = rot(yrot, -1);
    yrot = rot(yrot, -1);

    if (verbose) print(yrot, bits);
    if (verbose) cout << "Y rot: " << print_ints(yrot, bits, 1) << endl;

    //Cleaning garbage in other slots
    fill(mask.begin(), mask.end(), 0.0);
    for (int z = 0; z < zslots; z++) {
        int stride = bits * bits / 2;
        for (int j = 0; j < bits; j++) {
            mask[j + z * stride] = 1;
        }
    }

    return binboot(mult(yrot, encode(mask, yrot->GetLevel())));
}

Ctxt CKKSController::eq_integer(const Ctxt &a, const Ctxt &b, int bits, int zslots) {
    Ctxt sum = square(sub(a, b));
    for (int i = 0; i < round(log2(bits)); i++) {
        sum = add(sum, rot(sum, pow(2, i)));
    }

    int deg = 119;

    if (bits <= 16) {
        deg = 59;
    } else if (bits == 32) {
        deg = 59;
    } else if (bits == 64) {
        deg = 119;
    } else if (bits == 128) {
        deg = 223;
    } else if (bits > 128) {
        deg = 425;
    }

    sum = context->EvalChebyshevFunction([](double x) -> double { if (x == 0) return 1; else return sin(x) / x; }, sum, 0, bits, deg);

    vector<double> correction(a->GetSlots());
    for (uint32_t i = 0; i < zslots; i++)
        correction[i * (bits * bits) / 2] = 1;

    return binboot(mult(sum, encode(correction, sum->GetLevel())));
}




void CKKSController::ascon_permutation(Ctxt &S, int zslots) {
    //Assuming 12 round
    uint64_t Rconst[12][5] = {};
    for (int r = 0; r < 12; r++) {
        Rconst[r][2] = 0xf0 - r * 0x10 + r * 0x1;
    }

    Ptxt Rconst_ptxt[12];
    for (int i = 0; i < 12; i++) {
        vector<uint128_t> t(zslots);
        t[0] = Rconst[i][0];
        t[1] = Rconst[i][1];
        t[2] = Rconst[i][2];
        t[3] = Rconst[i][3];
        t[4] = Rconst[i][4];
        Rconst_ptxt[i] = encode_multi_int(t, 64, 12);
    }

    /*
    uint64_t MASK64[5] = {
            0xFFFFFFFFFFFFFFFFULL,
            0xFFFFFFFFFFFFFFFFULL,
            0xFFFFFFFFFFFFFFFFULL,
            0xFFFFFFFFFFFFFFFFULL,
            0xFFFFFFFFFFFFFFFFULL
    };
    */
    ;

    auto time = steady_clock::now();

    for (int i = 0; i < 12; i++) {
        S = square(sub(S, Rconst_ptxt[i]));

        cout << S->GetLevel() << endl;

        //Srot = rot(S, -1)
        Ctxt Srot = rot_integers(S, 64, -1);
        /*
         * Correcting the last term that does not go in the first
         */
        Ctxt masked_last = mask_one_slot(Srot, 64, 5);
        Srot = mask_one_slots(Srot, 64, {0, 2, 4});
        Srot = add(Srot, rot_integers(masked_last, 64, 5));


        //Srot[1] = 0, Srot[3] = 0
        //Srot = mask_zero_slots(Srot, 64, {1, 3});

        //Srot[3] = 0
        //Srot = mask_zero_slot(Srot, 64, 3);

        S = square(sub(S, Srot));

        vector<double> mask_inverse(S->GetSlots());
        for (int j = 0; j < 5; j++) {
            for (int z = 0; z < 64; z++) {
                mask_inverse[(j * (64 * 64) / 2) + z] = 1;
            }
        }


        Ctxt T = sub(encode(mask_inverse, S->GetLevel()), S);

        Srot = rot_integers(S, 64, 1);

        /*
         * Correcting the first term that does not go in the last
         */
        masked_last = mask_one_slot(Srot, 64, zslots - 1);
        //Srot = mask_zero_slot(Srot, 64, 4);
        Srot = add(Srot, rot_integers(masked_last, 64, zslots - 5));

        T = mult(T, Srot);

        Ctxt Trot = rot_integers(T, 64, 1);
        /*
         * Correcting the last term that does not go in the first
         */
        masked_last = mask_one_slot(Trot, 64, zslots - 1);
        Trot = mask_zero_slot(Trot, 64, 4);
        Trot = add(Trot, rot_integers(masked_last, 64, zslots - 5));

        S = square(sub(S, Trot));

        Srot = rot_integers(S, 64, -1);
        /*
         * s[4] = 0, but s[2] = 0xFFFFFFFFFFFFFFFF
         */
        Ctxt Srot_clone = Srot->Clone();

        Srot = mask_zero_slots(Srot, 64, {0, 2, 4});
        Srot = add(Srot, rot_integers(mask_one_slot(Srot_clone, 64, 5), 64, 5));

        vector<double> all_ones(Srot->GetSlots());
        for (int j = (64 * 64) / 2 * 2; j < ((64 * 64) / 2 * 2) + 64; j++) all_ones[j] = 1;

        Srot = add(Srot, encode(all_ones, Srot->GetLevel()));

        S = square(sub(S, Srot));

        //There is garbage around, but the first 5 slots are correct
        //S = mask_one_slots(S, 64, {0, 1, 2, 3, 4});

        Ctxt S0 = local_rot_masked(S, 64, 0, 19);

        S0 = add(S0, local_rot_masked(S, 64, 1, 61));
        S0 = add(S0, local_rot_masked(S, 64, 2, 1));
        S0 = add(S0, local_rot_masked(S, 64, 3, 10));
        S0 = add(S0, local_rot_masked(S, 64, 4, 7));
        S0 = correct_local_rot(S0, 64, zslots);


        Ctxt S1 = local_rot_masked(S, 64, 0, 28);
        S1 = add(S1, local_rot_masked(S, 64, 1, 39));
        S1 = add(S1, local_rot_masked(S, 64, 2, 6));
        S1 = add(S1, local_rot_masked(S, 64, 3, 17));
        S1 = add(S1, local_rot_masked(S, 64, 4, 41));
        S1 = correct_local_rot(S1, 64, zslots);

        S = square(sub(mask_one_slots(S, 64, {0, 1, 2, 3, 4}), square(sub(S0, S1))));

        S = binboot(S);

        S = mask_one_slots(S, 64, {0, 1, 2, 3, 4});

        cout << "Iteration " << i << ", " << print_ints(S, 64, zslots) << endl;
    }

    print_duration(time, "Hashing took: ");
}

Ctxt CKKSController::mask_zero_slot(const Ctxt &c, int bits, int slot) {
    vector<double> mask(c->GetSlots());

    for (uint32_t i = 0; i < mask.size(); i++) {
        mask[i] = 1;
    }

    for (int i = 0; i < bits; i++) {
        mask[slot * ((bits * bits) / 2) + i] = 0;
    }

    return mult(c, mask);
}

Ctxt CKKSController::mask_zero_slots(const Ctxt &c, int bits, vector<int> slots) {
    vector<double> mask(c->GetSlots());

    for (uint32_t i = 0; i < mask.size(); i++) {
        mask[i] = 1;
    }

    for (uint32_t j = 0; j < slots.size(); j++) {
        for (int i = 0; i < bits; i++) {
            mask[slots[j] * ((bits * bits) / 2) + i] = 0;
        }
    }

    return mult(c, mask);
}

Ctxt CKKSController::mask_one_slot(const Ctxt &c, int bits, int slot) {
    vector<double> mask(c->GetSlots());

    for (int i = 0; i < bits; i++) {
        mask[(slot * ((bits * bits) / 2)) + i] = 1;
    }

    return mult(c, mask);
}

Ctxt CKKSController::mask_one_slots(const Ctxt &c, int bits, vector<int> slots) {
    vector<double> mask(c->GetSlots());

    for (uint32_t i = 0; i < mask.size(); i++) {
        mask[i] = 0;
    }

    for (uint32_t j = 0; j < slots.size(); j++) {
        for (int i = 0; i < bits; i++) {
            mask[slots[j] * ((bits * bits) / 2) + i] = 1;
        }
    }

    return mult(c, mask);
}

Ctxt CKKSController::local_rot_masked(const Ctxt &c, int bits, int slot, int index_rot) {
    Ctxt res = rot(c, -bits);
    res = rot(res, index_rot);

    vector<double> mask(c->GetSlots());

    for (int i = (bits * bits) / 2 * slot; i < ((bits * bits) / 2 * slot) + bits * 2; i++) {
        mask[i] = 1;
    }

    return mult(res, mask);

}

Ctxt CKKSController::correct_local_rot(const Ctxt &c, int bits, int zslots) {
    //Maschero i 'bits' bits dopo ogni slot e li ruoto di bits indietro
    vector<double> mask;

    for (int i = 0; i < zslots; i++) {
        for (int j = 0; j < bits; j++) {
            mask.push_back(1);
        }
        for (int j = 0; j < ((bits * bits) / 2) - bits; j++) {
            mask.push_back(0);
        }
    }

    Ctxt result = mult(c, encode(mask, c->GetLevel()));
    result = rot(result, -bits);
    return rot(add(c, result), bits);

}

Ctxt CKKSController::reduce(const Ctxt &c) {
    return context->EvalSub(context->EvalMult(c, 2), context->EvalSquare(c));
}

Ctxt CKKSController::bootstrap(const Ctxt &c) {
    //cout << "Lv boot : " << c->GetLevel() << endl;
    Ctxt cboot = context->EvalBootstrap(c);
    //cout << "Lv after: " << cboot->GetLevel() << endl;
    return cboot;
}

Ctxt CKKSController::binboot(const Ctxt &c) {
    //cout << "Input level : " << c->GetLevel() << endl;
    Ctxt cboot = context->EvalBootstrapStCFirstBits(c);
    //cout << "Output level: " << cboot->GetLevel() << endl;
    return cboot;
}

Ctxt CKKSController::chebyshev(const Ctxt &c, vector<double> coeffs, int a, int b) {
    return context->EvalChebyshevSeries(c, coeffs, a, b);
}

Ctxt CKKSController::chebyshev_batch(const Ctxt &c, vector<vector<double>> coeffs, int a, int b) {
    return context->EvalChebyshevSeriesPSBatch(c, coeffs, a, b);
}

Ctxt CKKSController::chebyshev_batch_rep(const Ctxt &c, vector<vector<double>> coeffs, int a, int b, int repetitions) {
    return context->EvalChebyshevSeriesPSBatchRepeated(c, coeffs, a, b, repetitions);
}

void CKKSController::print(const Ctxt &c) {
    int s = c->GetSlots();

    Ptxt result;
    context->Decrypt(key_pair.secretKey, c, &result);
    result->SetSlots(s);
    vector<double> v = result->GetRealPackedValue();

    cout << "(Level: " << c->GetLevel() << ") [ ";

    for (int i = 0; i < s; i += 1) {
        string segno = "";
        if (v[i] > 0) {
            segno = "";
        } else {
            segno = "-";
            v[i] = -v[i];
        }


        if (static_cast<uint32_t>(i) == slots - 1) {
            if (abs(v[i]) <= 0.0001)
                cout << "0 ]";
            else {
                cout << segno << v[i] << " ]";
            }
        } else {
            if (abs(v[i]) <= 0.0001)
                cout << "0" << " ";
            else
                cout << segno << v[i] << " ";
        }
    }

    cout << endl;
}

void CKKSController::print(const Ctxt &c, int slots) {
    int s = slots;

    Ptxt result;
    context->Decrypt(key_pair.secretKey, c, &result);
    result->SetSlots(s);
    vector<double> v = result->GetRealPackedValue();

    cout << "(Level: " << c->GetLevel() << ") [ ";

    for (int i = 0; i < s; i += 1) {
        string segno = "";
        if (v[i] > 0) {
            segno = "";
        } else {
            segno = "-";
            v[i] = -v[i];
        }


        if (static_cast<uint32_t>(i) == static_cast<uint32_t>(slots - 1)) {
            if (abs(v[i]) <= 0.0001)
                cout << "0 ]";
            else {
                cout << segno << v[i] << " ]";
            }
        } else {
            if (abs(v[i]) <= 0.0001)
                cout << "0" << " ";
            else
                cout << segno << v[i] << " ";
        }
    }

    cout << endl;
}

string CKKSController::print_ints(const Ctxt &c, int bits, int s, bool doublebits) {
    return to_string_uint128(ptxt_to_vec(decode(decrypt(c)), s, bits, doublebits));
}

void CKKSController::print_moduli_chain(const lbcrypto::DCRTPoly &poly) {
    cout << "log(N): " << std::round(log(context->GetRingDimension())/log(2)) << ", circuit depth: " << depth << endl;
}

CryptoContext<DCRTPoly> CKKSController::get_context() {
    return context;
}
