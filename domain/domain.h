/*
 * CBoot - C Project Bootstrapping Tool v0.3.1
 * Domain data model
 */

#ifndef DOMAIN_H
#define DOMAIN_H

#include <stddef.h>

/* ------------------------------------------------------------------ */
/* Domain type enumeration                                             */
/* ------------------------------------------------------------------ */

typedef enum {
	DOMAIN_MODULE,
	DOMAIN_FUNCTION,
	DOMAIN_STRUCT,
	DOMAIN_TYPE,
	DOMAIN_MACRO,
	DOMAIN_VARIABLE,
	DOMAIN_MEMBER
} DomainType;

/* ------------------------------------------------------------------ */
/* Mode enumerations                                                   */
/* ------------------------------------------------------------------ */

typedef enum {
	MOD_MODE_SRC,       /* 源代码模式（生成 .c） */
	MOD_MODE_STATIC,    /* 预编译静态库 (.a)，value 字段存放路径 */
	MOD_MODE_DYNAMIC,   /* 预编译动态库 (.so)，value 字段存放路径 */
	MOD_MODE_EXTERNAL   /* im 导入的 API 引用 */
} ModMode;

typedef enum {
	COMPILER_NORMAL,
	COMPILER_EXE,
	COMPILER_SL,
	COMPILER_DL
} CompilerMode;

typedef enum {
	API_MODE_API,
	API_MODE_NORMAL
} ApiMode;

typedef enum {
	TYPE_MODE_RENAME,
	TYPE_MODE_STRUCT,
	TYPE_MODE_API_RENAME,
	TYPE_MODE_API_STRUCT
} TypeMode;

typedef enum {
	VAR_MODE_STATIC,
	VAR_MODE_NORMAL
} VarMode;

/* ------------------------------------------------------------------ */
/* Run mode                                                            */
/* ------------------------------------------------------------------ */

typedef enum {
	MODE_INTERACTIVE,
	MODE_BATCH,
	MODE_FINE_TUNE,
	MODE_TO_CBOOT,
	MODE_HELP
} RunMode;

/* ------------------------------------------------------------------ */
/* Comment                                                             */
/* ------------------------------------------------------------------ */

typedef struct Comment {
	char *target;       /* parameter or member name */
	char *text;         /* comment text */
} Comment;

/* ------------------------------------------------------------------ */
/* Domain (base of all domain types)                                    */
/* ------------------------------------------------------------------ */

typedef struct Domain {
	DomainType   type;
	char        *name;
	char        *comment;
	struct Domain *parent;
	struct Domain **children;
	int           child_count;
	int           child_capacity;
	Comment      *comments;
	int           comment_count;
	int           comment_capacity;
} Domain;

/* ------------------------------------------------------------------ */
/* ModuleDomain (domain with module-specific data)                      */
/* ------------------------------------------------------------------ */

typedef struct ModuleDomain {
	Domain      base;
	ModMode     mode;
	CompilerMode compiler;
	char       *value;
	char       *code;
	char       **includes;
	int         include_count;
	int         include_capacity;
	char       **dependencies;
	int         dep_count;
	int         dep_capacity;
} ModuleDomain;

/* ------------------------------------------------------------------ */
/* FunctionDomain (domain with function-specific data)                  */
/* ------------------------------------------------------------------ */

typedef struct FunctionDomain {
	Domain  base;
	ApiMode mode;
	char   *return_type;
	char   *call;      /* calling convention: __cdecl, __stdcall, __fastcall, etc. */
	char   *code;
	char   *value;
} FunctionDomain;

/* ------------------------------------------------------------------ */
/* StructDomain (domain with struct-specific data)                      */
/* ------------------------------------------------------------------ */

typedef struct StructDomain {
	Domain  base;
	ApiMode mode;
} StructDomain;

/* ------------------------------------------------------------------ */
/* TypeDomain (domain with typedef-specific data)                       */
/* ------------------------------------------------------------------ */

typedef struct TypeDomain {
	Domain   base;
	TypeMode mode;
	char    *value;
} TypeDomain;

/* ------------------------------------------------------------------ */
/* MacroDomain (domain with macro-specific data)                        */
/* ------------------------------------------------------------------ */

typedef struct MacroDomain {
	Domain  base;
	ApiMode mode;
	char   *value;
} MacroDomain;

/* ------------------------------------------------------------------ */
/* VariableDomain (domain with variable-specific data)                  */
/* ------------------------------------------------------------------ */

typedef struct VariableDomain {
	Domain  base;
	VarMode mode;
	char   *type;
	char   *value;
} VariableDomain;

/* ------------------------------------------------------------------ */
/* MemberDomain (domain with struct-member-specific data)               */
/* ------------------------------------------------------------------ */

typedef struct MemberDomain {
	Domain  base;
	char   *type;
} MemberDomain;

/* ------------------------------------------------------------------ */
/* Dependency record (for im command - API-only import)                */
/* ------------------------------------------------------------------ */

typedef struct Dependency {
	char *importer;   /* module path that uses the API (e.g. "/a/b") */
	char *source;     /* source module path providing the API (e.g. "/a/c") */
	char *cboot_file; /* .cboot file the API was imported from */
} Dependency;

/* ------------------------------------------------------------------ */
/* Project (top-level state)                                            */
/* ------------------------------------------------------------------ */

typedef struct Project {
	char    *name;
	Domain  *root;
	Domain  *current;
	int      has_generated;
	char    *cboot_file;
	int      fine_tune_mode;
	char   **imported_projects;   /* for `in` command: copied subprojects */
	int      import_count;
	int      import_capacity;
	char   **imported_libs;
	int      lib_count;
	int      lib_capacity;
	Dependency *dependencies;    /* for `im` command: API-only deps */
	int      dep_count;
	int      dep_capacity;
} Project;

/* ------------------------------------------------------------------ */
/* Global state                                                        */
/* ------------------------------------------------------------------ */

extern Project *g_proj;
extern RunMode  g_mode;
extern int      g_force;
extern int      g_running;

/* ------------------------------------------------------------------ */
/* Domain creation functions                                            */
/* ------------------------------------------------------------------ */

Domain        *domain_domain_new(DomainType type, const char *name, size_t struct_size);
ModuleDomain  *domain_module_domain_new(const char *name);
FunctionDomain *domain_function_domain_new(const char *name, const char *return_type);
StructDomain  *domain_struct_domain_new(const char *name);
TypeDomain    *domain_type_domain_new(const char *name);
MacroDomain   *domain_macro_domain_new(const char *name);
VariableDomain *domain_variable_domain_new(const char *name, const char *type);
MemberDomain  *domain_member_domain_new(const char *name, const char *type);

/* ------------------------------------------------------------------ */
/* Domain tree operations                                               */
/* ------------------------------------------------------------------ */

void    domain_domain_add_child(Domain *parent, Domain *child);
Domain *domain_domain_find_child(Domain *parent, const char *name);
Domain *domain_domain_find_child_by_type(Domain *parent, const char *name, DomainType type);
void    domain_domain_delete(Domain *domain);
void    domain_domain_remove_child(Domain *parent, Domain *child);

/* ------------------------------------------------------------------ */
/* Domain search utilities                                              */
/* ------------------------------------------------------------------ */

Domain *domain_domain_find_nearest_of_type(Domain *from, DomainType type);
Domain *domain_domain_find_in_tree(Domain *root, DomainType type, const char *name);
char   *domain_domain_get_path(Domain *domain);
int     domain_domain_is_api(Domain *domain);

/* Find an API-visible item in submodules (recursive).
 * Searches direct children first, then recurses into module children
 * looking for API-mode items of the given name.
 * Used for name conflict detection and type resolution. */
Domain *domain_domain_find_api_in_submodules(Domain *scope, const char *name);

/* Check if a name would conflict with any API item from submodules.
 * Returns the conflicting domain if found, NULL otherwise. */
Domain *domain_domain_check_api_name_conflict(Domain *scope, const char *name);

/* ------------------------------------------------------------------ */
/* Comment operations                                                   */
/* ------------------------------------------------------------------ */

void     domain_domain_set_comment(Domain *domain, const char *text);
void     domain_domain_set_child_comment(Domain *domain, const char *target, const char *text);
Comment *domain_find_child_comment(Domain *domain, const char *target);

/* ------------------------------------------------------------------ */
/* Value and mode operations                                            */
/* ------------------------------------------------------------------ */

void domain_domain_set_value(Domain *domain, const char *value);
void domain_domain_set_mode(Domain *domain, int mode);
void domain_domain_set_code(Domain *domain, const char *code);
void domain_domain_set_call(Domain *domain, const char *call);
const char *domain_domain_get_call(Domain *domain);

/* ------------------------------------------------------------------ */
/* Project operations                                                   */
/* ------------------------------------------------------------------ */

Project *domain_project_new(const char *name);
void     domain_project_free(Project *proj);

/* Add a dependency record (for im command) */
void     domain_project_add_dependency(Project *proj, const char *importer_path,
                                const char *source_path, const char *cboot_file);

/* Check if a dependency already exists (importer->source) */
int      domain_project_has_dependency(Project *proj, const char *importer_path,
                                const char *source_path);

/* ------------------------------------------------------------------ */
/* Type detection                                                       */
/* ------------------------------------------------------------------ */

int domain_is_builtin_type(const char *type_name);

#endif /* DOMAIN_H */