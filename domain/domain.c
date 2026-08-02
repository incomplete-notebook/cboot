/* domain.c - CBoot generated (compiler: normal) */
/* Module: domain */

/*
 * CBoot - C Project Bootstrapping Tool v0.5.0
 * Domain tree operations
 *
 * 父级模块：仅包含入口转发函数（调度 / 调用层）
 * 具体实现放子模块 domain/core/，以实现：
 *   - 父模块圈复杂度低（只有转发调用）
 *   - 依赖图复杂度高的函数集中在子模块，
 *     修改子模块仅影响子模块本身，父模块保持稳定。
 */

#include "cboot.h"

/* ---------- 子模块 domain/core 导出的实现 ---------- */
Domain * domain_core_domain_new(DomainType type, const char *name, size_t struct_size);
ModuleDomain * domain_core_module_domain_new(const char *name);
FunctionDomain * domain_core_function_domain_new(const char *name, const char *return_type);
StructDomain * domain_core_struct_domain_new(const char *name);
TypeDomain * domain_core_type_domain_new(const char *name);
MacroDomain * domain_core_macro_domain_new(const char *name);
VariableDomain * domain_core_variable_domain_new(const char *name, const char *type);
MemberDomain * domain_core_member_domain_new(const char *name, const char *type);
void domain_core_domain_add_child(Domain *parent, Domain *child);
Domain * domain_core_domain_find_child(Domain *parent, const char *name);
Domain * domain_core_domain_find_child_by_type(Domain *parent, const char *name, DomainType type);
void domain_core_free_type_fields(Domain *domain);
void domain_core_domain_delete(Domain *domain);
void domain_core_domain_remove_child(Domain *parent, Domain *child);
Domain * domain_core_domain_find_nearest_of_type(Domain *from, DomainType type);
Domain * domain_core_domain_find_in_tree(Domain *root, DomainType type, const char *name);
char * domain_core_domain_get_path(Domain *domain);
int domain_core_domain_is_api(Domain *domain);
Domain * domain_core_domain_find_api_in_submodules(Domain *scope, const char *name);
Domain * domain_core_domain_check_api_name_conflict(Domain *scope, const char *name);
void domain_core_domain_set_comment(Domain *domain, const char *text);
void domain_core_domain_set_child_comment(Domain *domain, const char *target, const char *text);
Comment * domain_core_find_child_comment(Domain *domain, const char *target);
void domain_core_domain_set_value(Domain *domain, const char *value);
void domain_core_domain_set_code(Domain *domain, const char *code);
void domain_core_domain_set_call(Domain *domain, const char *call);
const char * domain_core_domain_get_call(Domain *domain);
void domain_core_domain_set_mode(Domain *domain, int mode);
Project * domain_core_project_new(const char *name);
void domain_core_project_free(Project *proj);
void domain_core_project_add_dependency(Project *proj, const char *importer_path,
                            const char *source_path, const char *cboot_file);
int domain_core_project_has_dependency(Project *proj, const char *importer_path,
                           const char *source_path);
int domain_core_project_would_cycle(Project *proj, const char *importer_path,
                                const char *source_path);
int domain_core_is_builtin_type(const char *type_name);

/* ---------- 父级转发层：只做调用，不做具体实现 ---------- */
Domain * domain_domain_new(DomainType type, const char *name, size_t struct_size) {
    return domain_core_domain_new(type, name, struct_size);
}

ModuleDomain * domain_module_domain_new(const char *name) {
    return domain_core_module_domain_new(name);
}

FunctionDomain * domain_function_domain_new(const char *name, const char *return_type) {
    return domain_core_function_domain_new(name, return_type);
}

StructDomain * domain_struct_domain_new(const char *name) {
    return domain_core_struct_domain_new(name);
}

TypeDomain * domain_type_domain_new(const char *name) {
    return domain_core_type_domain_new(name);
}

MacroDomain * domain_macro_domain_new(const char *name) {
    return domain_core_macro_domain_new(name);
}

VariableDomain * domain_variable_domain_new(const char *name, const char *type) {
    return domain_core_variable_domain_new(name, type);
}

MemberDomain * domain_member_domain_new(const char *name, const char *type) {
    return domain_core_member_domain_new(name, type);
}

void domain_domain_add_child(Domain *parent, Domain *child) {
    domain_core_domain_add_child(parent, child);
}

Domain * domain_domain_find_child(Domain *parent, const char *name) {
    return domain_core_domain_find_child(parent, name);
}

Domain * domain_domain_find_child_by_type(Domain *parent, const char *name, DomainType type) {
    return domain_core_domain_find_child_by_type(parent, name, type);
}

void domain_free_type_fields(Domain *domain) {
    domain_core_free_type_fields(domain);
}

void domain_domain_delete(Domain *domain) {
    domain_core_domain_delete(domain);
}

void domain_domain_remove_child(Domain *parent, Domain *child) {
    domain_core_domain_remove_child(parent, child);
}

Domain * domain_domain_find_nearest_of_type(Domain *from, DomainType type) {
    return domain_core_domain_find_nearest_of_type(from, type);
}

Domain * domain_domain_find_in_tree(Domain *root, DomainType type, const char *name) {
    return domain_core_domain_find_in_tree(root, type, name);
}

char * domain_domain_get_path(Domain *domain) {
    return domain_core_domain_get_path(domain);
}

int domain_domain_is_api(Domain *domain) {
    return domain_core_domain_is_api(domain);
}

Domain * domain_domain_find_api_in_submodules(Domain *scope, const char *name) {
    return domain_core_domain_find_api_in_submodules(scope, name);
}

Domain * domain_domain_check_api_name_conflict(Domain *scope, const char *name) {
    return domain_core_domain_check_api_name_conflict(scope, name);
}

void domain_domain_set_comment(Domain *domain, const char *text) {
    domain_core_domain_set_comment(domain, text);
}

void domain_domain_set_child_comment(Domain *domain, const char *target, const char *text) {
    domain_core_domain_set_child_comment(domain, target, text);
}

Comment * domain_find_child_comment(Domain *domain, const char *target) {
    return domain_core_find_child_comment(domain, target);
}

void domain_domain_set_value(Domain *domain, const char *value) {
    domain_core_domain_set_value(domain, value);
}

void domain_domain_set_code(Domain *domain, const char *code) {
    domain_core_domain_set_code(domain, code);
}

void domain_domain_set_call(Domain *domain, const char *call) {
    domain_core_domain_set_call(domain, call);
}

const char * domain_domain_get_call(Domain *domain) {
    return domain_core_domain_get_call(domain);
}

void domain_domain_set_mode(Domain *domain, int mode) {
    domain_core_domain_set_mode(domain, mode);
}

void domain_domain_set_test(Domain *domain, int cov, int pass) {
    if (!domain || domain->type != DOMAIN_FUNCTION) return;
    FunctionDomain *func = (FunctionDomain *)domain;
    func->test_cov = cov;
    func->test_pass = pass;
}

Project * domain_project_new(const char *name) {
    return domain_core_project_new(name);
}

void domain_project_free(Project *proj) {
    domain_core_project_free(proj);
}

void domain_project_add_dependency(Project *proj, const char *importer_path,
                            const char *source_path, const char *cboot_file) {
    domain_core_project_add_dependency(proj, importer_path, source_path, cboot_file);
}

int domain_project_has_dependency(Project *proj, const char *importer_path,
                           const char *source_path) {
    return domain_core_project_has_dependency(proj, importer_path, source_path);
}

int domain_project_would_cycle(Project *proj, const char *importer_path,
                                const char *source_path) {
    return domain_core_project_would_cycle(proj, importer_path, source_path);
}

int domain_is_builtin_type(const char *type_name) {
    return domain_core_is_builtin_type(type_name);
}
