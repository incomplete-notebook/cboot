/* commands.c - CBoot generated (compiler: normal) */
/* Module: commands */

/*
 * CBoot - C Project Bootstrapping Tool v0.3.1
 * Command handlers (新规范 v0.3.1)
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
static const char *commands_mode_str(Domain *d) {
    if (!d) return "";
    switch (d->type) {
        case DOMAIN_MODULE: {
            ModuleDomain *mod = (ModuleDomain *)d;
            switch (mod->mode) {
                case MOD_MODE_SRC:      return "src";
                case MOD_MODE_STATIC:   return "static";
                case MOD_MODE_DYNAMIC:  return "dynamic";
                case MOD_MODE_EXTERNAL: return "external";
            }
            return "?";
        }
        case DOMAIN_FUNCTION: {
            FunctionDomain *f = (FunctionDomain *)d;
            return f->mode == API_MODE_API ? "api" : "normal";
        }
        case DOMAIN_STRUCT: {
            StructDomain *s = (StructDomain *)d;
            return s->mode == API_MODE_API ? "api" : "normal";
        }
        case DOMAIN_TYPE: {
            TypeDomain *t = (TypeDomain *)d;
            switch (t->mode) {
                case TYPE_MODE_RENAME:     return "rename";
                case TYPE_MODE_STRUCT:     return "struct";
                case TYPE_MODE_API_RENAME: return "api rename";
                case TYPE_MODE_API_STRUCT: return "api struct";
            }
            return "?";
        }
        case DOMAIN_MACRO: {
            MacroDomain *m = (MacroDomain *)d;
            return m->mode == API_MODE_API ? "api" : "normal";
        }
        case DOMAIN_VARIABLE: {
            VariableDomain *v = (VariableDomain *)d;
            return v->mode == VAR_MODE_STATIC ? "static" : "normal";
        }
        default: return "";
    }
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

/* commands_cmd_mem - 创建成员/参数 (mem <name> <type>, 上下文敏感) */
int commands_cmd_mem(const char *name, const char *type) {
    if (!name || !type) {
        printf("用法: mem <名称> <类型>\n");
        return -1;
    }

    DomainType cur_type = g_proj->current->type;

    /* Context-sensitive: function→参数, struct/type→成员 */
    if (cur_type == DOMAIN_FUNCTION) {
        /* Adding a parameter */
        if (commands_check_name_dup(name, g_proj->current) != 0)
            return -1;

        if (commands_check_type(type) != 0)
            return -1;

        MemberDomain *m = domain_member_domain_new(name, type);
        if (!m) {
            printf("错误: 无法创建参数\n");
            return -1;
        }

        domain_domain_add_child(g_proj->current, (Domain *)m);
        /* 不改变 current 作用域，便于连续添加多个参数 */
        printf("参数 %s %s 已添加。\n", type, name);
        return 0;
    }
    else if (cur_type == DOMAIN_STRUCT) {
        /* Adding a struct member */
        if (commands_check_name_dup(name, g_proj->current) != 0)
            return -1;

        if (commands_check_type(type) != 0)
            return -1;

        MemberDomain *m = domain_member_domain_new(name, type);
        if (!m) {
            printf("错误: 无法创建成员\n");
            return -1;
        }

        domain_domain_add_child(g_proj->current, (Domain *)m);
        /* 不改变 current 作用域，便于连续添加多个成员 */
        printf("成员 %s %s 已添加。\n", type, name);
        return 0;
    }
    else if (cur_type == DOMAIN_TYPE) {
        TypeDomain *td = (TypeDomain *)g_proj->current;
        if (td->mode == TYPE_MODE_STRUCT || td->mode == TYPE_MODE_API_STRUCT) {
            /* Adding a member to struct-mode type */
            if (commands_check_name_dup(name, g_proj->current) != 0)
                return -1;

            if (commands_check_type(type) != 0)
                return -1;

            MemberDomain *m = domain_member_domain_new(name, type);
            if (!m) {
                printf("错误: 无法创建成员\n");
                return -1;
            }

            domain_domain_add_child(g_proj->current, (Domain *)m);
            /* 不改变 current 作用域，便于连续添加多个成员 */
            printf("成员 %s %s 已添加。\n", type, name);
            return 0;
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

/* commands_cmd_mode - 设置模式 */
int commands_cmd_mode(const char *text) {
    if (!text) {
        printf("用法: mode <模式>\n");
        return -1;
    }

    Domain *cur = g_proj->current;
    int becoming_api = 0;

    /* Determine if we're switching to API mode */
    switch (cur->type) {
        case DOMAIN_FUNCTION:
        case DOMAIN_STRUCT:
        case DOMAIN_MACRO:
            if (utils_str_eq(text, "api")) becoming_api = 1;
            break;
        case DOMAIN_TYPE:
            if (utils_str_eq(text, "api rename") || utils_str_eq(text, "api struct"))
                becoming_api = 1;
            break;
        default:
            break;
    }

    /* If becoming API, check for name conflicts in parent modules */
    if (becoming_api && cur->parent) {
        Domain *parent = cur->parent;
        /* Check all ancestor modules for conflicts */
        while (parent) {
            if (parent->type == DOMAIN_MODULE &&
                parent != cur->parent /* skip direct parent - already has the name */) {
                Domain *conflict = domain_domain_find_child(parent, cur->name);
                if (conflict && conflict->type != DOMAIN_MODULE) {
                    char *path = domain_domain_get_path(conflict);
                    printf("错误: 设为 api 后名称 '%s' 将与父模块域冲突 (%s [%s])\n",
                           cur->name, path, commands_domain_type_name(conflict->type));
                    free(path);
                    return -1;
                }
                /* Check sibling submodule API items too */
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
    }

    switch (cur->type) {
        case DOMAIN_MODULE: {
            if (utils_str_eq(text, "src")) {
                domain_domain_set_mode(cur, MOD_MODE_SRC);
            } else if (utils_str_eq(text, "static")) {
                domain_domain_set_mode(cur, MOD_MODE_STATIC);
            } else if (utils_str_eq(text, "dynamic")) {
                domain_domain_set_mode(cur, MOD_MODE_DYNAMIC);
            } else if (utils_str_eq(text, "external")) {
                domain_domain_set_mode(cur, MOD_MODE_EXTERNAL);
            } else {
                printf("错误: 模块模式只能是 src/static/dynamic/external\n");
                return -1;
            }
            break;
        }
        case DOMAIN_FUNCTION:
        case DOMAIN_STRUCT: {
            if (utils_str_eq(text, "api")) {
                domain_domain_set_mode(cur, API_MODE_API);
            } else if (utils_str_eq(text, "normal")) {
                domain_domain_set_mode(cur, API_MODE_NORMAL);
            } else {
                printf("错误: API模式只能是 api 或 normal\n");
                return -1;
            }
            break;
        }
        case DOMAIN_TYPE: {
            if (utils_str_eq(text, "rename")) {
                domain_domain_set_mode(cur, TYPE_MODE_RENAME);
            } else if (utils_str_eq(text, "struct")) {
                domain_domain_set_mode(cur, TYPE_MODE_STRUCT);
            } else if (utils_str_eq(text, "api rename")) {
                domain_domain_set_mode(cur, TYPE_MODE_API_RENAME);
            } else if (utils_str_eq(text, "api struct")) {
                domain_domain_set_mode(cur, TYPE_MODE_API_STRUCT);
            } else {
                printf("错误: 类型模式只能是 rename/struct/api rename/api struct\n");
                return -1;
            }
            break;
        }
        case DOMAIN_MACRO: {
            if (utils_str_eq(text, "api")) {
                domain_domain_set_mode(cur, API_MODE_API);
            } else if (utils_str_eq(text, "normal")) {
                domain_domain_set_mode(cur, API_MODE_NORMAL);
            } else {
                printf("错误: 宏模式只能是 api 或 normal\n");
                return -1;
            }
            break;
        }
        case DOMAIN_VARIABLE: {
            if (utils_str_eq(text, "static")) {
                domain_domain_set_mode(cur, VAR_MODE_STATIC);
            } else if (utils_str_eq(text, "normal")) {
                domain_domain_set_mode(cur, VAR_MODE_NORMAL);
            } else {
                printf("错误: 变量模式只能是 static 或 normal\n");
                return -1;
            }
            break;
        }
        case DOMAIN_MEMBER:
            printf("成员域不支持设置 mode\n");
            return -1;
    }

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
int commands_cmd_cd(const char *path) {
    if (!path || path[0] == '\0') {
        /* cd without args: show current path */
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

    /* "/" -> root */
    if (utils_str_eq(path, "/")) {
        g_proj->current = g_proj->root;
        printf("已进入: %s\n", g_proj->current->name);
        return 0;
    }

    Domain *start = g_proj->current;
    const char *p = path;

    /* "/xxx" prefix -> navigate from root */
    if (path[0] == '/') {
        start = g_proj->root;
        p = path + 1;
        if (*p == '\0') {
            g_proj->current = start;
            printf("已进入: %s\n", g_proj->current->name);
            return 0;
        }
    }

    /* Split by "/" and walk */
    char segment[MAX_NAME_LEN];
    Domain *cur = start;

    while (*p) {
        int i = 0;
        while (*p && *p != '/' && i < MAX_NAME_LEN - 1)
            segment[i++] = *p++;
        segment[i] = '\0';
        if (*p == '/') p++;

        if (segment[0] == '\0') continue;

        Domain *child = domain_domain_find_child(cur, segment);
        if (!child) {
            printf("cd: 未找到 '%s'\n", segment);
            return -1;
        }
        cur = child;
    }

    g_proj->current = cur;
    printf("已进入: %s\n", g_proj->current->name);
    return 0;
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
    printf("确认删除 %s '%s'? (y/N): ", commands_domain_type_name(child->type), name);
    fflush(stdout);

    char line[MAX_LINE_LEN];
    if (fgets(line, sizeof(line), stdin) == NULL) {
        printf("已取消。\n");
        return 0;
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

int commands_cmd_find(const char *type_filter, const char *pattern, int flags) {
    if (!type_filter) {
        printf("用法: find <类型:mod,void,struct,type,def,var,mem,all> <字符> [-a|-an]\n");
        return -1;
    }

    DomainType filter_type = -1;  /* -1 = all */

    if (utils_str_eq(type_filter, "mod"))     filter_type = DOMAIN_MODULE;
    else if (utils_str_eq(type_filter, "void"))    filter_type = DOMAIN_FUNCTION;
    else if (utils_str_eq(type_filter, "struct"))  filter_type = DOMAIN_STRUCT;
    else if (utils_str_eq(type_filter, "type"))    filter_type = DOMAIN_TYPE;
    else if (utils_str_eq(type_filter, "def"))     filter_type = DOMAIN_MACRO;
    else if (utils_str_eq(type_filter, "var"))     filter_type = DOMAIN_VARIABLE;
    else if (utils_str_eq(type_filter, "mem"))     filter_type = DOMAIN_MEMBER;
    else if (utils_str_eq(type_filter, "all"))     filter_type = -1;

    /*
     * flags: bit 0 = -a (recursive), bits 1-7 = depth for -an
     * For simplicity: flags == 0 = current only, flags == 1 = -a full recursive,
     * flags > 1 = depth limit
     */
    int max_depth = 0;
    if (flags == 0) {
        max_depth = 0;  /* current only */
    } else if (flags == 1) {
        max_depth = -1; /* unlimited */
    } else {
        max_depth = flags; /* depth limit */
    }

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

    /* List children */
    if (target->child_count > 0) {
        printf("  子域 (%d):\n", target->child_count);
        for (int i = 0; i < target->child_count; i++) {
            Domain *child = target->children[i];
            printf("    [%s] %s", commands_domain_type_name(child->type), child->name);

            /* Show type for members and variables */
            if (child->type == DOMAIN_MEMBER) {
                MemberDomain *mb = (MemberDomain *)child;
                printf(" : %s", mb->type);
            } else if (child->type == DOMAIN_VARIABLE) {
                VariableDomain *v = (VariableDomain *)child;
                printf(" : %s", v->type);
            } else if (child->type == DOMAIN_FUNCTION) {
                FunctionDomain *f = (FunctionDomain *)child;
                printf(" : %s()", f->return_type);
            }

            if (child->comment) printf(" - %s", child->comment);
            printf("\n");
        }
    } else if (target->child_count == 0 && !name) {
        printf("  (无子域)\n");
    }

    return 0;
}

/* ================================================================== */
/* 移动: mv <src> <target>                                              */
/* ================================================================== */

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

    /* Navigate to target */
    Domain *target_parent = g_proj->current;
    const char *target_name = target;

    /* Check if target is a path */
    const char *slash = strrchr(target, '/');
    if (slash) {
        /* Navigate to the parent directory of target */
        char parent_path[MAX_NAME_LEN];
        int parent_len = (int)(slash - target);
        if (parent_len == 0) {
            target_parent = g_proj->root;
            target_name = slash + 1;
        } else {
            strncpy(parent_path, target, parent_len);
            parent_path[parent_len] = '\0';

            /* Save current and navigate */
            Domain *saved = g_proj->current;
            g_proj->current = target_parent;
            if (commands_cmd_cd(parent_path) != 0) {
                g_proj->current = saved;
                return -1;
            }
            target_parent = g_proj->current;
            g_proj->current = saved;
            target_name = slash + 1;
        }
    }

    /* Check target doesn't exist */
    if (domain_domain_find_child(target_parent, target_name)) {
        printf("错误: 目标 '%s' 已存在\n", target_name);
        return -1;
    }

    /* Check for conflict with submodule API items */
    Domain *api_conflict = domain_domain_check_api_name_conflict(target_parent, target_name);
    if (api_conflict && api_conflict != src_domain) {
        char *path = domain_domain_get_path(api_conflict);
        printf("错误: 目标 '%s' 与子模块 API 项冲突 (%s [%s])\n",
               target_name, path, commands_domain_type_name(api_conflict->type));
        free(path);
        return -1;
    }

    /* Remove from source */
    domain_domain_remove_child(g_proj->current, src_domain);

    /* Rename if needed */
    if (!utils_str_eq(src_domain->name, target_name)) {
        free(src_domain->name);
        src_domain->name = utils_str_dup(target_name);
    }

    /* Add to target */
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
} AnalyzeFunc;

typedef struct {
    char text[64];          /* 标识符保留原文；数字/字符串/字符归一化 */
} AnalyzeToken;

static void analyze_collect_modules(Domain *d, AnalyzeMod *mods, int *count, int cap)
{
    if (!d || *count >= cap) return;

    if (d->type == DOMAIN_MODULE) {
        ModuleDomain *md = (ModuleDomain *)d;
        if (md->mode == MOD_MODE_SRC && md->code && md->code[0] != '\0') {
            AnalyzeMod *m = &mods[*count];
            /* 构造模块路径名：从根走到当前模块 */
            m->name[0] = '\0';
            Domain *p = d;
            char parts[16][128];
            int n = 0;
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
                if (m->name[0] != '\0') strncat(m->name, "/", sizeof(m->name) - strlen(m->name) - 1);
                strncat(m->name, parts[i], sizeof(m->name) - strlen(m->name) - 1);
            }
            m->code = md->code;
            m->code_lines = 0;
            (*count)++;
        }
    }

    for (int i = 0; i < d->child_count; i++)
        analyze_collect_modules(d->children[i], mods, count, cap);
}

static int analyze_count_code_lines(const char *code)
{
    if (!code) return 0;
    int lines = 0;
    int has_code = 0;          /* 当前行是否已出现代码字符 */
    enum { ST_NORMAL, ST_LINE_CMT, ST_BLOCK_CMT, ST_STRING, ST_CHAR } state = ST_NORMAL;
    const char *p = code;
    while (*p) {
        char c = *p;
        switch (state) {
        case ST_NORMAL:
            if (c == '/' && p[1] == '/') { state = ST_LINE_CMT; p++; }
            else if (c == '/' && p[1] == '*') { state = ST_BLOCK_CMT; p++; }
            else if (c == '"') { state = ST_STRING; has_code = 1; }
            else if (c == '\'') { state = ST_CHAR; has_code = 1; }
            else if (c == '\n') { if (has_code) lines++; has_code = 0; }
            else if (!isspace((unsigned char)c)) has_code = 1;
            break;
        case ST_LINE_CMT:
            if (c == '\n') { if (has_code) lines++; has_code = 0; state = ST_NORMAL; }
            break;
        case ST_BLOCK_CMT:
            if (c == '*' && p[1] == '/') { state = ST_NORMAL; p++; }
            else if (c == '\n') { if (has_code) lines++; has_code = 0; }
            break;
        case ST_STRING:
            if (c == '\\') { p++; }
            else if (c == '"') { state = ST_NORMAL; }
            else if (c == '\n') { if (has_code) lines++; has_code = 0; }
            break;
        case ST_CHAR:
            if (c == '\\') { p++; }
            else if (c == '\'') { state = ST_NORMAL; }
            else if (c == '\n') { if (has_code) lines++; has_code = 0; }
            break;
        }
        p++;
    }
    if (has_code) lines++;
    return lines;
}

/* 从源码中提取函数：匹配 "type name(...)" 后跟 "{" 的模式 */
static void analyze_extract_functions(const char *mod_name, const char *code,
                                       AnalyzeFunc *funcs, int *count, int cap)
{
    if (!code || !*code) return;
    const char *p = code;
    while (*p && *count < cap) {
        /* 跳过字符串和注释 */
        if (*p == '"') { p++; while (*p && *p != '"') { if (*p == '\\') p++; if (*p) p++; } if (*p) p++; continue; }
        if (*p == '\'') { p++; while (*p && *p != '\'') { if (*p == '\\') p++; if (*p) p++; } if (*p) p++; continue; }
        if (p[0] == '/' && p[1] == '/') { while (*p && *p != '\n') p++; continue; }
        if (p[0] == '/' && p[1] == '*') { p += 2; while (*p && !(p[0] == '*' && p[1] == '/')) p++; if (*p) p += 2; continue; }
        if (*p == '#') { while (*p && *p != '\n') p++; continue; }

        /* 记录行首位置（可能为函数定义开头） */
        if (p == code || p[-1] == '\n') {
            const char *start = p;
            /* 跳过前导空白 */
            while (*start && isspace((unsigned char)*start)) start++;
            if (*start == '\0') { p++; continue; }

            /* 查找参数列表闭合后跟 "{"
             * 跟踪第一次 depth 升到 1 的 '(' (open_paren) 与最近一次 depth 回到 0 的 ')' (close_paren) */
            const char *paren = start;
            int depth = 0;
            const char *open_paren = NULL;   /* 第一个 '(' */
            const char *close_paren = NULL;  /* 与 open_paren 匹配的 ')' */
            while (*paren && *paren != '{' && *paren != ';') {
                if (*paren == '(') {
                    if (depth == 0) open_paren = paren;
                    depth++;
                } else if (*paren == ')') {
                    depth--;
                    if (depth == 0 && open_paren) close_paren = paren;
                }
                paren++;
            }
            /* 检查是否找到参数列表闭合后跟 "{" */
            if (depth == 0 && *paren == '{' && open_paren && close_paren) {
                /* 从 open_paren 回溯找到函数名 */
                const char *brace = paren;
                const char *name_end = open_paren;
                while (name_end > start && isspace((unsigned char)name_end[-1])) name_end--;
                const char *name_start = name_end;
                while (name_start > start && (isalnum((unsigned char)name_start[-1]) || name_start[-1] == '_'))
                    name_start--;

                /* 检查返回类型是否存在（确保不是 if/for/while 等语句） */
                int has_type = 0;
                for (const char *c = start; c < name_start; c++) {
                    if (!isspace((unsigned char)*c)) { has_type = 1; break; }
                }

                if (has_type && name_start < name_end) {
                    /* 提取函数名 */
                    int nlen = (int)(name_end - name_start);
                    if (nlen > 0 && nlen < 127) {
                        /* 排除控制流关键字 */
                        const char *keywords[] = {"if", "for", "while", "switch", "return", "sizeof", "else", "do", 0};
                        int is_kw = 0;
                        for (int k = 0; keywords[k]; k++) {
                            if (nlen == (int)strlen(keywords[k]) && strncmp(name_start, keywords[k], nlen) == 0) {
                                is_kw = 1; break;
                            }
                        }
                        if (!is_kw) {
                            /* 找到函数定义，提取函数体 */
                            const char *body_start = brace + 1;
                            const char *body_end = body_start;
                            int brace_depth = 1;
                            while (*body_end && brace_depth > 0) {
                                if (*body_end == '{') brace_depth++;
                                else if (*body_end == '}') brace_depth--;
                                else if (*body_end == '"') {
                                    body_end++;
                                    while (*body_end && *body_end != '"') {
                                        if (*body_end == '\\') body_end++;
                                        if (*body_end) body_end++;
                                    }
                                } else if (*body_end == '\'') {
                                    body_end++;
                                    while (*body_end && *body_end != '\'') {
                                        if (*body_end == '\\') body_end++;
                                        if (*body_end) body_end++;
                                    }
                                }
                                if (body_end < body_start) break;
                                body_end++;
                            }
                            if (brace_depth == 0) {
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
                                p = body_end;
                                continue;
                            }
                        }
                    }
                }
            }
        }
        p++;
    }
}

/* 简单词法分析：将代码拆分为 token，用于 n-gram 比对 */
static int analyze_tokenize(const char *code, AnalyzeToken *toks, int cap)
{
    int count = 0;
    const char *p = code;
    while (*p && count < cap) {
        /* 跳过空白 */
        if (isspace((unsigned char)*p)) { p++; continue; }
        /* 跳过注释 */
        if (p[0] == '/' && p[1] == '/') { while (*p && *p != '\n') p++; continue; }
        if (p[0] == '/' && p[1] == '*') { p += 2; while (*p && !(p[0] == '*' && p[1] == '/')) p++; if (*p) p += 2; continue; }

        /* 字符串 -> 归一化为 "STR" */
        if (*p == '"') {
            strncpy(toks[count].text, "\"STR\"", 63);
            toks[count].text[63] = '\0';
            count++;
            p++;
            while (*p && *p != '"') { if (*p == '\\') p++; if (*p) p++; }
            if (*p) p++;
            continue;
        }
        /* 字符 -> 归一化为 'C' */
        if (*p == '\'') {
            strncpy(toks[count].text, "'C'", 63);
            toks[count].text[63] = '\0';
            count++;
            p++;
            while (*p && *p != '\'') { if (*p == '\\') p++; if (*p) p++; }
            if (*p) p++;
            continue;
        }
        /* 数字 -> 归一化为 NUM */
        if (isdigit((unsigned char)*p) || (*p == '.' && isdigit((unsigned char)p[1]))) {
            strncpy(toks[count].text, "NUM", 63);
            toks[count].text[63] = '\0';
            count++;
            while (isdigit((unsigned char)*p) || *p == '.' || *p == 'x' || *p == 'X' || (isdigit((unsigned char)*p))) p++;
            continue;
        }
        /* 标识符 */
        if (isalpha((unsigned char)*p) || *p == '_') {
            int len = 0;
            while ((isalnum((unsigned char)*p) || *p == '_') && len < 63) {
                toks[count].text[len++] = *p++;
            }
            toks[count].text[len] = '\0';
            while (isalnum((unsigned char)*p) || *p == '_') p++;
            count++;
            continue;
        }
        /* 运算符和其他符号 */
        if (ispunct((unsigned char)*p)) {
            /* 多字符运算符 */
            if (p[0] && p[1]) {
                char two[3] = { p[0], p[1], '\0' };
                if (two[0] == '=' && two[1] == '=' || two[0] == '!' && two[1] == '=' ||
                    two[0] == '<' && two[1] == '=' || two[0] == '>' && two[1] == '=' ||
                    two[0] == '&' && two[1] == '&' || two[0] == '|' && two[1] == '|' ||
                    two[0] == '+' && two[1] == '+' || two[0] == '-' && two[1] == '-' ||
                    two[0] == '-' && two[1] == '>' || two[0] == '<' && two[1] == '<' ||
                    two[0] == '>' && two[1] == '>' || two[0] == '+' && two[1] == '=' ||
                    two[0] == '-' && two[1] == '=' || two[0] == '*' && two[1] == '=' ||
                    two[0] == '/' && two[1] == '=' || two[0] == '%' && two[1] == '=' ||
                    two[0] == '&' && two[1] == '=' || two[0] == '|' && two[1] == '=' ||
                    two[0] == '^' && two[1] == '=' || two[0] == '.' && two[1] == '.' ) {
                    strncpy(toks[count].text, two, 63);
                    toks[count].text[63] = '\0';
                    count++;
                    p += 2;
                    continue;
                }
            }
            toks[count].text[0] = *p;
            toks[count].text[1] = '\0';
            count++;
            p++;
            continue;
        }
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

int commands_cmd_analyze(void)
{
    if (!g_proj || !g_proj->root) {
        printf("错误: 无活动项目\n");
        return -1;
    }

    /* 若项目为空但有 .cboot，则加载之 */
    if (g_proj->root->child_count == 0 && utils_file_exists(".cboot")) {
        domain_project_free(g_proj);
        g_proj = domain_project_new("cboot_project");
        g_skip_gen = 1;  /* 加载时跳过 gen，避免破坏源码 */
        parser_parse_cboot_script(".cboot");
        g_skip_gen = 0;
    }

    printf("=== CBoot 代码分析报告 ===\n\n");

    /* 1. 有效代码行数 */
    AnalyzeMod mods[ANALYZE_MAX_MODS];
    int mod_count = 0;
    analyze_collect_modules(g_proj->root, mods, &mod_count, ANALYZE_MAX_MODS);

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

    /* 2. 圈复杂度 */
    AnalyzeFunc funcs[ANALYZE_MAX_FUNCS];
    int func_count = 0;
    for (int i = 0; i < mod_count && func_count < ANALYZE_MAX_FUNCS; i++)
        analyze_extract_functions(mods[i].name, mods[i].code, funcs, &func_count, ANALYZE_MAX_FUNCS);

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
        const char *marker = "";
        if (cc > 15) marker = " *** 高";
        else if (cc > 10) marker = " ** 中";
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

    /* 3. 代码重复率 (基于 token n-gram 比对) */
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
            int is_dup = 0;
            for (int j = 0; j < func_count && !is_dup; j++) {
                if (j == i) continue;
                int gj = tok_counts[j] - ANALYZE_NGRAM_SIZE + 1;
                if (gj <= 0) continue;
                for (int g2 = 0; g2 < gj; g2++) {
                    if (analyze_ngram_equal(&all_toks[i][g], &all_toks[j][g2])) {
                        is_dup = 1;
                        break;
                    }
                }
            }
            if (is_dup) dup_ngrams++;
        }
    }

    double dup_rate = (total_ngrams > 0) ? (double)dup_ngrams / total_ngrams * 100.0 : 0.0;
    printf("  n-gram 大小: %d tokens\n", ANALYZE_NGRAM_SIZE);
    printf("  总 n-gram 数: %d\n", total_ngrams);
    printf("  重复 n-gram 数: %d\n", dup_ngrams);
    printf("  代码重复率: %.1f%%\n", dup_rate);

    if (dup_ngrams > 0) {
        printf("\n  重复代码片段 (最多显示 10 处):\n");
        int shown = 0;
        for (int i = 0; i < func_count && shown < 10; i++) {
            int gi = tok_counts[i] - ANALYZE_NGRAM_SIZE + 1;
            if (gi <= 0) continue;
            for (int g = 0; g < gi && shown < 10; g++) {
                for (int j = i + 1; j < func_count; j++) {
                    int gj = tok_counts[j] - ANALYZE_NGRAM_SIZE + 1;
                    if (gj <= 0) continue;
                    int found = 0;
                    for (int g2 = 0; g2 < gj; g2++) {
                        if (analyze_ngram_equal(&all_toks[i][g], &all_toks[j][g2])) {
                            printf("    %s [%s] <-> %s [%s]: ",
                                   funcs[i].name, funcs[i].module,
                                   funcs[j].name, funcs[j].module);
                            for (int k = 0; k < ANALYZE_NGRAM_SIZE && k < 6; k++)
                                printf("%s ", all_toks[i][g + k].text);
                            if (ANALYZE_NGRAM_SIZE > 6) printf("...");
                            printf("\n");
                            found = 1;
                            break;
                        }
                    }
                    if (found) { shown++; break; }
                }
            }
        }
    }

    printf("\n=== 分析完成 ===\n");

    for (int i = 0; i < func_count; i++) {
        free(all_toks[i]);
        free(funcs[i].code);
    }
    free(all_toks);
    free(tok_counts);
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
static void commands_copy_api_items(Domain *src_mod, Domain *dst_mod)
{
    for (int i = 0; i < src_mod->child_count; i++) {
        Domain *child = src_mod->children[i];
        if (!child) continue;

        /* Only copy API items */
        if (!domain_domain_is_api(child)) continue;

        /* Clone the API item (shallow - no deep children for simplicity) */
        Domain *clone = NULL;
        switch (child->type) {
            case DOMAIN_FUNCTION: {
                FunctionDomain *f = domain_function_domain_new(child->name,
                                                        ((FunctionDomain *)child)->return_type);
                f->mode = API_MODE_API;
                if (child->comment) domain_domain_set_comment((Domain *)f, child->comment);
                /* Copy parameters */
                for (int j = 0; j < child->child_count; j++) {
                    Domain *p = child->children[j];
                    if (p->type == DOMAIN_MEMBER) {
                        MemberDomain *mp = domain_member_domain_new(p->name,
                                                              ((MemberDomain *)p)->type);
                        if (p->comment) domain_domain_set_comment((Domain *)mp, p->comment);
                        domain_domain_add_child((Domain *)f, (Domain *)mp);
                    }
                }
                clone = (Domain *)f;
                break;
            }
            case DOMAIN_STRUCT: {
                StructDomain *s = domain_struct_domain_new(child->name);
                s->mode = API_MODE_API;
                if (child->comment) domain_domain_set_comment((Domain *)s, child->comment);
                /* Copy members */
                for (int j = 0; j < child->child_count; j++) {
                    Domain *m = child->children[j];
                    if (m->type == DOMAIN_MEMBER) {
                        MemberDomain *mm = domain_member_domain_new(m->name,
                                                              ((MemberDomain *)m)->type);
                        if (m->comment) domain_domain_set_comment((Domain *)mm, m->comment);
                        domain_domain_add_child((Domain *)s, (Domain *)mm);
                    }
                }
                clone = (Domain *)s;
                break;
            }
            case DOMAIN_TYPE: {
                TypeDomain *t = domain_type_domain_new(child->name);
                TypeDomain *src_t = (TypeDomain *)child;
                t->mode = src_t->mode;
                if (src_t->value) domain_domain_set_value((Domain *)t, src_t->value);
                if (child->comment) domain_domain_set_comment((Domain *)t, child->comment);
                /* Copy members for struct-mode types */
                for (int j = 0; j < child->child_count; j++) {
                    Domain *m = child->children[j];
                    if (m->type == DOMAIN_MEMBER) {
                        MemberDomain *mm = domain_member_domain_new(m->name,
                                                              ((MemberDomain *)m)->type);
                        if (m->comment) domain_domain_set_comment((Domain *)mm, m->comment);
                        domain_domain_add_child((Domain *)t, (Domain *)mm);
                    }
                }
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

        if (clone) {
            domain_domain_add_child(dst_mod, clone);
        }
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

int commands_cmd_im(const char *path) {
    if (!path) {
        printf("用法: im <模块名|.cboot 文件>\n");
        return -1;
    }

    /* im 只能在模块作用域中使用 */
    Domain *importer = g_proj->current;
    if (importer->type != DOMAIN_MODULE) {
        printf("错误: im 只能在模块作用域中使用\n");
        return -1;
    }

    /* 获取导入者路径 */
    char *importer_path = domain_domain_get_path(importer);

    /* 判断是项目内模块名还是 .cboot 文件 */
    int len = (int)strlen(path);
    int is_cboot_file = (len > 6 && strcmp(path + len - 6, ".cboot") == 0);

    if (!is_cboot_file) {
        /* 项目内兄弟模块导入：im <模块名>
         * 在当前模块的祖先链中查找同名兄弟模块，复制其 API 项 */
        Domain *parent = importer->parent;
        Domain *src_mod = NULL;

        /* 向上遍历祖先，查找同名子模块 */
        while (parent && !src_mod) {
            for (int i = 0; i < parent->child_count; i++) {
                Domain *child = parent->children[i];
                if (child->type == DOMAIN_MODULE &&
                    child != importer &&
                    child->name && strcmp(child->name, path) == 0) {
                    src_mod = child;
                    break;
                }
            }
            parent = parent->parent;
        }

        if (!src_mod) {
            printf("错误: 未找到项目内模块 '%s'\n", path);
            free(importer_path);
            return -1;
        }

        /* 检查是否已导入 */
        if (domain_domain_find_child(importer, path)) {
            printf("提示: 模块 '%s' 已导入，跳过\n", path);
            free(importer_path);
            return 0;
        }

        /* 在当前模块下创建外部引用模块 */
        ModuleDomain *api_ref = domain_module_domain_new(path);
        api_ref->mode = MOD_MODE_EXTERNAL;
        char cmt_buf[MAX_PATH_LEN];
        snprintf(cmt_buf, sizeof(cmt_buf), "[API 引用] 从项目内模块 %s 导入", path);
        domain_domain_set_comment((Domain *)api_ref, cmt_buf);

        /* 复制 API 项 */
        commands_copy_api_items_recursive(src_mod, (Domain *)api_ref);

        /* 添加到当前模块 */
        domain_domain_add_child(importer, (Domain *)api_ref);

        /* 记录依赖链 */
        char source_path_buf[MAX_PATH_LEN];
        char *src_path = domain_domain_get_path(src_mod);
        snprintf(source_path_buf, sizeof(source_path_buf), "%s", src_path);
        domain_project_add_dependency(g_proj, importer_path, source_path_buf, path);
        free(src_path);

        printf("已从项目内模块 '%s' 导入 API 定义到 %s\n", path, importer_path);
        free(importer_path);
        return 0;
    }

    /* 外部 .cboot 文件导入（原有逻辑） */
    if (!utils_file_exists(path)) {
        printf("错误: 文件 '%s' 不存在\n", path);
        free(importer_path);
        return -1;
    }

    /* 解析 .cboot 文件到一个临时 Project */
    Project *tmp_proj = domain_project_new("_im_tmp");
    if (!tmp_proj) {
        free(importer_path);
        printf("错误: 无法创建临时项目\n");
        return -1;
    }

    /* 保存当前项目状态，切换到临时项目解析 */
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

    /* 获取源模块路径（使用项目名作为标识） */
    char source_path_buf[MAX_PATH_LEN];
    snprintf(source_path_buf, sizeof(source_path_buf), "/%s", tmp_proj->root->name);
    char *source_path = utils_str_dup(source_path_buf);
    /* 获取源项目名 */
    const char *src_name = tmp_proj->root->name;

    /* 检查依赖是否已存在 */
    if (domain_project_has_dependency(g_proj, importer_path, source_path)) {
        printf("提示: 依赖 '%s' -> '%s' 已存在，跳过\n", importer_path, source_path);
        g_proj = saved_proj;
        domain_project_free(tmp_proj);
        free(importer_path);
        free(source_path);
        return 0;
    }

    /* 在当前模块下创建一个子模块作为 API 引用占位 */
    /* 模块名使用源项目名 */
    if (domain_domain_find_child(importer, src_name)) {
        printf("错误: 当前模块下已存在同名子域 '%s'\n", src_name);
        domain_project_free(tmp_proj);
        free(importer_path);
        free(source_path);
        return -1;
    }

    ModuleDomain *api_ref = domain_module_domain_new(src_name);
    api_ref->mode = MOD_MODE_EXTERNAL;  /* 标记为外部模块 */
    char cmt_buf[MAX_PATH_LEN];
    snprintf(cmt_buf, sizeof(cmt_buf), "[API 引用] 从 %s 导入", path);
    domain_domain_set_comment((Domain *)api_ref, cmt_buf);

    /* 从 tmp_proj 复制 API 项到 api_ref */
    commands_copy_api_items_recursive(tmp_proj->root, (Domain *)api_ref);

    /* 添加到当前模块 */
    domain_domain_add_child(importer, (Domain *)api_ref);

    /* 记录依赖链 */
    domain_project_add_dependency(g_proj, importer_path, source_path, path);

    printf("已从 %s 导入 API 定义到 %s（作为外部引用模块 %s）\n",
           path, importer_path, src_name);
    printf("依赖链: %s -> %s\n", importer_path, source_path);

    domain_project_free(tmp_proj);
    free(importer_path);
    free(source_path);
    return 0;
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

    /* Check if source file exists */
    if (!utils_file_exists(file_path)) {
        printf("警告: 资源文件 '%s' 不存在\n", file_path);
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











