/* typecheck.c - CBoot generated (compiler: normal) */
/* Module: typecheck */

/*
 * CBoot - Type Checker Implementation v0.5.0
 *
 * Validates C types against built-in types and user-defined types
 * in the domain tree. Supports typedef resolution and value validation
 * for built-in types.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "typecheck.h"

/* ================================================================== */
/* Built-in types table                                                 */
/* ================================================================== */

static const char *builtin_types[] = {
    "void",
    "char",
    "short",
    "int",
    "long",
    "float",
    "double",
    "signed",
    "unsigned",
    "size_t",
    "ssize_t",
    "int8_t",
    "int16_t",
    "int32_t",
    "int64_t",
    "uint8_t",
    "uint16_t",
    "uint32_t",
    "uint64_t",
    "bool",
    "FILE",
    /* Compound types */
    "unsigned char",
    "unsigned short",
    "unsigned int",
    "unsigned long",
    "signed char",
    "signed int",
    "long int",
    "long long",
    "short int",
    "long double",
    "long unsigned int",
    NULL
};

/* ================================================================== */
/* Integer built-in type names (for value validation)                    */
/* ================================================================== */

static const char *integer_types[] = {
    "char", "short", "int", "long", "signed", "unsigned",
    "size_t", "ssize_t",
    "int8_t", "int16_t", "int32_t", "int64_t",
    "uint8_t", "uint16_t", "uint32_t", "uint64_t",
    NULL
};

/* ================================================================== */
/* Floating-point built-in type names (for value validation)            */
/* ================================================================== */

static const char *float_types[] = {
    "float", "double",
    NULL
};

/* ================================================================== */
/* Strip trailing pointer asterisks and whitespace from a type name     */
/* Returns a pointer into the provided buffer (which is modified).      */
/* ================================================================== */

static char *typecheck_strip_pointer(char *buf, int len)
{
    /* Trim trailing whitespace */
    while (len > 0 && isspace((unsigned char)buf[len - 1])) {
        buf[--len] = '\0';
    }

    /* Strip trailing asterisks */
    while (len > 0 && buf[len - 1] == '*') {
        buf[--len] = '\0';
        /* Also strip whitespace between * and the base type */
        while (len > 0 && isspace((unsigned char)buf[len - 1])) {
            buf[--len] = '\0';
        }
    }

    return buf;
}

/* ================================================================== */
/* Strip leading type qualifiers (struct, const, volatile, etc.)       */
/* Returns a pointer into the provided buffer (modified).              */
/* ================================================================== */

/* 检查 buf 是否以某个 qualifier 开头（后跟空格/星号/末尾）；
 * 匹配成功返回 qualifier 长度，否则返回 0 */
static size_t typecheck_match_qualifier(const char *buf) {
    static const char *qualifiers[] = {
        "struct", "const", "volatile", "restrict",
        "unsigned", "signed", "long", "short",
        NULL
    };
    for (int i = 0; qualifiers[i] != NULL; i++) {
        size_t qlen = strlen(qualifiers[i]);
        if (strncmp(buf, qualifiers[i], qlen) != 0) continue;
        char c = buf[qlen];
        if (c == ' ' || c == '\0' || c == '*') return qlen;
    }
    return 0;
}

static char *typecheck_strip_qualifiers(char *buf)
{
    /* Skip leading whitespace */
    while (*buf && isspace((unsigned char)*buf)) buf++;

    /* 反复剥离前导 qualifier */
    size_t qlen;
    while ((qlen = typecheck_match_qualifier(buf)) > 0) {
        buf += qlen;
        while (*buf && isspace((unsigned char)*buf)) buf++;
    }

    return buf;
}

/* ================================================================== */
/* typecheck_type_checker_is_builtin                                              */
/* ================================================================== */

int typecheck_type_checker_is_builtin(const char *type_name)
{
    if (!type_name) return 0;

    for (int i = 0; builtin_types[i] != NULL; i++) {
        if (strcmp(type_name, builtin_types[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

/* ================================================================== */
/* typecheck_is_integer_type - check if a type name is an integer built-in        */
/* ================================================================== */

static int typecheck_is_integer_type(const char *type_name)
{
    if (!type_name) return 0;

    for (int i = 0; integer_types[i] != NULL; i++) {
        if (strcmp(type_name, integer_types[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

/* ================================================================== */
/* typecheck_is_float_type - check if a type name is a float built-in             */
/* ================================================================== */

static int typecheck_is_float_type(const char *type_name)
{
    if (!type_name) return 0;

    for (int i = 0; float_types[i] != NULL; i++) {
        if (strcmp(type_name, float_types[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

/* ================================================================== */
/* typecheck_is_accept_any_value - types that accept any value                    */
/* ================================================================== */

static int typecheck_is_accept_any_value(const char *type_name)
{
    if (!type_name) return 0;

    return (strcmp(type_name, "void") == 0 ||
            strcmp(type_name, "FILE") == 0 ||
            strcmp(type_name, "bool") == 0);
}

/* ================================================================== */
/* typecheck_type_checker_init                                                    */
/* ================================================================== */

void typecheck_type_checker_init(TypeChecker *tc, Domain *scope)
{
    if (!tc) return;

    tc->scope     = scope;
    tc->error_buf = NULL;
    tc->error_len = 0;
}

/* ================================================================== */
/* typecheck_type_checker_find_api_type_in_submodules - find API-mode type/struct items
 * in direct child modules only (not recursive)
 * API 只提升一级，只看直接子模块
 * ================================================================== */

static Domain *typecheck_type_checker_find_api_type_in_submodules(Domain *scope, const char *name)
{
    if (!scope || !name) return NULL;

    for (int i = 0; i < scope->child_count; i++) {
        Domain *child = scope->children[i];
        if (!child || child->type != DOMAIN_MODULE) continue;

        /* 在子模块的直接子节点中查找 API 类型/结构体 */
        for (int j = 0; j < child->child_count; j++) {
            Domain *g = child->children[j];
            if (!g) continue;
            if (g->type != DOMAIN_STRUCT && g->type != DOMAIN_TYPE) continue;
            if (!g->name || strcmp(g->name, name) != 0) continue;
            if (domain_domain_is_api(g)) return g;
        }
    }

    return NULL;
}

/* ================================================================== */
/* typecheck_type_checker_validate                                                */
/* ================================================================== */

/* 剥离类型字符串中的数组维度后缀（[N] 或 [MACRO]）和指针星号 */
static void typecheck_strip_array_dims(char *buf, int *len_ptr) {
    int len = *len_ptr;
    while (len > 0 && buf[len - 1] == ']') {
        int bracket = len - 2;
        while (bracket >= 0 && buf[bracket] != '[') bracket--;
        if (bracket < 0) break;
        len = bracket;
        buf[len] = '\0';
        while (len > 0 && isspace((unsigned char)buf[len - 1])) {
            len--;
            buf[len] = '\0';
        }
    }
    typecheck_strip_pointer(buf, len);
    *len_ptr = len;
}

/* 在当前 scope 子节点中查找匹配的用户定义类型 */
static int typecheck_find_named_type_in_scope(Domain *scope, const char *base) {
    for (int i = 0; i < scope->child_count; i++) {
        Domain *child = scope->children[i];
        if (!child) continue;
        if ((child->type == DOMAIN_STRUCT || child->type == DOMAIN_TYPE) &&
            child->name && strcmp(child->name, base) == 0) {
            return 1;
        }
    }
    return 0;
}

int typecheck_type_checker_validate(TypeChecker *tc, const char *type_name)
{
    if (!tc || !type_name) return -1;

    char buf[256];
    int len = (int)strlen(type_name);
    if (len >= 256) len = 255;
    memcpy(buf, type_name, len);
    buf[len] = '\0';

    typecheck_strip_array_dims(buf, &len);

    /* Strip leading qualifiers (struct, const, etc.) */
    char *base = typecheck_strip_qualifiers(buf);

    if (typecheck_type_checker_is_builtin(base)) return 0;

    /* Find the starting module (nearest module ancestor of scope) */
    Domain *start_module = tc->scope;
    while (start_module && start_module->type != DOMAIN_MODULE) {
        start_module = start_module->parent;
    }

    /* Walk up the scope tree */
    Domain *scope = tc->scope;
    while (scope != NULL) {
        if (typecheck_find_named_type_in_scope(scope, base)) return 0;
        /* Only check submodule API types in the starting module (API 只提升一级) */
        if (scope == start_module && scope->type == DOMAIN_MODULE &&
            typecheck_type_checker_find_api_type_in_submodules(scope, base)) {
            return 0;
        }
        scope = scope->parent;
    }

    return -1;
}

/* ================================================================== */
/* type_checker_resolve_typedef                                         */
/* ================================================================== */

/* 从 TypeDomain 提取 typedef 的目标类型名；非 rename 类型返回 NULL */
static const char *typecheck_typedef_value(Domain *d) {
    if (d->type != DOMAIN_TYPE) return NULL;
    TypeDomain *td = (TypeDomain *)d;
    if (td->mode == TYPE_MODE_RENAME || td->mode == TYPE_MODE_API_RENAME) {
        return td->value;  /* 可能为 NULL（无 value 时保持原名） */
    }
    return NULL;  /* 非 rename 类型，调用方返回原 type_name */
}

/* 在 scope 子节点中查找指定名称的 TYPE 域 */
static Domain *typecheck_find_type_in_scope(Domain *scope, const char *name) {
    for (int i = 0; i < scope->child_count; i++) {
        Domain *child = scope->children[i];
        if (!child) continue;
        if (child->type == DOMAIN_TYPE && child->name &&
            strcmp(child->name, name) == 0) {
            return child;
        }
    }
    return NULL;
}

const char *type_checker_resolve_typedef(TypeChecker *tc, const char *type_name)
{
    if (!tc || !type_name) return type_name;

    Domain *scope = tc->scope;
    while (scope != NULL) {
        Domain *found = typecheck_find_type_in_scope(scope, type_name);
        if (found) {
            const char *val = typecheck_typedef_value(found);
            return val ? val : type_name;
        }

        if (scope->type == DOMAIN_MODULE) {
            Domain *sub = typecheck_type_checker_find_api_type_in_submodules(scope, type_name);
            if (sub) {
                const char *val = typecheck_typedef_value(sub);
                return val ? val : type_name;
            }
        }
        scope = scope->parent;
    }
    return type_name;
}

/* ================================================================== */
/* Value validation helpers                                           */
/* ================================================================== */

/* 跳过尾部空白并检查是否到达字符串末尾 */
static int typecheck_check_end(const char *p) {
    while (isspace((unsigned char)*p)) p++;
    return (*p == '\0') ? 0 : -1;
}

static int typecheck_validate_integer_value(const char *value)
{
    const char *p = value;
    while (isspace((unsigned char)*p)) p++;

    /* Hex: 0x or 0X prefix */
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
        if (!isxdigit((unsigned char)*p)) return -1;
        while (isxdigit((unsigned char)*p)) p++;
        return typecheck_check_end(p);
    }

    /* Optional sign */
    if (*p == '+' || *p == '-') p++;
    if (!isdigit((unsigned char)*p)) return -1;
    while (isdigit((unsigned char)*p)) p++;
    return typecheck_check_end(p);
}

/* 跳过连续数字字符，返回是否跳过了至少一个数字 */
static int typecheck_skip_digits(const char **pp) {
    const char *p = *pp;
    int has = 0;
    while (isdigit((unsigned char)*p)) { has = 1; p++; }
    *pp = p;
    return has;
}

/* 跳过指数部分 e[+/-]digits，返回是否跳过了指数 */
static int typecheck_skip_exponent(const char **pp) {
    const char *p = *pp;
    if (*p != 'e' && *p != 'E') return 0;
    p++;
    if (*p == '+' || *p == '-') p++;
    if (!isdigit((unsigned char)*p)) return -1;
    while (isdigit((unsigned char)*p)) p++;
    *pp = p;
    return 1;
}

static int typecheck_validate_float_value(const char *value)
{
    const char *p = value;
    while (isspace((unsigned char)*p)) p++;
    if (*p == '+' || *p == '-') p++;

    int has_digits = typecheck_skip_digits(&p);

    if (*p == '.') {
        p++;
        if (typecheck_skip_digits(&p)) has_digits = 1;
    }

    if (!has_digits) return -1;

    if (typecheck_skip_exponent(&p) < 0) return -1;

    if (*p == 'f' || *p == 'F') p++;
    return typecheck_check_end(p);
}

/* ================================================================== */
/* typecheck_type_checker_validate_value                                          */
/* ================================================================== */

int typecheck_type_checker_validate_value(const char *type_name, const char *value)
{
    if (!type_name || !value) return -1;

    char buf[256];
    int len = (int)strlen(type_name);
    if (len >= 256) len = 255;
    memcpy(buf, type_name, len);
    buf[len] = '\0';
    typecheck_strip_pointer(buf, len);

    if (typecheck_is_accept_any_value(buf)) return 0;
    if (typecheck_is_integer_type(buf)) return typecheck_validate_integer_value(value);
    if (typecheck_is_float_type(buf)) return typecheck_validate_float_value(value);

    /* User-defined types accept any value */
    return 0;
}
