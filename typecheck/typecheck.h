/*
 * CBoot - Type Checker v0.3.1
 *
 * Validates C types against built-in types and user-defined types
 * in the domain tree. Supports typedef resolution and value validation
 * for built-in types.
 */

#ifndef TYPECHECK_H
#define TYPECHECK_H

#include "domain/domain.h"

/* ------------------------------------------------------------------ */
/* TypeChecker                                                         */
/* ------------------------------------------------------------------ */

typedef struct TypeChecker {
    Domain *scope;
    char   *error_buf;
    int     error_len;
} TypeChecker;

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

void        typecheck_type_checker_init(TypeChecker *tc, Domain *scope);
int         typecheck_type_checker_validate(TypeChecker *tc, const char *type_name);
int         typecheck_type_checker_is_builtin(const char *type_name);
const char *type_checker_resolve_typedef(TypeChecker *tc, const char *type_name);
int         typecheck_type_checker_validate_value(const char *type_name, const char *value);

#endif /* TYPECHECK_H */