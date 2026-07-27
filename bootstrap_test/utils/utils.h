/* utils.h - CBoot generated (API declarations only) */
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 确保目录存在，不存在则创建
int ensure_dir(char* path);

// 字符串相等比较
int str_eq(char* a, char* b);

// 字符串复制
char* str_dup(char* src);

// 去除字符串首尾的引号
void strip_quotes(char* str);

#endif /* UTILS_H */
