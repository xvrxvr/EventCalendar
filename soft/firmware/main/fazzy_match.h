#pragma once

#include "prnbuf.h"

// Damerau–Levenshtein distance evaluator
class DV {
    const char* a;
    const char* b;

    unsigned d(int i, int j)
    {
        unsigned min_val = -1u;
        
        if (i < 0 || j < 0) return -1u;
        if (i == 0 && j == 0) return 0;

        const auto m = [&](int score) {min_val = std::min(min_val, score);};

        if (i > 0) m(d(i-1, j) + 1);
        if (j > 0) m(d(i, j-1) + 1);
        if (i > 0 && j > 0) m(d(i-1, j-1) + (a[i] != b[j]));
        if (i > 1 && j > 1 && a[i] == b[j-1] && a[i-1] == b[j]) m(d(i-2, j-2) + (a[i] != b[j]));
        return min_val;
    }

public:
    DV(const char* a, const char* b) : a(a), b(b) {}

    unsigned score() const {return d(strlen(a)-1, strlen(b)-1);}
};


class DVPlus {
    PrnBuf buf;
    char* ptrs[2];
    char* iters[2][2];

    int iter(int idx)
    {
        char* & src = iters[idx][0];
        char* & dst = iters[idx][1];
        while(*src)
        {
            if (*src == ',') {++src; return '.';}
            if (isdigit(*src) || *src == '.') return *src++;
            *dst ++= *src++;
        }
        *dst = 0;
        return 0;
    }

public:
    DVPlus(const char* a, const char* b)
    {
        buf.strcpy(a);
        buf.cat_fill(0, 1);
        int len = buf.length();
        buf.strcat(b);
        iters[0][0] = iters[0][1] = ptrs[0] = buf.c_str();
        iters[1][1] = iters[1][1] = ptrs[1] = ptrs[0]+len;
    }

    size_t size(int idx) const {return strlen(ptrs[idx]);}

    unsigned score()
    {
        bool digits = false;
        for(;;)
        {
            int s1 = iter(0);
            int s2 = iter(1);
            if (s1 == 0 && s2 == 0) break;
            digits = true;
            if (s1 != s2) return -1u;
        }
        int l1 = strlen(ptrs[0]);
        int l2 = strlen(ptrs[1]);
        if (l1 == 0 || l2 == 0) return digits ? (l1+l2) : -1u;
        return DV(ptrs[0], ptrs[1]).score();
    }
};
