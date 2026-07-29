/* parser.h - CBoot generated (API declarations only) */
#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declarations for struct types used in API signatures */
typedef struct Project Project;

// 输出项目
int parser_parse_cboot(char* filepath, struct Project* proj);

int parser_try_cboot_ref(const char* token);

int parser_parse_cboot_script(const char* filename);

#endif /* PARSER_H */
