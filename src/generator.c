/*
 * CBoot - C Project Bootstrapping Tool v2.0
 * Code generator (新规范 v2.0)
 *
 * 生成规则:
 *  - 一个mod一个目录
 *  - mod下的非mod子域放在 <mod name>.c 和 <mod name>.h 中
 *  - .h 只放置被标记为 api 的 def/type/函数接口定义
 *  - 自动给每个模块生成 CMakeLists.txt
 *  - 自动生成最小 .cboot
 *  - 顶层 CMakeLists.txt
 */

#include "cboot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/* Forward declarations                                               */
/* ------------------------------------------------------------------ */

static void generate_module(Domain *mod, const char *parent_dir);
static void generate_mod_c(Domain *mod, const char *dir);
static void generate_mod_h(Domain *mod, const char *dir);
static void generate_mod_cmake(Domain *mod, const char *dir);
static void generate_mod_cboot(Domain *mod, const char *dir);
static void generate_top_cmake(Project *proj);
static void generate_top_main(Project *proj);
static void collect_all_modules(Domain *root, Domain ***list, int *count, int *capacity);

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static int is_api(Domain *d) {
    if (!d) return 0;
    switch (d->type) {
        case DOMAIN_FUNCTION:
            return ((FunctionDomain *)d)->mode == API_MODE_API;
        case DOMAIN_STRUCT:
            return ((StructDomain *)d)->mode == API_MODE_API;
        case DOMAIN_TYPE:
            return ((TypeDomain *)d)->mode == TYPE_MODE_API_RENAME ||
                   ((TypeDomain *)d)->mode == TYPE_MODE_API_STRUCT;
        case DOMAIN_MACRO:
            return ((MacroDomain *)d)->mode == API_MODE_API;
        default:
            return 0;
    }
}

static void collect_all_modules(Domain *root, Domain ***list, int *count, int *capacity) {
    if (!root) return;

    if (root->type == DOMAIN_MODULE && root->parent) {
        if (*count >= *capacity) {
            *capacity = (*capacity == 0) ? 16 : (*capacity) * 2;
            *list = (Domain **)realloc(*list, sizeof(Domain *) * (*capacity));
        }
        (*list)[*count] = root;
        (*count)++;
    }

    for (int i = 0; i < root->child_count; i++) {
        collect_all_modules(root->children[i], list, count, capacity);
    }
}

/* ================================================================== */
/* generate_project - top-level code generator                         */
/* ================================================================== */

int generate_project(Project *proj) {
    if (!proj || !proj->root) return -1;

    /* Create top-level directories */
    ensure_dir("src");

    /* Generate each module recursively */
    generate_module(proj->root, "src");

    /* Generate top-level CMakeLists.txt */
    generate_top_cmake(proj);

    /* Generate top-level main.c */
    generate_top_main(proj);

    proj->has_generated = 1;
    return 0;
}

/* ================================================================== */
/* generate_module - recursively generate code for a module            */
/* ================================================================== */

static void generate_module(Domain *mod, const char *parent_dir) {
    if (!mod || mod->type != DOMAIN_MODULE) return;

    char mod_dir[MAX_PATH_LEN];

    /* Create module directory */
    snprintf(mod_dir, sizeof(mod_dir), "%s/%s", parent_dir, mod->name);
    ensure_dir(mod_dir);

    /* Generate .c and .h */
    generate_mod_c(mod, mod_dir);
    generate_mod_h(mod, mod_dir);

    /* Generate CMakeLists.txt */
    generate_mod_cmake(mod, mod_dir);

    /* Generate minimal .cboot */
    generate_mod_cboot(mod, mod_dir);

    /* Generate 4 markdown docs */
    generate_module_docs(mod, mod_dir);

    /* Process child modules */
    for (int i = 0; i < mod->child_count; i++) {
        Domain *child = mod->children[i];
        if (child->type == DOMAIN_MODULE) {
            generate_module(child, mod_dir);
        }
    }
}

/* ================================================================== */
/* generate_mod_c - write <mod>.c file                                 */
/* ================================================================== */

static void write_c_includes(Domain *mod, FILE *f) {
    ModuleDomain *md = (ModuleDomain *)mod;

    /* Include self header */
    fprintf(f, "#include \"%s.h\"\n", mod->name);

    /* Include parent module's header */
    if (mod->parent && mod->parent->type == DOMAIN_MODULE) {
        fprintf(f, "#include \"../%s.h\"\n", mod->parent->name);
    }

    /* Include dependencies */
    for (int i = 0; i < md->dep_count; i++) {
        fprintf(f, "#include \"%s.h\"\n", md->dependencies[i]);
    }

    /* Include system/local headers */
    for (int i = 0; i < md->include_count; i++) {
        fprintf(f, "#include %s\n", md->includes[i]);
    }

    fprintf(f, "\n");
}

static void write_c_defs(Domain *mod, FILE *f) {
    for (int i = 0; i < mod->child_count; i++) {
        Domain *child = mod->children[i];
        if (child->type == DOMAIN_MACRO) {
            MacroDomain *md = (MacroDomain *)child;
            if (child->comment) fprintf(f, "// %s\n", child->comment);
            if (md->value) {
                fprintf(f, "#define %s %s\n", child->name, md->value);
            } else {
                fprintf(f, "#define %s\n", child->name);
            }
            fprintf(f, "\n");
        }
    }
}

static void write_c_types(Domain *mod, FILE *f) {
    for (int i = 0; i < mod->child_count; i++) {
        Domain *child = mod->children[i];

        if (child->type == DOMAIN_STRUCT) {
            if (child->comment) fprintf(f, "// %s\n", child->comment);
            fprintf(f, "typedef struct %s {\n", child->name);
            for (int j = 0; j < child->child_count; j++) {
                Domain *mem = child->children[j];
                if (mem->type == DOMAIN_MEMBER) {
                    MemberDomain *mb = (MemberDomain *)mem;
                    if (mem->comment) {
                        fprintf(f, "    %s %s;  // %s\n", mb->type, mem->name, mem->comment);
                    } else {
                        fprintf(f, "    %s %s;\n", mb->type, mem->name);
                    }
                }
            }
            fprintf(f, "} %s;\n\n", child->name);
        }
        else if (child->type == DOMAIN_TYPE) {
            TypeDomain *td = (TypeDomain *)child;
            if (child->comment) fprintf(f, "// %s\n", child->comment);
            if (td->mode == TYPE_MODE_RENAME || td->mode == TYPE_MODE_API_RENAME) {
                if (td->value) {
                    fprintf(f, "typedef %s %s;\n", td->value, child->name);
                } else {
                    fprintf(f, "typedef int %s;  /* 未指定底层类型 */\n", child->name);
                }
            } else {
                /* struct mode */
                fprintf(f, "typedef struct %s {\n", child->name);
                for (int j = 0; j < child->child_count; j++) {
                    Domain *mem = child->children[j];
                    if (mem->type == DOMAIN_MEMBER) {
                        MemberDomain *mb = (MemberDomain *)mem;
                        if (mem->comment) {
                            fprintf(f, "    %s %s;  // %s\n", mb->type, mem->name, mem->comment);
                        } else {
                            fprintf(f, "    %s %s;\n", mb->type, mem->name);
                        }
                    }
                }
                fprintf(f, "} %s;\n", child->name);
            }
            fprintf(f, "\n");
        }
    }
}

static void write_c_variables(Domain *mod, FILE *f) {
    /* Only write module-level variables (direct children of module that are VARIABLE) */
    for (int i = 0; i < mod->child_count; i++) {
        Domain *child = mod->children[i];
        if (child->type == DOMAIN_VARIABLE) {
            VariableDomain *v = (VariableDomain *)child;
            if (child->comment) fprintf(f, "// %s\n", child->comment);
            if (v->mode == VAR_MODE_STATIC) {
                fprintf(f, "static %s %s", v->type, child->name);
            } else {
                fprintf(f, "%s %s", v->type, child->name);
            }
            if (v->value) {
                fprintf(f, " = %s", v->value);
            }
            fprintf(f, ";\n\n");
        }
    }
}

static void write_c_functions(Domain *mod, FILE *f) {
    for (int i = 0; i < mod->child_count; i++) {
        Domain *child = mod->children[i];
        if (child->type != DOMAIN_FUNCTION) continue;

        FunctionDomain *func = (FunctionDomain *)child;

        /* Comment */
        if (child->comment) fprintf(f, "// %s\n", child->comment);
        if (func->value) fprintf(f, "// 业务逻辑: %s\n", func->value);

        /* Function signature */
        fprintf(f, "%s %s(", func->return_type, child->name);
        int first = 1;
        for (int j = 0; j < child->child_count; j++) {
            Domain *param = child->children[j];
            if (param->type == DOMAIN_MEMBER) {
                MemberDomain *mb = (MemberDomain *)param;
                if (!first) fprintf(f, ", ");
                fprintf(f, "%s %s", mb->type, param->name);
                first = 0;
            }
        }
        fprintf(f, ") {\n");

        /* Local variables */
        int has_vars = 0;
        for (int j = 0; j < child->child_count; j++) {
            Domain *var = child->children[j];
            if (var->type == DOMAIN_VARIABLE) {
                VariableDomain *v = (VariableDomain *)var;
                if (var->comment) fprintf(f, "    // %s\n", var->comment);
                if (v->mode == VAR_MODE_STATIC) {
                    fprintf(f, "    static %s %s", v->type, var->name);
                } else {
                    fprintf(f, "    %s %s", v->type, var->name);
                }
                if (v->value) fprintf(f, " = %s", v->value);
                fprintf(f, ";\n");
                has_vars = 1;
            }
        }
        if (has_vars) fprintf(f, "\n");

        /* Function body */
        if (func->code) {
            fprintf(f, "%s\n", func->code);
        } else {
            fprintf(f, "    // TODO: implement\n");
        }

        fprintf(f, "}\n\n");
    }
}

static void generate_mod_c(Domain *mod, const char *dir) {
    char file_path[MAX_PATH_LEN];
    snprintf(file_path, sizeof(file_path), "%s/%s.c", dir, mod->name);

    FILE *f = fopen(file_path, "w");
    if (!f) {
        fprintf(stderr, "generator: 无法创建 '%s'\n", file_path);
        return;
    }

    fprintf(f, "/* %s.c - CBoot generated */\n", mod->name);
    fprintf(f, "/* Module: %s */\n\n", mod->name);

    write_c_includes(mod, f);
    write_c_defs(mod, f);
    write_c_types(mod, f);
    write_c_variables(mod, f);
    write_c_functions(mod, f);

    fclose(f);
}

/* ================================================================== */
/* generate_mod_h - write <mod>.h file (API only)                      */
/* ================================================================== */

static void generate_mod_h(Domain *mod, const char *dir) {
    char file_path[MAX_PATH_LEN];
    snprintf(file_path, sizeof(file_path), "%s/%s.h", dir, mod->name);

    FILE *f = fopen(file_path, "w");
    if (!f) {
        fprintf(stderr, "generator: 无法创建 '%s'\n", file_path);
        return;
    }

    /* Include guard */
    char guard[MAX_NAME_LEN * 2];
    snprintf(guard, sizeof(guard), "%s_H", mod->name);
    for (int i = 0; guard[i]; i++)
        guard[i] = (char)toupper((unsigned char)guard[i]);

    fprintf(f, "/* %s.h - CBoot generated (API declarations only) */\n", mod->name);
    fprintf(f, "#ifndef %s\n", guard);
    fprintf(f, "#define %s\n\n", guard);

    fprintf(f, "#include <stdio.h>\n");
    fprintf(f, "#include <stdlib.h>\n");
    fprintf(f, "#include <string.h>\n\n");

    /* API macros */
    int has_api = 0;
    for (int i = 0; i < mod->child_count; i++) {
        Domain *child = mod->children[i];
        if (child->type == DOMAIN_MACRO && is_api(child)) {
            MacroDomain *m = (MacroDomain *)child;
            if (child->comment) fprintf(f, "// %s\n", child->comment);
            if (m->value) {
                fprintf(f, "#define %s %s\n", child->name, m->value);
            } else {
                fprintf(f, "#define %s\n", child->name);
            }
            has_api = 1;
        }
    }
    if (has_api) fprintf(f, "\n");

    /* API types */
    for (int i = 0; i < mod->child_count; i++) {
        Domain *child = mod->children[i];
        if (child->type == DOMAIN_TYPE && is_api(child)) {
            TypeDomain *td = (TypeDomain *)child;
            if (child->comment) fprintf(f, "// %s\n", child->comment);
            if (td->mode == TYPE_MODE_API_RENAME) {
                if (td->value) {
                    fprintf(f, "typedef %s %s;\n", td->value, child->name);
                } else {
                    fprintf(f, "typedef int %s;\n", child->name);
                }
            } else {
                fprintf(f, "typedef struct %s %s;\n", child->name, child->name);
            }
            fprintf(f, "\n");
        }
        else if (child->type == DOMAIN_STRUCT && is_api(child)) {
            if (child->comment) fprintf(f, "// %s\n", child->comment);
            fprintf(f, "typedef struct %s %s;\n\n", child->name, child->name);
        }
    }

    /* API function declarations */
    for (int i = 0; i < mod->child_count; i++) {
        Domain *child = mod->children[i];
        if (child->type == DOMAIN_FUNCTION && is_api(child)) {
            FunctionDomain *func = (FunctionDomain *)child;
            if (child->comment) fprintf(f, "// %s\n", child->comment);
            fprintf(f, "%s %s(", func->return_type, child->name);
            int first = 1;
            for (int j = 0; j < child->child_count; j++) {
                Domain *param = child->children[j];
                if (param->type == DOMAIN_MEMBER) {
                    MemberDomain *mb = (MemberDomain *)param;
                    if (!first) fprintf(f, ", ");
                    fprintf(f, "%s %s", mb->type, param->name);
                    first = 0;
                }
            }
            fprintf(f, ");\n\n");
        }
    }

    fprintf(f, "#endif /* %s */\n", guard);
    fclose(f);
}

/* ================================================================== */
/* generate_mod_cmake - write CMakeLists.txt for a module              */
/* ================================================================== */

static void generate_mod_cmake(Domain *mod, const char *dir) {
    char file_path[MAX_PATH_LEN];
    snprintf(file_path, sizeof(file_path), "%s/CMakeLists.txt", dir);

    FILE *f = fopen(file_path, "w");
    if (!f) {
        fprintf(stderr, "generator: 无法创建 '%s'\n", file_path);
        return;
    }

    fprintf(f, "# CMakeLists.txt for module %s (CBoot generated)\n\n", mod->name);
    fprintf(f, "set(MODULE_SOURCES\n");
    fprintf(f, "    ${CMAKE_CURRENT_SOURCE_DIR}/%s.c\n", mod->name);
    fprintf(f, ")\n\n");

    /* Add child modules */
    int has_subdirs = 0;
    for (int i = 0; i < mod->child_count; i++) {
        if (mod->children[i]->type == DOMAIN_MODULE) {
            if (!has_subdirs) {
                fprintf(f, "# Sub-modules\n");
                has_subdirs = 1;
            }
            fprintf(f, "add_subdirectory(%s)\n", mod->children[i]->name);
        }
    }

    fprintf(f, "\n# Export sources to parent scope\n");
    fprintf(f, "set(MODULE_SOURCES ${MODULE_SOURCES} PARENT_SCOPE)\n");

    fclose(f);
}

/* ================================================================== */
/* generate_mod_cboot - write minimal .cboot for a module              */
/* ================================================================== */

static void generate_mod_cboot(Domain *mod, const char *dir) {
    char file_path[MAX_PATH_LEN];
    snprintf(file_path, sizeof(file_path), "%s/.cboot", dir);

    FILE *f = fopen(file_path, "w");
    if (!f) {
        fprintf(stderr, "generator: 无法创建 '%s'\n", file_path);
        return;
    }

    fprintf(f, "# CBoot minimal file for module %s\n", mod->name);
    fprintf(f, "mod %s\n", mod->name);

    if (mod->comment) {
        fprintf(f, "cmt \"%s\"\n", mod->comment);
    }

    /* Write all children */
    for (int i = 0; i < mod->child_count; i++) {
        Domain *child = mod->children[i];

        switch (child->type) {
            case DOMAIN_MODULE:
                /* Will be handled by recursive generate */
                break;
            case DOMAIN_FUNCTION: {
                FunctionDomain *func = (FunctionDomain *)child;
                fprintf(f, "void %s %s\n", child->name, func->return_type);
                if (func->mode == API_MODE_API) fprintf(f, "mode api\n");
                if (child->comment) fprintf(f, "cmt \"%s\"\n", child->comment);
                if (func->value) fprintf(f, "value \"%s\"\n", func->value);
                /* Parameters */
                for (int j = 0; j < child->child_count; j++) {
                    Domain *param = child->children[j];
                    if (param->type == DOMAIN_MEMBER) {
                        MemberDomain *mb = (MemberDomain *)param;
                        fprintf(f, "mem %s %s\n", param->name, mb->type);
                        if (param->comment) fprintf(f, "cmt \"%s\"\n", param->comment);
                    }
                }
                break;
            }
            case DOMAIN_STRUCT: {
                fprintf(f, "struct %s\n", child->name);
                if (is_api(child)) fprintf(f, "mode api\n");
                if (child->comment) fprintf(f, "cmt \"%s\"\n", child->comment);
                for (int j = 0; j < child->child_count; j++) {
                    Domain *mem = child->children[j];
                    if (mem->type == DOMAIN_MEMBER) {
                        MemberDomain *mb = (MemberDomain *)mem;
                        fprintf(f, "mem %s %s\n", mem->name, mb->type);
                        if (mem->comment) fprintf(f, "cmt \"%s\"\n", mem->comment);
                    }
                }
                break;
            }
            case DOMAIN_TYPE: {
                TypeDomain *td = (TypeDomain *)child;
                fprintf(f, "type %s\n", child->name);
                if (td->mode == TYPE_MODE_RENAME) fprintf(f, "mode rename\n");
                else if (td->mode == TYPE_MODE_API_RENAME) fprintf(f, "mode api rename\n");
                else if (td->mode == TYPE_MODE_API_STRUCT) fprintf(f, "mode api struct\n");
                if (child->comment) fprintf(f, "cmt \"%s\"\n", child->comment);
                if (td->value) fprintf(f, "value \"%s\"\n", td->value);
                for (int j = 0; j < child->child_count; j++) {
                    Domain *mem = child->children[j];
                    if (mem->type == DOMAIN_MEMBER) {
                        MemberDomain *mb = (MemberDomain *)mem;
                        fprintf(f, "mem %s %s\n", mem->name, mb->type);
                        if (mem->comment) fprintf(f, "cmt \"%s\"\n", mem->comment);
                    }
                }
                break;
            }
            case DOMAIN_MACRO: {
                MacroDomain *md = (MacroDomain *)child;
                fprintf(f, "def %s\n", child->name);
                if (md->mode == API_MODE_API) fprintf(f, "mode api\n");
                if (child->comment) fprintf(f, "cmt \"%s\"\n", child->comment);
                if (md->value) fprintf(f, "value \"%s\"\n", md->value);
                break;
            }
            case DOMAIN_VARIABLE: {
                VariableDomain *v = (VariableDomain *)child;
                fprintf(f, "var %s %s\n", child->name, v->type);
                if (v->mode == VAR_MODE_STATIC) fprintf(f, "mode static\n");
                if (child->comment) fprintf(f, "cmt \"%s\"\n", child->comment);
                if (v->value) fprintf(f, "value \"%s\"\n", v->value);
                break;
            }
            case DOMAIN_MEMBER:
                /* Members are always children of their parent, not standalone */
                break;
        }
    }

    fprintf(f, "\n# End of .cboot for %s\n", mod->name);
    fclose(f);
}

/* ================================================================== */
/* generate_top_cmake - write top-level CMakeLists.txt                 */
/* ================================================================== */

static void generate_top_cmake(Project *proj) {
    FILE *f = fopen("CMakeLists.txt", "w");
    if (!f) {
        fprintf(stderr, "generator: 无法创建 CMakeLists.txt\n");
        return;
    }

    fprintf(f, "cmake_minimum_required(VERSION 3.10)\n");
    fprintf(f, "project(%s C)\n\n", proj->name);
    fprintf(f, "set(CMAKE_C_STANDARD 11)\n");
    fprintf(f, "set(CMAKE_C_STANDARD_REQUIRED ON)\n\n");

    /* Collect all modules */
    Domain **modules = NULL;
    int mod_count = 0, mod_capacity = 0;
    collect_all_modules(proj->root, &modules, &mod_count, &mod_capacity);

    /* Add subdirectories for each module under src */
    fprintf(f, "# Module subdirectories\n");
    for (int i = 0; i < mod_count; i++) {
        /* Build path relative to src */
        char rel_path[MAX_PATH_LEN] = "src";
        Domain *ancestors[64];
        int depth = 0;
        Domain *d = modules[i];
        while (d && d->parent && d != proj->root) {
            ancestors[depth++] = d;
            d = d->parent;
        }
        /* Reverse to get path from root */
        for (int j = depth - 1; j >= 0; j--) {
            if (ancestors[j]->type == DOMAIN_MODULE && ancestors[j] != proj->root) {
                strcat(rel_path, "/");
                strcat(rel_path, ancestors[j]->name);
            }
        }
        fprintf(f, "add_subdirectory(%s)\n", rel_path);
    }

    free(modules);
    fprintf(f, "\n");

    /* Main executable */
    fprintf(f, "add_executable(%s src/main.c)\n\n", proj->name);

    /* Include directories */
    fprintf(f, "target_include_directories(%s PRIVATE ${CMAKE_SOURCE_DIR}/src)\n", proj->name);

    fprintf(f, "\n# End of CMakeLists.txt\n");
    fclose(f);
}

/* ================================================================== */
/* generate_top_main - write main.c entry point                        */
/* ================================================================== */

static void generate_top_main(Project *proj) {
    FILE *f = fopen("src/main.c", "w");
    if (!f) {
        fprintf(stderr, "generator: 无法创建 src/main.c\n");
        return;
    }

    fprintf(f, "/* main.c - CBoot generated entry point */\n\n");
    fprintf(f, "#include <stdio.h>\n\n");

    /* Include all top-level module headers */
    for (int i = 0; i < proj->root->child_count; i++) {
        Domain *child = proj->root->children[i];
        if (child->type == DOMAIN_MODULE) {
            fprintf(f, "#include \"%s/%s.h\"\n", child->name, child->name);
        }
    }

    fprintf(f, "\nint main(int argc, char **argv) {\n");
    fprintf(f, "    (void)argc;\n");
    fprintf(f, "    (void)argv;\n");
    fprintf(f, "    printf(\"Hello from %s!\\\\n\");\n", proj->name);
    fprintf(f, "    return 0;\n");
    fprintf(f, "}\n");

    fclose(f);
}