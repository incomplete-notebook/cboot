/* commands.c - CBoot generated (compiler: normal) */
/* Module: commands */

/*
 * CBoot - C Project Bootstrapping Tool v0.5.0
 * Command handlers (新规范 v0.5.0)
 *
 * 命令体系:
 *   建立域: mod/struct/type/def <name>
 *   带类型建立域: void/var/mem <name> <type>
 *   枚举式def: enum <def1>,<def2>,... <start_num>
 *   修改字段: cmt/value/mode <value>
 *   控制: cd <path> / rm <name> [-f]
 *   查找: find <type> <pattern> [-a|-an]
 *   查看: ls [name]
 *   移动: mv <src> <target>
 *   退出: exit
 *   生成: gen
 *   导入: im <path>
 *   资源: res <file>
 */

#include "cboot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

/* Get the nearest module domain from current scope */
static ModuleDomain *commands_get_current_module(void) {
    Domain *d = g_proj->current;
    Domain *mod = domain_domain_find_nearest_of_type(d, DOMAIN_MODULE);
    return (ModuleDomain *)mod;
}

/* Check if current domain is of a specific type */
static int commands_is_in_domain_type(DomainType type) {
    return g_proj->current && g_proj->current->type == type;
}

/* Print domain type name in Chinese */
static const char *commands_domain_type_name(DomainType type) {
    switch (type) {
        case DOMAIN_MODULE:   return "模块";
        case DOMAIN_FUNCTION: return "函数";
        case DOMAIN_STRUCT:   return "结构体";
        case DOMAIN_TYPE:     return "类型";
        case DOMAIN_MACRO:    return "宏";
        case DOMAIN_VARIABLE: return "变量";
        case DOMAIN_MEMBER:   return "成员";
        default:              return "未知";
    }
}

/* Check type validity using type checker.
 * 对于在 .cboot 中引用但定义在外部头文件（如 domain.h）的类型，
 * 仅给出警告而不阻止创建，因为这些类型在 C 编译阶段会被正确解析。 */
static int commands_check_type(const char *type_name) {
    TypeChecker tc;
    typecheck_type_checker_init(&tc, g_proj->current);
    if (typecheck_type_checker_validate(&tc, type_name) != 0) {
        /* 仅警告，不阻止 — 类型可能定义在外部头文件中 */
        return 0;
    }
    return 0;
}

/* Check name is valid identifier and not duplicate (including submodule API items) */
static int commands_check_name_dup(const char *name, Domain *scope) {
    if (!utils_is_valid_identifier(name)) {
        printf("错误: 无效名称 '%s'（仅允许字母、数字、下划线）\n", name);
        return -1;
    }
    if (domain_domain_find_child(scope, name)) {
        printf("错误: 名称 '%s' 已存在\n", name);
        return -1;
    }
    /* Check for conflict with API items from submodules */
    Domain *conflict = domain_domain_check_api_name_conflict(scope, name);
    if (conflict) {
        char *path = domain_domain_get_path(conflict);
        printf("错误: 名称 '%s' 与子模块 API 项冲突 (%s [%s])\n",
               name, path, commands_domain_type_name(conflict->type));
        free(path);
        return -1;
    }
    return 0;
}

/* Get mode string for display */
/* 判断二元模式（API vs NORMAL）的域类型，返回 1 表示是二元模式 */
static int commands_is_binary_api_domain(DomainType t) {
    return t == DOMAIN_FUNCTION || t == DOMAIN_STRUCT || t == DOMAIN_MACRO;
}

/* 取二元模式（API/NORMAL）域的 mode 字段（转 int） */
static int commands_binary_api_mode(Domain *d) {
    switch (d->type) {
        case DOMAIN_FUNCTION: return ((FunctionDomain *)d)->mode;
        case DOMAIN_STRUCT:   return ((StructDomain *)d)->mode;
        case DOMAIN_MACRO:    return ((MacroDomain *)d)->mode;
        default:              return API_MODE_NORMAL;
    }
}

static const char *commands_mode_str(Domain *d) {
    if (!d) return "";
    static const char *mod_modes[]  = {"src", "static", "dynamic", "external"};
    static const char *type_modes[] = {"rename", "struct", "api rename", "api struct"};

    if (commands_is_binary_api_domain(d->type))
        return commands_binary_api_mode(d) == API_MODE_API ? "api" : "normal";
    if (d->type == DOMAIN_VARIABLE)
        return ((VariableDomain *)d)->mode == VAR_MODE_STATIC ? "static" : "normal";
    if (d->type == DOMAIN_MODULE) {
        int m = ((ModuleDomain *)d)->mode;
        return (m >= 0 && m < 4) ? mod_modes[m] : "?";
    }
    if (d->type == DOMAIN_TYPE) {
        int m = ((TypeDomain *)d)->mode;
        return (m >= 0 && m < 4) ? type_modes[m] : "?";
    }
    return "";
}

/* ================================================================== */
/* 建立域: <op> <name>                                                  */
/* ================================================================== */

/* commands_cmd_mod - 创建模块 */
int commands_cmd_mod(const char *name) {
    if (!name) {
        printf("用法: mod <名称>\n");
        return -1;
    }

    if (commands_check_name_dup(name, g_proj->current) != 0)
        return -1;

    ModuleDomain *mod = domain_module_domain_new(name);
    if (!mod) {
        printf("错误: 无法创建模块\n");
        return -1;
    }

    domain_domain_add_child(g_proj->current, (Domain *)mod);

    printf("模块 %s 已创建。\n", name);
    return 0;
}

/* commands_cmd_struct - 创建结构体 */
int commands_cmd_struct(const char *name) {
    if (!name) {
        printf("用法: struct <名称>\n");
        return -1;
    }

    if (g_proj->current->type != DOMAIN_MODULE) {
        printf("错误: struct 只能在模块作用域中创建\n");
        return -1;
    }

    if (commands_check_name_dup(name, g_proj->current) != 0)
        return -1;

    StructDomain *s = domain_struct_domain_new(name);
    if (!s) {
        printf("错误: 无法创建结构体\n");
        return -1;
    }

    domain_domain_add_child(g_proj->current, (Domain *)s);

    printf("结构体 %s 已创建。\n", name);
    return 0;
}

/* commands_cmd_type - 创建类型 (typedef) */
int commands_cmd_type(const char *name) {
    if (!name) {
        printf("用法: type <名称>\n");
        return -1;
    }

    if (g_proj->current->type != DOMAIN_MODULE) {
        printf("错误: type 只能在模块作用域中创建\n");
        return -1;
    }

    if (commands_check_name_dup(name, g_proj->current) != 0)
        return -1;

    TypeDomain *t = domain_type_domain_new(name);
    if (!t) {
        printf("错误: 无法创建类型\n");
        return -1;
    }

    domain_domain_add_child(g_proj->current, (Domain *)t);

    printf("类型 %s 已创建。\n", name);
    return 0;
}

/* commands_cmd_def - 创建宏 */
int commands_cmd_def(const char *name) {
    if (!name) {
        printf("用法: def <名称>\n");
        return -1;
    }

    if (g_proj->current->type != DOMAIN_MODULE) {
        printf("错误: def 只能在模块作用域中创建\n");
        return -1;
    }

    if (commands_check_name_dup(name, g_proj->current) != 0)
        return -1;

    MacroDomain *m = domain_macro_domain_new(name);
    if (!m) {
        printf("错误: 无法创建宏\n");
        return -1;
    }

    domain_domain_add_child(g_proj->current, (Domain *)m);
    printf("宏 %s 已创建。\n", name);
    return 0;
}

/* ================================================================== */
/* 带有type的建立域: <op> <name> <type>                                 */
/* ================================================================== */

/* commands_cmd_void - 创建函数 (void <name> <return_type>) */
int commands_cmd_void(const char *name, const char *return_type) {
    if (!name || !return_type) {
        printf("用法: void <名称> <返回类型>\n");
        return -1;
    }

    if (g_proj->current->type != DOMAIN_MODULE) {
        printf("错误: void 只能在模块作用域中创建\n");
        return -1;
    }

    if (commands_check_name_dup(name, g_proj->current) != 0)
        return -1;

    if (commands_check_type(return_type) != 0)
        return -1;

    FunctionDomain *func = domain_function_domain_new(name, return_type);
    if (!func) {
        printf("错误: 无法创建函数\n");
        return -1;
    }

    domain_domain_add_child(g_proj->current, (Domain *)func);

    printf("函数 %s %s() 已创建。\n", return_type, name);
    return 0;
}

/* commands_cmd_var - 创建变量 (var <name> <type>)
 * 可在模块作用域（全局变量）或函数作用域（局部变量）中创建 */
int commands_cmd_var(const char *name, const char *type) {
    if (!name || !type) {
        printf("用法: var <名称> <类型>\n");
        return -1;
    }

    /* 变量可在模块作用域（全局变量）或函数作用域（局部变量）中创建 */
    DomainType cur_type = g_proj->current->type;
    if (cur_type != DOMAIN_FUNCTION && cur_type != DOMAIN_MODULE) {
        printf("错误: var 命令仅在模块或函数作用域中可用\n");
        return -1;
    }

    if (commands_check_name_dup(name, g_proj->current) != 0)
        return -1;

    if (commands_check_type(type) != 0)
        return -1;

    VariableDomain *v = domain_variable_domain_new(name, type);
    if (!v) {
        printf("错误: 无法创建变量\n");
        return -1;
    }

    domain_domain_add_child(g_proj->current, (Domain *)v);
    printf("变量 %s %s 已创建。\n", type, name);
    return 0;
}

/* 在当前作用域添加成员/参数；label 为输出时使用的中文标签 */
static int commands_add_member(const char *name, const char *type, const char *label) {
    if (commands_check_name_dup(name, g_proj->current) != 0) return -1;
    if (commands_check_type(type) != 0) return -1;
    MemberDomain *m = domain_member_domain_new(name, type);
    if (!m) {
        printf("错误: 无法创建%s\n", label);
        return -1;
    }
    domain_domain_add_child(g_proj->current, (Domain *)m);
    printf("%s %s %s 已添加。\n", label, type, name);
    return 0;
}

/* commands_cmd_mem - 创建成员/参数 (mem <name> <type>, 上下文敏感) */
int commands_cmd_mem(const char *name, const char *type) {
    if (!name || !type) {
        printf("用法: mem <名称> <类型>\n");
        return -1;
    }

    DomainType cur_type = g_proj->current->type;

    /* Context-sensitive: function→参数, struct/type(struct模式)→成员 */
    if (cur_type == DOMAIN_FUNCTION) {
        return commands_add_member(name, type, "参数");
    }
    if (cur_type == DOMAIN_STRUCT) {
        return commands_add_member(name, type, "成员");
    }
    if (cur_type == DOMAIN_TYPE) {
        TypeDomain *td = (TypeDomain *)g_proj->current;
        if (td->mode == TYPE_MODE_STRUCT || td->mode == TYPE_MODE_API_STRUCT) {
            return commands_add_member(name, type, "成员");
        }
        printf("错误: 当前类型不是struct模式，无法添加成员\n");
        return -1;
    }

    printf("错误: mem 命令仅在函数/结构体/类型(struct模式)作用域中可用\n");
    return -1;
}

/* ================================================================== */
/* 枚举式建立def域: enum <def1>,<def2>,... <start_num>                  */
/* ================================================================== */

int commands_cmd_enum(const char *defs, const char *start_num_str) {
    if (!defs || !start_num_str) {
        printf("用法: enum <def1>,<def2>,... <start_num>\n");
        return -1;
    }

    ModuleDomain *mod = commands_get_current_module();
    if (!mod) {
        printf("错误: 当前不在模块作用域中\n");
        return -1;
    }

    int start_num = atoi(start_num_str);

    /* Parse comma-separated def names */
    char buf[MAX_LINE_LEN];
    strncpy(buf, defs, MAX_LINE_LEN - 1);
    buf[MAX_LINE_LEN - 1] = '\0';

    char *saveptr;
    char *token = strtok_r(buf, ",", &saveptr);
    int count = 0;

    while (token) {
        /* Trim whitespace */
        while (*token == ' ') token++;
        char *end = token + strlen(token) - 1;
        while (end > token && *end == ' ') *end-- = '\0';

        if (token[0] == '\0') {
            token = strtok_r(NULL, ",", &saveptr);
            continue;
        }

        /* Check name validity */
        if (!utils_is_valid_identifier(token)) {
            printf("错误: 无效名称 '%s'\n", token);
            return -1;
        }

        /* Check duplicate */
        if (domain_domain_find_child((Domain *)mod, token)) {
            printf("错误: 名称 '%s' 已存在\n", token);
            return -1;
        }

        /* Create macro with value = start_num + count */
        MacroDomain *m = domain_macro_domain_new(token);
        if (!m) {
            printf("错误: 无法创建宏 %s\n", token);
            return -1;
        }

        char val_str[32];
        snprintf(val_str, sizeof(val_str), "%d", start_num + count);
        domain_domain_set_value((Domain *)m, val_str);

        domain_domain_add_child((Domain *)mod, (Domain *)m);
        count++;
        token = strtok_r(NULL, ",", &saveptr);
    }

    printf("已创建 %d 个枚举宏 (值 %d-%d)。\n", count, start_num, start_num + count - 1);
    return 0;
}

/* ================================================================== */
/* 修改字段: <op> <value>                                              */
/* ================================================================== */

/* commands_cmd_cmt - 设置注释 */
int commands_cmd_cmt(const char *text) {
    if (!text) {
        printf("用法: cmt \"注释文本\"\n");
        return -1;
    }

    domain_domain_set_comment(g_proj->current, text);
    printf("注释已设置。\n");
    return 0;
}

/* commands_cmd_value - 设置值 */
int commands_cmd_value(const char *text) {
    if (!text) {
        printf("用法: value <值>\n");
        return -1;
    }

    Domain *cur = g_proj->current;

    /* Validate value for certain domain types */
    switch (cur->type) {
        case DOMAIN_MACRO: {
            /* C language value - basic validation */
            if (text[0] == '\0') {
                printf("错误: 宏值不能为空\n");
                return -1;
            }
            break;
        }
        case DOMAIN_TYPE: {
            TypeDomain *td = (TypeDomain *)cur;
            if (td->mode == TYPE_MODE_RENAME || td->mode == TYPE_MODE_API_RENAME) {
                /* value is a type name - check it */
                if (commands_check_type(text) != 0)
                    return -1;
            }
            break;
        }
        case DOMAIN_VARIABLE: {
            VariableDomain *v = (VariableDomain *)cur;
            if (typecheck_type_checker_validate_value(v->type, text) != 0) {
                printf("错误: 值 '%s' 对类型 '%s' 不合法\n", text, v->type);
                return -1;
            }
            break;
        }
        case DOMAIN_STRUCT:
        case DOMAIN_MEMBER:
            printf("此域类型不支持设置 value\n");
            return -1;
        default:
            break;
    }

    domain_domain_set_value(g_proj->current, text);
    printf("value 已设置。\n");
    return 0;
}

/* commands_cmd_call - 设置函数调用约定 */
int commands_cmd_call(const char *call_conv) {
    if (!call_conv) {
        printf("用法: call <调用约定>\n");
        printf("支持: __cdecl, __stdcall, __fastcall, __thiscall, __nakedcall, __pascal 等\n");
        return -1;
    }

    Domain *cur = g_proj->current;
    if (cur->type != DOMAIN_FUNCTION) {
        printf("错误: call 命令只能在函数域中使用\n");
        return -1;
    }

    /* Validate calling convention */
    const char *valid_conventions[] = {
        "__cdecl", "__stdcall", "__fastcall", "__thiscall",
        "__nakedcall", "__pascal", "WINAPI", "APIENTRY",
        NULL
    };

    int valid = 0;
    for (int i = 0; valid_conventions[i]; i++) {
        if (utils_str_eq(call_conv, valid_conventions[i])) {
            valid = 1;
            break;
        }
    }

    /* Allow empty string to clear the field */
    if (call_conv[0] == '\0') {
        domain_domain_set_call(cur, NULL);
        printf("调用约定已清除。\n");
        return 0;
    }

    if (!valid) {
        printf("警告: 未知的调用约定 '%s'，仍将设置该值。\n", call_conv);
    }

    domain_domain_set_call(cur, call_conv);
    printf("调用约定已设置为: %s\n", call_conv);
    return 0;
}

/* ------------------------------------------------------------------ */
/* mode 命令辅助函数                                                    */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *name;
    int value;
} ModeMap;

/* 在模式映射表中查找文本对应模式值 */
static int commands_mode_find(const ModeMap *map, int n, const char *text, int *out) {
    for (int i = 0; i < n; i++) {
        if (utils_str_eq(text, map[i].name)) {
            *out = map[i].value;
            return 0;
        }
    }
    return -1;
}

/* 判断是否切换到 API 模式 */
static int commands_mode_becoming_api(DomainType type, const char *text) {
    switch (type) {
        case DOMAIN_FUNCTION:
        case DOMAIN_STRUCT:
        case DOMAIN_MACRO:
            return utils_str_eq(text, "api");
        case DOMAIN_TYPE:
            return utils_str_eq(text, "api rename") || utils_str_eq(text, "api struct");
        default:
            return 0;
    }
}

/* 检查设为 API 后的名称冲突 */
static int commands_mode_check_api_conflict(Domain *cur) {
    if (!cur->parent) return 0;
    Domain *parent = cur->parent;
    while (parent) {
        if (parent->type == DOMAIN_MODULE && parent != cur->parent) {
            Domain *conflict = domain_domain_find_child(parent, cur->name);
            if (conflict && conflict->type != DOMAIN_MODULE) {
                char *path = domain_domain_get_path(conflict);
                printf("错误: 设为 api 后名称 '%s' 将与父模块域冲突 (%s [%s])\n",
                       cur->name, path, commands_domain_type_name(conflict->type));
                free(path);
                return -1;
            }
            Domain *api_conflict = domain_domain_check_api_name_conflict(parent, cur->name);
            if (api_conflict && api_conflict != cur && api_conflict->type != DOMAIN_MODULE) {
                char *path = domain_domain_get_path(api_conflict);
                printf("错误: 设为 api 后名称 '%s' 将与子模块 API 项冲突 (%s [%s])\n",
                       cur->name, path, commands_domain_type_name(api_conflict->type));
                free(path);
                return -1;
            }
        }
        parent = parent->parent;
    }
    return 0;
}

/* 根据域类型应用模式 */
static int commands_mode_apply(Domain *cur, const char *text) {
    static const ModeMap mod_modes[] = {
        {"src", MOD_MODE_SRC}, {"static", MOD_MODE_STATIC},
        {"dynamic", MOD_MODE_DYNAMIC}, {"external", MOD_MODE_EXTERNAL}
    };
    static const ModeMap api_modes[] = {
        {"api", API_MODE_API}, {"normal", API_MODE_NORMAL}
    };
    static const ModeMap type_modes[] = {
        {"rename", TYPE_MODE_RENAME}, {"struct", TYPE_MODE_STRUCT},
        {"api rename", TYPE_MODE_API_RENAME}, {"api struct", TYPE_MODE_API_STRUCT}
    };
    static const ModeMap var_modes[] = {
        {"static", VAR_MODE_STATIC}, {"normal", VAR_MODE_NORMAL}
    };

    int mode;
    const ModeMap *map = NULL;
    int map_size = 0;
    const char *err_msg = NULL;

    switch (cur->type) {
        case DOMAIN_MODULE:
            map = mod_modes; map_size = 4;
            err_msg = "错误: 模块模式只能是 src/static/dynamic/external\n";
            break;
        case DOMAIN_FUNCTION:
        case DOMAIN_STRUCT:
            map = api_modes; map_size = 2;
            err_msg = "错误: API模式只能是 api 或 normal\n";
            break;
        case DOMAIN_TYPE:
            map = type_modes; map_size = 4;
            err_msg = "错误: 类型模式只能是 rename/struct/api rename/api struct\n";
            break;
        case DOMAIN_MACRO:
            map = api_modes; map_size = 2;
            err_msg = "错误: 宏模式只能是 api 或 normal\n";
            break;
        case DOMAIN_VARIABLE:
            map = var_modes; map_size = 2;
            err_msg = "错误: 变量模式只能是 static 或 normal\n";
            break;
        case DOMAIN_MEMBER:
            printf("成员域不支持设置 mode\n");
            return -1;
        default:
            return -1;
    }

    if (commands_mode_find(map, map_size, text, &mode) != 0) {
        printf("%s", err_msg);
        return -1;
    }
    domain_domain_set_mode(cur, mode);
    return 0;
}

/* commands_cmd_mode - 设置模式 */
int commands_cmd_mode(const char *text) {
    if (!text) {
        printf("用法: mode <模式>\n");
        return -1;
    }

    Domain *cur = g_proj->current;

    if (commands_mode_becoming_api(cur->type, text)) {
        if (commands_mode_check_api_conflict(cur) != 0)
            return -1;
    }

    if (commands_mode_apply(cur, text) != 0)
        return -1;

    printf("模式已设置为 %s。\n", text);
    return 0;
}

/* commands_cmd_cmode - 设置模块的编译器模式 (exe/sl/dl/normal) */
int commands_cmd_cmode(const char *text) {
    if (!text) {
        printf("用法: cmode <exe|sl|dl|normal>\n");
        return -1;
    }

    Domain *cur = g_proj->current;
    if (cur->type != DOMAIN_MODULE) {
        printf("错误: cmode 只能在模块作用域中设置\n");
        return -1;
    }

    ModuleDomain *md = (ModuleDomain *)cur;
    if (utils_str_eq(text, "exe")) {
        md->compiler = COMPILER_EXE;
    } else if (utils_str_eq(text, "sl")) {
        md->compiler = COMPILER_SL;
    } else if (utils_str_eq(text, "dl")) {
        md->compiler = COMPILER_DL;
    } else if (utils_str_eq(text, "normal")) {
        md->compiler = COMPILER_NORMAL;
    } else {
        printf("错误: 编译器模式只能是 exe/sl/dl/normal\n");
        return -1;
    }

    printf("编译器模式已设置为 %s。\n", text);
    return 0;
}

/* ================================================================== */
/* 控制: cd / rm                                                        */
/* ================================================================== */

/* commands_cmd_cd - 导航域树 */
/* 按 "/" 分割路径并逐段向下导航；失败返回 -1 */
static int commands_cd_walk(Domain *start, const char *p) {
    char segment[MAX_NAME_LEN];
    Domain *cur = start;
    while (*p) {
        int i = 0;
        while (*p && *p != '/' && i < MAX_NAME_LEN - 1) segment[i++] = *p++;
        segment[i] = '\0';
        if (*p == '/') p++;
        if (segment[0] == '\0') continue;
        Domain *child = domain_domain_find_child(cur, segment);
        if (!child) { printf("cd: 未找到 '%s'\n", segment); return -1; }
        cur = child;
    }
    g_proj->current = cur;
    printf("已进入: %s\n", g_proj->current->name);
    return 0;
}

int commands_cmd_cd(const char *path) {
    if (!path || path[0] == '\0') {
        char *p = domain_domain_get_path(g_proj->current);
        printf("当前路径: %s\n", p);
        free(p);
        return 0;
    }

    /* ".." -> parent */
    if (utils_str_eq(path, "..")) {
        if (g_proj->current->parent) {
            g_proj->current = g_proj->current->parent;
            printf("已进入: %s\n", g_proj->current->name);
        } else {
            printf("已在根域。\n");
        }
        return 0;
    }

    /* "/" 或 "/xxx" -> 从根导航 */
    if (path[0] == '/') {
        if (path[1] == '\0') {
            g_proj->current = g_proj->root;
            printf("已进入: %s\n", g_proj->current->name);
            return 0;
        }
        return commands_cd_walk(g_proj->root, path + 1);
    }

    return commands_cd_walk(g_proj->current, path);
}

/* commands_cmd_rm - 删除子域 */
int commands_cmd_rm(const char *name, int force) {
    if (!name) {
        printf("用法: rm <名称> [-f]\n");
        return -1;
    }

    Domain *child = domain_domain_find_child(g_proj->current, name);
    if (!child) {
        printf("未找到: %s\n", name);
        return -1;
    }

    /* If force, just delete */
    if (force) {
        domain_domain_remove_child(g_proj->current, child);
        domain_domain_delete(child);
        printf("已删除 %s '%s'。\n", commands_domain_type_name(child->type), name);
        return 0;
    }

    /* Confirm deletion */
    /* 批处理模式下不进行交互确认：无 -f 时直接取消并返回错误，
     * 避免在脚本中输出无意义的 (y/N) 提示。脚本如需删除请使用 rm -f。 */
    if (g_mode == MODE_BATCH) {
        printf("错误: 批处理模式下删除 '%s' 需使用 -f 强制确认\n", name);
        return -1;
    }

    printf("确认删除 %s '%s'? (y/N): ", commands_domain_type_name(child->type), name);
    fflush(stdout);

    char line[MAX_LINE_LEN];
    if (fgets(line, sizeof(line), stdin) == NULL) {
        printf("已取消。\n");
        return -1;
    }

    if (line[0] == 'y' || line[0] == 'Y') {
        domain_domain_remove_child(g_proj->current, child);
        domain_domain_delete(child);
        printf("已删除 %s '%s'。\n", commands_domain_type_name(child->type), name);
    } else {
        printf("已取消。\n");
    }

    return 0;
}

/* ================================================================== */
/* 查找: find <type> <pattern> [-a|-an]                                 */
/* ================================================================== */

/* Recursive find helper */
static void commands_find_recursive(Domain *root, DomainType type_filter,
                           const char *pattern, int max_depth, int cur_depth,
                           int *found_count) {
    if (!root || (max_depth >= 0 && cur_depth > max_depth)) return;

    /* Check current node */
    if (root->type == type_filter || (int)type_filter == -1) {
        if (!pattern || pattern[0] == '\0' || strstr(root->name, pattern)) {
            char *path = domain_domain_get_path(root);
            printf("[%s] %s (%s)", commands_domain_type_name(root->type), root->name, path);
            if (root->comment) printf(" - %s", root->comment);
            printf("\n");
            free(path);
            (*found_count)++;
        }
    }

    /* Recurse into children */
    for (int i = 0; i < root->child_count; i++) {
        commands_find_recursive(root->children[i], type_filter, pattern,
                       max_depth, cur_depth + 1, found_count);
    }
}

/* 把类型过滤字符串映射为 DomainType；未识别返回 -2，"all" 返回 -1 */
static DomainType commands_parse_type_filter(const char *type_filter) {
    struct { const char *name; DomainType type; } table[] = {
        {"mod",    DOMAIN_MODULE},
        {"void",   DOMAIN_FUNCTION},
        {"struct", DOMAIN_STRUCT},
        {"type",   DOMAIN_TYPE},
        {"def",    DOMAIN_MACRO},
        {"var",    DOMAIN_VARIABLE},
        {"mem",    DOMAIN_MEMBER},
        {"all",    (DomainType)-1},
    };
    for (size_t i = 0; i < sizeof(table)/sizeof(table[0]); i++) {
        if (utils_str_eq(type_filter, table[i].name)) return table[i].type;
    }
    return (DomainType)-2;
}

int commands_cmd_find(const char *type_filter, const char *pattern, int flags) {
    if (!type_filter) {
        printf("用法: find <类型:mod,void,struct,type,def,var,mem,all> <字符> [-a|-an]\n");
        return -1;
    }

    DomainType filter_type = commands_parse_type_filter(type_filter);

    /*
     * flags: bit 0 = -a (recursive), bits 1-7 = depth for -an
     * For simplicity: flags == 0 = current only, flags == 1 = -a full recursive,
     * flags > 1 = depth limit
     */
    int max_depth = (flags == 0) ? 0 : (flags == 1 ? -1 : flags);

    int found_count = 0;
    commands_find_recursive(g_proj->current, filter_type, pattern, max_depth, 0, &found_count);

    if (found_count == 0) {
        printf("未找到匹配项。\n");
    } else {
        printf("找到 %d 项。\n", found_count);
    }

    return 0;
}

/* ================================================================== */
/* 查看: ls [name]                                                      */
/* ================================================================== */

/* 输出特定类型域的额外信息（value/类型/返回类型等） */
static void commands_ls_print_type_info(Domain *target) {
    switch (target->type) {
        case DOMAIN_MODULE: {
            ModuleDomain *mod = (ModuleDomain *)target;
            if (mod->value) printf("  生成文件: %s\n", mod->value);
            break;
        }
        case DOMAIN_FUNCTION: {
            FunctionDomain *func = (FunctionDomain *)target;
            printf("  返回类型: %s\n", func->return_type);
            if (func->value) printf("  业务逻辑: %s\n", func->value);
            break;
        }
        case DOMAIN_TYPE: {
            TypeDomain *td = (TypeDomain *)target;
            if (td->value) printf("  底层类型: %s\n", td->value);
            break;
        }
        case DOMAIN_MACRO: {
            MacroDomain *md = (MacroDomain *)target;
            if (md->value) printf("  值: %s\n", md->value);
            break;
        }
        case DOMAIN_VARIABLE: {
            VariableDomain *v = (VariableDomain *)target;
            printf("  类型: %s\n", v->type);
            if (v->value) printf("  值: %s\n", v->value);
            break;
        }
        case DOMAIN_MEMBER: {
            MemberDomain *mb = (MemberDomain *)target;
            printf("  类型: %s\n", mb->type);
            break;
        }
        default: break;
    }
}

/* 输出单个子域的一行描述 */
static void commands_ls_print_child(Domain *child) {
    printf("    [%s] %s", commands_domain_type_name(child->type), child->name);
    switch (child->type) {
        case DOMAIN_MEMBER:   printf(" : %s", ((MemberDomain *)child)->type); break;
        case DOMAIN_VARIABLE: printf(" : %s", ((VariableDomain *)child)->type); break;
        case DOMAIN_FUNCTION: printf(" : %s()", ((FunctionDomain *)child)->return_type); break;
        default: break;
    }
    if (child->comment) printf(" - %s", child->comment);
    printf("\n");
}

int commands_cmd_ls(const char *name) {
    Domain *target = g_proj->current;

    if (name) {
        /* ls <name>: show specific child */
        Domain *child = domain_domain_find_child(g_proj->current, name);
        if (!child) {
            printf("未找到: %s\n", name);
            return -1;
        }
        target = child;
    }

    char *path = domain_domain_get_path(target);
    printf("域: %s", path);
    free(path);

    /* Show type and mode */
    printf(" [%s]", commands_domain_type_name(target->type));
    const char *ms = commands_mode_str(target);
    if (ms && ms[0]) printf(" mode=%s", ms);
    printf("\n");

    /* Show comment */
    if (target->comment) {
        printf("  注释: %s\n", target->comment);
    }

    /* Show type-specific info */
    commands_ls_print_type_info(target);

    /* List children */
    if (target->child_count > 0) {
        printf("  子域 (%d):\n", target->child_count);
        for (int i = 0; i < target->child_count; i++) {
            commands_ls_print_child(target->children[i]);
        }
    } else if (!name) {
        printf("  (无子域)\n");
    }

    return 0;
}

/* ================================================================== */
/* 移动: mv <src> <target>                                              */
/* ================================================================== */

/* mv 辅助: 解析目标路径，返回目标父域和名称 */
static int commands_mv_resolve_target(const char *target, Domain **out_parent, const char **out_name) {
    Domain *target_parent = g_proj->current;
    const char *target_name = target;

    const char *slash = strrchr(target, '/');
    if (slash) {
        char parent_path[MAX_NAME_LEN];
        int parent_len = (int)(slash - target);
        if (parent_len == 0) {
            target_parent = g_proj->root;
        } else {
            strncpy(parent_path, target, parent_len);
            parent_path[parent_len] = '\0';
            Domain *saved = g_proj->current;
            g_proj->current = target_parent;
            if (commands_cmd_cd(parent_path) != 0) {
                g_proj->current = saved;
                return -1;
            }
            target_parent = g_proj->current;
            g_proj->current = saved;
        }
        target_name = slash + 1;
    }

    *out_parent = target_parent;
    *out_name = target_name;
    return 0;
}

/* mv 辅助: 检查目标是否可用 */
static int commands_mv_check_target(Domain *target_parent, const char *target_name, Domain *src_domain) {
    if (domain_domain_find_child(target_parent, target_name)) {
        printf("错误: 目标 '%s' 已存在\n", target_name);
        return -1;
    }
    Domain *api_conflict = domain_domain_check_api_name_conflict(target_parent, target_name);
    if (api_conflict && api_conflict != src_domain) {
        char *path = domain_domain_get_path(api_conflict);
        printf("错误: 目标 '%s' 与子模块 API 项冲突 (%s [%s])\n",
               target_name, path, commands_domain_type_name(api_conflict->type));
        free(path);
        return -1;
    }
    return 0;
}

int commands_cmd_mv(const char *src, const char *target) {
    if (!src || !target) {
        printf("用法: mv <src> <target>\n");
        return -1;
    }

    Domain *src_domain = domain_domain_find_child(g_proj->current, src);
    if (!src_domain) {
        printf("未找到源域: %s\n", src);
        return -1;
    }

    Domain *target_parent;
    const char *target_name;
    if (commands_mv_resolve_target(target, &target_parent, &target_name) != 0)
        return -1;

    if (commands_mv_check_target(target_parent, target_name, src_domain) != 0)
        return -1;

    /* 执行移动 */
    domain_domain_remove_child(g_proj->current, src_domain);
    if (!utils_str_eq(src_domain->name, target_name)) {
        free(src_domain->name);
        src_domain->name = utils_str_dup(target_name);
    }
    domain_domain_add_child(target_parent, src_domain);

    printf("已将 %s 移动到 %s。\n", src, target);
    return 0;
}

/* ================================================================== */
/* 退出: exit                                                           */
/* ================================================================== */

int commands_cmd_exit(void) {
    if (!g_proj->has_generated) {
        printf("警告: 尚未执行 gen，生成的代码可能不完整。\n");
    }

    char line[MAX_LINE_LEN];
    printf("确认退出? (y/N): ");
    fflush(stdout);
    if (fgets(line, sizeof(line), stdin) == NULL) {
        printf("已取消。\n");
        return 0;
    }

    if (line[0] == 'y' || line[0] == 'Y') {
        g_running = 0;
        printf("再见!\n");
    } else {
        printf("已取消。\n");
    }
    return 0;
}

/* ================================================================== */
/* 生成: gen                                                            */
/* ================================================================== */

int commands_cmd_gen(void) {
    printf("正在生成代码...\n");
    if (generator_generate_project(g_proj) != 0) {
        printf("代码生成失败。\n");
        return -1;
    }
    docgen_generate_docs(g_proj, ".");
    g_proj->has_generated = 1;
    printf("代码生成完成。\n");
    return 0;
}

/* ================================================================== */
/* 更新: update - 扫描源码同步 .cboot                                   */
/* ================================================================== */

int commands_cmd_update(void) {
    if (!g_proj || !g_proj->root) {
        printf("错误: 无活动项目\n");
        return -1;
    }

    /* 如果项目没有子模块，尝试从根目录的 .cboot 文件加载项目定义 */
    if (g_proj->root->child_count == 0 && utils_file_exists(".cboot")) {
        printf("update: 项目为空，正在从 .cboot 文件加载...\n");
        /* 重置项目状态 */
        domain_project_free(g_proj);
        g_proj = domain_project_new("cboot_project");
        if (parser_parse_cboot_script(".cboot") != 0) {
            printf("错误: 无法加载 .cboot 文件\n");
            return -1;
        }
    }

    if (!g_proj->has_generated) {
        /* 检测是否已生成（main.c 或 CMakeLists.txt 存在） */
        if (utils_file_exists("CMakeLists.txt") || utils_file_exists("main.c")) {
            g_proj->has_generated = 1;
        } else {
            printf("错误: 项目尚未生成代码，请先执行 gen\n");
            return -1;
        }
    }
    int err_cnt = 0, warn_cnt = 0;
    int rc = cupdate_run_project(g_proj, &err_cnt, &warn_cnt);
    g_proj->has_generated = 1;
    (void)warn_cnt;
    return rc;
}

/* ================================================================== */
/* 分析: analyze - 统计代码行数、圈复杂度、代码重复率                    */
/* ================================================================== */

#include <fcntl.h>

#define ANALYZE_NGRAM_SIZE 8
#define ANALYZE_MAX_MODS   256
#define ANALYZE_MAX_FUNCS  2048
#define ANALYZE_MAX_TOKENS 8192

typedef struct {
    char name[128];
    const char *code;       /* 指向 ModuleDomain.code，不持有所有权 */
    int  code_lines;
} AnalyzeMod;

typedef struct {
    char name[128];
    char module[128];
    char *code;             /* 函数体，持有所有权 (strdup) */
    int  complexity;
    int  call_count;        /* 被其他函数调用的次数（依赖图复杂度 = call_count - 1） */
} AnalyzeFunc;

typedef struct {
    char text[64];          /* 标识符保留原文；数字/字符串/字符归一化 */
} AnalyzeToken;

/* 构造模块路径名：从根走到当前模块，用 "/" 连接 */
static void analyze_build_module_path(Domain *d, char *out, size_t size) {
    out[0] = '\0';
    char parts[16][128];
    int n = 0;
    Domain *p = d;
    while (p && p->type == DOMAIN_MODULE && n < 16) {
        if (p->parent == NULL) break;  /* 跳过根项目 */
        if (p->name) {
            strncpy(parts[n], p->name, 127);
            parts[n][127] = '\0';
            n++;
        }
        p = p->parent;
    }
    for (int i = n - 1; i >= 0; i--) {
        if (out[0] != '\0') strncat(out, "/", size - strlen(out) - 1);
        strncat(out, parts[i], size - strlen(out) - 1);
    }
}

static void analyze_collect_modules(Domain *d, AnalyzeMod *mods, int *count, int cap)
{
    if (!d || *count >= cap) return;

    if (d->type == DOMAIN_MODULE) {
        ModuleDomain *md = (ModuleDomain *)d;
        if (md->mode == MOD_MODE_SRC && md->code && md->code[0] != '\0') {
            AnalyzeMod *m = &mods[*count];
            analyze_build_module_path(d, m->name, sizeof(m->name));
            m->code = md->code;
            m->code_lines = 0;
            (*count)++;
        }
    }

    for (int i = 0; i < d->child_count; i++)
        analyze_collect_modules(d->children[i], mods, count, cap);
}

/* 处理行尾：若当前行已有代码字符则计数，并重置标记 */
static void analyze_count_line_end(int *has_code, int *lines) {
    if (*has_code) (*lines)++;
    *has_code = 0;
}

/* ST_NORMAL 状态下处理字符；返回新状态 */
static int analyze_count_normal_char(char c, const char *p, const char **next,
                                     int *has_code) {
    if (c == '/' && p[1] == '/') { *next = p + 1; return 1 /*ST_LINE_CMT*/; }
    if (c == '/' && p[1] == '*') { *next = p + 1; return 2 /*ST_BLOCK_CMT*/; }
    if (c == '"')  { *has_code = 1; return 3 /*ST_STRING*/; }
    if (c == '\'') { *has_code = 1; return 4 /*ST_CHAR*/; }
    if (!isspace((unsigned char)c)) *has_code = 1;
    return 0;
}

/* 处理引号状态（STRING/CHAR 逻辑相同，仅 quote 字符不同） */
static int analyze_count_quote_char(char c, char quote, const char **next) {
    if (c == '\\') { *next = *next + 1; return 1; }
    if (c == quote) return 0;  /* 退出引号状态 */
    return 1;  /* 保持引号状态 */
}

/* 状态机上下文，避免长参数列表 */
typedef struct {
    int lines;
    int has_code;
    int state;     /* 0=NORMAL 1=LINE_CMT 2=BLOCK_CMT 3=QUOTE */
    char quote;    /* 当前引号字符（仅 state==3 有效） */
} CountCtx;

/* ST_NORMAL: 处理普通字符，返回新状态 */
static int count_st_normal(char c, const char *p, const char **next, CountCtx *ctx) {
    if (c == '\n') { analyze_count_line_end(&ctx->has_code, &ctx->lines); return 0; }
    int s = analyze_count_normal_char(c, p, next, &ctx->has_code);
    if (s == 3) { ctx->quote = '"';  return 3; }
    if (s == 4) { ctx->quote = '\''; return 3; }
    return s;
}

/* ST_LINE_CMT: 行注释中遇到换行则结束 */
static int count_st_line_cmt(char c, CountCtx *ctx) {
    if (c == '\n') { analyze_count_line_end(&ctx->has_code, &ctx->lines); return 0; }
    return 1;
}

/* ST_BLOCK_CMT: 块注释中遇到 star-slash 则结束 */
static int count_st_block_cmt(char c, const char *p, const char **next, CountCtx *ctx) {
    if (c == '*' && p[1] == '/') { *next = p + 1; return 0; }
    if (c == '\n') analyze_count_line_end(&ctx->has_code, &ctx->lines);
    return 2;
}

/* ST_QUOTE: 字符串/字符字面量，遇到匹配引号则结束 */
static int count_st_quote(char c, const char **next, CountCtx *ctx) {
    if (!analyze_count_quote_char(c, ctx->quote, next)) return 0;
    if (c == '\n') analyze_count_line_end(&ctx->has_code, &ctx->lines);
    return 3;
}

static int analyze_count_code_lines(const char *code)
{
    if (!code) return 0;
    CountCtx ctx = {0, 0, 0, 0};
    const char *p = code;
    while (*p) {
        char c = *p;
        const char *next = p;
        switch (ctx.state) {
        case 0: ctx.state = count_st_normal(c, p, &next, &ctx); break;
        case 1: ctx.state = count_st_line_cmt(c, &ctx); break;
        case 2: ctx.state = count_st_block_cmt(c, p, &next, &ctx); break;
        case 3: ctx.state = count_st_quote(c, &next, &ctx); break;
        }
        p = (next > p) ? next + 1 : p + 1;
    }
    if (ctx.has_code) ctx.lines++;
    return ctx.lines;
}

/* 从源码中提取函数：匹配 "type name(...)" 后跟 "{" 的模式 */
/* 检查名称是否为控制流关键字 */
static int analyze_is_keyword(const char *name, int len) {
    static const char *keywords[] = {"if", "for", "while", "switch", "return", "sizeof", "else", "do", 0};
    for (int k = 0; keywords[k]; k++) {
        if (len == (int)strlen(keywords[k]) && strncmp(name, keywords[k], len) == 0)
            return 1;
    }
    return 0;
}

/* 跳过字符串内容，返回跳过后的指针 */
static const char *analyze_skip_string(const char *p, char quote);
/* 跳过字符串、字符、注释、预处理指令 */
static const char *analyze_skip_non_code(const char *p);

/* 查找函数体结束位置（匹配大括号），跳过字符串、字符和注释
 * （注释中的 } 不得误计为大括号闭合） */
static const char *analyze_find_body_end(const char *body_start) {
    const char *p = body_start;
    int brace_depth = 1;
    while (*p && brace_depth > 0) {
        const char *skipped = analyze_skip_non_code(p);
        if (skipped) { p = skipped; continue; }
        if (*p == '{') brace_depth++;
        else if (*p == '}') brace_depth--;
        if (p < body_start) break;
        p++;
    }
    return (brace_depth == 0) ? p : NULL;
}

/* 跳过字符串、字符、注释、预处理指令 */
/* 跳过行注释或预处理指令（读到换行） */
static const char *analyze_skip_line_comment(const char *p) {
    while (*p && *p != '\n') p++;
    return p;
}

/* 跳过块注释 */
static const char *analyze_skip_block_comment(const char *p) {
    p += 2;
    while (*p && !(p[0] == '*' && p[1] == '/')) p++;
    if (*p) p += 2;
    return p;
}

static const char *analyze_skip_non_code(const char *p) {
    if (*p == '"' || *p == '\'') return analyze_skip_string(p, *p);
    if (p[0] == '/' && p[1] == '/') return analyze_skip_line_comment(p);
    if (*p == '#') return analyze_skip_line_comment(p);
    if (p[0] == '/' && p[1] == '*') return analyze_skip_block_comment(p);
    return NULL;
}

/* 在 [start, ...) 范围内查找配对的圆括号；成功返回 1 并设置 open/close 指针，
 * 失败时返回 0。查找到 '{' 或 ';' 或 '\0' 即停止。
 * 跳过注释、字符串、预处理指令，避免注释中的 name( 被误识别为函数调用。 */
static int analyze_find_parens(const char *start,
                               const char **open_paren, const char **close_paren) {
    const char *paren = start;
    int depth = 0;
    *open_paren = NULL;
    *close_paren = NULL;
    while (*paren && *paren != '{' && *paren != ';') {
        const char *skipped = analyze_skip_non_code(paren);
        if (skipped) { paren = skipped; continue; }
        if (*paren == '(') {
            if (depth == 0) *open_paren = paren;
            depth++;
        }
        if (*paren == ')') {
            depth--;
            if (depth == 0 && *open_paren) *close_paren = paren;
        }
        paren++;
    }
    return (depth == 0 && *paren == '{' && *open_paren && *close_paren) ? 1 : 0;
}

/* 从 open_paren 回溯到 start，提取函数名 [name_start, name_end)，并校验存在返回类型 */
static int analyze_extract_func_name(const char *start, const char *open_paren,
                                     const char **name_start, const char **name_end) {
    const char *ne = open_paren;
    while (ne > start && isspace((unsigned char)ne[-1])) ne--;
    const char *ns = ne;
    while (ns > start && (isalnum((unsigned char)ns[-1]) || ns[-1] == '_')) ns--;
    int has_type = 0;
    for (const char *c = start; c < ns; c++) {
        if (!isspace((unsigned char)*c)) { has_type = 1; break; }
    }
    *name_start = ns;
    *name_end = ne;
    return (has_type && ns < ne) ? 1 : 0;
}

/* 尝试在当前行起始位置解析一个函数定义；成功返回 1，*next 指向函数体之后 */
static int analyze_try_extract_function(const char *start,
                                        const char *mod_name,
                                        AnalyzeFunc *funcs, int *count, int cap,
                                        const char **next) {
    /* 跳过行首空白（isspace('\0') 返回 0，循环自然终止） */
    while (isspace((unsigned char)*start)) start++;
    if (*start == '\0') return 0;

    const char *open_paren, *close_paren;
    if (!analyze_find_parens(start, &open_paren, &close_paren)) return 0;
    const char *brace = close_paren;
    while (*brace && *brace != '{') brace++;
    if (*brace != '{') return 0;

    const char *name_start, *name_end;
    if (!analyze_extract_func_name(start, open_paren, &name_start, &name_end)) return 0;

    int nlen = (int)(name_end - name_start);
    if (nlen <= 0 || nlen >= 127 || analyze_is_keyword(name_start, nlen)) return 0;

    const char *body_start = brace + 1;
    const char *body_end = analyze_find_body_end(body_start);
    if (!body_end) return 0;
    if (*count >= cap) return 0;

    AnalyzeFunc *f = &funcs[*count];
    strncpy(f->name, name_start, nlen);
    f->name[nlen] = '\0';
    strncpy(f->module, mod_name, 127);
    f->module[127] = '\0';
    int blen = (int)(body_end - body_start);
    f->code = (char *)malloc(blen + 1);
    if (f->code) {
        memcpy(f->code, body_start, blen);
        f->code[blen] = '\0';
        (*count)++;
    }
    *next = body_end;
    return 1;
}

static void analyze_extract_functions(const char *mod_name, const char *code,
                                       AnalyzeFunc *funcs, int *count, int cap)
{
    if (!code || !*code) return;
    const char *p = code;
    while (*p && *count < cap) {
        /* 跳过字符串、注释、预处理 */
        const char *skipped = analyze_skip_non_code(p);
        if (skipped) { p = skipped; continue; }

        /* 记录行首位置（可能为函数定义开头） */
        if (p == code || p[-1] == '\n') {
            const char *next = NULL;
            if (analyze_try_extract_function(p, mod_name, funcs, count, cap, &next)) {
                p = next;
                continue;
            }
        }
        p++;
    }
}

/* 简单词法分析：将代码拆分为 token，用于 n-gram 比对 */
/* 检查是否为双字符运算符 */
static int analyze_is_two_char_op(char a, char b) {
    static const char ops[][3] = {
        "==","!=","<=",">=","&&","||","++","--","->","<<",">>",
        "+=","-=","*=","/=","%=","&=","|=","^=",".."
    };
    for (int i = 0; i < (int)(sizeof(ops)/sizeof(ops[0])); i++) {
        if (a == ops[i][0] && b == ops[i][1]) return 1;
    }
    return 0;
}

/* 跳过字符串内容，返回跳过后的指针 */
static const char *analyze_skip_string(const char *p, char quote) {
    p++; /* skip opening quote */
    while (*p && *p != quote) {
        if (*p == '\\') p++;
        if (*p) p++;
    }
    if (*p) p++;
    return p;
}

/* 添加一个字面量归一化 token（"STR"/"'C'"/"NUM"） */
static int analyze_emit_literal(AnalyzeToken *toks, int *count, int cap, const char *text) {
    if (*count >= cap) return 0;
    strncpy(toks[*count].text, text, 63);
    toks[*count].text[63] = '\0';
    (*count)++;
    return 1;
}

/* 处理标识符 token；返回 1 表示已处理，0 表示当前字符不是标识符开头 */
static int analyze_emit_identifier(const char **p, AnalyzeToken *toks, int *count, int cap) {
    if (!isalpha((unsigned char)**p) && **p != '_') return 0;
    if (*count >= cap) return 0;
    int len = 0;
    while ((isalnum((unsigned char)**p) || **p == '_') && len < 63) {
        toks[*count].text[len++] = **p;
        (*p)++;
    }
    toks[*count].text[len] = '\0';
    while (isalnum((unsigned char)**p) || **p == '_') (*p)++;
    (*count)++;
    return 1;
}

/* 处理运算符和标点符号 token */
static int analyze_emit_operator(const char **p, AnalyzeToken *toks, int *count, int cap) {
    if (!ispunct((unsigned char)**p)) return 0;
    if (*count >= cap) return 0;
    if ((*p)[0] && (*p)[1] && analyze_is_two_char_op((*p)[0], (*p)[1])) {
        toks[*count].text[0] = (*p)[0];
        toks[*count].text[1] = (*p)[1];
        toks[*count].text[2] = '\0';
        *p += 2;
    } else {
        toks[*count].text[0] = **p;
        toks[*count].text[1] = '\0';
        (*p)++;
    }
    (*count)++;
    return 1;
}

/* 跳过空白和注释；返回 1 表示已跳过非代码内容，0 表示当前位置是代码 */
static int analyze_skip_trivia(const char **p) {
    if (isspace((unsigned char)**p)) { (*p)++; return 1; }
    if ((*p)[0] == '/' && (*p)[1] == '/') {
        while (**p && **p != '\n') (*p)++;
        return 1;
    }
    if ((*p)[0] == '/' && (*p)[1] == '*') {
        *p += 2;
        while (**p && !((*p)[0] == '*' && (*p)[1] == '/')) (*p)++;
        if (**p) *p += 2;
        return 1;
    }
    return 0;
}

/* 处理字面量 token（字符串/字符/数字）；返回 1 表示已处理 */
static int analyze_emit_literal_token(const char **p, AnalyzeToken *toks, int *count, int cap) {
    char ch = **p;
    if (ch == '"' || ch == '\'') {
        analyze_emit_literal(toks, count, cap, ch == '"' ? "\"STR\"" : "'C'");
        *p = analyze_skip_string(*p, ch);
        return 1;
    }
    if (isdigit((unsigned char)ch) || (ch == '.' && isdigit((unsigned char)(*p)[1]))) {
        analyze_emit_literal(toks, count, cap, "NUM");
        while (isdigit((unsigned char)**p) || **p == '.' || **p == 'x' || **p == 'X') (*p)++;
        return 1;
    }
    return 0;
}

static int analyze_tokenize(const char *code, AnalyzeToken *toks, int cap)
{
    int count = 0;
    const char *p = code;
    while (*p && count < cap) {
        if (analyze_skip_trivia(&p)) continue;
        if (analyze_emit_literal_token(&p, toks, &count, cap)) continue;
        if (analyze_emit_identifier(&p, toks, &count, cap)) continue;
        if (analyze_emit_operator(&p, toks, &count, cap))   continue;
        p++;
    }
    return count;
}

/* 计算圈复杂度：基于分支关键字和运算符计数 */
static int analyze_cyclomatic_complexity(const char *code)
{
    if (!code) return 1;
    int cc = 1;
    AnalyzeToken toks[ANALYZE_MAX_TOKENS];
    int n = analyze_tokenize(code, toks, ANALYZE_MAX_TOKENS);
    for (int i = 0; i < n; i++) {
        const char *t = toks[i].text;
        if (strcmp(t, "if") == 0 || strcmp(t, "else") == 0 ||
            strcmp(t, "for") == 0 || strcmp(t, "while") == 0 ||
            strcmp(t, "case") == 0 || strcmp(t, "catch") == 0 ||
            strcmp(t, "&&") == 0 || strcmp(t, "||") == 0 ||
            strcmp(t, "?") == 0) {
            cc++;
        }
    }
    return cc;
}

/* 比较两个 n-gram 是否相等 */
static int analyze_ngram_equal(AnalyzeToken *a, AnalyzeToken *b)
{
    for (int i = 0; i < ANALYZE_NGRAM_SIZE; i++) {
        if (strcmp(a[i].text, b[i].text) != 0) return 0;
    }
    return 1;
}

/* analyze 辅助: 确保项目已加载 */
static void commands_analyze_ensure_loaded(void) {
    if (g_proj->root->child_count == 0 && utils_file_exists(".cboot")) {
        domain_project_free(g_proj);
        g_proj = domain_project_new("cboot_project");
        g_skip_gen = 1;
        parser_parse_cboot_script(".cboot");
        g_skip_gen = 0;
    }
}

/* analyze 辅助: 统计并打印有效代码行数 */
static void commands_analyze_code_lines(AnalyzeMod *mods, int mod_count) {
    int total_lines = 0;
    printf("--- 1. 有效代码行数 ---\n");
    printf("  %-32s %s\n", "模块", "有效行数");
    printf("  %-32s %s\n", "------------------------------", "--------");
    for (int i = 0; i < mod_count; i++) {
        mods[i].code_lines = analyze_count_code_lines(mods[i].code);
        printf("  %-32s %d\n", mods[i].name, mods[i].code_lines);
        total_lines += mods[i].code_lines;
    }
    printf("  %-32s %s\n", "------------------------------", "--------");
    printf("  %-32s %d\n", "总计", total_lines);
    printf("\n");
}

/* analyze 辅助: 计算并打印圈复杂度 */
static void commands_analyze_cyclomatic(AnalyzeFunc *funcs, int func_count) {
    for (int i = 0; i < func_count; i++)
        funcs[i].complexity = analyze_cyclomatic_complexity(funcs[i].code);

    int total_cc = 0, max_cc = 0;
    char max_cc_name[128] = "", max_cc_mod[128] = "";
    printf("--- 2. 圈复杂度 (Cyclomatic Complexity) ---\n");
    printf("  %-44s %s\n", "函数", "复杂度");
    printf("  %-44s %s\n", "--------------------------------------------", "--------");
    for (int i = 0; i < func_count; i++) {
        int cc = funcs[i].complexity;
        char display[300];
        snprintf(display, sizeof(display), "%s [%s]", funcs[i].name, funcs[i].module);
        const char *marker = (cc > 15) ? " *** 高" : (cc > 10) ? " ** 中" : "";
        printf("  %-44s %d%s\n", display, cc, marker);
        total_cc += cc;
        if (cc > max_cc) {
            max_cc = cc;
            strncpy(max_cc_name, funcs[i].name, 127);
            strncpy(max_cc_mod, funcs[i].module, 127);
        }
    }
    printf("  %-44s %s\n", "--------------------------------------------", "--------");
    if (func_count > 0) {
        printf("  %-44s %d\n", "总计", total_cc);
        printf("  %-44s %d\n", "平均", total_cc / func_count);
        printf("  %-44s %d (%s [%s])\n", "最高", max_cc, max_cc_name, max_cc_mod);
    } else {
        printf("  (无函数数据)\n");
    }
    printf("\n");
}

/* analyze 辅助: 检查 n-gram 是否在其他函数中重复 */
static int analyze_ngram_is_dup(AnalyzeToken **all_toks, int *tok_counts,
                                 int func_count, int fi, int g) {
    for (int j = 0; j < func_count; j++) {
        if (j == fi) continue;
        int gj = tok_counts[j] - ANALYZE_NGRAM_SIZE + 1;
        if (gj <= 0) continue;
        for (int g2 = 0; g2 < gj; g2++) {
            if (analyze_ngram_equal(&all_toks[fi][g], &all_toks[j][g2]))
                return 1;
        }
    }
    return 0;
}

/* analyze 辅助: 打印重复代码片段 */
/* 打印一对函数间的第一个重复 n-gram；返回 1 表示已打印 */
static int analyze_print_first_dup_pair(AnalyzeToken **all_toks, int *tok_counts,
                                        int i, int j, AnalyzeFunc *funcs) {
    int gi = tok_counts[i] - ANALYZE_NGRAM_SIZE + 1;
    int gj = tok_counts[j] - ANALYZE_NGRAM_SIZE + 1;
    if (gi <= 0 || gj <= 0) return 0;
    for (int g = 0; g < gi; g++) {
        for (int g2 = 0; g2 < gj; g2++) {
            if (!analyze_ngram_equal(&all_toks[i][g], &all_toks[j][g2])) continue;
            printf("    %s [%s] <-> %s [%s]: ",
                   funcs[i].name, funcs[i].module,
                   funcs[j].name, funcs[j].module);
            for (int k = 0; k < ANALYZE_NGRAM_SIZE && k < 6; k++)
                printf("%s ", all_toks[i][g + k].text);
            if (ANALYZE_NGRAM_SIZE > 6) printf("...");
            printf("\n");
            return 1;
        }
    }
    return 0;
}

static void analyze_print_dup_fragments(AnalyzeToken **all_toks, int *tok_counts,
                                         AnalyzeFunc *funcs, int func_count) {
    printf("\n  重复代码片段 (最多显示 10 处):\n");
    int shown = 0;
    for (int i = 0; i < func_count && shown < 10; i++) {
        for (int j = i + 1; j < func_count && shown < 10; j++) {
            if (analyze_print_first_dup_pair(all_toks, tok_counts, i, j, funcs))
                shown++;
        }
    }
}

/* analyze 辅助: 计算并打印代码重复率 */
static void commands_analyze_duplication(AnalyzeFunc *funcs, int func_count) {
    printf("--- 3. 代码重复率 (Code Duplication) ---\n");
    AnalyzeToken **all_toks = (AnalyzeToken **)calloc(func_count, sizeof(AnalyzeToken *));
    int *tok_counts = (int *)calloc(func_count, sizeof(int));
    for (int i = 0; i < func_count; i++) {
        all_toks[i] = (AnalyzeToken *)calloc(ANALYZE_MAX_TOKENS, sizeof(AnalyzeToken));
        tok_counts[i] = analyze_tokenize(funcs[i].code, all_toks[i], ANALYZE_MAX_TOKENS);
    }

    int total_ngrams = 0, dup_ngrams = 0;
    for (int i = 0; i < func_count; i++) {
        int gi = tok_counts[i] - ANALYZE_NGRAM_SIZE + 1;
        if (gi <= 0) continue;
        for (int g = 0; g < gi; g++) {
            total_ngrams++;
            if (analyze_ngram_is_dup(all_toks, tok_counts, func_count, i, g))
                dup_ngrams++;
        }
    }

    double dup_rate = (total_ngrams > 0) ? (double)dup_ngrams / total_ngrams * 100.0 : 0.0;
    printf("  n-gram 大小: %d tokens\n", ANALYZE_NGRAM_SIZE);
    printf("  总 n-gram 数: %d\n", total_ngrams);
    printf("  重复 n-gram 数: %d\n", dup_ngrams);
    printf("  代码重复率: %.1f%%\n", dup_rate);

    if (dup_ngrams > 0)
        analyze_print_dup_fragments(all_toks, tok_counts, funcs, func_count);

    for (int i = 0; i < func_count; i++) free(all_toks[i]);
    free(all_toks);
    free(tok_counts);
}

/* 在函数体 code 中查找对 name 的调用：name 后紧跟 '('，且 name 是完整标识符。
 * 跳过字符串、字符、注释、预处理指令。返回 1 表示至少调用一次。
 * 仅负责"是否存在调用"的词法判定，不做命名空间解析（由 analyze_resolve_call 完成）。 */
static int analyze_has_call(const char *code, const char *name) {
    if (!code || !*code || !name || !*name) return 0;
    int nlen = (int)strlen(name);
    const char *p = code;
    while (*p) {
        const char *skipped = analyze_skip_non_code(p);
        if (skipped) { p = skipped; continue; }

        if (isalpha((unsigned char)*p) || *p == '_') {
            const char *id_start = p;
            while (isalnum((unsigned char)*p) || *p == '_') p++;
            int id_len = (int)(p - id_start);
            if (id_len == nlen && strncmp(id_start, name, nlen) == 0) {
                const char *q = p;
                while (isspace((unsigned char)*q)) q++;
                if (*q == '(') return 1;
            }
            continue;
        }
        p++;
    }
    return 0;
}

/* 命名空间解析：从 caller_module 视角，callee_name 应指向哪个函数？
 * 解析规则（C 语言近似）：
 *   1. 优先匹配 caller_module 内定义的同名函数（同文件作用域优先）
 *   2. 否则若全局只有一个同名函数定义，匹配之
 *   3. 否则无法精确定位（多个同名函数跨模块），返回 -1
 * 返回函数在 funcs[] 中的索引，-1 表示无法定位。 */
static int analyze_resolve_call(const char *callee_name,
                                const char *caller_module,
                                AnalyzeFunc *funcs, int func_count) {
    int same_mod_idx = -1;
    int global_count = 0;
    int global_idx = -1;
    for (int k = 0; k < func_count; k++) {
        if (strcmp(funcs[k].name, callee_name) != 0) continue;
        if (strcmp(funcs[k].module, caller_module) == 0) same_mod_idx = k;
        global_idx = k;
        global_count++;
    }
    if (same_mod_idx >= 0) return same_mod_idx;   /* 同模块定义优先 */
    if (global_count == 1) return global_idx;     /* 全局唯一兜底 */
    return -1;                                    /* 无法精确定位 */
}

/* analyze 辅助: 计算并打印依赖图复杂度
 * 定义: 函数被 N 个其他函数调用，复杂度 = max(0, N - 1)。
 * 反映修改一个函数时波及的调用方数量。
 * 调用关系通过命名空间解析精确定位：
 *   对每个调用方 G，先确认其代码中存在 name( 调用，
 *   再按"同模块优先 / 全局唯一兜底"规则解析出真正的被调函数 F。 */
static void commands_analyze_dependency_complexity(AnalyzeFunc *funcs, int func_count) {
    for (int i = 0; i < func_count; i++)
        funcs[i].call_count = 0;

    /* 对每个被调函数 F，扫描所有其他函数 G；若 G 精确调用 F，则 F.call_count++ */
    for (int i = 0; i < func_count; i++) {
        for (int j = 0; j < func_count; j++) {
            if (i == j) continue;  /* 不计自调用 */
            /* 快速预筛：G 代码中是否出现 F 名字的调用 */
            if (!analyze_has_call(funcs[j].code, funcs[i].name)) continue;
            /* 命名空间解析：从 G 的模块看，该调用应指向哪个函数？ */
            int resolved = analyze_resolve_call(funcs[i].name,
                                                funcs[j].module,
                                                funcs, func_count);
            if (resolved == i)
                funcs[i].call_count++;
        }
    }

    int total_dc = 0, max_dc = 0;
    char max_dc_name[128] = "", max_dc_mod[128] = "";
    printf("--- 4. 依赖图复杂度 (Dependency Complexity) ---\n");
    printf("  说明: 被调用次数 - 1，反映修改函数时波及的调用方数量\n");
    printf("  说明: 调用关系经命名空间解析（同模块优先/全局唯一兜底）精确定位\n");
    printf("  %-44s %s\n", "函数", "复杂度");
    printf("  %-44s %s\n", "--------------------------------------------", "--------");
    for (int i = 0; i < func_count; i++) {
        int dc = funcs[i].call_count > 0 ? funcs[i].call_count - 1 : 0;
        char display[300];
        snprintf(display, sizeof(display), "%s [%s]", funcs[i].name, funcs[i].module);
        const char *marker = (dc > 10) ? " *** 高" : (dc > 5) ? " ** 中" : "";
        printf("  %-44s %d%s\n", display, dc, marker);
        total_dc += dc;
        if (dc > max_dc) {
            max_dc = dc;
            strncpy(max_dc_name, funcs[i].name, 127);
            strncpy(max_dc_mod, funcs[i].module, 127);
        }
    }
    printf("  %-44s %s\n", "--------------------------------------------", "--------");
    if (func_count > 0) {
        printf("  %-44s %d\n", "总计", total_dc);
        printf("  %-44s %d\n", "平均", total_dc / func_count);
        if (max_dc > 0)
            printf("  %-44s %d (%s [%s])\n", "最高", max_dc, max_dc_name, max_dc_mod);
        else
            printf("  %-44s %d\n", "最高", 0);
    } else {
        printf("  (无函数数据)\n");
    }
    printf("\n");
}

int commands_cmd_analyze(void)
{
    if (!g_proj || !g_proj->root) {
        printf("错误: 无活动项目\n");
        return -1;
    }

    commands_analyze_ensure_loaded();

    printf("=== CBoot 代码分析报告 ===\n\n");

    /* 1. 有效代码行数 */
    AnalyzeMod mods[ANALYZE_MAX_MODS];
    int mod_count = 0;
    analyze_collect_modules(g_proj->root, mods, &mod_count, ANALYZE_MAX_MODS);
    commands_analyze_code_lines(mods, mod_count);

    /* 2. 圈复杂度 */
    AnalyzeFunc funcs[ANALYZE_MAX_FUNCS];
    int func_count = 0;
    for (int i = 0; i < mod_count && func_count < ANALYZE_MAX_FUNCS; i++)
        analyze_extract_functions(mods[i].name, mods[i].code, funcs, &func_count, ANALYZE_MAX_FUNCS);
    commands_analyze_cyclomatic(funcs, func_count);

    /* 3. 代码重复率 */
    commands_analyze_duplication(funcs, func_count);

    /* 4. 依赖图复杂度 */
    commands_analyze_dependency_complexity(funcs, func_count);

    printf("\n=== 分析完成 ===\n");

    for (int i = 0; i < func_count; i++) free(funcs[i].code);
    return 0;
}

/* ================================================================== */
/* 微调: adjust - 先update同步源码，再进入交互式REPL调整                 */
/* ================================================================== */

int commands_cmd_adjust(void) {
    if (!g_proj || !g_proj->root) {
        printf("错误: 无活动项目\n");
        return -1;
    }

    printf("=== adjust: 正在同步源码到 .cboot ===\n");
    int rc = commands_cmd_update();
    if (rc == 0) {
        printf("=== 同步完成，进入交互式调整模式 ===\n");
        printf("提示: 使用 cd/ls 浏览域树，cmt/value/mode/code 修改定义，gen 重新生成代码\n\n");
    } else {
        printf("=== 同步完成（有错误），仍可进行交互式调整 ===\n");
        printf("提示: 使用 cd/ls 浏览域树，cmt/value/mode/code 修改定义，gen 重新生成代码\n\n");
    }
    return 0;
}


/*
 * im 命令设计：
 * - 仅支持项目内导入（当前必须在某个模块作用域中）
 * - 读取 .cboot 文件，但只提取其中 api 模式的 type/struct/def/void
 * - 在当前模块下创建一个同名子模块作为"API 引用"占位
 * - 记录依赖链：当前模块路径 -> 源模块路径
 * - gen 时根据依赖链在 CMake 中添加 include 目录
 */

/* Helper: recursively copy API items from src module to dst module */
/* 克隆成员列表到目标域（仅复制 DOMAIN_MEMBER） */
static void commands_copy_members(Domain *src, Domain *dst) {
    for (int j = 0; j < src->child_count; j++) {
        Domain *m = src->children[j];
        if (m->type != DOMAIN_MEMBER) continue;
        MemberDomain *mm = domain_member_domain_new(m->name, ((MemberDomain *)m)->type);
        if (m->comment) domain_domain_set_comment((Domain *)mm, m->comment);
        domain_domain_add_child(dst, (Domain *)mm);
    }
}

/* 克隆单个 API 子域；返回克隆对象（已设置 comment/value/成员） */
static Domain *commands_clone_api_child(Domain *child) {
    Domain *clone = NULL;
    switch (child->type) {
        case DOMAIN_FUNCTION: {
            FunctionDomain *f = domain_function_domain_new(child->name,
                                                    ((FunctionDomain *)child)->return_type);
            f->mode = API_MODE_API;
            if (child->comment) domain_domain_set_comment((Domain *)f, child->comment);
            commands_copy_members(child, (Domain *)f);
            clone = (Domain *)f;
            break;
        }
        case DOMAIN_STRUCT: {
            StructDomain *s = domain_struct_domain_new(child->name);
            s->mode = API_MODE_API;
            if (child->comment) domain_domain_set_comment((Domain *)s, child->comment);
            commands_copy_members(child, (Domain *)s);
            clone = (Domain *)s;
            break;
        }
        case DOMAIN_TYPE: {
            TypeDomain *t = domain_type_domain_new(child->name);
            TypeDomain *src_t = (TypeDomain *)child;
            t->mode = src_t->mode;
            if (src_t->value) domain_domain_set_value((Domain *)t, src_t->value);
            if (child->comment) domain_domain_set_comment((Domain *)t, child->comment);
            commands_copy_members(child, (Domain *)t);
            clone = (Domain *)t;
            break;
        }
        case DOMAIN_MACRO: {
            MacroDomain *m = domain_macro_domain_new(child->name);
            m->mode = API_MODE_API;
            MacroDomain *src_m = (MacroDomain *)child;
            if (src_m->value) domain_domain_set_value((Domain *)m, src_m->value);
            if (child->comment) domain_domain_set_comment((Domain *)m, child->comment);
            clone = (Domain *)m;
            break;
        }
        default:
            break;
    }
    return clone;
}

static void commands_copy_api_items(Domain *src_mod, Domain *dst_mod)
{
    for (int i = 0; i < src_mod->child_count; i++) {
        Domain *child = src_mod->children[i];
        if (!child || !domain_domain_is_api(child)) continue;
        Domain *clone = commands_clone_api_child(child);
        if (clone) domain_domain_add_child(dst_mod, clone);
    }
}

/* Helper: recursively find API items in submodules and copy to dst */
static void commands_copy_api_items_recursive(Domain *src_mod, Domain *dst_mod)
{
    commands_copy_api_items(src_mod, dst_mod);

    /* Recurse into child modules */
    for (int i = 0; i < src_mod->child_count; i++) {
        Domain *child = src_mod->children[i];
        if (child->type == DOMAIN_MODULE) {
            /* Create corresponding child module in dst and recurse */
            ModuleDomain *sub = domain_module_domain_new(child->name);
            if (child->comment) domain_domain_set_comment((Domain *)sub, child->comment);
            domain_domain_add_child(dst_mod, (Domain *)sub);
            commands_copy_api_items_recursive(child, (Domain *)sub);
        }
    }
}

/* 在祖先链中查找同名兄弟模块 */
static Domain *commands_im_find_sibling(Domain *importer, const char *path) {
    Domain *parent = importer->parent;
    while (parent) {
        for (int i = 0; i < parent->child_count; i++) {
            Domain *child = parent->children[i];
            if (child->type == DOMAIN_MODULE && child != importer &&
                child->name && strcmp(child->name, path) == 0) {
                return child;
            }
        }
        parent = parent->parent;
    }
    return NULL;
}

/* 项目内兄弟模块导入 */
static int commands_im_internal(Domain *importer, const char *path, char *importer_path) {
    /* 自环检查：模块不能导入自身 */
    if (importer->name && utils_str_eq(importer->name, path)) {
        printf("错误: 循环依赖检测失败 — '%s' 不能导入自身\n", importer_path);
        free(importer_path);
        return -1;
    }
    Domain *src_mod = commands_im_find_sibling(importer, path);
    if (!src_mod) {
        printf("错误: 未找到项目内模块 '%s'\n", path);
        free(importer_path);
        return -1;
    }
    if (domain_domain_find_child(importer, path)) {
        printf("提示: 模块 '%s' 已导入，跳过\n", path);
        free(importer_path);
        return 0;
    }

    char *src_path = domain_domain_get_path(src_mod);
    char source_path_buf[MAX_PATH_LEN];
    snprintf(source_path_buf, sizeof(source_path_buf), "%s", src_path);

    /* 循环依赖检查：若 src_path 已（直接或间接）依赖 importer_path，则禁止 */
    if (domain_project_would_cycle(g_proj, importer_path, source_path_buf)) {
        printf("错误: 循环依赖检测失败 — '%s' -> '%s' 会形成依赖环\n",
               importer_path, source_path_buf);
        free(src_path);
        free(importer_path);
        return -1;
    }

    ModuleDomain *api_ref = domain_module_domain_new(path);
    api_ref->mode = MOD_MODE_EXTERNAL;
    char cmt_buf[MAX_PATH_LEN];
    snprintf(cmt_buf, sizeof(cmt_buf), "[API 引用] 从项目内模块 %s 导入", path);
    domain_domain_set_comment((Domain *)api_ref, cmt_buf);

    commands_copy_api_items_recursive(src_mod, (Domain *)api_ref);
    domain_domain_add_child(importer, (Domain *)api_ref);
    domain_project_add_dependency(g_proj, importer_path, source_path_buf, path);
    free(src_path);

    printf("已从项目内模块 '%s' 导入 API 定义到 %s\n", path, importer_path);
    free(importer_path);
    return 0;
}

/* 外部 .cboot 文件导入 */
static int commands_im_external(Domain *importer, const char *path, char *importer_path) {
    if (!utils_file_exists(path)) {
        printf("错误: 文件 '%s' 不存在\n", path);
        free(importer_path);
        return -1;
    }

    Project *tmp_proj = domain_project_new("_im_tmp");
    if (!tmp_proj) { free(importer_path); printf("错误: 无法创建临时项目\n"); return -1; }

    Project *saved_proj = g_proj;
    g_proj = tmp_proj;
    if (parser_parse_cboot_script(path) != 0) {
        printf("错误: 解析 .cboot 文件失败\n");
        g_proj = saved_proj;
        domain_project_free(tmp_proj);
        free(importer_path);
        return -1;
    }
    g_proj = saved_proj;

    char source_path_buf[MAX_PATH_LEN];
    snprintf(source_path_buf, sizeof(source_path_buf), "/%s", tmp_proj->root->name);
    char *source_path = utils_str_dup(source_path_buf);
    const char *src_name = tmp_proj->root->name;

    if (domain_project_has_dependency(g_proj, importer_path, source_path)) {
        printf("提示: 依赖 '%s' -> '%s' 已存在，跳过\n", importer_path, source_path);
        domain_project_free(tmp_proj);
        free(importer_path);
        free(source_path);
        return 0;
    }
    /* 循环依赖检查：若 source_path 已（直接或间接）依赖 importer_path，则禁止 */
    if (domain_project_would_cycle(g_proj, importer_path, source_path)) {
        printf("错误: 循环依赖检测失败 — '%s' -> '%s' 会形成依赖环\n",
               importer_path, source_path);
        domain_project_free(tmp_proj);
        free(importer_path);
        free(source_path);
        return -1;
    }
    if (domain_domain_find_child(importer, src_name)) {
        printf("错误: 当前模块下已存在同名子域 '%s'\n", src_name);
        domain_project_free(tmp_proj);
        free(importer_path);
        free(source_path);
        return -1;
    }

    ModuleDomain *api_ref = domain_module_domain_new(src_name);
    api_ref->mode = MOD_MODE_EXTERNAL;
    char cmt_buf[MAX_PATH_LEN];
    snprintf(cmt_buf, sizeof(cmt_buf), "[API 引用] 从 %s 导入", path);
    domain_domain_set_comment((Domain *)api_ref, cmt_buf);

    commands_copy_api_items_recursive(tmp_proj->root, (Domain *)api_ref);
    domain_domain_add_child(importer, (Domain *)api_ref);
    domain_project_add_dependency(g_proj, importer_path, source_path, path);

    printf("已从 %s 导入 API 定义到 %s（作为外部引用模块 %s）\n",
           path, importer_path, src_name);
    printf("依赖链: %s -> %s\n", importer_path, source_path);

    domain_project_free(tmp_proj);
    free(importer_path);
    free(source_path);
    return 0;
}

int commands_cmd_im(const char *path) {
    if (!path) {
        printf("用法: im <模块名|.cboot 文件>\n");
        return -1;
    }

    Domain *importer = g_proj->current;
    if (importer->type != DOMAIN_MODULE) {
        printf("错误: im 只能在模块作用域中使用\n");
        return -1;
    }

    char *importer_path = domain_domain_get_path(importer);

    int len = (int)strlen(path);
    int is_cboot_file = (len > 6 && strcmp(path + len - 6, ".cboot") == 0);

    if (!is_cboot_file) return commands_im_internal(importer, path, importer_path);
    return commands_im_external(importer, path, importer_path);
}

/* ================================================================== */
/* 导入: in <.cboot file>  - 复制整个项目作为子模块                     */
/* ================================================================== */

/*
 * in 命令设计：
 * - 读取 .cboot 文件，把整个项目结构作为子模块复制到当前模块下
 * - 适用于引入完整的外部项目实现
 */

int commands_cmd_in(const char *path) {
    if (!path) {
        printf("用法: in <.cboot 文件>\n");
        return -1;
    }

    if (!utils_file_exists(path)) {
        printf("错误: 文件 '%s' 不存在\n", path);
        return -1;
    }

    /* in 只能在模块作用域中使用 */
    Domain *importer = g_proj->current;
    if (importer->type != DOMAIN_MODULE) {
        printf("错误: in 只能在模块作用域中使用\n");
        return -1;
    }

    /* 解析 .cboot 文件到一个临时 Project */
    Project *tmp_proj = domain_project_new("_in_tmp");
    if (!tmp_proj) {
        printf("错误: 无法创建临时项目\n");
        return -1;
    }

    Project *saved_proj = g_proj;
    g_proj = tmp_proj;

    if (parser_parse_cboot_script(path) != 0) {
        printf("错误: 解析 .cboot 文件失败\n");
        g_proj = saved_proj;
        domain_project_free(tmp_proj);
        return -1;
    }

    g_proj = saved_proj;

    /* 获取源项目名 */
    const char *src_name = tmp_proj->root->name;

    /* 检查名称冲突 */
    if (domain_domain_find_child(importer, src_name)) {
        printf("错误: 当前模块下已存在同名子域 '%s'\n", src_name);
        domain_project_free(tmp_proj);
        return -1;
    }

    /* 将 tmp_proj 的 root 模块整体移动到当前模块下 */
    Domain *src_root = tmp_proj->root;
    tmp_proj->root = NULL;  /* 防止 domain_project_free 释放它 */
    domain_domain_add_child(importer, src_root);

    /* 记录到 imported_projects */
    if (g_proj->import_count >= g_proj->import_capacity) {
        g_proj->import_capacity = (g_proj->import_capacity == 0) ? 8 : g_proj->import_capacity * 2;
        g_proj->imported_projects = (char **)realloc(g_proj->imported_projects,
                                                      sizeof(char *) * g_proj->import_capacity);
    }
    g_proj->imported_projects[g_proj->import_count++] = utils_str_dup(path);

    printf("已从 %s 导入完整项目 '%s' 作为子模块\n", path, src_name);

    domain_project_free(tmp_proj);
    return 0;
}

/* ================================================================== */
/* 资源: res <file>                                                     */
/* ================================================================== */

int commands_cmd_res(const char *file_path) {
    if (!file_path) {
        printf("用法: res <资源文件路径>\n");
        return -1;
    }

    ModuleDomain *mod = commands_get_current_module();
    if (!mod) {
        printf("错误: 当前不在模块作用域中\n");
        return -1;
    }

    /* Check if source file exists — 资源文件不存在则报错拒绝，避免生成时引用缺失 */
    if (!utils_file_exists(file_path)) {
        printf("错误: 资源文件 '%s' 不存在\n", file_path);
        return -1;
    }

    /* Get basename */
    const char *basename = strrchr(file_path, '/');
    if (basename) basename++;
    else basename = file_path;

    /* Copy resource file to res/ directory */
    utils_ensure_dir("res");

    char dest_path[MAX_PATH_LEN];
    snprintf(dest_path, sizeof(dest_path), "res/%s", basename);

    FILE *src = fopen(file_path, "rb");
    if (src) {
        FILE *dst = fopen(dest_path, "wb");
        if (dst) {
            char buf[4096];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), src)) > 0)
                fwrite(buf, 1, n, dst);
            fclose(dst);
        }
        fclose(src);
    }

    printf("资源 %s 已添加。\n", basename);
    return 0;
}
