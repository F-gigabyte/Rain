#include <convert.h>
#include <stdlib.h>
#include <ryu/ryu_parse.h>
#include <ryu/ryu.h>
#include <rain_memory.h>
#include <utf8.h>

bool str_to_int(int64_t* res, const char* str, size_t len)
{
    if(len > 2)
    {
        if(str[0] == '0')
        {
            switch(str[1])
            {
                case 'X':
                case 'x':
                {
                    return str_hex_to_int(res, str + 2, len - 2);
                }
                case 'B':
                case 'b':
                {
                    return str_bin_to_int(res, str + 2, len - 2);
                }
                case 'O':
                case 'o':
                {
                    return str_oct_to_int(res, str + 2, len - 2);
                }
                default:
                {
                    break;
                }
            }
            if(len > 3)
            {
                uint32_t letter = decode_utf8_char(str + 1, NULL, len - 1);
                switch(letter)
                {
                    // ш
                    case L'Ш':
                    case L'ш':
                    {
                        return str_hex_to_int(res, str + 3, len - 3);
                    }
                    // в
                    case L'В':
                    case L'в':
                    {
                        return str_oct_to_int(res, str + 3, len - 3);
                    }
                    // д
                    case L'Д':
                    case L'д':
                    {
                        return str_bin_to_int(res, str + 3, len - 3);
                    }
                    default:
                    {
                        break;
                    }
                }
            }
        }
    }
    return str_dec_to_int(res, str, len);
}

static bool bin_digit(char c)
{
    return '0' <= c && c <= '1';
}

static bool oct_digit(char c)
{
    return '0' <= c && c <= '7';
}

static bool dec_digit(char c)
{
    return '0' <= c && c <= '9';
}

static bool hex_digit(uint32_t c)
{
    return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <=c && c <= 'F') || (L'а' <= c && c <= L'д') || (L'А' <= c && c <= L'Д') || (L'Ґ' <= c && c <= L'ґ');
}

/*
 *  0111 -> 7
 *  1000
 *  1001 -> -8 + 1 = -7
 */
bool str_dec_to_int(int64_t* res, const char* str, size_t len)
{
    if(len == 0)
    {
        return false;
    }
    bool neg = false;
    if(str[0] == '-')
    {
        str++;
        neg = true;
        len--;
        if(len == 0)
        {
            return false;
        }
    }
    if(len > 19)
    {
        return false;
    }
    uint64_t mag = 0;
    for(size_t i = 0; i < len; i++)
    {
        if(!dec_digit(str[i]))
        {
            return false;
        }
        mag = mag * 10 + (str[i] - '0');
        if((mag >= 0x8000000000000000 && !neg) || (mag > ((uint64_t)1 << 63)))
        {
            return false;
        }
    }
    if(neg)
    {
        mag = (~mag) + 1;
    }
    *res = (int64_t)mag;
    return true;
}

bool str_hex_to_int(int64_t* res, const char* str, size_t len)
{
    if(len > 32 || len == 0)
    {
        return false;
    }
    uint64_t mag = 0;
    size_t digits = 0;
    for(size_t i = 0; i < len;)
    {
        uint8_t eaten = 0;
        wchar_t letter = decode_utf8_char(str + i, &eaten, len - i);
        if(!hex_digit(letter))
        {
            return false;
        }
        if(letter <= '9')
        {
            mag = mag * 16 + (letter - '0'); 
        }
        else if(letter <= 'F')
        {
            mag = mag * 16 + (letter - 'A') + 10;
        }
        else if(letter <= 'f')
        {
            mag = mag * 16 + (letter - 'a') + 10;
        }
        else if(letter ==  L'Д'|| letter == L'д')
        {
            mag = mag * 16 + 15;
        }
        else if(letter <= L'Г')
        {
            mag = mag * 16 + (letter - L'А') + 10;
        }
        else if(letter <= L'г')
        {
            mag = mag * 16 + (letter - L'а') + 10;
        }
        else
        {
            mag = mag * 16 + 14;
        }
        digits++;
        i += eaten;
    }
    if(digits > 16)
    {
        return false;
    }
    *res = (int64_t)mag;
    return true;
}

bool str_oct_to_int(int64_t* res, const char* str, size_t len)
{
    if(len > 22 || len == 0)
    {
        return false;
    }
    uint64_t mag = 0;
    for(size_t i = 0; i < len; i++)
    {
        if(!oct_digit(str[i]))
        {
            return false;
        }
        uint64_t next_mag = mag * 8 + (str[i] - '0');
        if(next_mag < mag)
        {
            return false;
        }
        mag = next_mag;
    }
    *res = (int64_t)mag;
    return true;
}

bool str_bin_to_int(int64_t* res, const char* str, size_t len)
{
    if(len > 64 || len == 0)
    {
        return false;
    }
    uint64_t mag = 0;
    for(size_t i = 0; i < len; i++)
    {
        if(!bin_digit(str[i]))
        {
            return false;
        }
        mag = mag * 2 + (str[i] - '0');
    }
    *res = (int64_t)mag;
    return true;
}

bool str_to_float(double* res, const char* str, size_t len)
{
    enum Status status = s2d_n(str, len, res);
    if(status != SUCCESS)
    {
        return false;
    }
    return true;
}

size_t get_num_digits(uint64_t mag, uint64_t base)
{
    size_t digits = 1;
    while(mag / base != 0)
    {
        digits++;
        mag /= base;
    }
    return digits;
}

char* int_to_dec_str(int64_t num)
{
    bool neg = num < 0 ? true : false;
    uint64_t mag = neg ? -num : num;
    size_t len = get_num_digits(mag, 10) + 1 + (neg ? 1 : 0);
    char* res = ALLOCATE(char, len);
    res[len - 1] = 0;
    if(neg)
    {
        res[0] = '-';
    }
    char* pos = res + (len - 2);
    *pos = '0';
    while(mag > 0)
    {
        *pos = (mag % 10) + '0';
        pos--;
        mag /= 10;
    }
    return res;
}

char* int_to_hex_str(int64_t num)
{
    uint64_t mag = num;
    size_t len = get_num_digits(num, 16) + 3;
    char* res = ALLOCATE(char, len);
    res[len - 1] = 0;
    res[0] = '0';
    res[1] = 'x';
    for(size_t i = len - 1; i > 1; i--)
    {
        uint8_t digit = mag & 0xf;
        if(digit < 10)
        {
            res[i] = digit + '0';
        }
        else
        {
            res[i] = (digit - 10) + 'a';
        }
        mag >>= 4;
    }
    return res;
}

char* int_to_oct_str(int64_t num)
{
    uint64_t mag = num;
    size_t len = get_num_digits(num, 8) + 3;
    char* res = ALLOCATE(char, len);
    res[len - 1] = 0;
    res[0] = '0';
    res[1] = 'o';
    for(size_t i = len - 1; i > 1; i--)
    {
        uint8_t digit = mag & 07;
        res[i] = digit + '0';
        mag >>= 3;
    }
    return res;
}

char* int_to_bin_str(int64_t num)
{
    uint64_t mag = num;
    size_t len = get_num_digits(num, 2) + 3;
    char* res = ALLOCATE(char, len);
    res[len - 1] = 0;
    res[0] = '0';
    res[1] = 'b';
    for(size_t i = len - 1; i > 1; i--)
    {
        res[i] = mag & 1;
        mag >>= 1;
    }
    return res;
}

char* float_to_str(double num)
{
    return d2s(num);
}
