/* parser.h - CBoot generated (API declarations only) */
#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parser_try_cboot_ref(const char* token);

int parser_parse_cboot_script(const char* filename);

#endif /* PARSER_H */
