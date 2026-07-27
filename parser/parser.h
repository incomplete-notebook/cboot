/* parser.h - CBoot generated (API declarations only) */
#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declarations for struct types used in API signatures */
typedef struct Project Project;

// 解析.cboot脚本文件
int parse_cboot(char* filepath, struct Project* proj);

// 分派单行脚本命令
int dispatch_script_line(char** tokens, int count);

#endif /* PARSER_H */
