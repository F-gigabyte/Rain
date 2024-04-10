#include <utf8.h>

// consumes bytes of utf-8 of the form 0b10xx xxxx -> true if valid or else false
static bool consume_middle(uint32_t* rep, const char* letter, uint8_t shift)
{
    if((*letter & 0xc0) == 0x80)
    {
        *rep |= (*letter & 0x3f) << shift;
        return true;
    }
    return false;
}

uint32_t decode_utf8_char(const char* letter, uint8_t* char_consumed, size_t len)
{
    uint32_t rep = 0;
    len == 0 ? (len = 4) : (len = len);
    if((*letter & 0x80) == 0)
    {
        if(char_consumed != NULL)
        {
            *char_consumed = 1;
        }
        rep = *letter;
        return rep;
    }
    else if(len > 1 && (*letter & 0xe0) == 0xc0)
    {
        rep |= ((uint32_t)(*letter & 0x1f)) << 6;
        letter++;
        if(consume_middle(&rep, letter, 0))
        {
            if(char_consumed != NULL)
            {
                *char_consumed = 2;
            }
            return rep;
        }
    }
    else if(len > 2 && (*letter & 0xf0) == 0xe0)
    {
        rep |= ((uint32_t)(*letter & 0xf)) << 12;
        letter++;
        if(consume_middle(&rep, letter, 6) && consume_middle(&rep, letter + 1, 0))
        {
            if(char_consumed != NULL)
            {
                *char_consumed = 3;
            }
            return rep;
        }
    }
    else if(len > 3 && (*letter & 0xf8) == 0xf0)
    {
        rep |= ((uint32_t)(*letter & 0x7)) << 18;
        letter++;
        if(consume_middle(&rep, letter, 12) && consume_middle(&rep, letter + 1, 6) && consume_middle(&rep, letter + 2, 0))
        {
            if(char_consumed != NULL)
            {
                *char_consumed = 4;
            }
            return rep;
        }
    }
    if(char_consumed != NULL)
    {
        *char_consumed = 0;
    }
    return 0;
}

uint32_t ord(const char* letter)
{
    return decode_utf8_char(letter, NULL, 0);
}
