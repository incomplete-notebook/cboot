/*
 * CBoot - Type Checker Implementation v2.0
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

static char *strip_pointer(char *buf, int len)
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
/* type_checker_is_builtin                                              */
/* ================================================================== */

int type_checker_is_builtin(const char *type_name)
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
/* is_integer_type - check if a type name is an integer built-in        */
/* ================================================================== */

static int is_integer_type(const char *type_name)
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
/* is_float_type - check if a type name is a float built-in             */
/* ================================================================== */

static int is_float_type(const char *type_name)
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
/* is_accept_any_value - types that accept any value                    */
/* ================================================================== */

static int is_accept_any_value(const char *type_name)
{
    if (!type_name) return 0;

    return (strcmp(type_name, "void") == 0 ||
            strcmp(type_name, "FILE") == 0 ||
            strcmp(type_name, "bool") == 0);
}

/* ================================================================== */
/* type_checker_init                                                    */
/* ================================================================== */

void type_checker_init(TypeChecker *tc, Domain *scope)
{
    if (!tc) return;

    tc->scope     = scope;
    tc->error_buf = NULL;
    tc->error_len = 0;
}

/* ================================================================== */
/* type_checker_validate                                                */
/* ================================================================== */

int type_checker_validate(TypeChecker *tc, const char *type_name)
{
    if (!tc || !type_name) return -1;

    /* Copy and strip trailing * and whitespace to get base type */
    char buf[256];
    int len = (int)strlen(type_name);
    if (len >= 256) len = 255;
    memcpy(buf, type_name, len);
    buf[len] = '\0';

    strip_pointer(buf, len);

    /* Check built-in types first */
    if (type_checker_is_builtin(buf)) {
        return 0;
    }

    /* Walk up the scope tree looking for user-defined types */
    Domain *scope = tc->scope;
    while (scope != NULL) {
        for (int i = 0; i < scope->child_count; i++) {
            Domain *child = scope->children[i];
            if (!child) continue;

            if ((child->type == DOMAIN_STRUCT || child->type == DOMAIN_TYPE) &&
                child->name && strcmp(child->name, buf) == 0) {
                return 0;
            }
        }
        scope = scope->parent;
    }

    return -1;
}

/* ================================================================== */
/* type_checker_resolve_typedef                                         */
/* ================================================================== */

const char *type_checker_resolve_typedef(TypeChecker *tc, const char *type_name)
{
    if (!tc || !type_name) return type_name;

    /* Walk up the scope tree looking for DOMAIN_TYPE with matching name */
    Domain *scope = tc->scope;
    while (scope != NULL) {
        for (int i = 0; i < scope->child_count; i++) {
            Domain *child = scope->children[i];
            if (!child) continue;

            if (child->type == DOMAIN_TYPE &&
                child->name && strcmp(child->name, type_name) == 0) {
                /* Found a typedef - check if it's a rename */
                TypeDomain *td = (TypeDomain *)child;
                if (td->mode == TYPE_MODE_RENAME ||
                    td->mode == TYPE_MODE_API_RENAME) {
                    /* Return the underlying type */
                    if (td->value) {
                        return td->value;
                    }
                }
                /* For struct modes, return the type_name itself */
                return type_name;
            }
        }
        scope = scope->parent;
    }

    /* Not found - return the type_name itself */
    return type_name;
}

/* ================================================================== */
/* type_checker_validate_value                                          */
/* ================================================================== */

int type_checker_validate_value(const char *type_name, const char *value)
{
    if (!type_name || !value) return -1;

    /* Copy and strip to base type */
    char buf[256];
    int len = (int)strlen(type_name);
    if (len >= 256) len = 255;
    memcpy(buf, type_name, len);
    buf[len] = '\0';
    strip_pointer(buf, len);

    /* Types that accept any value */
    if (is_accept_any_value(buf)) {
        return 0;
    }

    /* Integer types - check for valid number format */
    if (is_integer_type(buf)) {
        const char *p = value;

        /* Skip leading whitespace */
        while (isspace((unsigned char)*p)) p++;

        /* Handle hex: 0x or 0X prefix */
        if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
            p += 2;
            if (!isxdigit((unsigned char)*p)) {
                return -1; /* Must have at least one hex digit */
            }
            while (isxdigit((unsigned char)*p)) p++;
            /* Skip trailing whitespace */
            while (isspace((unsigned char)*p)) p++;
            if (*p != '\0') return -1; /* Extra characters */
            return 0;
        }

        /* Handle optional sign */
        if (*p == '+' || *p == '-') p++;

        /* Must have at least one digit */
        if (!isdigit((unsigned char)*p)) {
            return -1;
        }
        while (isdigit((unsigned char)*p)) p++;

        /* Skip trailing whitespace */
        while (isspace((unsigned char)*p)) p++;

        if (*p != '\0') return -1; /* Extra characters */
        return 0;
    }

    /* Float types - check for valid float format */
    if (is_float_type(buf)) {
        const char *p = value;

        /* Skip leading whitespace */
        while (isspace((unsigned char)*p)) p++;

        /* Handle optional sign */
        if (*p == '+' || *p == '-') p++;

        /* Must have digits before or after decimal point */
        int has_digits = 0;
        while (isdigit((unsigned char)*p)) {
            has_digits = 1;
            p++;
        }

        if (*p == '.') {
            p++;
            while (isdigit((unsigned char)*p)) {
                has_digits = 1;
                p++;
            }
        }

        if (!has_digits) return -1;

        /* Optional exponent */
        if (*p == 'e' || *p == 'E') {
            p++;
            if (*p == '+' || *p == '-') p++;
            if (!isdigit((unsigned char)*p)) return -1;
            while (isdigit((unsigned char)*p)) p++;
        }

        /* Optional f/F suffix */
        if (*p == 'f' || *p == 'F') p++;

        /* Skip trailing whitespace */
        while (isspace((unsigned char)*p)) p++;

        if (*p != '\0') return -1;
        return 0;
    }

    /* For user-defined types (structs, typedefs, etc.), accept any value */
    return 0;
}