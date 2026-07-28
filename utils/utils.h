/* utils.h - CBoot generated (API declarations only) */
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 分词
char** tokenize(char* line, int* count);

// 释放token
void utils_free_tokens(char** tokens, int count);

// 去空白
char* trim(char* str);

// 创建目录
void utils_ensure_dir(char* path);

// 文件是否存在
int utils_file_exists(char* path);

// 有效标识符
int utils_is_valid_identifier(char* name);

// 去引号
void utils_strip_quotes(char* str);

#endif /* UTILS_H */
