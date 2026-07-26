/*
 * CBoot - C Project Bootstrapping Tool v2.0
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
	MOD_MODE_INTERNAL,
	MOD_MODE_EXTERNAL
} ModMode;

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
	Domain   base;
	ModMode  mode;
	char    *value;
	char   **includes;
	int      include_count;
	int      include_capacity;
	char   **dependencies;
	int      dep_count;
	int      dep_capacity;
} ModuleDomain;

/* ------------------------------------------------------------------ */
/* FunctionDomain (domain with function-specific data)                  */
/* ------------------------------------------------------------------ */

typedef struct FunctionDomain {
	Domain  base;
	ApiMode mode;
	char   *return_type;
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
/* Project (top-level state)                                            */
/* ------------------------------------------------------------------ */

typedef struct Project {
	char    *name;
	Domain  *root;
	Domain  *current;
	int      has_generated;
	char    *cboot_file;
	int      fine_tune_mode;
	char   **imported_projects;
	int      import_count;
	int      import_capacity;
	char   **imported_libs;
	int      lib_count;
	int      lib_capacity;
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

Domain        *domain_new(DomainType type, const char *name, size_t struct_size);
ModuleDomain  *module_domain_new(const char *name);
FunctionDomain *function_domain_new(const char *name, const char *return_type);
StructDomain  *struct_domain_new(const char *name);
TypeDomain    *type_domain_new(const char *name);
MacroDomain   *macro_domain_new(const char *name);
VariableDomain *variable_domain_new(const char *name, const char *type);
MemberDomain  *member_domain_new(const char *name, const char *type);

/* ------------------------------------------------------------------ */
/* Domain tree operations                                               */
/* ------------------------------------------------------------------ */

void    domain_add_child(Domain *parent, Domain *child);
Domain *domain_find_child(Domain *parent, const char *name);
Domain *domain_find_child_by_type(Domain *parent, const char *name, DomainType type);
void    domain_delete(Domain *domain);
void    domain_remove_child(Domain *parent, Domain *child);

/* ------------------------------------------------------------------ */
/* Domain search utilities                                              */
/* ------------------------------------------------------------------ */

Domain *domain_find_nearest_of_type(Domain *from, DomainType type);
Domain *domain_find_in_tree(Domain *root, DomainType type, const char *name);
char   *domain_get_path(Domain *domain);
int     domain_is_api(Domain *domain);

/* ------------------------------------------------------------------ */
/* Comment operations                                                   */
/* ------------------------------------------------------------------ */

void     domain_set_comment(Domain *domain, const char *text);
void     domain_set_child_comment(Domain *domain, const char *target, const char *text);
Comment *domain_find_child_comment(Domain *domain, const char *target);

/* ------------------------------------------------------------------ */
/* Value and mode operations                                            */
/* ------------------------------------------------------------------ */

void domain_set_value(Domain *domain, const char *value);
void domain_set_mode(Domain *domain, int mode);

/* ------------------------------------------------------------------ */
/* Project operations                                                   */
/* ------------------------------------------------------------------ */

Project *project_new(const char *name);
void     project_free(Project *proj);

/* ------------------------------------------------------------------ */
/* Type detection                                                       */
/* ------------------------------------------------------------------ */

int is_builtin_type(const char *type_name);

#endif /* DOMAIN_H */