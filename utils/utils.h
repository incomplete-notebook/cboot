/* utils.h - CBoot generated (API declarations only) */
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 分词
char** tokenize(char* line, int* count);

// 释放token
void free_tokens(char** tokens, int count);

// 去空白
char* trim(char* str);

// 创建目录
void ensure_dir(char* path);

// 文件是否存在
int file_exists(char* path);

// 有效标识符
int is_valid_identifier(char* name);

// 去引号
void strip_quotes(char* str);

#endif /* UTILS_H */
