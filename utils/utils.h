/* utils.h - CBoot generated (API declarations only) */
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 目录路径
void utils_ensure_dir(const char* path);

// 字符串B
int utils_str_eq(const char* a, const char* b);

// 源字符串
char* utils_str_dup(const char* str);

// 待处理字符串
void utils_strip_quotes(char* str);

char** utils_tokenize(const char* line, int* count);

void utils_free_tokens(char** tokens, int count);

int utils_str_startswith(const char* str, const char* prefix);

int utils_parse_c_decl(const char* decl, char* type_out, int type_size, char* name_out, int name_size);

char* utils_extract_base_type(const char* type_decl);

char* utils_extract_type_from_decl(const char* decl);

char* utils_extract_name_from_decl(const char* decl);

int utils_is_valid_identifier(const char* name);

int utils_file_exists(const char* path);

char* utils_trim(char* str);

#endif /* UTILS_H */
