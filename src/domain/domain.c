/*
 * CBoot - C Project Bootstrapping Tool v2.0
 * Domain tree operations
 */

#include "cboot.h"

/* ================================================================== */
/* Domain creation                                                      */
/* ================================================================== */

Domain *domain_new(DomainType type, const char *name, size_t struct_size)
{
	Domain *domain = (Domain *)calloc(1, struct_size);
	if (!domain) return NULL;

	domain->type = type;
	domain->name = str_dup(name);
	domain->parent = NULL;
	domain->child_count = 0;
	domain->child_capacity = 8;
	domain->children = (Domain **)malloc(sizeof(Domain *) * domain->child_capacity);
	domain->comment = NULL;
	domain->comments = (Comment *)malloc(sizeof(Comment) * 4);
	domain->comment_count = 0;
	domain->comment_capacity = 4;

	return domain;
}

ModuleDomain *module_domain_new(const char *name)
{
	Domain *base = domain_new(DOMAIN_MODULE, name, sizeof(ModuleDomain));
	if (!base) return NULL;

	ModuleDomain *mod = (ModuleDomain *)base;
	mod->mode = MOD_MODE_INTERNAL;
	mod->compiler = COMPILER_NORMAL;
	mod->value = NULL;
	mod->include_capacity = 4;
	mod->includes = (char **)malloc(sizeof(char *) * mod->include_capacity);
	mod->include_count = 0;
	mod->dep_capacity = 4;
	mod->dependencies = (char **)malloc(sizeof(char *) * mod->dep_capacity);
	mod->dep_count = 0;

	return mod;
}

FunctionDomain *function_domain_new(const char *name, const char *return_type)
{
	Domain *base = domain_new(DOMAIN_FUNCTION, name, sizeof(FunctionDomain));
	if (!base) return NULL;

	FunctionDomain *func = (FunctionDomain *)base;
	func->mode = API_MODE_NORMAL;
	func->return_type = str_dup(return_type ? return_type : "void");
	func->code = str_dup("//请在这里输入代码");
	func->value = NULL;

	return func;
}

StructDomain *struct_domain_new(const char *name)
{
	Domain *base = domain_new(DOMAIN_STRUCT, name, sizeof(StructDomain));
	if (!base) return NULL;

	StructDomain *s = (StructDomain *)base;
	s->mode = API_MODE_NORMAL;

	return s;
}

TypeDomain *type_domain_new(const char *name)
{
	Domain *base = domain_new(DOMAIN_TYPE, name, sizeof(TypeDomain));
	if (!base) return NULL;

	TypeDomain *t = (TypeDomain *)base;
	t->mode = TYPE_MODE_STRUCT;
	t->value = NULL;

	return t;
}

MacroDomain *macro_domain_new(const char *name)
{
	Domain *base = domain_new(DOMAIN_MACRO, name, sizeof(MacroDomain));
	if (!base) return NULL;

	MacroDomain *m = (MacroDomain *)base;
	m->mode = API_MODE_NORMAL;
	m->value = NULL;

	return m;
}

VariableDomain *variable_domain_new(const char *name, const char *type)
{
	Domain *base = domain_new(DOMAIN_VARIABLE, name, sizeof(VariableDomain));
	if (!base) return NULL;

	VariableDomain *v = (VariableDomain *)base;
	v->mode = VAR_MODE_NORMAL;
	v->type = str_dup(type ? type : "int");
	v->value = NULL;

	return v;
}

MemberDomain *member_domain_new(const char *name, const char *type)
{
	Domain *base = domain_new(DOMAIN_MEMBER, name, sizeof(MemberDomain));
	if (!base) return NULL;

	MemberDomain *m = (MemberDomain *)base;
	m->type = str_dup(type ? type : "int");

	return m;
}

/* ================================================================== */
/* Domain tree operations                                               */
/* ================================================================== */

void domain_add_child(Domain *parent, Domain *child)
{
	if (!parent || !child) return;

	if (parent->child_count >= parent->child_capacity) {
		parent->child_capacity *= 2;
		parent->children = (Domain **)realloc(parent->children,
		                                      sizeof(Domain *) * parent->child_capacity);
	}

	parent->children[parent->child_count] = child;
	parent->child_count++;
	child->parent = parent;
}

Domain *domain_find_child(Domain *parent, const char *name)
{
	if (!parent || !name) return NULL;

	for (int i = 0; i < parent->child_count; i++) {
		if (parent->children[i] && str_eq(parent->children[i]->name, name)) {
			return parent->children[i];
		}
	}

	return NULL;
}

Domain *domain_find_child_by_type(Domain *parent, const char *name, DomainType type)
{
	if (!parent || !name) return NULL;

	for (int i = 0; i < parent->child_count; i++) {
		Domain *child = parent->children[i];
		if (child && child->type == type && str_eq(child->name, name)) {
			return child;
		}
	}

	return NULL;
}

void domain_delete(Domain *domain)
{
	if (!domain) return;

	/* Free type-specific data */
	switch (domain->type) {
	case DOMAIN_MODULE: {
		ModuleDomain *mod = (ModuleDomain *)domain;
		free(mod->value);
		for (int i = 0; i < mod->include_count; i++) free(mod->includes[i]);
		free(mod->includes);
		for (int i = 0; i < mod->dep_count; i++) free(mod->dependencies[i]);
		free(mod->dependencies);
		break;
	}
	case DOMAIN_FUNCTION: {
		FunctionDomain *func = (FunctionDomain *)domain;
		free(func->return_type);
		free(func->code);
		free(func->value);
		break;
	}
	case DOMAIN_STRUCT:
		/* StructDomain has no allocated fields beyond the base */
		break;
	case DOMAIN_TYPE: {
		TypeDomain *t = (TypeDomain *)domain;
		free(t->value);
		break;
	}
	case DOMAIN_MACRO: {
		MacroDomain *m = (MacroDomain *)domain;
		free(m->value);
		break;
	}
	case DOMAIN_VARIABLE: {
		VariableDomain *v = (VariableDomain *)domain;
		free(v->type);
		free(v->value);
		break;
	}
	case DOMAIN_MEMBER: {
		MemberDomain *m = (MemberDomain *)domain;
		free(m->type);
		break;
	}
	}

	/* Recursively delete children */
	for (int i = 0; i < domain->child_count; i++) {
		domain_delete(domain->children[i]);
	}
	free(domain->children);

	/* Free comments */
	for (int i = 0; i < domain->comment_count; i++) {
		free(domain->comments[i].target);
		free(domain->comments[i].text);
	}
	free(domain->comments);

	/* Free name and comment */
	free(domain->comment);
	free(domain->name);
	free(domain);
}

void domain_remove_child(Domain *parent, Domain *child)
{
	if (!parent || !child) return;

	for (int i = 0; i < parent->child_count; i++) {
		if (parent->children[i] == child) {
			/* Shift remaining elements */
			for (int j = i; j < parent->child_count - 1; j++) {
				parent->children[j] = parent->children[j + 1];
			}
			parent->child_count--;
			return;
		}
	}
}

/* ================================================================== */
/* Domain search utilities                                              */
/* ================================================================== */

Domain *domain_find_nearest_of_type(Domain *from, DomainType type)
{
	if (!from) return NULL;

	Domain *d = from;
	while (d != NULL) {
		if (d->type == type) {
			return d;
		}
		d = d->parent;
	}

	return NULL;
}

Domain *domain_find_in_tree(Domain *root, DomainType type, const char *name)
{
	if (!root) return NULL;

	/* Check current node */
	if (root->type == type && str_eq(root->name, name)) {
		return root;
	}

	/* Recursively search children */
	for (int i = 0; i < root->child_count; i++) {
		Domain *found = domain_find_in_tree(root->children[i], type, name);
		if (found) return found;
	}

	return NULL;
}

char *domain_get_path(Domain *domain)
{
	if (!domain) return NULL;

	/* If domain is root (no parent), return "/" */
	if (domain->parent == NULL) {
		char *path = (char *)malloc(2);
		if (path) {
			path[0] = '/';
			path[1] = '\0';
		}
		return path;
	}

	/* Walk up to root, collecting names */
	int depth = 0;
	Domain *temp = domain;
	while (temp != NULL) {
		depth++;
		temp = temp->parent;
	}

	/* Allocate an array of domain pointers */
	Domain **chain = (Domain **)malloc(sizeof(Domain *) * depth);
	if (!chain) return NULL;
	temp = domain;
	for (int i = depth - 1; i >= 0; i--) {
		chain[i] = temp;
		temp = temp->parent;
	}

	/* Calculate total path length */
	int total_len = 0;
	for (int i = 0; i < depth; i++) {
		total_len += (int)strlen(chain[i]->name) + 1; /* +1 for '/' */
	}
	total_len++; /* null terminator */

	char *path = (char *)malloc(total_len);
	if (!path) {
		free(chain);
		return NULL;
	}

	/* Build path, skip the root node (chain[0] is root) */
	path[0] = '\0';
	for (int i = 1; i < depth; i++) {
		strcat(path, "/");
		strcat(path, chain[i]->name);
	}
	if (path[0] == '\0') {
		strcat(path, "/");
	}

	free(chain);
	return path;
}

int domain_is_api(Domain *domain)
{
	if (!domain) return 0;

	switch (domain->type) {
	case DOMAIN_FUNCTION:
		return ((FunctionDomain *)domain)->mode == API_MODE_API;
	case DOMAIN_STRUCT:
		return ((StructDomain *)domain)->mode == API_MODE_API;
	case DOMAIN_MACRO:
		return ((MacroDomain *)domain)->mode == API_MODE_API;
	case DOMAIN_TYPE: {
		TypeMode m = ((TypeDomain *)domain)->mode;
		return (m == TYPE_MODE_API_RENAME || m == TYPE_MODE_API_STRUCT);
	}
	default:
		return 0;
	}
}

/* ================================================================== */
/* API submodule search (name bubbling)                                */
/* ================================================================== */

Domain *domain_find_api_in_submodules(Domain *scope, const char *name)
{
	if (!scope || !name) return NULL;

	/* Recursively look in all child modules for API items with matching name */
	for (int i = 0; i < scope->child_count; i++) {
		Domain *child = scope->children[i];
		if (!child) continue;

		if (child->type == DOMAIN_MODULE) {
			/* First check if this module has an API item with the name */
			for (int j = 0; j < child->child_count; j++) {
				Domain *grandchild = child->children[j];
				if (!grandchild) continue;
				if (grandchild->name && strcmp(grandchild->name, name) == 0 &&
				    domain_is_api(grandchild)) {
					return grandchild;
				}
			}
			/* Then recurse into submodules */
			Domain *found = domain_find_api_in_submodules(child, name);
			if (found) return found;
		}
	}

	return NULL;
}

Domain *domain_check_api_name_conflict(Domain *scope, const char *name)
{
	return domain_find_api_in_submodules(scope, name);
}

/* ================================================================== */
/* Comment operations                                                  */
/* ================================================================== */

void domain_set_comment(Domain *domain, const char *text)
{
	if (!domain) return;

	free(domain->comment);
	domain->comment = str_dup(text);
}

void domain_set_child_comment(Domain *domain, const char *target, const char *text)
{
	if (!domain || !target) return;

	/* Find existing comment with matching target */
	for (int i = 0; i < domain->comment_count; i++) {
		if (str_eq(domain->comments[i].target, target)) {
			free(domain->comments[i].text);
			domain->comments[i].text = str_dup(text);
			return;
		}
	}

	/* Not found, add new */
	if (domain->comment_count >= domain->comment_capacity) {
		domain->comment_capacity *= 2;
		domain->comments = (Comment *)realloc(domain->comments,
		                                      sizeof(Comment) * domain->comment_capacity);
	}

	domain->comments[domain->comment_count].target = str_dup(target);
	domain->comments[domain->comment_count].text = str_dup(text);
	domain->comment_count++;
}

Comment *domain_find_child_comment(Domain *domain, const char *target)
{
	if (!domain || !target) return NULL;

	for (int i = 0; i < domain->comment_count; i++) {
		if (str_eq(domain->comments[i].target, target)) {
			return &domain->comments[i];
		}
	}

	return NULL;
}

/* ================================================================== */
/* Value and mode operations                                            */
/* ================================================================== */

void domain_set_value(Domain *domain, const char *value)
{
	if (!domain) return;

	switch (domain->type) {
	case DOMAIN_MODULE: {
		ModuleDomain *mod = (ModuleDomain *)domain;
		free(mod->value);
		mod->value = str_dup(value);
		break;
	}
	case DOMAIN_FUNCTION: {
		FunctionDomain *func = (FunctionDomain *)domain;
		free(func->value);
		func->value = str_dup(value);
		break;
	}
	case DOMAIN_TYPE: {
		TypeDomain *t = (TypeDomain *)domain;
		free(t->value);
		t->value = str_dup(value);
		break;
	}
	case DOMAIN_MACRO: {
		MacroDomain *m = (MacroDomain *)domain;
		free(m->value);
		m->value = str_dup(value);
		break;
	}
	case DOMAIN_VARIABLE: {
		VariableDomain *v = (VariableDomain *)domain;
		free(v->value);
		v->value = str_dup(value);
		break;
	}
	case DOMAIN_STRUCT:
	case DOMAIN_MEMBER:
		/* No value field to set */
		break;
	}
}

void domain_set_mode(Domain *domain, int mode)
{
	if (!domain) return;

	switch (domain->type) {
	case DOMAIN_MODULE:
		((ModuleDomain *)domain)->mode = (ModMode)mode;
		break;
	case DOMAIN_FUNCTION:
		((FunctionDomain *)domain)->mode = (ApiMode)mode;
		break;
	case DOMAIN_STRUCT:
		((StructDomain *)domain)->mode = (ApiMode)mode;
		break;
	case DOMAIN_TYPE:
		((TypeDomain *)domain)->mode = (TypeMode)mode;
		break;
	case DOMAIN_MACRO:
		((MacroDomain *)domain)->mode = (ApiMode)mode;
		break;
	case DOMAIN_VARIABLE:
		((VariableDomain *)domain)->mode = (VarMode)mode;
		break;
	case DOMAIN_MEMBER:
		/* No mode field to set */
		break;
	}
}

/* ================================================================== */
/* Project operations                                                  */
/* ================================================================== */

Project *project_new(const char *name)
{
	Project *proj = (Project *)calloc(1, sizeof(Project));
	if (!proj) return NULL;

	proj->name = str_dup(name);
	proj->root = (Domain *)module_domain_new(name);
	proj->current = proj->root;
	proj->has_generated = 0;
	proj->cboot_file = NULL;
	proj->fine_tune_mode = 0;

	proj->imported_projects = NULL;
	proj->import_count = 0;
	proj->import_capacity = 0;
	proj->imported_libs = NULL;
	proj->lib_count = 0;
	proj->lib_capacity = 0;
	proj->dependencies = NULL;
	proj->dep_count = 0;
	proj->dep_capacity = 0;

	return proj;
}

void project_free(Project *proj)
{
	if (!proj) return;

	for (int i = 0; i < proj->import_count; i++)
		free(proj->imported_projects[i]);
	free(proj->imported_projects);

	for (int i = 0; i < proj->lib_count; i++)
		free(proj->imported_libs[i]);
	free(proj->imported_libs);

	for (int i = 0; i < proj->dep_count; i++) {
		free(proj->dependencies[i].importer);
		free(proj->dependencies[i].source);
		free(proj->dependencies[i].cboot_file);
	}
	free(proj->dependencies);

	free(proj->cboot_file);
	domain_delete(proj->root);
	free(proj->name);
	free(proj);
}

/* ================================================================== */
/* Dependency operations (for im command)                              */
/* ================================================================== */

void project_add_dependency(Project *proj, const char *importer_path,
                            const char *source_path, const char *cboot_file)
{
	if (!proj || !importer_path || !source_path) return;

	if (proj->dep_count >= proj->dep_capacity) {
		proj->dep_capacity = (proj->dep_capacity == 0) ? 8 : proj->dep_capacity * 2;
		proj->dependencies = (Dependency *)realloc(proj->dependencies,
		                                            sizeof(Dependency) * proj->dep_capacity);
	}

	proj->dependencies[proj->dep_count].importer = str_dup(importer_path);
	proj->dependencies[proj->dep_count].source = str_dup(source_path);
	proj->dependencies[proj->dep_count].cboot_file = cboot_file ? str_dup(cboot_file) : NULL;
	proj->dep_count++;
}

int project_has_dependency(Project *proj, const char *importer_path,
                           const char *source_path)
{
	if (!proj || !importer_path || !source_path) return 0;

	for (int i = 0; i < proj->dep_count; i++) {
		if (str_eq(proj->dependencies[i].importer, importer_path) &&
		    str_eq(proj->dependencies[i].source, source_path)) {
			return 1;
		}
	}
	return 0;
}

/* ================================================================== */
/* Type detection                                                      */
/* ================================================================== */

int is_builtin_type(const char *type_name)
{
	if (!type_name) return 0;

	/* Strip trailing whitespace and asterisks for pointer checking */
	char buf[256];
	int len = (int)strlen(type_name);
	if (len >= 256) len = 255;
	memcpy(buf, type_name, len);
	buf[len] = '\0';

	/* Trim trailing whitespace */
	while (len > 0 && isspace((unsigned char)buf[len - 1])) {
		buf[--len] = '\0';
	}

	/* Check if it ends with * (pointer type) */
	int is_ptr = 0;
	char base_type[256];
	if (len > 0 && buf[len - 1] == '*') {
		is_ptr = 1;
		int i = len - 1;
		while (i > 0 && (buf[i] == '*' || isspace((unsigned char)buf[i]))) {
			i--;
		}
		while (i > 0 && isspace((unsigned char)buf[i])) {
			i--;
		}
		int blen = i + 1;
		if (blen >= 256) blen = 255;
		memcpy(base_type, buf, blen);
		base_type[blen] = '\0';
		while (blen > 0 && isspace((unsigned char)base_type[blen - 1])) {
			base_type[--blen] = '\0';
		}
	} else {
		strcpy(base_type, buf);
	}

	const char *check = is_ptr ? base_type : buf;

	/* Check against builtin types */
	const char *builtins[] = {
		"void", "char", "short", "int", "long", "float", "double",
		"signed", "unsigned", "size_t", "ssize_t",
		"int8_t", "int16_t", "int32_t", "int64_t",
		"uint8_t", "uint16_t", "uint32_t", "uint64_t",
		"bool", "FILE",
		NULL
	};

	for (int i = 0; builtins[i] != NULL; i++) {
		if (strcmp(check, builtins[i]) == 0) {
			return 1;
		}
	}

	/* Also check compound types */
	const char *compound[] = {
		"unsigned int", "unsigned short", "unsigned long", "unsigned char",
		"signed int", "signed char",
		"long int", "long long", "short int",
		"long double", "long unsigned int",
		NULL
	};

	for (int i = 0; compound[i] != NULL; i++) {
		if (strcmp(check, compound[i]) == 0) {
			return 1;
		}
	}

	return 0;
}