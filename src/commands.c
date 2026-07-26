/*
 * CBoot - C Project Bootstrapping Tool v2.0
 * Command handlers (新规范 v2.0)
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
static ModuleDomain *get_current_module(void) {
    Domain *d = g_proj->current;
    Domain *mod = domain_find_nearest_of_type(d, DOMAIN_MODULE);
    return (ModuleDomain *)mod;
}

/* Check if current domain is of a specific type */
static int is_in_domain_type(DomainType type) {
    return g_proj->current && g_proj->current->type == type;
}

/* Check type validity using type checker */
static int check_type(const char *type_name) {
    TypeChecker tc;
    type_checker_init(&tc, g_proj->current);
    if (type_checker_validate(&tc, type_name) != 0) {
        printf("错误: 类型 '%s' 未定义\n", type_name);
        return -1;
    }
    return 0;
}

/* Check name is valid identifier and not duplicate */
static int check_name_dup(const char *name, Domain *scope) {
    if (!is_valid_identifier(name)) {
        printf("错误: 无效名称 '%s'（仅允许字母、数字、下划线）\n", name);
        return -1;
    }
    if (domain_find_child(scope, name)) {
        printf("错误: 名称 '%s' 已存在\n", name);
        return -1;
    }
    return 0;
}

/* Print domain type name in Chinese */
static const char *domain_type_name(DomainType type) {
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

/* Get mode string for display */
static const char *mode_str(Domain *d) {
    if (!d) return "";
    switch (d->type) {
        case DOMAIN_MODULE: {
            ModuleDomain *mod = (ModuleDomain *)d;
            return mod->mode == MOD_MODE_INTERNAL ? "internal" : "external";
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

/* cmd_mod - 创建模块 */
int cmd_mod(const char *name) {
    if (!name) {
        printf("用法: mod <名称>\n");
        return -1;
    }

    if (check_name_dup(name, g_proj->current) != 0)
        return -1;

    ModuleDomain *mod = module_domain_new(name);
    if (!mod) {
        printf("错误: 无法创建模块\n");
        return -1;
    }

    domain_add_child(g_proj->current, (Domain *)mod);
    g_proj->current = (Domain *)mod;  /* auto-cd */

    printf("模块 %s 已创建，已进入。\n", name);
    return 0;
}

/* cmd_struct - 创建结构体 */
int cmd_struct(const char *name) {
    if (!name) {
        printf("用法: struct <名称>\n");
        return -1;
    }

    ModuleDomain *mod = get_current_module();
    if (!mod) {
        printf("错误: 当前不在模块作用域中\n");
        return -1;
    }

    if (check_name_dup(name, (Domain *)mod) != 0)
        return -1;

    StructDomain *s = struct_domain_new(name);
    if (!s) {
        printf("错误: 无法创建结构体\n");
        return -1;
    }

    domain_add_child((Domain *)mod, (Domain *)s);
    g_proj->current = (Domain *)s;  /* auto-cd */

    printf("结构体 %s 已创建，已进入。\n", name);
    return 0;
}

/* cmd_type - 创建类型 (typedef) */
int cmd_type(const char *name) {
    if (!name) {
        printf("用法: type <名称>\n");
        return -1;
    }

    ModuleDomain *mod = get_current_module();
    if (!mod) {
        printf("错误: 当前不在模块作用域中\n");
        return -1;
    }

    if (check_name_dup(name, (Domain *)mod) != 0)
        return -1;

    TypeDomain *t = type_domain_new(name);
    if (!t) {
        printf("错误: 无法创建类型\n");
        return -1;
    }

    domain_add_child((Domain *)mod, (Domain *)t);
    g_proj->current = (Domain *)t;  /* auto-cd */

    printf("类型 %s 已创建，已进入。\n", name);
    return 0;
}

/* cmd_def - 创建宏 */
int cmd_def(const char *name) {
    if (!name) {
        printf("用法: def <名称>\n");
        return -1;
    }

    ModuleDomain *mod = get_current_module();
    if (!mod) {
        printf("错误: 当前不在模块作用域中\n");
        return -1;
    }

    if (check_name_dup(name, (Domain *)mod) != 0)
        return -1;

    MacroDomain *m = macro_domain_new(name);
    if (!m) {
        printf("错误: 无法创建宏\n");
        return -1;
    }

    domain_add_child((Domain *)mod, (Domain *)m);
    g_proj->current = (Domain *)m;  /* auto-cd */
    printf("宏 %s 已创建。\n", name);
    return 0;
}

/* ================================================================== */
/* 带有type的建立域: <op> <name> <type>                                 */
/* ================================================================== */

/* cmd_void - 创建函数 (void <name> <return_type>) */
int cmd_void(const char *name, const char *return_type) {
    if (!name || !return_type) {
        printf("用法: void <名称> <返回类型>\n");
        return -1;
    }

    ModuleDomain *mod = get_current_module();
    if (!mod) {
        printf("错误: 当前不在模块作用域中\n");
        return -1;
    }

    if (check_name_dup(name, (Domain *)mod) != 0)
        return -1;

    if (check_type(return_type) != 0)
        return -1;

    FunctionDomain *func = function_domain_new(name, return_type);
    if (!func) {
        printf("错误: 无法创建函数\n");
        return -1;
    }

    domain_add_child((Domain *)mod, (Domain *)func);
    g_proj->current = (Domain *)func;  /* auto-cd */

    printf("函数 %s %s() 已创建，已进入。\n", return_type, name);
    return 0;
}

/* cmd_var - 创建变量 (var <name> <type>) */
int cmd_var(const char *name, const char *type) {
    if (!name || !type) {
        printf("用法: var <名称> <类型>\n");
        return -1;
    }

    /* Variable must be inside a function */
    if (!is_in_domain_type(DOMAIN_FUNCTION)) {
        printf("错误: var 命令仅在函数作用域中可用\n");
        return -1;
    }

    if (check_name_dup(name, g_proj->current) != 0)
        return -1;

    if (check_type(type) != 0)
        return -1;

    VariableDomain *v = variable_domain_new(name, type);
    if (!v) {
        printf("错误: 无法创建变量\n");
        return -1;
    }

    domain_add_child(g_proj->current, (Domain *)v);
    g_proj->current = (Domain *)v;  /* auto-cd */
    printf("变量 %s %s 已创建。\n", type, name);
    return 0;
}

/* cmd_mem - 创建成员/参数 (mem <name> <type>, 上下文敏感) */
int cmd_mem(const char *name, const char *type) {
    if (!name || !type) {
        printf("用法: mem <名称> <类型>\n");
        return -1;
    }

    DomainType cur_type = g_proj->current->type;

    /* Context-sensitive: function→参数, struct/type→成员 */
    if (cur_type == DOMAIN_FUNCTION) {
        /* Adding a parameter */
        if (check_name_dup(name, g_proj->current) != 0)
            return -1;

        if (check_type(type) != 0)
            return -1;

        MemberDomain *m = member_domain_new(name, type);
        if (!m) {
            printf("错误: 无法创建参数\n");
            return -1;
        }

        domain_add_child(g_proj->current, (Domain *)m);
        g_proj->current = (Domain *)m;  /* auto-cd */
        printf("参数 %s %s 已添加。\n", type, name);
        return 0;
    }
    else if (cur_type == DOMAIN_STRUCT) {
        /* Adding a struct member */
        if (check_name_dup(name, g_proj->current) != 0)
            return -1;

        if (check_type(type) != 0)
            return -1;

        MemberDomain *m = member_domain_new(name, type);
        if (!m) {
            printf("错误: 无法创建成员\n");
            return -1;
        }

        domain_add_child(g_proj->current, (Domain *)m);
        g_proj->current = (Domain *)m;  /* auto-cd */
        printf("成员 %s %s 已添加。\n", type, name);
        return 0;
    }
    else if (cur_type == DOMAIN_TYPE) {
        TypeDomain *td = (TypeDomain *)g_proj->current;
        if (td->mode == TYPE_MODE_STRUCT || td->mode == TYPE_MODE_API_STRUCT) {
            /* Adding a member to struct-mode type */
            if (check_name_dup(name, g_proj->current) != 0)
                return -1;

            if (check_type(type) != 0)
                return -1;

            MemberDomain *m = member_domain_new(name, type);
            if (!m) {
                printf("错误: 无法创建成员\n");
                return -1;
            }

            domain_add_child(g_proj->current, (Domain *)m);
            g_proj->current = (Domain *)m;  /* auto-cd */
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

int cmd_enum(const char *defs, const char *start_num_str) {
    if (!defs || !start_num_str) {
        printf("用法: enum <def1>,<def2>,... <start_num>\n");
        return -1;
    }

    ModuleDomain *mod = get_current_module();
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
        if (!is_valid_identifier(token)) {
            printf("错误: 无效名称 '%s'\n", token);
            return -1;
        }

        /* Check duplicate */
        if (domain_find_child((Domain *)mod, token)) {
            printf("错误: 名称 '%s' 已存在\n", token);
            return -1;
        }

        /* Create macro with value = start_num + count */
        MacroDomain *m = macro_domain_new(token);
        if (!m) {
            printf("错误: 无法创建宏 %s\n", token);
            return -1;
        }

        char val_str[32];
        snprintf(val_str, sizeof(val_str), "%d", start_num + count);
        domain_set_value((Domain *)m, val_str);

        domain_add_child((Domain *)mod, (Domain *)m);
        count++;
        token = strtok_r(NULL, ",", &saveptr);
    }

    printf("已创建 %d 个枚举宏 (值 %d-%d)。\n", count, start_num, start_num + count - 1);
    return 0;
}

/* ================================================================== */
/* 修改字段: <op> <value>                                              */
/* ================================================================== */

/* cmd_cmt - 设置注释 */
int cmd_cmt(const char *text) {
    if (!text) {
        printf("用法: cmt \"注释文本\"\n");
        return -1;
    }

    domain_set_comment(g_proj->current, text);
    printf("注释已设置。\n");
    return 0;
}

/* cmd_value - 设置值 */
int cmd_value(const char *text) {
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
                if (check_type(text) != 0)
                    return -1;
            }
            break;
        }
        case DOMAIN_VARIABLE: {
            VariableDomain *v = (VariableDomain *)cur;
            if (type_checker_validate_value(v->type, text) != 0) {
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

    domain_set_value(g_proj->current, text);
    printf("value 已设置。\n");
    return 0;
}

/* cmd_mode - 设置模式 */
int cmd_mode(const char *text) {
    if (!text) {
        printf("用法: mode <模式>\n");
        return -1;
    }

    Domain *cur = g_proj->current;

    switch (cur->type) {
        case DOMAIN_MODULE: {
            if (str_eq(text, "internal")) {
                domain_set_mode(cur, MOD_MODE_INTERNAL);
            } else if (str_eq(text, "external")) {
                domain_set_mode(cur, MOD_MODE_EXTERNAL);
            } else {
                printf("错误: 模块模式只能是 internal 或 external\n");
                return -1;
            }
            break;
        }
        case DOMAIN_FUNCTION:
        case DOMAIN_STRUCT: {
            if (str_eq(text, "api")) {
                domain_set_mode(cur, API_MODE_API);
            } else if (str_eq(text, "normal")) {
                domain_set_mode(cur, API_MODE_NORMAL);
            } else {
                printf("错误: API模式只能是 api 或 normal\n");
                return -1;
            }
            break;
        }
        case DOMAIN_TYPE: {
            if (str_eq(text, "rename")) {
                domain_set_mode(cur, TYPE_MODE_RENAME);
            } else if (str_eq(text, "struct")) {
                domain_set_mode(cur, TYPE_MODE_STRUCT);
            } else if (str_eq(text, "api rename")) {
                domain_set_mode(cur, TYPE_MODE_API_RENAME);
            } else if (str_eq(text, "api struct")) {
                domain_set_mode(cur, TYPE_MODE_API_STRUCT);
            } else {
                printf("错误: 类型模式只能是 rename/struct/api rename/api struct\n");
                return -1;
            }
            break;
        }
        case DOMAIN_MACRO: {
            if (str_eq(text, "api")) {
                domain_set_mode(cur, API_MODE_API);
            } else if (str_eq(text, "normal")) {
                domain_set_mode(cur, API_MODE_NORMAL);
            } else {
                printf("错误: 宏模式只能是 api 或 normal\n");
                return -1;
            }
            break;
        }
        case DOMAIN_VARIABLE: {
            if (str_eq(text, "static")) {
                domain_set_mode(cur, VAR_MODE_STATIC);
            } else if (str_eq(text, "normal")) {
                domain_set_mode(cur, VAR_MODE_NORMAL);
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

/* ================================================================== */
/* 控制: cd / rm                                                        */
/* ================================================================== */

/* cmd_cd - 导航域树 */
int cmd_cd(const char *path) {
    if (!path || path[0] == '\0') {
        /* cd without args: show current path */
        char *p = domain_get_path(g_proj->current);
        printf("当前路径: %s\n", p);
        free(p);
        return 0;
    }

    /* ".." -> parent */
    if (str_eq(path, "..")) {
        if (g_proj->current->parent) {
            g_proj->current = g_proj->current->parent;
            printf("已进入: %s\n", g_proj->current->name);
        } else {
            printf("已在根域。\n");
        }
        return 0;
    }

    /* "/" -> root */
    if (str_eq(path, "/")) {
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

        Domain *child = domain_find_child(cur, segment);
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

/* cmd_rm - 删除子域 */
int cmd_rm(const char *name, int force) {
    if (!name) {
        printf("用法: rm <名称> [-f]\n");
        return -1;
    }

    Domain *child = domain_find_child(g_proj->current, name);
    if (!child) {
        printf("未找到: %s\n", name);
        return -1;
    }

    /* If force, just delete */
    if (force) {
        domain_remove_child(g_proj->current, child);
        domain_delete(child);
        printf("已删除 %s '%s'。\n", domain_type_name(child->type), name);
        return 0;
    }

    /* Confirm deletion */
    printf("确认删除 %s '%s'? (y/N): ", domain_type_name(child->type), name);
    fflush(stdout);

    char line[MAX_LINE_LEN];
    if (fgets(line, sizeof(line), stdin) == NULL) {
        printf("已取消。\n");
        return 0;
    }

    if (line[0] == 'y' || line[0] == 'Y') {
        domain_remove_child(g_proj->current, child);
        domain_delete(child);
        printf("已删除 %s '%s'。\n", domain_type_name(child->type), name);
    } else {
        printf("已取消。\n");
    }

    return 0;
}

/* ================================================================== */
/* 查找: find <type> <pattern> [-a|-an]                                 */
/* ================================================================== */

/* Recursive find helper */
static void find_recursive(Domain *root, DomainType type_filter,
                           const char *pattern, int max_depth, int cur_depth,
                           int *found_count) {
    if (!root || (max_depth >= 0 && cur_depth > max_depth)) return;

    /* Check current node */
    if (root->type == type_filter || (int)type_filter == -1) {
        if (!pattern || pattern[0] == '\0' || strstr(root->name, pattern)) {
            char *path = domain_get_path(root);
            printf("[%s] %s (%s)", domain_type_name(root->type), root->name, path);
            if (root->comment) printf(" - %s", root->comment);
            printf("\n");
            free(path);
            (*found_count)++;
        }
    }

    /* Recurse into children */
    for (int i = 0; i < root->child_count; i++) {
        find_recursive(root->children[i], type_filter, pattern,
                       max_depth, cur_depth + 1, found_count);
    }
}

int cmd_find(const char *type_filter, const char *pattern, int flags) {
    if (!type_filter) {
        printf("用法: find <类型:mod,void,struct,type,def,var,mem,all> <字符> [-a|-an]\n");
        return -1;
    }

    DomainType filter_type = -1;  /* -1 = all */

    if (str_eq(type_filter, "mod"))     filter_type = DOMAIN_MODULE;
    else if (str_eq(type_filter, "void"))    filter_type = DOMAIN_FUNCTION;
    else if (str_eq(type_filter, "struct"))  filter_type = DOMAIN_STRUCT;
    else if (str_eq(type_filter, "type"))    filter_type = DOMAIN_TYPE;
    else if (str_eq(type_filter, "def"))     filter_type = DOMAIN_MACRO;
    else if (str_eq(type_filter, "var"))     filter_type = DOMAIN_VARIABLE;
    else if (str_eq(type_filter, "mem"))     filter_type = DOMAIN_MEMBER;
    else if (str_eq(type_filter, "all"))     filter_type = -1;

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
    find_recursive(g_proj->current, filter_type, pattern, max_depth, 0, &found_count);

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

int cmd_ls(const char *name) {
    Domain *target = g_proj->current;

    if (name) {
        /* ls <name>: show specific child */
        Domain *child = domain_find_child(g_proj->current, name);
        if (!child) {
            printf("未找到: %s\n", name);
            return -1;
        }
        target = child;
    }

    char *path = domain_get_path(target);
    printf("域: %s", path);
    free(path);

    /* Show type and mode */
    printf(" [%s]", domain_type_name(target->type));
    const char *ms = mode_str(target);
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
            printf("    [%s] %s", domain_type_name(child->type), child->name);

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

int cmd_mv(const char *src, const char *target) {
    if (!src || !target) {
        printf("用法: mv <src> <target>\n");
        return -1;
    }

    Domain *src_domain = domain_find_child(g_proj->current, src);
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
            if (cmd_cd(parent_path) != 0) {
                g_proj->current = saved;
                return -1;
            }
            target_parent = g_proj->current;
            g_proj->current = saved;
            target_name = slash + 1;
        }
    }

    /* Check target doesn't exist */
    if (domain_find_child(target_parent, target_name)) {
        printf("错误: 目标 '%s' 已存在\n", target_name);
        return -1;
    }

    /* Remove from source */
    domain_remove_child(g_proj->current, src_domain);

    /* Rename if needed */
    if (!str_eq(src_domain->name, target_name)) {
        free(src_domain->name);
        src_domain->name = str_dup(target_name);
    }

    /* Add to target */
    domain_add_child(target_parent, src_domain);

    printf("已将 %s 移动到 %s。\n", src, target);
    return 0;
}

/* ================================================================== */
/* 退出: exit                                                           */
/* ================================================================== */

int cmd_exit(void) {
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

int cmd_gen(void) {
    printf("正在生成代码...\n");
    if (generate_project(g_proj) != 0) {
        printf("代码生成失败。\n");
        return -1;
    }
    generate_docs(g_proj, "docs");
    g_proj->has_generated = 1;
    printf("代码生成完成。\n");
    return 0;
}

/* ================================================================== */
/* 导入: im <.cboot file>                                               */
/* ================================================================== */

int cmd_im(const char *path) {
    if (!path) {
        printf("用法: im <.cboot 文件>\n");
        return -1;
    }

    if (!file_exists(path)) {
        printf("错误: 文件 '%s' 不存在\n", path);
        return -1;
    }

    printf("正在导入: %s\n", path);
    if (parse_cboot_script(path) != 0) {
        printf("导入失败。\n");
        return -1;
    }

    printf("导入完成。\n");
    return 0;
}

/* ================================================================== */
/* 资源: res <file>                                                     */
/* ================================================================== */

int cmd_res(const char *file_path) {
    if (!file_path) {
        printf("用法: res <资源文件路径>\n");
        return -1;
    }

    ModuleDomain *mod = get_current_module();
    if (!mod) {
        printf("错误: 当前不在模块作用域中\n");
        return -1;
    }

    /* Check if source file exists */
    if (!file_exists(file_path)) {
        printf("警告: 资源文件 '%s' 不存在\n", file_path);
    }

    /* Get basename */
    const char *basename = strrchr(file_path, '/');
    if (basename) basename++;
    else basename = file_path;

    /* Copy resource file to res/ directory */
    ensure_dir("res");

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