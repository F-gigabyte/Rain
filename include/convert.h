#ifndef RAIN_CONVERT_H
#define RAIN_CONVERT_H

#include <common.h>

bool str_to_int(int64_t* res, const char* str, size_t len);
bool str_dec_to_int(int64_t* res, const char* str, size_t len);
bool str_hex_to_int(int64_t* res, const char* str, size_t len);
bool str_oct_to_int(int64_t* res, const char* str, size_t len);
bool str_bin_to_int(int64_t* res, const char* str, size_t len);
bool str_to_float(double* res, const char* str, size_t len);
char* int_to_dec_str(int64_t num);
char* int_to_hex_str(int64_t num);
char* int_to_oct_str(int64_t num);
char* int_to_bin_str(int64_t num);
char* float_to_str(double num);

#endif
