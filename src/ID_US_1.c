/* Portable US_RndT state and US_LineInput scan translation. */
#include "ID_US_1.h"

static const uint8_t wg_random_table[256] =
{
    0,8,109,220,222,241,149,107,75,248,254,140,16,66,74,21,
    211,47,80,242,154,27,205,128,161,89,77,36,95,110,85,48,
    212,140,211,249,22,79,200,50,28,188,52,140,202,120,68,145,
    62,70,184,190,91,197,152,224,149,104,25,178,252,182,202,182,
    141,197,4,81,181,242,145,42,39,227,156,198,225,193,219,93,
    122,175,249,0,175,143,70,239,46,246,163,53,163,109,168,135,
    2,235,25,92,20,145,138,77,69,166,78,176,173,212,166,113,
    94,161,41,50,239,49,111,164,70,60,2,37,171,75,136,156,
    11,56,42,146,138,229,73,146,77,61,98,196,135,106,63,197,
    195,86,96,203,113,101,170,247,181,113,80,250,108,7,255,237,
    129,226,79,107,112,166,103,241,24,223,239,120,198,58,60,82,
    128,3,184,66,143,224,145,224,81,206,163,45,63,90,168,114,
    59,33,159,95,28,139,123,98,125,196,15,70,194,253,54,14,
    109,226,71,17,161,93,186,87,244,138,20,52,123,251,26,36,
    17,46,52,231,232,76,31,221,84,37,216,165,212,106,197,242,
    98,43,39,175,254,145,190,84,118,222,187,136,120,163,236,249
};

static const uint8_t id_us_ascii_names[128] =
{
    0,27,'1','2','3','4','5','6','7','8','9','0','-','=',8,9,
    'q','w','e','r','t','y','u','i','o','p','[',']',13,0,'a','s',
    'd','f','g','h','j','k','l',';',39,'`',0,92,'z','x','c','v',
    'b','n','m',',','.','/',0,'*',0,' ',0,0,0,0,0,0,
    0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+','1',
    '2','3','0',127
};

static const uint8_t id_us_shift_names[128] =
{
    0,27,'!','@','#','$','%','^','&','*','(',')','_','+',8,9,
    'Q','W','E','R','T','Y','U','I','O','P','{','}',13,0,'A','S',
    'D','F','G','H','J','K','L',':',34,'~',0,'|','Z','X','C','V',
    'B','N','M','<','>','?',0,'*',0,' ',0,0,0,0,0,0,
    0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+','1',
    '2','3','0',127
};

void WG_RandomSeed(wg_random_t *random, uint8_t index)
{
    if (random != 0)
    {
        random->index = index;
    }
}

uint8_t WG_RandomNext(wg_random_t *random)
{
    if (random == 0)
    {
        return 0;
    }
    ++random->index;
    return wg_random_table[random->index];
}

char ID_US_ScanToASCII(uint16_t scan_code, int shifted, int caps_lock)
{
    uint8_t character;

    if (scan_code >= 128U)
    {
        return 0;
    }
    character = shifted ? id_us_shift_names[scan_code]
                        : id_us_ascii_names[scan_code];
    if (caps_lock && character >= 'a' && character <= 'z')
    {
        character = (uint8_t)(character - ('a' - 'A'));
    }
    else if (caps_lock && character >= 'A' && character <= 'Z')
    {
        character = (uint8_t)(character + ('a' - 'A'));
    }
    return (char)character;
}

const char *ID_US_ScanName(uint16_t scan_code)
{
    static char single[2];
    char character;

    switch (scan_code)
    {
        case 0x01U: return "Esc";
        case 0x0dU: return "+";
        case 0x0eU: return "BkSp";
        case 0x0fU: return "Tab";
        case 0x1cU: return "Enter";
        case 0x1dU: return "Ctrl";
        case 0x2aU: return "LShft";
        case 0x2bU: return "|";
        case 0x36U: return "RShft";
        case 0x37U: return "PrtSc";
        case 0x38U: return "Alt";
        case 0x39U: return "Space";
        case 0x3aU: return "CapsLk";
        case 0x3bU: return "F1";
        case 0x3cU: return "F2";
        case 0x3dU: return "F3";
        case 0x3eU: return "F4";
        case 0x3fU: return "F5";
        case 0x40U: return "F6";
        case 0x41U: return "F7";
        case 0x42U: return "F8";
        case 0x43U: return "F9";
        case 0x44U: return "F10";
        case 0x45U: return "NumLk";
        case 0x46U: return "ScrlLk";
        case 0x47U: return "Home";
        case 0x48U: return "Up";
        case 0x49U: return "PgUp";
        case 0x4aU: return "-";
        case 0x4bU: return "Left";
        case 0x4cU: return "5";
        case 0x4dU: return "Right";
        case 0x4eU: return "+";
        case 0x4fU: return "End";
        case 0x50U: return "Down";
        case 0x51U: return "PgDn";
        case 0x52U: return "Ins";
        case 0x53U: return "Del";
        case 0x57U: return "F11";
        case 0x58U: return "F12";
        default: break;
    }
    character = ID_US_ScanToASCII(scan_code, 0, 1);
    if (character < 32 || character >= 127)
    {
        return "?";
    }
    single[0] = character;
    single[1] = '\0';
    return single;
}
