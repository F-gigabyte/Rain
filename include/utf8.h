#ifndef RAIN_UTF8_H
#define RAIN_UTF8_H

#include <common.h>

// string of utf-8 characters and returns the next unicode character and how many characters were consumed (max 4)
// returns 0 if the character is not utf-8 and doesn't go beyond null byte
// if char_consumed is NULL, it isn't used
// if len is not 0, is the maximum number of chars that can be consumed
wchar_t decode_utf8_char(const char* letter, uint8_t* char_consumed, size_t len);

#endif
