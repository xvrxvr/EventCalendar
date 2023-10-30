#include "common.h"


static const uint8_t s_b0_df[] = {
    0xE2, 0x96, 0x91, 0xE2, 0x96, 0x92, 0xE2, 0x96, 0x93, 0xE2, 0x94, 0x82, 0xE2, 0x94, 0xA4, 0xE2, 0x95, 0xA1, 0xE2, 0x95, 0xA2, 0xE2, 0x95, 0x96, 
    0xE2, 0x95, 0x95, 0xE2, 0x95, 0xA3, 0xE2, 0x95, 0x91, 0xE2, 0x95, 0x97, 0xE2, 0x95, 0x9D, 0xE2, 0x95, 0x9C, 0xE2, 0x95, 0x9B, 0xE2, 0x94, 0x90,
    0xE2, 0x94, 0x94, 0xE2, 0x94, 0xB4, 0xE2, 0x94, 0xAC, 0xE2, 0x94, 0x9C, 0xE2, 0x94, 0x80, 0xE2, 0x94, 0xBC, 0xE2, 0x95, 0x9E, 0xE2, 0x95, 0x9F, 
    0xE2, 0x95, 0x9A, 0xE2, 0x95, 0x94, 0xE2, 0x95, 0xA9, 0xE2, 0x95, 0xA6, 0xE2, 0x95, 0xA0, 0xE2, 0x95, 0x90, 0xE2, 0x95, 0xAC, 0xE2, 0x95, 0xA7,
    0xE2, 0x95, 0xA8, 0xE2, 0x95, 0xA4, 0xE2, 0x95, 0xA5, 0xE2, 0x95, 0x99, 0xE2, 0x95, 0x98, 0xE2, 0x95, 0x92, 0xE2, 0x95, 0x93, 0xE2, 0x95, 0xAB, 
    0xE2, 0x95, 0xAA, 0xE2, 0x94, 0x98, 0xE2, 0x94, 0x8C, 0xE2, 0x96, 0x88, 0xE2, 0x96, 0x84, 0xE2, 0x96, 0x8C, 0xE2, 0x96, 0x90, 0xE2, 0x96, 0x80
};

static const uint8_t s_f0_ff[] = {
    0xD0, 0x81, 0x00, 0xD1, 0x91, 0x00, 0xD0, 0x84, 0x00, 0xD1, 0x94, 0x00, 0xD0, 0x87, 0x00, 0xD1, 0x97, 0x00, 0xD0, 0x8E, 0x00, 0xD1, 0x9E, 0x00,
    0xC2, 0xB0, 0x00, 0xE2, 0x88, 0x99, 0xC2, 0xB7, 0x00, 0xE2, 0x88, 0x9A, 0xE2, 0x84, 0x96, 0xC2, 0xA4, 0x00, 0xE2, 0x96, 0xA0, 0x20, 0x00, 0x00
};

void utf8_to_dos(char* sym)
{
    uint8_t* p = (uint8_t*)sym;
    uint8_t* dst = p;
#define S(from, to) case 0x##from: *dst++ = 0x##to; break
#define U default: *dst++ = '?'; while(*p & 0x80) ++p; break
    while(*p)
    {
        if (*p < 0x80) *dst++ = *p++; else
        switch(*p++)
        {
            case 0xD0:
                if (*p >= 0x90 && *p <= 0xBF) *dst++ = *p++ - 0x10; else
                switch(*p++) {S(81, F0); S(84, F2); S(87, F4); S(8E, F6); U;}
                break;
            case 0xD1:
                if (*p >= 0x80 && *p <= 0x8F) *dst++ = *p++ + (0xE0 - 0x80); else
                switch(*p++) {S(91, F1); S(94, F3); S(97, F5); S(9E, F7); U;}
                break;
            case 0xC2:
                switch(*p++) {S(B0, F8); S(B7, FA); S(A4, FD); U;} 
                break;
            case 0xE2:
                switch(*p++)
                {
                    case 0x88: switch(*p++) {S(99, F9); S(9A, FB); U;} break;
                    case 0x84: switch(*p++) {S(96, FC); U;} break;
                    case 0x96: switch(*p++) {S(A0, FE); U;} break;
                    default:
                    {
                        uint8_t s1 = *p++;
                        uint8_t s2 = *p++;
                        const uint8_t* table = s_b0_df + 1;
                        *dst = '?';
                        for(int i=0; i < 48; ++i, table +=3)
                        {
                            if (table[0] == s1 && table[1] == s2) {*dst = 0xB0 + i; break;}
                        }
                        break;
                    }
                }
                break;
            U;
        }
    }
    *dst = 0;
}
#undef U
#undef S

U dos_to_utf8(char sym)
{
    U result{};
    uint8_t s = uint8_t(sym);
    if (s < 0x80) {result.b[0] = sym; return result;}

    if (s <= 0xAF) {result.b[0] = 0xD0; result.b[1] = s+0x10;} else
    if (s <= 0xDF) {memcpy(result.b, s_b0_df + (s-0xDF)*3, 3);} else
    if (s <= 0xEF) {result.b[0] = 0xD1; result.b[1] = s-0x60;} 
    else {memcpy(result.b, s_f0_ff + (s-0xF0)*3, 3);}
    return result;
// 80-AF -> 0410-043F                | D0 90 - D0 BF
// B0-BF -> 2591     2592     2593     2502     2524     2561     2562     2556     2555     2563     2551     2557     255D     255C     255B     2510   X  
//          E2 96 91 E2 96 92 E2 96 93 E2 94 82 E2 94 A4 E2 95 A1 E2 95 A2 E2 95 96 E2 95 95 E2 95 A3 E2 95 91 E2 95 97 E2 95 9D E2 95 9C E2 95 9B E2 94 90
// C0-CF -> 2514 2534 252C 251C 2500 253C 255E 255F 255A 2554 2569 2566 2560 2550 256C 2567   X  E2 94 94 E2 94 B4 E2 94 AC E2 94 9C E2 94 80 E2 94 BC E2 95 9E E2 95 9F E2 95 9A E2 95 94 E2 95 A9 E2 95 A6 E2 95 A0 E2 95 90 E2 95 AC E2 95 A7
// D0-DF -> 2568 2564 2565 2559 2558 2552 2553 256B 256A 2518 250C 2588 2584 258C 2590 2580   X  E2 95 A8 E2 95 A4 E2 95 A5 E2 95 99 E2 95 98 E2 95 92 E2 95 93 E2 95 AB E2 95 AA E2 94 98 E2 94 8C E2 96 88 E2 96 84 E2 96 8C E2 96 90 E2 96 80
// E0-EF -> 0440-044F                | D1 80 - D1 8F
// F0 F1 -> 0401 0451                | D0 81,  D1 91
// F0-FF -> 0401  0451  0404  0454  0407  0457  040E  045E  00B0  2219     00B7  221A     2116     00A4  25A0     0020
//          D0 81 D1 91 D0 84 D1 94 D0 87 D1 97 D0 8E D1 9E C2 B0 E2 88 99 C2 B7 E2 88 9A E2 84 96 C2 A4 E2 96 A0 20
//          F0    F1    F2    F3    F4    F5    F6    F7    F8    F9       FA    FB       FC       FD    FE       FF
}
