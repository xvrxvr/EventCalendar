﻿#pragma once

#include "prnbuf.h"

// Damerau–Levenshtein distance evaluator
class DV {
    const char* a;
    const char* b;
    const size_t len_a;
    std::unique_ptr<uint8_t[]> cache;

    uint8_t& idx(int i, int j) {return cache[i+j*len_a];}

    unsigned d(int i, int j)
    {
        unsigned min_val = -1u;
        
        if (i == 0 && j == 0) return 0;

        uint8_t &c = idx(i, j);
        if (c != 0xFF) return c;

        const auto m = [&](unsigned score) {min_val = std::min(min_val, score);};

        if (i > 0) m(d(i-1, j) + 1);
        if (j > 0) m(d(i, j-1) + 1);
        if (i > 0 && j > 0) m(d(i-1, j-1) + (a[i] != b[j]));
        if (i > 1 && j > 1 && a[i] == b[j-1] && a[i-1] == b[j]) m(d(i-2, j-2) + (a[i] != b[j]));
        c = std::min(254u, min_val);
        return min_val;
    }

public:
    DV(const char* a, const char* b) : a(a), b(b), len_a(strlen(a)) {}

    unsigned score()
    {
        size_t len_b = strlen(b);
        size_t total = len_a * len_b;
        cache.reset(new uint8_t[total]);
        memset(cache.get(), 0xFF, total);
        return d(len_a-1, len_b-1);
    }
};


class DVPlus {
    static void separate(const std::string_view &src, Prn& body, Prn& digits)
    {
        for(auto s: src)
        {
            if (s == '.' || s==',') digits.cat_fill('.', 1); else
            if (isdigit(s)) digits.cat_fill(s, 1); else
            if (isalpha(s) || (s&0x80)) body.cat_fill(upcase(s), 1);
        }
    }

public:
    static unsigned compare(const std::string_view &a, const std::string_view &b, int& a_length)
    {
        Prn a_body, a_digits, b_body, b_digits;
        separate(a, a_body, a_digits);
        separate(b, b_body, b_digits);
        a_length = a_body.length();
        if (strcmp(a_digits.c_str(), b_digits.c_str())) return -1u;
        if (a_body.length() == 0) return b_body.length();
        if (b_body.length() == 0) return a_body.length();
        return DV(a_body.c_str(), b_body.c_str()).score();
    }
};
