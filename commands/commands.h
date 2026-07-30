/* commands.h - CBoot generated (API declarations only) */
#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ANALYZE_NGRAM_SIZE 8
#define ANALYZE_MAX_MODS 256
#define ANALYZE_MAX_FUNCS 2048
#define ANALYZE_MAX_TOKENS 8192

typedef struct AnalyzeMod;

typedef struct AnalyzeFunc;

typedef struct AnalyzeToken;

int commands_cmd_mod(const char* name);

int commands_cmd_struct(const char* name);

int commands_cmd_type(const char* name);

int commands_cmd_def(const char* name);

int commands_cmd_void(const char* name, const char* return_type);

int commands_cmd_var(const char* name, const char* type);

int commands_cmd_mem(const char* name, const char* type);

int commands_cmd_enum(const char* defs, const char* start_num_str);

int commands_cmd_cmt(const char* text);

int commands_cmd_value(const char* text);

int commands_cmd_call(const char* call_conv);

int commands_cmd_mode(const char* text);

int commands_cmd_cmode(const char* text);

int commands_cmd_cd(const char* path);

int commands_cmd_rm(const char* name, int force);

int commands_cmd_find(const char* type_filter, const char* pattern, int flags);

int commands_cmd_ls(const char* name);

int commands_cmd_mv(const char* src, const char* target);

int commands_cmd_exit();

int commands_cmd_gen();

int commands_cmd_update();

int commands_cmd_analyze();

int commands_cmd_adjust();

int commands_cmd_im(const char* path);

int commands_cmd_in(const char* path);

int commands_cmd_res(const char* file_path);

#endif /* COMMANDS_H */
