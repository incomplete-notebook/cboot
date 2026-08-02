/*
 * test_commands.c - commands/commands.c 单元测试
 *
 * 通过 #include "commands/commands.c" 方式包含被测源文件以测试 static 函数。
 * commands.c 依赖: utils, domain/core, domain, typecheck, cupdate(*),
 *                   parser/mock, generator/mock, docgen/mock, analyze/mock
 * cupdate.c 调用 generator_generate_cboot_only，commands.c 调用
 *   generator_generate_project / docgen_generate_docs /
 *   parser_parse_cboot_script / commands_cmd_analyze_impl —— 均 mock。
 */
#include "test.h"
#include "utils/utils.c"
#include "domain/core/core.c"
#include "domain/domain.c"
#include "typecheck/typecheck.c"
#include "cupdate/cupdate.c"
#include "cupdate/cupdate_lexer.c"
#include "cupdate/cupdate_parser.c"
#include "commands/commands.c"

#include <unistd.h>
#include <fcntl.h>

/* ------------------------------------------------------------------ */
/* 全局状态（cboot.h 中 extern 声明，此处定义）                        */
/* ------------------------------------------------------------------ */
Project *g_proj     = NULL;
RunMode  g_mode     = MODE_INTERACTIVE;
int      g_force    = 0;
int      g_running  = 1;
int      g_skip_gen = 0;
char     g_script_dir[MAX_PATH_LEN] = ".";

/* ------------------------------------------------------------------ */
/* mock: 外部模块函数                                                  */
/* ------------------------------------------------------------------ */
static int g_mock_gen_return    = 0;
static int g_mock_gen_calls     = 0;
static int g_mock_docgen_calls  = 0;
static int g_mock_parser_return = 0;
static int g_mock_parser_calls  = 0;
static int g_mock_analyze_return = 0;

int  generator_generate_project(Project *proj)     { (void)proj; g_mock_gen_calls++; return g_mock_gen_return; }
int  generator_generate_cboot_only(Project *proj)   { (void)proj; return 0; }   /* cupdate.c 调用 */
int  docgen_generate_docs(Project *proj, const char *dir) { (void)proj; (void)dir; g_mock_docgen_calls++; return 0; }
int  parser_parse_cboot_script(const char *filename) { (void)filename; g_mock_parser_calls++; return g_mock_parser_return; }
int  commands_cmd_analyze_impl(void)                { return g_mock_analyze_return; }

/* ------------------------------------------------------------------ */
/* 测试辅助                                                            */
/* ------------------------------------------------------------------ */

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz < 0) sz = 0;
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    buf[rd] = '\0';
    fclose(f);
    return buf;
}

static int contains(const char *hay, const char *needle) {
    if (!hay || !needle) return 0;
    return strstr(hay, needle) != NULL;
}
static int contains_not(const char *hay, const char *needle) {
    if (!hay || !needle) return 1;
    return strstr(hay, needle) == NULL;
}
#define ASSERT_CONTAINS(c, n, m)     ASSERT_TRUE(contains(c, n), m)
#define ASSERT_NOT_CONTAINS(c, n, m) ASSERT_TRUE(contains_not(c, n), m)

/* 每个测试用例独立作用域 */
#define CASE(name)   TEST_BEGIN(name); {
#define ENDCASE      TEST_END(); }

/* 重建一个干净的全局项目 */
static void reset_proj(void) {
    if (g_proj) domain_project_free(g_proj);
    g_proj = domain_project_new("test");
    g_proj->has_generated = 0;
}

/* 创建（或重建）一个干净的临时目录 */
static void make_tmp_dir(const char *path) {
    char cmd[MAX_PATH_LEN * 2];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s' && mkdir -p '%s'", path, path);
    system(cmd);
}

static char g_orig_cwd[MAX_PATH_LEN];
static void enter_tmp(const char *path) {
    make_tmp_dir(path);
    chdir(path);
}
static void leave_tmp(const char *path) {
    chdir(g_orig_cwd);
    char cmd[MAX_PATH_LEN * 2];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", path);
    system(cmd);
}

/* 把字符串喂给 stdin（用 dup2 保存/恢复） */
static int g_saved_stdin = -1;
static void feed_stdin(const char *s) {
    g_saved_stdin = dup(STDIN_FILENO);
    FILE *f = tmpfile();
    ASSERT_TRUE(f != NULL, "tmpfile 应成功");
    if (!f) return;
    fputs(s, f);
    fflush(f);       /* 把缓冲区数据刷到 fd，否则 dup2 后读不到 */
    rewind(f);       /* 重置 fd 偏移到 0 */
    dup2(fileno(f), STDIN_FILENO);
    fclose(f);       /* 关闭 tmpfile fd，STDIN_FILENO 仍有效 */
    /* stdin FILE* 可能缓存了之前的 EOF 标志，必须清除否则 fgets 直接返回 NULL */
    clearerr(stdin);
}
static void restore_stdin(void) {
    if (g_saved_stdin >= 0) {
        dup2(g_saved_stdin, STDIN_FILENO);
        close(g_saved_stdin);
        g_saved_stdin = -1;
        clearerr(stdin);
    }
}

/* 写一个临时文件 */
static int write_temp_file(const char *path, const char *content) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    fputs(content, f);
    fclose(f);
    return 0;
}

/* 在 current 项目根下挂一个模块并 cd 进去 */
static ModuleDomain *setup_mod(const char *name) {
    ModuleDomain *m = domain_module_domain_new(name);
    domain_domain_add_child(g_proj->root, (Domain *)m);
    g_proj->current = (Domain *)m;
    return m;
}

/* ================================================================== */
/* 1. 静态 helper: commands_domain_type_name                            */
/* ================================================================== */
TEST_SUITE(test_helper_domain_type_name) {
    CASE("domain_type_name: 各类型中文名");
    ASSERT_EQ_STR(commands_domain_type_name(DOMAIN_MODULE),   "模块",   "MODULE");
    ASSERT_EQ_STR(commands_domain_type_name(DOMAIN_FUNCTION), "函数",   "FUNCTION");
    ASSERT_EQ_STR(commands_domain_type_name(DOMAIN_STRUCT),   "结构体", "STRUCT");
    ASSERT_EQ_STR(commands_domain_type_name(DOMAIN_TYPE),     "类型",   "TYPE");
    ASSERT_EQ_STR(commands_domain_type_name(DOMAIN_MACRO),    "宏",     "MACRO");
    ASSERT_EQ_STR(commands_domain_type_name(DOMAIN_VARIABLE), "变量",   "VARIABLE");
    ASSERT_EQ_STR(commands_domain_type_name(DOMAIN_MEMBER),   "成员",   "MEMBER");
    ASSERT_EQ_STR(commands_domain_type_name((DomainType)999), "未知",   "default");
    ENDCASE;
}

/* ================================================================== */
/* 2. commands_is_in_domain_type                                       */
/* ================================================================== */
TEST_SUITE(test_helper_is_in_domain_type) {
    CASE("is_in_domain_type: current==NULL 返回 0");
    reset_proj();
    Domain *saved = g_proj->current;
    g_proj->current = NULL;
    ASSERT_EQ_INT(commands_is_in_domain_type(DOMAIN_MODULE), 0, "NULL current");
    g_proj->current = saved;
    ENDCASE;

    CASE("is_in_domain_type: 类型匹配/不匹配");
    reset_proj();
    ASSERT_EQ_INT(commands_is_in_domain_type(DOMAIN_MODULE), 1, "root 是 MODULE");
    ASSERT_EQ_INT(commands_is_in_domain_type(DOMAIN_FUNCTION), 0, "root 非 FUNCTION");
    setup_mod("m");
    ASSERT_EQ_INT(commands_is_in_domain_type(DOMAIN_MODULE), 1, "进入 module 仍 MODULE");
    ENDCASE;
}

/* ================================================================== */
/* 3. commands_is_binary_api_domain / commands_binary_api_mode         */
/* ================================================================== */
TEST_SUITE(test_helper_binary_api) {
    CASE("is_binary_api_domain: FUNCTION/STRUCT/MACRO 为 1，其它为 0");
    ASSERT_EQ_INT(commands_is_binary_api_domain(DOMAIN_FUNCTION), 1, "FUNCTION");
    ASSERT_EQ_INT(commands_is_binary_api_domain(DOMAIN_STRUCT),   1, "STRUCT");
    ASSERT_EQ_INT(commands_is_binary_api_domain(DOMAIN_MACRO),     1, "MACRO");
    ASSERT_EQ_INT(commands_is_binary_api_domain(DOMAIN_MODULE),   0, "MODULE");
    ASSERT_EQ_INT(commands_is_binary_api_domain(DOMAIN_TYPE),     0, "TYPE");
    ASSERT_EQ_INT(commands_is_binary_api_domain(DOMAIN_VARIABLE), 0, "VARIABLE");
    ASSERT_EQ_INT(commands_is_binary_api_domain(DOMAIN_MEMBER),   0, "MEMBER");
    ENDCASE;

    CASE("binary_api_mode: 取各域 mode 字段");
    FunctionDomain *f = domain_function_domain_new("f", "int");
    f->mode = API_MODE_API;
    ASSERT_EQ_INT(commands_binary_api_mode((Domain *)f), API_MODE_API, "FUNCTION api");
    f->mode = API_MODE_NORMAL;
    ASSERT_EQ_INT(commands_binary_api_mode((Domain *)f), API_MODE_NORMAL, "FUNCTION normal");
    domain_domain_delete((Domain *)f);

    StructDomain *s = domain_struct_domain_new("S");
    s->mode = API_MODE_API;
    ASSERT_EQ_INT(commands_binary_api_mode((Domain *)s), API_MODE_API, "STRUCT api");
    domain_domain_delete((Domain *)s);

    MacroDomain *m = domain_macro_domain_new("M");
    m->mode = API_MODE_NORMAL;
    ASSERT_EQ_INT(commands_binary_api_mode((Domain *)m), API_MODE_NORMAL, "MACRO normal");
    domain_domain_delete((Domain *)m);

    /* default 分支返回 API_MODE_NORMAL */
    ModuleDomain *mod = domain_module_domain_new("mod");
    ASSERT_EQ_INT(commands_binary_api_mode((Domain *)mod), API_MODE_NORMAL, "MODULE default normal");
    domain_domain_delete((Domain *)mod);
    ENDCASE;
}

/* ================================================================== */
/* 4. commands_mode_str                                                */
/* ================================================================== */
TEST_SUITE(test_helper_mode_str) {
    CASE("mode_str: NULL 返回空串");
    ASSERT_EQ_STR(commands_mode_str(NULL), "", "NULL");
    ENDCASE;

    CASE("mode_str: 二元 API 域 api/normal");
    FunctionDomain *f = domain_function_domain_new("f", "int");
    f->mode = API_MODE_API;
    ASSERT_EQ_STR(commands_mode_str((Domain *)f), "api", "FUNCTION api");
    f->mode = API_MODE_NORMAL;
    ASSERT_EQ_STR(commands_mode_str((Domain *)f), "normal", "FUNCTION normal");
    domain_domain_delete((Domain *)f);

    StructDomain *s = domain_struct_domain_new("S");
    s->mode = API_MODE_API;
    ASSERT_EQ_STR(commands_mode_str((Domain *)s), "api", "STRUCT api");
    domain_domain_delete((Domain *)s);

    MacroDomain *m = domain_macro_domain_new("M");
    m->mode = API_MODE_API;
    ASSERT_EQ_STR(commands_mode_str((Domain *)m), "api", "MACRO api");
    domain_domain_delete((Domain *)m);
    ENDCASE;

    CASE("mode_str: VARIABLE static/normal");
    VariableDomain *v = domain_variable_domain_new("v", "int");
    v->mode = VAR_MODE_STATIC;
    ASSERT_EQ_STR(commands_mode_str((Domain *)v), "static", "VAR static");
    v->mode = VAR_MODE_NORMAL;
    ASSERT_EQ_STR(commands_mode_str((Domain *)v), "normal", "VAR normal");
    domain_domain_delete((Domain *)v);
    ENDCASE;

    CASE("mode_str: MODULE 四种 mode");
    ModuleDomain *mod = domain_module_domain_new("mod");
    mod->mode = MOD_MODE_SRC;       ASSERT_EQ_STR(commands_mode_str((Domain *)mod), "src",      "SRC");
    mod->mode = MOD_MODE_STATIC;    ASSERT_EQ_STR(commands_mode_str((Domain *)mod), "static",   "STATIC");
    mod->mode = MOD_MODE_DYNAMIC;   ASSERT_EQ_STR(commands_mode_str((Domain *)mod), "dynamic",  "DYNAMIC");
    mod->mode = MOD_MODE_EXTERNAL;  ASSERT_EQ_STR(commands_mode_str((Domain *)mod), "external", "EXTERNAL");
    mod->mode = (ModMode)999;       ASSERT_EQ_STR(commands_mode_str((Domain *)mod), "?",        "非法 mode");
    domain_domain_delete((Domain *)mod);
    ENDCASE;

    CASE("mode_str: TYPE 四种 mode + 非法");
    TypeDomain *t = domain_type_domain_new("T");
    t->mode = TYPE_MODE_RENAME;     ASSERT_EQ_STR(commands_mode_str((Domain *)t), "rename",      "RENAME");
    t->mode = TYPE_MODE_STRUCT;     ASSERT_EQ_STR(commands_mode_str((Domain *)t), "struct",      "STRUCT");
    t->mode = TYPE_MODE_API_RENAME; ASSERT_EQ_STR(commands_mode_str((Domain *)t), "api rename",  "API_RENAME");
    t->mode = TYPE_MODE_API_STRUCT; ASSERT_EQ_STR(commands_mode_str((Domain *)t), "api struct",  "API_STRUCT");
    t->mode = (TypeMode)999;        ASSERT_EQ_STR(commands_mode_str((Domain *)t), "?",            "非法 mode");
    domain_domain_delete((Domain *)t);
    ENDCASE;

    CASE("mode_str: 其它类型返回空串");
    MemberDomain *mb = domain_member_domain_new("mb", "int");
    ASSERT_EQ_STR(commands_mode_str((Domain *)mb), "", "MEMBER 空");
    domain_domain_delete((Domain *)mb);
    ENDCASE;
}

/* ================================================================== */
/* 5. commands_get_current_module                                      */
/* ================================================================== */
TEST_SUITE(test_helper_get_current_module) {
    CASE("get_current_module: 在 root 返回 root");
    reset_proj();
    ModuleDomain *got = commands_get_current_module();
    ASSERT_TRUE(got == (ModuleDomain *)g_proj->root, "root 即当前模块");
    ENDCASE;

    CASE("get_current_module: 在子模块返回该模块");
    reset_proj();
    ModuleDomain *m = setup_mod("m");
    ModuleDomain *got = commands_get_current_module();
    ASSERT_TRUE(got == m, "返回当前子模块");
    ENDCASE;

    CASE("get_current_module: 在函数内返回最近模块");
    reset_proj();
    ModuleDomain *m = setup_mod("m");
    FunctionDomain *f = domain_function_domain_new("f", "int");
    domain_domain_add_child((Domain *)m, (Domain *)f);
    g_proj->current = (Domain *)f;
    ModuleDomain *got = commands_get_current_module();
    ASSERT_TRUE(got == m, "返回祖先模块 m");
    ENDCASE;

    CASE("get_current_module: 无模块祖先返回 NULL");
    reset_proj();
    FunctionDomain *f = domain_function_domain_new("f", "int");  /* 孤立无 parent */
    g_proj->current = (Domain *)f;
    ASSERT_NULL(commands_get_current_module(), "无模块祖先应返回 NULL");
    domain_domain_delete((Domain *)f);
    ENDCASE;
}

/* ================================================================== */
/* 6. commands_check_type                                              */
/* ================================================================== */
TEST_SUITE(test_helper_check_type) {
    CASE("check_type: 内置类型/未知类型均返回 0（仅警告）");
    reset_proj();
    ASSERT_EQ_INT(commands_check_type("int"), 0, "int 返回 0");
    ASSERT_EQ_INT(commands_check_type("UnknownType"), 0, "未知类型也返回 0");
    ASSERT_EQ_INT(commands_check_type("char*"), 0, "char* 返回 0");
    ENDCASE;
}

/* ================================================================== */
/* 7. commands_check_name_dup                                          */
/* ================================================================== */
TEST_SUITE(test_helper_check_name_dup) {
    CASE("check_name_dup: NULL 名称返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_check_name_dup(NULL, g_proj->root), -1, "NULL");
    ENDCASE;

    CASE("check_name_dup: 无效名称返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_check_name_dup("1bad", g_proj->root), -1, "数字开头");
    ASSERT_EQ_INT(commands_check_name_dup("a-b", g_proj->root), -1, "含连字符");
    ASSERT_EQ_INT(commands_check_name_dup("", g_proj->root), -1, "空串");
    ENDCASE;

    CASE("check_name_dup: 重复直接子名返回 -1");
    reset_proj();
    setup_mod("dup");
    g_proj->current = g_proj->root;
    ASSERT_EQ_INT(commands_check_name_dup("dup", g_proj->root), -1, "重复");
    ENDCASE;

    CASE("check_name_dup: 与子模块 API 项冲突返回 -1");
    reset_proj();
    /* root -> modA(api funcX) ; 在 root 下创建同名 funcX 应冲突 */
    ModuleDomain *modA = domain_module_domain_new("modA");
    domain_domain_add_child(g_proj->root, (Domain *)modA);
    FunctionDomain *fx = domain_function_domain_new("funcX", "int");
    fx->mode = API_MODE_API;
    domain_domain_add_child((Domain *)modA, (Domain *)fx);
    ASSERT_EQ_INT(commands_check_name_dup("funcX", g_proj->root), -1, "API 冲突");
    ENDCASE;

    CASE("check_name_dup: 合法名称返回 0");
    reset_proj();
    ASSERT_EQ_INT(commands_check_name_dup("ok_name", g_proj->root), 0, "合法");
    ENDCASE;
}

/* ================================================================== */
/* 8. commands_mode_find / commands_mode_becoming_api                  */
/* ================================================================== */
TEST_SUITE(test_helper_mode_find_becoming) {
    CASE("mode_find: 命中/未命中");
    ModeMap map[] = {{"a", 1}, {"b", 2}, {"c", 3}};
    int out;
    ASSERT_EQ_INT(commands_mode_find(map, 3, "a", &out), 0, "命中 a");
    ASSERT_EQ_INT(out, 1, "值=1");
    ASSERT_EQ_INT(commands_mode_find(map, 3, "c", &out), 0, "命中 c");
    ASSERT_EQ_INT(out, 3, "值=3");
    ASSERT_EQ_INT(commands_mode_find(map, 3, "z", &out), -1, "未命中 z");
    ASSERT_EQ_INT(commands_mode_find(map, 0, "a", &out), -1, "空表");
    ENDCASE;

    CASE("mode_becoming_api: 各类型分支");
    ASSERT_EQ_INT(commands_mode_becoming_api(DOMAIN_FUNCTION, "api"), 1, "FUNCTION api");
    ASSERT_EQ_INT(commands_mode_becoming_api(DOMAIN_STRUCT, "api"), 1, "STRUCT api");
    ASSERT_EQ_INT(commands_mode_becoming_api(DOMAIN_MACRO, "api"), 1, "MACRO api");
    ASSERT_EQ_INT(commands_mode_becoming_api(DOMAIN_FUNCTION, "normal"), 0, "FUNCTION normal 非");
    ASSERT_EQ_INT(commands_mode_becoming_api(DOMAIN_TYPE, "api rename"), 1, "TYPE api rename");
    ASSERT_EQ_INT(commands_mode_becoming_api(DOMAIN_TYPE, "api struct"), 1, "TYPE api struct");
    ASSERT_EQ_INT(commands_mode_becoming_api(DOMAIN_TYPE, "rename"), 0, "TYPE rename 非");
    ASSERT_EQ_INT(commands_mode_becoming_api(DOMAIN_MODULE, "api"), 0, "MODULE 不识别");
    ASSERT_EQ_INT(commands_mode_becoming_api(DOMAIN_VARIABLE, "api"), 0, "VARIABLE 不识别");
    ENDCASE;
}

/* ================================================================== */
/* 9. commands_parse_type_filter                                        */
/* ================================================================== */
TEST_SUITE(test_helper_parse_type_filter) {
    CASE("parse_type_filter: 各类型字符串映射");
    ASSERT_EQ_INT((int)commands_parse_type_filter("mod"),    (int)DOMAIN_MODULE,   "mod");
    ASSERT_EQ_INT((int)commands_parse_type_filter("void"),   (int)DOMAIN_FUNCTION, "void");
    ASSERT_EQ_INT((int)commands_parse_type_filter("struct"), (int)DOMAIN_STRUCT,   "struct");
    ASSERT_EQ_INT((int)commands_parse_type_filter("type"),   (int)DOMAIN_TYPE,    "type");
    ASSERT_EQ_INT((int)commands_parse_type_filter("def"),    (int)DOMAIN_MACRO,   "def");
    ASSERT_EQ_INT((int)commands_parse_type_filter("var"),    (int)DOMAIN_VARIABLE, "var");
    ASSERT_EQ_INT((int)commands_parse_type_filter("mem"),    (int)DOMAIN_MEMBER,  "mem");
    ASSERT_EQ_INT((int)commands_parse_type_filter("all"),    -1,                  "all=-1");
    ASSERT_EQ_INT((int)commands_parse_type_filter("unknown"), -2,                  "未识别=-2");
    ENDCASE;
}

/* ================================================================== */
/* 10. commands_mode_apply                                             */
/* ================================================================== */
TEST_SUITE(test_helper_mode_apply) {
    CASE("mode_apply: MODULE 四种 + 非法");
    reset_proj();
    ModuleDomain *m = setup_mod("m");
    ASSERT_EQ_INT(commands_mode_apply((Domain *)m, "src"), 0,      "src");  ASSERT_EQ_INT(m->mode, MOD_MODE_SRC, "src");
    ASSERT_EQ_INT(commands_mode_apply((Domain *)m, "static"), 0,   "static"); ASSERT_EQ_INT(m->mode, MOD_MODE_STATIC, "static");
    ASSERT_EQ_INT(commands_mode_apply((Domain *)m, "dynamic"), 0,  "dynamic"); ASSERT_EQ_INT(m->mode, MOD_MODE_DYNAMIC, "dynamic");
    ASSERT_EQ_INT(commands_mode_apply((Domain *)m, "external"), 0, "external"); ASSERT_EQ_INT(m->mode, MOD_MODE_EXTERNAL, "external");
    ASSERT_EQ_INT(commands_mode_apply((Domain *)m, "bad"), -1, "非法");
    ENDCASE;

    CASE("mode_apply: FUNCTION api/normal + 非法");
    reset_proj();
    FunctionDomain *f = domain_function_domain_new("f", "int");
    domain_domain_add_child(g_proj->root, (Domain *)f);
    g_proj->current = (Domain *)f;
    ASSERT_EQ_INT(commands_mode_apply((Domain *)f, "api"), 0, "api");    ASSERT_EQ_INT(f->mode, API_MODE_API, "api");
    ASSERT_EQ_INT(commands_mode_apply((Domain *)f, "normal"), 0, "normal"); ASSERT_EQ_INT(f->mode, API_MODE_NORMAL, "normal");
    ASSERT_EQ_INT(commands_mode_apply((Domain *)f, "src"), -1, "FUNCTION 不接受 src");
    ENDCASE;

    CASE("mode_apply: TYPE 四种 + 非法");
    reset_proj();
    TypeDomain *t = domain_type_domain_new("T");
    domain_domain_add_child(g_proj->root, (Domain *)t);
    g_proj->current = (Domain *)t;
    ASSERT_EQ_INT(commands_mode_apply((Domain *)t, "rename"), 0,     "rename"); ASSERT_EQ_INT(t->mode, TYPE_MODE_RENAME, "rename");
    ASSERT_EQ_INT(commands_mode_apply((Domain *)t, "struct"), 0,     "struct"); ASSERT_EQ_INT(t->mode, TYPE_MODE_STRUCT, "struct");
    ASSERT_EQ_INT(commands_mode_apply((Domain *)t, "api rename"), 0, "api rename"); ASSERT_EQ_INT(t->mode, TYPE_MODE_API_RENAME, "api rename");
    ASSERT_EQ_INT(commands_mode_apply((Domain *)t, "api struct"), 0, "api struct"); ASSERT_EQ_INT(t->mode, TYPE_MODE_API_STRUCT, "api struct");
    ASSERT_EQ_INT(commands_mode_apply((Domain *)t, "xxx"), -1, "非法");
    ENDCASE;

    CASE("mode_apply: VARIABLE static/normal + 非法");
    reset_proj();
    VariableDomain *v = domain_variable_domain_new("v", "int");
    domain_domain_add_child(g_proj->root, (Domain *)v);
    g_proj->current = (Domain *)v;
    ASSERT_EQ_INT(commands_mode_apply((Domain *)v, "static"), 0, "static"); ASSERT_EQ_INT(v->mode, VAR_MODE_STATIC, "static");
    ASSERT_EQ_INT(commands_mode_apply((Domain *)v, "normal"), 0, "normal"); ASSERT_EQ_INT(v->mode, VAR_MODE_NORMAL, "normal");
    ASSERT_EQ_INT(commands_mode_apply((Domain *)v, "api"), -1, "VAR 不接受 api");
    ENDCASE;

    CASE("mode_apply: MEMBER 拒绝 + default 拒绝");
    reset_proj();
    MemberDomain *mb = domain_member_domain_new("mb", "int");
    domain_domain_add_child(g_proj->root, (Domain *)mb);
    g_proj->current = (Domain *)mb;
    ASSERT_EQ_INT(commands_mode_apply((Domain *)mb, "static"), -1, "MEMBER 拒绝");
    ENDCASE;
}

/* ================================================================== */
/* 11. commands_mode_check_api_conflict                                 */
/* ================================================================== */
TEST_SUITE(test_helper_mode_check_api_conflict) {
    CASE("check_api_conflict: 无 parent 返回 0");
    FunctionDomain *f = domain_function_domain_new("f", "int");
    ASSERT_EQ_INT(commands_mode_check_api_conflict((Domain *)f), 0, "无 parent");
    domain_domain_delete((Domain *)f);
    ENDCASE;

    CASE("check_api_conflict: 与祖模块 API 项冲突返回 -1");
    reset_proj();
    /* root -> modA -> funcX(待设api); root -> modB -> funcX(已是 api) */
    ModuleDomain *modA = domain_module_domain_new("modA");
    ModuleDomain *modB = domain_module_domain_new("modB");
    domain_domain_add_child(g_proj->root, (Domain *)modA);
    domain_domain_add_child(g_proj->root, (Domain *)modB);
    FunctionDomain *fxA = domain_function_domain_new("funcX", "int");
    FunctionDomain *fxB = domain_function_domain_new("funcX", "int");
    fxB->mode = API_MODE_API;
    domain_domain_add_child((Domain *)modA, (Domain *)fxA);
    domain_domain_add_child((Domain *)modB, (Domain *)fxB);
    ASSERT_EQ_INT(commands_mode_check_api_conflict((Domain *)fxA), -1, "应检测到冲突");
    ENDCASE;

    CASE("check_api_conflict: 无冲突返回 0");
    reset_proj();
    ModuleDomain *modA = domain_module_domain_new("modA");
    ModuleDomain *modB = domain_module_domain_new("modB");
    domain_domain_add_child(g_proj->root, (Domain *)modA);
    domain_domain_add_child(g_proj->root, (Domain *)modB);
    FunctionDomain *fxA = domain_function_domain_new("funcX", "int");
    FunctionDomain *fxB = domain_function_domain_new("funcX", "int");
    fxB->mode = API_MODE_NORMAL;  /* 非 API，不冲突 */
    domain_domain_add_child((Domain *)modA, (Domain *)fxA);
    domain_domain_add_child((Domain *)modB, (Domain *)fxB);
    ASSERT_EQ_INT(commands_mode_check_api_conflict((Domain *)fxA), 0, "无冲突");
    ENDCASE;

    CASE("check_api_conflict: 与祖模块直接子名冲突返回 -1");
    reset_proj();
    /* root 直接子 funcX (非 module); modA 下 funcX 设 api 应冲突 */
    FunctionDomain *rootFx = domain_function_domain_new("funcX", "int");
    domain_domain_add_child(g_proj->root, (Domain *)rootFx);
    ModuleDomain *modA = domain_module_domain_new("modA");
    domain_domain_add_child(g_proj->root, (Domain *)modA);
    FunctionDomain *fxA = domain_function_domain_new("funcX", "int");
    domain_domain_add_child((Domain *)modA, (Domain *)fxA);
    ASSERT_EQ_INT(commands_mode_check_api_conflict((Domain *)fxA), -1, "直接子名冲突");
    ENDCASE;
}

/* ================================================================== */
/* 12. commands_cmd_mod                                                */
/* ================================================================== */
TEST_SUITE(test_cmd_mod) {
    CASE("mod: NULL 名称返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_mod(NULL), -1, "NULL");
    ENDCASE;

    CASE("mod: 无效名称返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_mod("1bad"), -1, "无效名");
    ENDCASE;

    CASE("mod: 重复名称返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_mod("dup"), 0, "首次创建");
    ASSERT_EQ_INT(commands_cmd_mod("dup"), -1, "重复创建");
    ENDCASE;

    CASE("mod: 成功创建并挂到当前");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_mod("m1"), 0, "返回 0");
    Domain *c = domain_domain_find_child(g_proj->root, "m1");
    ASSERT_NOT_NULL(c, "应找到 m1");
    ASSERT_EQ_INT(c->type, DOMAIN_MODULE, "类型 MODULE");
    ENDCASE;
}

/* ================================================================== */
/* 13. commands_cmd_struct / type / def                                */
/* ================================================================== */
TEST_SUITE(test_cmd_struct_type_def) {
    CASE("struct: NULL/非模块作用域/重复/成功");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_struct(NULL), -1, "NULL");
    /* 进入非模块作用域 */
    FunctionDomain *f = domain_function_domain_new("f", "int");
    domain_domain_add_child(g_proj->root, (Domain *)f);
    g_proj->current = (Domain *)f;
    ASSERT_EQ_INT(commands_cmd_struct("S"), -1, "非模块作用域");
    g_proj->current = g_proj->root;
    ASSERT_EQ_INT(commands_cmd_struct("S"), 0, "成功");
    ASSERT_EQ_INT(commands_cmd_struct("S"), -1, "重复");
    ASSERT_NOT_NULL(domain_domain_find_child(g_proj->root, "S"), "找到 S");
    ENDCASE;

    CASE("type: NULL/非模块/重复/成功");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_type(NULL), -1, "NULL");
    FunctionDomain *f = domain_function_domain_new("f", "int");
    domain_domain_add_child(g_proj->root, (Domain *)f);
    g_proj->current = (Domain *)f;
    ASSERT_EQ_INT(commands_cmd_type("T"), -1, "非模块");
    g_proj->current = g_proj->root;
    ASSERT_EQ_INT(commands_cmd_type("T"), 0, "成功");
    ASSERT_EQ_INT(commands_cmd_type("T"), -1, "重复");
    ENDCASE;

    CASE("def: NULL/非模块/重复/成功");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_def(NULL), -1, "NULL");
    FunctionDomain *f = domain_function_domain_new("f", "int");
    domain_domain_add_child(g_proj->root, (Domain *)f);
    g_proj->current = (Domain *)f;
    ASSERT_EQ_INT(commands_cmd_def("D"), -1, "非模块");
    g_proj->current = g_proj->root;
    ASSERT_EQ_INT(commands_cmd_def("D"), 0, "成功");
    ASSERT_EQ_INT(commands_cmd_def("D"), -1, "重复");
    ENDCASE;
}

/* ================================================================== */
/* 14. commands_cmd_void / var                                         */
/* ================================================================== */
TEST_SUITE(test_cmd_void_var) {
    CASE("void: NULL/非模块/重复/成功");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_void(NULL, "int"), -1, "name NULL");
    ASSERT_EQ_INT(commands_cmd_void("f", NULL), -1, "rtype NULL");
    FunctionDomain *exist = domain_function_domain_new("g", "int");
    domain_domain_add_child(g_proj->root, (Domain *)exist);
    g_proj->current = (Domain *)exist;
    ASSERT_EQ_INT(commands_cmd_void("f", "int"), -1, "非模块作用域");
    g_proj->current = g_proj->root;
    ASSERT_EQ_INT(commands_cmd_void("f", "int"), 0, "成功");
    ASSERT_EQ_INT(commands_cmd_void("f", "int"), -1, "重复");
    Domain *c = domain_domain_find_child(g_proj->root, "f");
    ASSERT_EQ_INT(c->type, DOMAIN_FUNCTION, "类型 FUNCTION");
    ASSERT_EQ_STR(((FunctionDomain *)c)->return_type, "int", "返回类型");
    ENDCASE;

    CASE("var: NULL/错误作用域/成功(模块)/成功(函数)");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_var(NULL, "int"), -1, "name NULL");
    ASSERT_EQ_INT(commands_cmd_var("v", NULL), -1, "type NULL");
    /* 在结构体作用域中应拒绝 */
    StructDomain *s = domain_struct_domain_new("S");
    domain_domain_add_child(g_proj->root, (Domain *)s);
    g_proj->current = (Domain *)s;
    ASSERT_EQ_INT(commands_cmd_var("v", "int"), -1, "struct 作用域拒绝");
    /* 模块作用域成功 */
    g_proj->current = g_proj->root;
    ASSERT_EQ_INT(commands_cmd_var("gv", "int"), 0, "模块作用域成功");
    /* 函数作用域成功 */
    FunctionDomain *f = domain_function_domain_new("f", "int");
    domain_domain_add_child(g_proj->root, (Domain *)f);
    g_proj->current = (Domain *)f;
    ASSERT_EQ_INT(commands_cmd_var("lv", "int"), 0, "函数作用域成功");
    Domain *c = domain_domain_find_child((Domain *)f, "lv");
    ASSERT_EQ_INT(c->type, DOMAIN_VARIABLE, "局部变量类型");
    ENDCASE;
}

/* ================================================================== */
/* 15. commands_cmd_mem (commands_add_member)                          */
/* ================================================================== */
TEST_SUITE(test_cmd_mem) {
    CASE("mem: NULL 参数返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_mem(NULL, "int"), -1, "name NULL");
    ASSERT_EQ_INT(commands_cmd_mem("m", NULL), -1, "type NULL");
    ENDCASE;

    CASE("mem: 函数作用域→参数");
    reset_proj();
    FunctionDomain *f = domain_function_domain_new("f", "int");
    domain_domain_add_child(g_proj->root, (Domain *)f);
    g_proj->current = (Domain *)f;
    ASSERT_EQ_INT(commands_cmd_mem("p1", "int"), 0, "添加参数");
    Domain *c = domain_domain_find_child((Domain *)f, "p1");
    ASSERT_EQ_INT(c->type, DOMAIN_MEMBER, "参数类型 MEMBER");
    ENDCASE;

    CASE("mem: 结构体作用域→成员");
    reset_proj();
    StructDomain *s = domain_struct_domain_new("S");
    domain_domain_add_child(g_proj->root, (Domain *)s);
    g_proj->current = (Domain *)s;
    ASSERT_EQ_INT(commands_cmd_mem("x", "int"), 0, "添加成员");
    ASSERT_NOT_NULL(domain_domain_find_child((Domain *)s, "x"), "找到成员");
    ENDCASE;

    CASE("mem: TYPE struct 模式→成员");
    reset_proj();
    TypeDomain *t = domain_type_domain_new("T");
    t->mode = TYPE_MODE_STRUCT;
    domain_domain_add_child(g_proj->root, (Domain *)t);
    g_proj->current = (Domain *)t;
    ASSERT_EQ_INT(commands_cmd_mem("k", "int"), 0, "struct 模式添加成员");
    ENDCASE;

    CASE("mem: TYPE api struct 模式→成员");
    reset_proj();
    TypeDomain *t = domain_type_domain_new("T");
    t->mode = TYPE_MODE_API_STRUCT;
    domain_domain_add_child(g_proj->root, (Domain *)t);
    g_proj->current = (Domain *)t;
    ASSERT_EQ_INT(commands_cmd_mem("k", "int"), 0, "api struct 模式添加成员");
    ENDCASE;

    CASE("mem: TYPE rename 模式拒绝");
    reset_proj();
    TypeDomain *t = domain_type_domain_new("T");
    t->mode = TYPE_MODE_RENAME;
    domain_domain_add_child(g_proj->root, (Domain *)t);
    g_proj->current = (Domain *)t;
    ASSERT_EQ_INT(commands_cmd_mem("k", "int"), -1, "rename 模式拒绝");
    ENDCASE;

    CASE("mem: 模块作用域拒绝");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_mem("k", "int"), -1, "模块作用域拒绝");
    ENDCASE;

    CASE("mem: 重复名称拒绝");
    reset_proj();
    StructDomain *s = domain_struct_domain_new("S");
    domain_domain_add_child(g_proj->root, (Domain *)s);
    g_proj->current = (Domain *)s;
    ASSERT_EQ_INT(commands_cmd_mem("x", "int"), 0, "首次");
    ASSERT_EQ_INT(commands_cmd_mem("x", "int"), -1, "重复");
    ENDCASE;
}

/* ================================================================== */
/* 16. commands_cmd_enum                                               */
/* ================================================================== */
TEST_SUITE(test_cmd_enum) {
    CASE("enum: NULL 参数返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_enum(NULL, "0"), -1, "defs NULL");
    ASSERT_EQ_INT(commands_cmd_enum("A", NULL), -1, "start NULL");
    ENDCASE;

    CASE("enum: 不在模块作用域返回 -1");
    reset_proj();
    /* 孤立 struct（无模块祖先）→ commands_get_current_module 返回 NULL */
    StructDomain *s = domain_struct_domain_new("S");
    Domain *sv = g_proj->current;
    g_proj->current = (Domain *)s;
    ASSERT_EQ_INT(commands_cmd_enum("A,B", "0"), -1, "无模块祖先拒绝");
    g_proj->current = sv;
    domain_domain_delete((Domain *)s);
    ENDCASE;

    CASE("enum: 单个 def 成功");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_enum("RED", "0"), 0, "成功");
    Domain *c = domain_domain_find_child(g_proj->root, "RED");
    ASSERT_NOT_NULL(c, "找到 RED");
    ASSERT_EQ_INT(c->type, DOMAIN_MACRO, "类型 MACRO");
    ASSERT_EQ_STR(((MacroDomain *)c)->value, "0", "值=0");
    ENDCASE;

    CASE("enum: 多个 def 递增编号");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_enum("RED,GREEN,BLUE", "1"), 0, "成功");
    ASSERT_EQ_STR(((MacroDomain *)domain_domain_find_child(g_proj->root, "RED"))->value,   "1", "RED=1");
    ASSERT_EQ_STR(((MacroDomain *)domain_domain_find_child(g_proj->root, "GREEN"))->value, "2", "GREEN=2");
    ASSERT_EQ_STR(((MacroDomain *)domain_domain_find_child(g_proj->root, "BLUE"))->value,  "3", "BLUE=3");
    ENDCASE;

    CASE("enum: 带空格的 token 被修剪");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_enum("  A  ,  B  ", "10"), 0, "修剪后成功");
    ASSERT_NOT_NULL(domain_domain_find_child(g_proj->root, "A"), "找到 A");
    ASSERT_NOT_NULL(domain_domain_find_child(g_proj->root, "B"), "找到 B");
    ASSERT_EQ_STR(((MacroDomain *)domain_domain_find_child(g_proj->root, "A"))->value, "10", "A=10");
    ENDCASE;

    CASE("enum: 无效名称返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_enum("1bad", "0"), -1, "无效名");
    ENDCASE;

    CASE("enum: 重复名称返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_enum("X", "0"), 0, "先建 X");
    ASSERT_EQ_INT(commands_cmd_enum("X,Y", "0"), -1, "重复 X");
    ENDCASE;

    CASE("enum: 在子模块中创建");
    reset_proj();
    setup_mod("m");
    ASSERT_EQ_INT(commands_cmd_enum("A,B", "5"), 0, "子模块中成功");
    ModuleDomain *m = (ModuleDomain *)g_proj->current;
    ASSERT_NOT_NULL(domain_domain_find_child((Domain *)m, "A"), "A 在模块下");
    ENDCASE;
}

/* ================================================================== */
/* 17. commands_cmd_cmt / value                                        */
/* ================================================================== */
TEST_SUITE(test_cmd_cmt_value) {
    CASE("cmt: NULL 返回 -1 / 成功设置注释");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_cmt(NULL), -1, "NULL");
    ASSERT_EQ_INT(commands_cmd_cmt("hello"), 0, "设置");
    ASSERT_EQ_STR(g_proj->current->comment, "hello", "注释已设置");
    ENDCASE;

    CASE("value: NULL 返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_value(NULL), -1, "NULL");
    ENDCASE;

    CASE("value: 宏空值拒绝 / 非空成功");
    reset_proj();
    MacroDomain *m = domain_macro_domain_new("M");
    domain_domain_add_child(g_proj->root, (Domain *)m);
    g_proj->current = (Domain *)m;
    ASSERT_EQ_INT(commands_cmd_value(""), -1, "宏空值拒绝");
    ASSERT_EQ_INT(commands_cmd_value("100"), 0, "宏值成功");
    ASSERT_EQ_STR(m->value, "100", "宏值=100");
    ENDCASE;

    CASE("value: TYPE rename 模式设置值");
    reset_proj();
    TypeDomain *t = domain_type_domain_new("T");
    t->mode = TYPE_MODE_RENAME;
    domain_domain_add_child(g_proj->root, (Domain *)t);
    g_proj->current = (Domain *)t;
    ASSERT_EQ_INT(commands_cmd_value("long"), 0, "rename 值成功");
    ASSERT_EQ_STR(t->value, "long", "底层类型 long");
    ENDCASE;

    CASE("value: VARIABLE 非法值拒绝 / 合法值成功");
    reset_proj();
    VariableDomain *v = domain_variable_domain_new("v", "int");
    domain_domain_add_child(g_proj->root, (Domain *)v);
    g_proj->current = (Domain *)v;
    ASSERT_EQ_INT(commands_cmd_value("notanumber"), -1, "int 类型非法值拒绝");
    ASSERT_EQ_INT(commands_cmd_value("42"), 0, "int 合法值");
    ASSERT_EQ_STR(v->value, "42", "值=42");
    ENDCASE;

    CASE("value: STRUCT/MEMBER 拒绝");
    reset_proj();
    StructDomain *s = domain_struct_domain_new("S");
    domain_domain_add_child(g_proj->root, (Domain *)s);
    g_proj->current = (Domain *)s;
    ASSERT_EQ_INT(commands_cmd_value("x"), -1, "STRUCT 拒绝 value");
    MemberDomain *mb = domain_member_domain_new("mb", "int");
    domain_domain_add_child((Domain *)s, (Domain *)mb);
    g_proj->current = (Domain *)mb;
    ASSERT_EQ_INT(commands_cmd_value("x"), -1, "MEMBER 拒绝 value");
    ENDCASE;

    CASE("value: 模块域 default 分支成功");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_value("somestring"), 0, "模块 default 成功");
    ENDCASE;
}

/* ================================================================== */
/* 18. commands_cmd_call                                               */
/* ================================================================== */
TEST_SUITE(test_cmd_call) {
    CASE("call: NULL/非函数拒绝");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_call(NULL), -1, "NULL");
    StructDomain *s = domain_struct_domain_new("S");
    domain_domain_add_child(g_proj->root, (Domain *)s);
    g_proj->current = (Domain *)s;
    ASSERT_EQ_INT(commands_cmd_call("__stdcall"), -1, "非函数拒绝");
    ENDCASE;

    CASE("call: 合法调用约定");
    reset_proj();
    FunctionDomain *f = domain_function_domain_new("f", "int");
    domain_domain_add_child(g_proj->root, (Domain *)f);
    g_proj->current = (Domain *)f;
    ASSERT_EQ_INT(commands_cmd_call("__cdecl"), 0, "__cdecl");
    ASSERT_EQ_STR(f->call, "__cdecl", "call=__cdecl");
    ASSERT_EQ_INT(commands_cmd_call("__stdcall"), 0, "__stdcall");
    ASSERT_EQ_STR(f->call, "__stdcall", "call=__stdcall");
    ASSERT_EQ_INT(commands_cmd_call("WINAPI"), 0, "WINAPI");
    ASSERT_EQ_STR(f->call, "WINAPI", "call=WINAPI");
    ENDCASE;

    CASE("call: 空串清除");
    reset_proj();
    FunctionDomain *f = domain_function_domain_new("f", "int");
    domain_domain_add_child(g_proj->root, (Domain *)f);
    g_proj->current = (Domain *)f;
    domain_domain_set_call((Domain *)f, "__stdcall");
    ASSERT_EQ_INT(commands_cmd_call(""), 0, "空串清除");
    ASSERT_NULL(f->call, "call 已清空");
    ENDCASE;

    CASE("call: 未知约定仍设置（警告）");
    reset_proj();
    FunctionDomain *f = domain_function_domain_new("f", "int");
    domain_domain_add_child(g_proj->root, (Domain *)f);
    g_proj->current = (Domain *)f;
    ASSERT_EQ_INT(commands_cmd_call("__weirdcc"), 0, "未知约定仍设置");
    ASSERT_EQ_STR(f->call, "__weirdcc", "call=__weirdcc");
    ENDCASE;
}

/* ================================================================== */
/* 19. commands_cmd_mode                                               */
/* ================================================================== */
TEST_SUITE(test_cmd_mode) {
    CASE("mode: NULL 返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_mode(NULL), -1, "NULL");
    ENDCASE;

    CASE("mode: 模块模式成功");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_mode("dynamic"), 0, "dynamic");
    ASSERT_EQ_INT(((ModuleDomain *)g_proj->current)->mode, MOD_MODE_DYNAMIC, "mode=dynamic");
    ENDCASE;

    CASE("mode: 非法模式返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_mode("bogus"), -1, "非法");
    ENDCASE;

    CASE("mode: 设为 api 触发冲突检测");
    reset_proj();
    /* root -> modA -> funcX; root -> modB -> funcX(api) → 设 modA.funcX api 应冲突 */
    ModuleDomain *modA = domain_module_domain_new("modA");
    ModuleDomain *modB = domain_module_domain_new("modB");
    domain_domain_add_child(g_proj->root, (Domain *)modA);
    domain_domain_add_child(g_proj->root, (Domain *)modB);
    FunctionDomain *fxA = domain_function_domain_new("funcX", "int");
    FunctionDomain *fxB = domain_function_domain_new("funcX", "int");
    fxB->mode = API_MODE_API;
    domain_domain_add_child((Domain *)modA, (Domain *)fxA);
    domain_domain_add_child((Domain *)modB, (Domain *)fxB);
    g_proj->current = (Domain *)fxA;
    ASSERT_EQ_INT(commands_cmd_mode("api"), -1, "冲突应拒绝");
    ENDCASE;

    CASE("mode: 设为 api 无冲突成功");
    reset_proj();
    ModuleDomain *m = setup_mod("m");
    FunctionDomain *f = domain_function_domain_new("f", "int");
    domain_domain_add_child((Domain *)m, (Domain *)f);
    g_proj->current = (Domain *)f;
    ASSERT_EQ_INT(commands_cmd_mode("api"), 0, "设 api 成功");
    ASSERT_EQ_INT(f->mode, API_MODE_API, "mode=api");
    ENDCASE;

    CASE("mode: MEMBER 拒绝");
    reset_proj();
    StructDomain *s = domain_struct_domain_new("S");
    domain_domain_add_child(g_proj->root, (Domain *)s);
    MemberDomain *mb = domain_member_domain_new("mb", "int");
    domain_domain_add_child((Domain *)s, (Domain *)mb);
    g_proj->current = (Domain *)mb;
    ASSERT_EQ_INT(commands_cmd_mode("normal"), -1, "MEMBER 拒绝");
    ENDCASE;
}

/* ================================================================== */
/* 20. commands_cmd_cmode                                              */
/* ================================================================== */
TEST_SUITE(test_cmd_cmode) {
    CASE("cmode: NULL/非模块拒绝");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_cmode(NULL), -1, "NULL");
    FunctionDomain *f = domain_function_domain_new("f", "int");
    domain_domain_add_child(g_proj->root, (Domain *)f);
    g_proj->current = (Domain *)f;
    ASSERT_EQ_INT(commands_cmd_cmode("exe"), -1, "非模块拒绝");
    ENDCASE;

    CASE("cmode: 四种模式成功");
    reset_proj();
    ModuleDomain *m = setup_mod("m");
    ASSERT_EQ_INT(commands_cmd_cmode("exe"), 0,    "exe");    ASSERT_EQ_INT(m->compiler, COMPILER_EXE,    "exe");
    ASSERT_EQ_INT(commands_cmd_cmode("sl"), 0,    "sl");     ASSERT_EQ_INT(m->compiler, COMPILER_SL,     "sl");
    ASSERT_EQ_INT(commands_cmd_cmode("dl"), 0,    "dl");     ASSERT_EQ_INT(m->compiler, COMPILER_DL,     "dl");
    ASSERT_EQ_INT(commands_cmd_cmode("normal"), 0, "normal"); ASSERT_EQ_INT(m->compiler, COMPILER_NORMAL, "normal");
    ENDCASE;

    CASE("cmode: 非法值拒绝");
    reset_proj();
    setup_mod("m");
    ASSERT_EQ_INT(commands_cmd_cmode("bogus"), -1, "非法");
    ENDCASE;
}

/* ================================================================== */
/* 21. commands_cmd_cd (commands_cd_walk)                              */
/* ================================================================== */
TEST_SUITE(test_cmd_cd) {
    CASE("cd: NULL/空串显示当前路径");
    reset_proj();
    setup_mod("m");
    ASSERT_EQ_INT(commands_cmd_cd(NULL), 0, "NULL 显示路径");
    ASSERT_EQ_INT(commands_cmd_cd(""), 0, "空串显示路径");
    ENDCASE;

    CASE("cd: .. 回到父节点 / 已在根");
    reset_proj();
    ModuleDomain *m = setup_mod("m");
    ASSERT_EQ_INT(commands_cmd_cd(".."), 0, "回父");
    ASSERT_TRUE(g_proj->current == g_proj->root, "回到 root");
    ASSERT_EQ_INT(commands_cmd_cd(".."), 0, "root 再 .. 不报错");
    ASSERT_TRUE(g_proj->current == g_proj->root, "仍在 root");
    ENDCASE;

    CASE("cd: / 回到根 / 绝对路径");
    reset_proj();
    ModuleDomain *m = setup_mod("m");
    FunctionDomain *f = domain_function_domain_new("f", "int");
    domain_domain_add_child((Domain *)m, (Domain *)f);
    ASSERT_EQ_INT(commands_cmd_cd("/"), 0, "/ 回根");
    ASSERT_TRUE(g_proj->current == g_proj->root, "在 root");
    ASSERT_EQ_INT(commands_cmd_cd("/m"), 0, "/m 绝对导航");
    ASSERT_TRUE(g_proj->current == (Domain *)m, "进入 m");
    ENDCASE;

    CASE("cd: 相对路径多段");
    reset_proj();
    ModuleDomain *m = setup_mod("m");
    FunctionDomain *f = domain_function_domain_new("f", "int");
    domain_domain_add_child((Domain *)m, (Domain *)f);
    g_proj->current = g_proj->root;
    ASSERT_EQ_INT(commands_cmd_cd("m/f"), 0, "m/f 导航");
    ASSERT_TRUE(g_proj->current == (Domain *)f, "进入 f");
    ENDCASE;

    CASE("cd: 未找到返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_cd("nope"), -1, "未找到");
    ASSERT_EQ_INT(commands_cmd_cd("/nope"), -1, "绝对未找到");
    ENDCASE;
}

/* ================================================================== */
/* 22. commands_cmd_rm                                                 */
/* ================================================================== */
TEST_SUITE(test_cmd_rm) {
    CASE("rm: NULL/未找到");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_rm(NULL, 0), -1, "NULL");
    ASSERT_EQ_INT(commands_cmd_rm("nope", 1), -1, "未找到");
    ENDCASE;

    CASE("rm: force 删除成功");
    reset_proj();
    commands_cmd_mod("m1");
    ASSERT_NOT_NULL(domain_domain_find_child(g_proj->root, "m1"), "存在");
    ASSERT_EQ_INT(commands_cmd_rm("m1", 1), 0, "force 删除");
    ASSERT_NULL(domain_domain_find_child(g_proj->root, "m1"), "已删除");
    ENDCASE;

    CASE("rm: 批处理模式无 -f 拒绝");
    reset_proj();
    commands_cmd_mod("m1");
    RunMode sv = g_mode;
    g_mode = MODE_BATCH;
    ASSERT_EQ_INT(commands_cmd_rm("m1", 0), -1, "batch 无 -f 拒绝");
    g_mode = sv;
    ASSERT_NOT_NULL(domain_domain_find_child(g_proj->root, "m1"), "未删除");
    ENDCASE;

    CASE("rm: 交互模式确认 y 删除");
    reset_proj();
    commands_cmd_mod("m1");
    RunMode sv = g_mode;
    g_mode = MODE_INTERACTIVE;
    feed_stdin("y\n");
    ASSERT_EQ_INT(commands_cmd_rm("m1", 0), 0, "确认 y");
    restore_stdin();
    g_mode = sv;
    ASSERT_NULL(domain_domain_find_child(g_proj->root, "m1"), "已删除");
    ENDCASE;

    CASE("rm: 交互模式确认 n 取消");
    reset_proj();
    commands_cmd_mod("m1");
    RunMode sv = g_mode;
    g_mode = MODE_INTERACTIVE;
    feed_stdin("n\n");
    ASSERT_EQ_INT(commands_cmd_rm("m1", 0), 0, "确认 n 取消");
    restore_stdin();
    g_mode = sv;
    ASSERT_NOT_NULL(domain_domain_find_child(g_proj->root, "m1"), "未删除");
    ENDCASE;

    CASE("rm: 交互模式 EOF 取消");
    reset_proj();
    commands_cmd_mod("m1");
    RunMode sv = g_mode;
    g_mode = MODE_INTERACTIVE;
    /* stdin 为 EOF（CI 模式）→ fgets 返回 NULL → 取消并返回 -1 */
    ASSERT_EQ_INT(commands_cmd_rm("m1", 0), -1, "EOF 取消返回 -1");
    g_mode = sv;
    ASSERT_NOT_NULL(domain_domain_find_child(g_proj->root, "m1"), "未删除");
    ENDCASE;
}

/* ================================================================== */
/* 23. commands_cmd_find (commands_find_recursive)                    */
/* ================================================================== */
TEST_SUITE(test_cmd_find) {
    CASE("find: NULL type_filter 返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_find(NULL, NULL, 0), -1, "NULL");
    ENDCASE;

    CASE("find: 当前层匹配");
    reset_proj();
    commands_cmd_mod("m1");
    commands_cmd_mod("m2");
    /* 当前=root，flags=0 仅当前层；mod 类型过滤 */
    ASSERT_EQ_INT(commands_cmd_find("mod", NULL, 0), 0, "应返回 0");
    ENDCASE;

    CASE("find: 未识别类型仍返回 0（无匹配）");
    reset_proj();
    commands_cmd_mod("m1");
    ASSERT_EQ_INT(commands_cmd_find("bogus", NULL, 0), 0, "未识别类型不报错");
    ENDCASE;

    CASE("find: 递归 -a");
    reset_proj();
    ModuleDomain *m = setup_mod("m");
    FunctionDomain *f = domain_function_domain_new("f", "int");
    domain_domain_add_child((Domain *)m, (Domain *)f);
    g_proj->current = g_proj->root;
    /* flags=1 全递归 */
    ASSERT_EQ_INT(commands_cmd_find("void", NULL, 1), 0, "递归 find");
    ENDCASE;

    CASE("find: 带模式匹配/无匹配");
    reset_proj();
    commands_cmd_mod("alpha");
    commands_cmd_mod("beta");
    ASSERT_EQ_INT(commands_cmd_find("mod", "alpha", 0), 0, "匹配 alpha");
    ASSERT_EQ_INT(commands_cmd_find("mod", "zzz", 0), 0, "无匹配");
    ENDCASE;
}

/* ================================================================== */
/* 24. commands_cmd_ls (ls_print_type_info / ls_print_child)          */
/* ================================================================== */
TEST_SUITE(test_cmd_ls) {
    CASE("ls: 默认显示当前 root");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_ls(NULL), 0, "ls root");
    ENDCASE;

    CASE("ls: 指定不存在的子域返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_ls("nope"), -1, "不存在");
    ENDCASE;

    CASE("ls: 指定存在的子域");
    reset_proj();
    commands_cmd_mod("m1");
    ASSERT_EQ_INT(commands_cmd_ls("m1"), 0, "ls m1");
    ENDCASE;

    CASE("ls: 显示各类型域的额外信息");
    reset_proj();
    /* 模块带 value */
    ModuleDomain *m = setup_mod("m");
    domain_domain_set_value((Domain *)m, "genfile.c");
    g_proj->current = (Domain *)m;
    ASSERT_EQ_INT(commands_cmd_ls(NULL), 0, "ls 模块");
    /* 函数带返回类型 */
    FunctionDomain *f = domain_function_domain_new("f", "int");
    domain_domain_set_value((Domain *)f, "biz logic");
    domain_domain_add_child((Domain *)m, (Domain *)f);
    g_proj->current = (Domain *)f;
    ASSERT_EQ_INT(commands_cmd_ls(NULL), 0, "ls 函数");
    /* 类型带 value */
    TypeDomain *t = domain_type_domain_new("T");
    domain_domain_set_value((Domain *)t, "long");
    domain_domain_add_child((Domain *)m, (Domain *)t);
    g_proj->current = (Domain *)t;
    ASSERT_EQ_INT(commands_cmd_ls(NULL), 0, "ls 类型");
    /* 宏带值 */
    MacroDomain *mac = domain_macro_domain_new("M");
    domain_domain_set_value((Domain *)mac, "100");
    domain_domain_add_child((Domain *)m, (Domain *)mac);
    g_proj->current = (Domain *)mac;
    ASSERT_EQ_INT(commands_cmd_ls(NULL), 0, "ls 宏");
    /* 变量带类型和值 */
    VariableDomain *v = domain_variable_domain_new("v", "int");
    domain_domain_set_value((Domain *)v, "5");
    domain_domain_add_child((Domain *)m, (Domain *)v);
    g_proj->current = (Domain *)v;
    ASSERT_EQ_INT(commands_cmd_ls(NULL), 0, "ls 变量");
    /* 成员带类型 */
    MemberDomain *mb = domain_member_domain_new("mb", "int");
    domain_domain_add_child((Domain *)m, (Domain *)mb);
    g_proj->current = (Domain *)mb;
    ASSERT_EQ_INT(commands_cmd_ls(NULL), 0, "ls 成员");
    ENDCASE;

    CASE("ls: 列出子域");
    reset_proj();
    commands_cmd_mod("m1");
    commands_cmd_mod("m2");
    ASSERT_EQ_INT(commands_cmd_ls(NULL), 0, "列出子域");
    ENDCASE;

    CASE("ls: 无子域显示提示");
    reset_proj();
    setup_mod("m");
    ASSERT_EQ_INT(commands_cmd_ls(NULL), 0, "无子域");
    ENDCASE;
}

/* ================================================================== */
/* 25. commands_cmd_mv (mv_resolve_target / mv_check_target)          */
/* ================================================================== */
TEST_SUITE(test_cmd_mv) {
    CASE("mv: NULL 参数返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_mv(NULL, "x"), -1, "src NULL");
    ASSERT_EQ_INT(commands_cmd_mv("x", NULL), -1, "target NULL");
    ENDCASE;

    CASE("mv: 源不存在返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_mv("nope", "x"), -1, "源不存在");
    ENDCASE;

    CASE("mv: 目标已存在返回 -1");
    reset_proj();
    commands_cmd_mod("a");
    commands_cmd_mod("b");
    ASSERT_EQ_INT(commands_cmd_mv("a", "b"), -1, "目标已存在");
    ENDCASE;

    CASE("mv: 重命名成功");
    reset_proj();
    commands_cmd_mod("a");
    ASSERT_EQ_INT(commands_cmd_mv("a", "b"), 0, "重命名");
    ASSERT_NULL(domain_domain_find_child(g_proj->root, "a"), "a 不在");
    ASSERT_NOT_NULL(domain_domain_find_child(g_proj->root, "b"), "b 存在");
    ENDCASE;

    CASE("mv: 移动到子模块下");
    reset_proj();
    commands_cmd_mod("parent");
    commands_cmd_mod("child");
    /* 把 child 移到 parent/child2 */
    ASSERT_EQ_INT(commands_cmd_mv("child", "parent/child2"), 0, "移动到子模块");
    ModuleDomain *parent = (ModuleDomain *)domain_domain_find_child(g_proj->root, "parent");
    ASSERT_NOT_NULL(domain_domain_find_child((Domain *)parent, "child2"), "parent 下有 child2");
    ASSERT_NULL(domain_domain_find_child(g_proj->root, "child"), "root 下无 child");
    ENDCASE;

    CASE("mv: 目标与 API 项冲突返回 -1");
    reset_proj();
    /* root -> modA(api funcX); root -> modB; mv modB funcX 应冲突 */
    ModuleDomain *modA = domain_module_domain_new("modA");
    ModuleDomain *modB = domain_module_domain_new("modB");
    domain_domain_add_child(g_proj->root, (Domain *)modA);
    domain_domain_add_child(g_proj->root, (Domain *)modB);
    FunctionDomain *fx = domain_function_domain_new("funcX", "int");
    fx->mode = API_MODE_API;
    domain_domain_add_child((Domain *)modA, (Domain *)fx);
    ASSERT_EQ_INT(commands_cmd_mv("modB", "funcX"), -1, "API 冲突");
    ENDCASE;

    CASE("mv: mv_resolve_target 根路径");
    reset_proj();
    commands_cmd_mod("a");
    Domain *tp; const char *tn;
    /* target="/b" → parent=root, name="b" */
    ASSERT_EQ_INT(commands_mv_resolve_target("/b", &tp, &tn), 0, "根路径解析");
    ASSERT_TRUE(tp == g_proj->root, "parent=root");
    ASSERT_EQ_STR(tn, "b", "name=b");
    ENDCASE;

    CASE("mv: mv_check_target 可用/冲突");
    reset_proj();
    ModuleDomain *m = setup_mod("m");
    FunctionDomain *f = domain_function_domain_new("f", "int");
    domain_domain_add_child((Domain *)m, (Domain *)f);
    Domain *tp = g_proj->root; const char *tn = "newname";
    ASSERT_EQ_INT(commands_mv_check_target(tp, tn, (Domain *)f), 0, "可用");
    /* 在 root 下创建同名子域以触发冲突 */
    g_proj->current = g_proj->root;
    commands_cmd_mod("dup");
    ASSERT_EQ_INT(commands_mv_check_target(g_proj->root, "dup", (Domain *)f), -1, "已存在");
    ENDCASE;
}

/* ================================================================== */
/* 26. commands_cmd_exit                                              */
/* ================================================================== */
TEST_SUITE(test_cmd_exit) {
    CASE("exit: 未 gen 时警告 + EOF 取消");
    reset_proj();
    g_proj->has_generated = 0;
    g_running = 1;
    int rc = commands_cmd_exit();
    ASSERT_EQ_INT(rc, 0, "返回 0");
    ASSERT_EQ_INT(g_running, 1, "EOF 取消 g_running 不变");
    ENDCASE;

    CASE("exit: 已 gen + 确认 y 退出");
    reset_proj();
    g_proj->has_generated = 1;
    g_running = 1;
    feed_stdin("y\n");
    int rc = commands_cmd_exit();
    restore_stdin();
    ASSERT_EQ_INT(rc, 0, "返回 0");
    ASSERT_EQ_INT(g_running, 0, "y 设置 g_running=0");
    ENDCASE;

    CASE("exit: 已 gen + 确认 n 取消");
    reset_proj();
    g_proj->has_generated = 1;
    g_running = 1;
    feed_stdin("n\n");
    int rc = commands_cmd_exit();
    restore_stdin();
    ASSERT_EQ_INT(rc, 0, "返回 0");
    ASSERT_EQ_INT(g_running, 1, "n 取消 g_running 不变");
    ENDCASE;
}

/* ================================================================== */
/* 27. commands_cmd_gen                                               */
/* ================================================================== */
TEST_SUITE(test_cmd_gen) {
    CASE("gen: mock 成功 → has_generated=1");
    reset_proj();
    g_mock_gen_return = 0;
    g_mock_docgen_calls = 0;
    ASSERT_EQ_INT(commands_cmd_gen(), 0, "返回 0");
    ASSERT_EQ_INT(g_proj->has_generated, 1, "has_generated=1");
    ASSERT_EQ_INT(g_mock_docgen_calls, 1, "docgen 调用一次");
    ENDCASE;

    CASE("gen: mock 失败 → 返回 -1");
    reset_proj();
    g_mock_gen_return = -1;
    ASSERT_EQ_INT(commands_cmd_gen(), -1, "失败返回 -1");
    ASSERT_EQ_INT(g_proj->has_generated, 0, "has_generated 仍 0");
    g_mock_gen_return = 0;
    ENDCASE;
}

/* ================================================================== */
/* 28. commands_cmd_update                                            */
/* ================================================================== */
TEST_SUITE(test_cmd_update) {
    CASE("update: g_proj NULL 返回 -1");
    reset_proj();
    Project *sv = g_proj;
    g_proj = NULL;
    ASSERT_EQ_INT(commands_cmd_update(), -1, "NULL 项目");
    g_proj = sv;
    ENDCASE;

    CASE("update: root NULL 返回 -1");
    reset_proj();
    Domain *r = g_proj->root;
    g_proj->root = NULL;
    ASSERT_EQ_INT(commands_cmd_update(), -1, "root NULL");
    g_proj->root = r;
    ENDCASE;

    CASE("update: 空项目无 .cboot 未生成 → -1");
    reset_proj();
    const char *dir = "/tmp/test_cmd_upd1";
    enter_tmp(dir);
    g_proj->has_generated = 0;
    ASSERT_EQ_INT(commands_cmd_update(), -1, "未生成应拒绝");
    leave_tmp(dir);
    ENDCASE;

    CASE("update: 空项目 + .cboot 解析失败 → -1");
    reset_proj();
    const char *dir = "/tmp/test_cmd_upd2";
    enter_tmp(dir);
    write_temp_file(".cboot", "project x\n");
    g_proj->has_generated = 0;
    g_mock_parser_return = -1;
    ASSERT_EQ_INT(commands_cmd_update(), -1, "解析失败");
    g_mock_parser_return = 0;
    leave_tmp(dir);
    ENDCASE;

    CASE("update: has_generated=1 成功路径");
    reset_proj();
    const char *dir = "/tmp/test_cmd_upd3";
    enter_tmp(dir);
    g_proj->has_generated = 1;
    /* 项目根是 SRC 模块但无 .c 文件 → cupdate_run_module 返回 0 */
    int rc = commands_cmd_update();
    ASSERT_EQ_INT(rc, 0, "成功路径返回 0");
    ASSERT_EQ_INT(g_proj->has_generated, 1, "has_generated 保持");
    leave_tmp(dir);
    ENDCASE;
}

/* ================================================================== */
/* 29. commands_cmd_analyze / adjust                                  */
/* ================================================================== */
TEST_SUITE(test_cmd_analyze_adjust) {
    CASE("analyze: 转发 mock 实现");
    reset_proj();
    g_mock_analyze_return = 0;
    ASSERT_EQ_INT(commands_cmd_analyze(), 0, "analyze 返回 mock 值");
    g_mock_analyze_return = -1;
    ASSERT_EQ_INT(commands_cmd_analyze(), -1, "analyze 返回 -1");
    g_mock_analyze_return = 0;
    ENDCASE;

    CASE("adjust: 无项目返回 -1");
    reset_proj();
    Project *sv = g_proj;
    g_proj = NULL;
    ASSERT_EQ_INT(commands_cmd_adjust(), -1, "无项目");
    g_proj = sv;
    ENDCASE;

    CASE("adjust: root NULL 返回 -1");
    reset_proj();
    Domain *r = g_proj->root;
    g_proj->root = NULL;
    ASSERT_EQ_INT(commands_cmd_adjust(), -1, "root NULL");
    g_proj->root = r;
    ENDCASE;

    CASE("adjust: update 成功返回 0");
    reset_proj();
    const char *dir = "/tmp/test_cmd_adj1";
    enter_tmp(dir);
    g_proj->has_generated = 1;
    ASSERT_EQ_INT(commands_cmd_adjust(), 0, "adjust 始终返回 0");
    leave_tmp(dir);
    ENDCASE;
}

/* ================================================================== */
/* 30. commands_cmd_im (im_internal / im_external / helpers)         */
/* ================================================================== */
TEST_SUITE(test_cmd_im) {
    CASE("im: NULL 返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_im(NULL), -1, "NULL");
    ENDCASE;

    CASE("im: 非模块作用域返回 -1");
    reset_proj();
    FunctionDomain *f = domain_function_domain_new("f", "int");
    domain_domain_add_child(g_proj->root, (Domain *)f);
    g_proj->current = (Domain *)f;
    ASSERT_EQ_INT(commands_cmd_im("x"), -1, "非模块拒绝");
    ENDCASE;

    CASE("im_internal: 自环检测");
    reset_proj();
    ModuleDomain *m = setup_mod("m");
    ASSERT_EQ_INT(commands_cmd_im("m"), -1, "导入自身拒绝");
    ENDCASE;

    CASE("im_internal: 未找到兄弟模块");
    reset_proj();
    setup_mod("m");
    ASSERT_EQ_INT(commands_cmd_im("nope"), -1, "未找到");
    ENDCASE;

    CASE("im_internal: 成功导入兄弟模块 API");
    reset_proj();
    ModuleDomain *modA = domain_module_domain_new("modA");
    ModuleDomain *modB = domain_module_domain_new("modB");
    domain_domain_add_child(g_proj->root, (Domain *)modA);
    domain_domain_add_child(g_proj->root, (Domain *)modB);
    FunctionDomain *fx = domain_function_domain_new("funcX", "int");
    fx->mode = API_MODE_API;
    domain_domain_add_child((Domain *)modB, (Domain *)fx);
    g_proj->current = (Domain *)modA;
    ASSERT_EQ_INT(commands_cmd_im("modB"), 0, "导入成功");
    /* modA 下应有 modB 引用模块 */
    Domain *ref = domain_domain_find_child((Domain *)modA, "modB");
    ASSERT_NOT_NULL(ref, "modA 下有 modB 引用");
    ASSERT_EQ_INT(ref->type, DOMAIN_MODULE, "引用是模块");
    ASSERT_EQ_INT(((ModuleDomain *)ref)->mode, MOD_MODE_EXTERNAL, "EXTERNAL 模式");
    /* 引用模块下应有 funcX 的 API 克隆 */
    Domain *fxClone = domain_domain_find_child(ref, "funcX");
    ASSERT_NOT_NULL(fxClone, "有 funcX 克隆");
    ASSERT_EQ_INT(((FunctionDomain *)fxClone)->mode, API_MODE_API, "克隆为 api");
    /* 依赖记录 */
    ASSERT_EQ_INT(g_proj->dep_count, 1, "依赖记录数=1");
    ENDCASE;

    CASE("im_internal: 已导入跳过");
    reset_proj();
    ModuleDomain *modA = domain_module_domain_new("modA");
    ModuleDomain *modB = domain_module_domain_new("modB");
    domain_domain_add_child(g_proj->root, (Domain *)modA);
    domain_domain_add_child(g_proj->root, (Domain *)modB);
    g_proj->current = (Domain *)modA;
    ASSERT_EQ_INT(commands_cmd_im("modB"), 0, "首次导入");
    ASSERT_EQ_INT(commands_cmd_im("modB"), 0, "重复导入跳过");
    ENDCASE;

    CASE("im_internal: 循环依赖检测");
    reset_proj();
    /* 让 modB 依赖 modA，则 modA 再导入 modB 应循环 */
    ModuleDomain *modA = domain_module_domain_new("modA");
    ModuleDomain *modB = domain_module_domain_new("modB");
    domain_domain_add_child(g_proj->root, (Domain *)modA);
    domain_domain_add_child(g_proj->root, (Domain *)modB);
    /* 模拟 modB 已导入 modA：在 modB 下建 modA 引用 + 依赖 */
    ModuleDomain *refA = domain_module_domain_new("modA");
    refA->mode = MOD_MODE_EXTERNAL;
    domain_domain_add_child((Domain *)modB, (Domain *)refA);
    domain_project_add_dependency(g_proj, "/modB", "/modA", NULL);
    g_proj->current = (Domain *)modA;
    ASSERT_EQ_INT(commands_cmd_im("modB"), -1, "循环依赖拒绝");
    ENDCASE;

    CASE("im_external: 文件不存在返回 -1");
    reset_proj();
    setup_mod("m");
    ASSERT_EQ_INT(commands_cmd_im("noexist.cboot"), -1, "文件不存在");
    ENDCASE;

    CASE("im_external: 解析失败返回 -1");
    reset_proj();
    setup_mod("m");
    const char *dir = "/tmp/test_cmd_im_ext1";
    enter_tmp(dir);
    write_temp_file("ext.cboot", "project ext\n");
    g_mock_parser_return = -1;
    ASSERT_EQ_INT(commands_cmd_im("ext.cboot"), -1, "解析失败");
    g_mock_parser_return = 0;
    leave_tmp(dir);
    ENDCASE;

    CASE("im_external: 成功导入外部文件");
    reset_proj();
    setup_mod("m");
    const char *dir = "/tmp/test_cmd_im_ext2";
    enter_tmp(dir);
    write_temp_file("ext.cboot", "project ext\n");
    g_mock_parser_return = 0;
    ASSERT_EQ_INT(commands_cmd_im("ext.cboot"), 0, "导入成功");
    /* current=m 下应有引用模块（名为临时项目 root 名） */
    ASSERT_EQ_INT(g_proj->current->child_count, 1, "m 下有 1 个引用模块");
    g_mock_parser_return = 0;
    leave_tmp(dir);
    ENDCASE;

    CASE("im_external: 已存在依赖跳过");
    reset_proj();
    setup_mod("m");
    const char *dir = "/tmp/test_cmd_im_ext3";
    enter_tmp(dir);
    write_temp_file("ext.cboot", "project ext\n");
    g_mock_parser_return = 0;
    /* 第一次导入 */
    ASSERT_EQ_INT(commands_cmd_im("ext.cboot"), 0, "首次导入");
    /* 再次导入相同 → 跳过 */
    ASSERT_EQ_INT(commands_cmd_im("ext.cboot"), 0, "重复跳过");
    g_mock_parser_return = 0;
    leave_tmp(dir);
    ENDCASE;
}

/* ================================================================== */
/* 31. im 内部 helper: copy_members / clone_api_child / find_sibling  */
/* ================================================================== */
TEST_SUITE(test_im_helpers) {
    CASE("im_find_sibling: 找到祖先链中的兄弟模块");
    reset_proj();
    ModuleDomain *root_m = setup_mod("top");
    ModuleDomain *a = domain_module_domain_new("a");
    ModuleDomain *b = domain_module_domain_new("b");
    domain_domain_add_child((Domain *)root_m, (Domain *)a);
    domain_domain_add_child((Domain *)root_m, (Domain *)b);
    g_proj->current = (Domain *)a;
    Domain *found = commands_im_find_sibling((Domain *)a, "b");
    ASSERT_TRUE(found == (Domain *)b, "找到兄弟 b");
    ENDCASE;

    CASE("im_find_sibling: 无兄弟返回 NULL");
    reset_proj();
    ModuleDomain *m = setup_mod("m");
    ASSERT_NULL(commands_im_find_sibling((Domain *)m, "nope"), "未找到");
    ENDCASE;

    CASE("copy_members: 仅克隆 MEMBER");
    reset_proj();
    StructDomain *src = domain_struct_domain_new("S");
    StructDomain *dst = domain_struct_domain_new("D");
    MemberDomain *m1 = domain_member_domain_new("x", "int");
    MemberDomain *m2 = domain_member_domain_new("y", "char*");
    domain_domain_set_comment((Domain *)m2, "cmt");
    FunctionDomain *f = domain_function_domain_new("f", "int");  /* 非 MEMBER，应跳过 */
    domain_domain_add_child((Domain *)src, (Domain *)m1);
    domain_domain_add_child((Domain *)src, (Domain *)m2);
    domain_domain_add_child((Domain *)src, (Domain *)f);
    commands_copy_members((Domain *)src, (Domain *)dst);
    ASSERT_EQ_INT(dst->base.child_count, 2, "仅克隆 2 个 MEMBER");
    ASSERT_NOT_NULL(domain_domain_find_child((Domain *)dst, "x"), "有 x");
    ASSERT_NOT_NULL(domain_domain_find_child((Domain *)dst, "y"), "有 y");
    ASSERT_NULL(domain_domain_find_child((Domain *)dst, "f"), "无 f");
    domain_domain_delete((Domain *)src);
    domain_domain_delete((Domain *)dst);
    ENDCASE;

    CASE("clone_api_child: 克隆各类型 API 子域");
    reset_proj();
    /* FUNCTION */
    FunctionDomain *f = domain_function_domain_new("f", "int");
    f->mode = API_MODE_API;
    domain_domain_set_comment((Domain *)f, "fc");
    MemberDomain *p = domain_member_domain_new("a", "int");
    domain_domain_add_child((Domain *)f, (Domain *)p);
    Domain *fc = commands_clone_api_child((Domain *)f);
    ASSERT_NOT_NULL(fc, "FUNCTION 克隆");
    ASSERT_EQ_INT(((FunctionDomain *)fc)->mode, API_MODE_API, "api 模式");
    ASSERT_NOT_NULL(domain_domain_find_child(fc, "a"), "克隆参数");
    domain_domain_delete((Domain *)f); domain_domain_delete(fc);

    /* STRUCT */
    StructDomain *s = domain_struct_domain_new("S");
    s->mode = API_MODE_API;
    Domain *sc = commands_clone_api_child((Domain *)s);
    ASSERT_NOT_NULL(sc, "STRUCT 克隆");
    ASSERT_EQ_INT(((StructDomain *)sc)->mode, API_MODE_API, "api");
    domain_domain_delete((Domain *)s); domain_domain_delete(sc);

    /* TYPE */
    TypeDomain *t = domain_type_domain_new("T");
    t->mode = TYPE_MODE_API_STRUCT;
    domain_domain_set_value((Domain *)t, "v");
    Domain *tc = commands_clone_api_child((Domain *)t);
    ASSERT_NOT_NULL(tc, "TYPE 克隆");
    ASSERT_EQ_INT(((TypeDomain *)tc)->mode, TYPE_MODE_API_STRUCT, "mode 保持");
    ASSERT_EQ_STR(((TypeDomain *)tc)->value, "v", "value 复制");
    domain_domain_delete((Domain *)t); domain_domain_delete(tc);

    /* MACRO */
    MacroDomain *m = domain_macro_domain_new("M");
    m->mode = API_MODE_API;
    domain_domain_set_value((Domain *)m, "10");
    Domain *mc = commands_clone_api_child((Domain *)m);
    ASSERT_NOT_NULL(mc, "MACRO 克隆");
    ASSERT_EQ_INT(((MacroDomain *)mc)->mode, API_MODE_API, "api");
    ASSERT_EQ_STR(((MacroDomain *)mc)->value, "10", "value 复制");
    domain_domain_delete((Domain *)m); domain_domain_delete(mc);
    ENDCASE;

    CASE("clone_api_child: 未知类型返回 NULL");
    VariableDomain *v = domain_variable_domain_new("v", "int");
    ASSERT_NULL(commands_clone_api_child((Domain *)v), "VARIABLE 返回 NULL");
    domain_domain_delete((Domain *)v);
    ENDCASE;

    CASE("copy_api_items: 仅复制 API 子项");
    reset_proj();
    ModuleDomain *src = domain_module_domain_new("src");
    ModuleDomain *dst = domain_module_domain_new("dst");
    FunctionDomain *fa = domain_function_domain_new("fa", "int");
    fa->mode = API_MODE_API;
    FunctionDomain *fp = domain_function_domain_new("fp", "int");
    fp->mode = API_MODE_NORMAL;  /* 非 API 跳过 */
    domain_domain_add_child((Domain *)src, (Domain *)fa);
    domain_domain_add_child((Domain *)src, (Domain *)fp);
    commands_copy_api_items((Domain *)src, (Domain *)dst);
    ASSERT_EQ_INT(dst->base.child_count, 1, "仅复制 1 个 API");
    ASSERT_NOT_NULL(domain_domain_find_child((Domain *)dst, "fa"), "有 fa");
    ASSERT_NULL(domain_domain_find_child((Domain *)dst, "fp"), "无 fp");
    domain_domain_delete((Domain *)src);
    domain_domain_delete((Domain *)dst);
    ENDCASE;

    CASE("copy_api_items_recursive: 递归复制子模块 API");
    reset_proj();
    ModuleDomain *src = domain_module_domain_new("src");
    ModuleDomain *sub = domain_module_domain_new("sub");
    FunctionDomain *fa = domain_function_domain_new("fa", "int");
    fa->mode = API_MODE_API;
    domain_domain_add_child((Domain *)src, (Domain *)sub);
    domain_domain_add_child((Domain *)sub, (Domain *)fa);
    ModuleDomain *dst = domain_module_domain_new("dst");
    commands_copy_api_items_recursive((Domain *)src, (Domain *)dst);
    /* dst 下应有 sub 子模块，且 sub 下有 fa */
    Domain *dstSub = domain_domain_find_child((Domain *)dst, "sub");
    ASSERT_NOT_NULL(dstSub, "有 sub 子模块");
    ASSERT_NOT_NULL(domain_domain_find_child(dstSub, "fa"), "sub 下有 fa");
    domain_domain_delete((Domain *)src);
    domain_domain_delete((Domain *)dst);
    ENDCASE;
}

/* ================================================================== */
/* 32. commands_cmd_in                                                */
/* ================================================================== */
TEST_SUITE(test_cmd_in) {
    CASE("in: NULL 返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_in(NULL), -1, "NULL");
    ENDCASE;

    CASE("in: 文件不存在返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_in("noexist.cboot"), -1, "文件不存在");
    ENDCASE;

    CASE("in: 非模块作用域返回 -1");
    reset_proj();
    FunctionDomain *f = domain_function_domain_new("f", "int");
    domain_domain_add_child(g_proj->root, (Domain *)f);
    g_proj->current = (Domain *)f;
    const char *dir = "/tmp/test_cmd_in1";
    enter_tmp(dir);
    write_temp_file("x.cboot", "project x\n");
    ASSERT_EQ_INT(commands_cmd_in("x.cboot"), -1, "非模块拒绝");
    leave_tmp(dir);
    ENDCASE;

    CASE("in: 解析失败返回 -1");
    reset_proj();
    setup_mod("m");
    const char *dir = "/tmp/test_cmd_in2";
    enter_tmp(dir);
    write_temp_file("x.cboot", "project x\n");
    g_mock_parser_return = -1;
    ASSERT_EQ_INT(commands_cmd_in("x.cboot"), -1, "解析失败");
    g_mock_parser_return = 0;
    leave_tmp(dir);
    ENDCASE;

    CASE("in: 成功导入完整项目");
    reset_proj();
    setup_mod("m");
    const char *dir = "/tmp/test_cmd_in3";
    enter_tmp(dir);
    write_temp_file("ext.cboot", "project ext\n");
    g_mock_parser_return = 0;
    ASSERT_EQ_INT(commands_cmd_in("ext.cboot"), 0, "导入成功");
    /* current=m 下应有导入的项目 root 模块 */
    ASSERT_EQ_INT(g_proj->current->child_count, 1, "m 下有 1 个子模块");
    ASSERT_EQ_INT(g_proj->import_count, 1, "imported_projects 记录 1");
    g_mock_parser_return = 0;
    leave_tmp(dir);
    ENDCASE;

    CASE("in: 同名子域冲突返回 -1");
    reset_proj();
    setup_mod("m");
    /* 先导入一次建好引用名（_in_tmp） */
    const char *dir = "/tmp/test_cmd_in4";
    enter_tmp(dir);
    write_temp_file("ext.cboot", "project ext\n");
    g_mock_parser_return = 0;
    ASSERT_EQ_INT(commands_cmd_in("ext.cboot"), 0, "首次导入");
    /* 再次导入同名应冲突 */
    ASSERT_EQ_INT(commands_cmd_in("ext.cboot"), -1, "同名冲突");
    g_mock_parser_return = 0;
    leave_tmp(dir);
    ENDCASE;
}

/* ================================================================== */
/* 33. commands_cmd_res                                               */
/* ================================================================== */
TEST_SUITE(test_cmd_res) {
    CASE("res: NULL 返回 -1");
    reset_proj();
    ASSERT_EQ_INT(commands_cmd_res(NULL), -1, "NULL");
    ENDCASE;

    CASE("res: 不在模块作用域返回 -1");
    reset_proj();
    StructDomain *s = domain_struct_domain_new("S");
    domain_domain_add_child(g_proj->root, (Domain *)s);
    g_proj->current = (Domain *)s;
    ASSERT_EQ_INT(commands_cmd_res("x.txt"), -1, "非模块拒绝");
    ENDCASE;

    CASE("res: 文件不存在返回 -1");
    reset_proj();
    setup_mod("m");
    ASSERT_EQ_INT(commands_cmd_res("noexist.txt"), -1, "文件不存在");
    ENDCASE;

    CASE("res: 成功复制资源文件");
    reset_proj();
    setup_mod("m");
    const char *dir = "/tmp/test_cmd_res1";
    enter_tmp(dir);
    /* 创建源资源文件 */
    write_temp_file("logo.bin", "RESDATA");
    ASSERT_EQ_INT(commands_cmd_res("logo.bin"), 0, "复制成功");
    ASSERT_TRUE(utils_file_exists("res/logo.bin"), "res 下有副本");
    char *c = read_file("res/logo.bin");
    ASSERT_CONTAINS(c, "RESDATA", "内容一致");
    free(c);
    leave_tmp(dir);
    ENDCASE;

    CASE("res: 带路径的资源取 basename");
    reset_proj();
    setup_mod("m");
    const char *dir = "/tmp/test_cmd_res2";
    enter_tmp(dir);
    make_tmp_dir("/tmp/test_cmd_res_src");
    write_temp_file("/tmp/test_cmd_res_src/icon.png", "PNGDATA");
    ASSERT_EQ_INT(commands_cmd_res("/tmp/test_cmd_res_src/icon.png"), 0, "带路径复制");
    ASSERT_TRUE(utils_file_exists("res/icon.png"), "res 下有 basename 副本");
    system("rm -rf /tmp/test_cmd_res_src");
    leave_tmp(dir);
    ENDCASE;
}

/* ================================================================== */
/* 主函数                                                             */
/* ================================================================== */
int main(void) {
    getcwd(g_orig_cwd, sizeof(g_orig_cwd));

    /* 全局项目初始化 */
    g_proj = domain_project_new("test");
    g_mode = MODE_INTERACTIVE;
    g_running = 1;

    printf("========== commands/commands.c 单元测试 ==========\n");

    RUN_SUITE(test_helper_domain_type_name);
    RUN_SUITE(test_helper_is_in_domain_type);
    RUN_SUITE(test_helper_binary_api);
    RUN_SUITE(test_helper_mode_str);
    RUN_SUITE(test_helper_get_current_module);
    RUN_SUITE(test_helper_check_type);
    RUN_SUITE(test_helper_check_name_dup);
    RUN_SUITE(test_helper_mode_find_becoming);
    RUN_SUITE(test_helper_parse_type_filter);
    RUN_SUITE(test_helper_mode_apply);
    RUN_SUITE(test_helper_mode_check_api_conflict);
    RUN_SUITE(test_cmd_mod);
    RUN_SUITE(test_cmd_struct_type_def);
    RUN_SUITE(test_cmd_void_var);
    RUN_SUITE(test_cmd_mem);
    RUN_SUITE(test_cmd_enum);
    RUN_SUITE(test_cmd_cmt_value);
    RUN_SUITE(test_cmd_call);
    RUN_SUITE(test_cmd_mode);
    RUN_SUITE(test_cmd_cmode);
    RUN_SUITE(test_cmd_cd);
    RUN_SUITE(test_cmd_rm);
    RUN_SUITE(test_cmd_find);
    RUN_SUITE(test_cmd_ls);
    RUN_SUITE(test_cmd_mv);
    RUN_SUITE(test_cmd_exit);
    RUN_SUITE(test_cmd_gen);
    RUN_SUITE(test_cmd_update);
    RUN_SUITE(test_cmd_analyze_adjust);
    RUN_SUITE(test_cmd_im);
    RUN_SUITE(test_im_helpers);
    RUN_SUITE(test_cmd_in);
    RUN_SUITE(test_cmd_res);

    /* 清理 */
    restore_stdin();
    chdir(g_orig_cwd);
    if (g_proj) domain_project_free(g_proj);
    g_proj = NULL;

    test_summary();
    return (g_test_fail > 0) ? 1 : 0;
}
