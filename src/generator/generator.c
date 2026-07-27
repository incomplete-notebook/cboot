/*
 * CBoot - C Project Bootstrapping Tool v2.0
 * Code generator (v3.0 - no-src-dir, hierarchical CMake, compiler modes)
 *
 * 生成规则:
 *  - 项目根即为源码根目录，不再有 src/ 子目录
 *  - 每个 mod 一个目录，同级存放 .c/.h/CMakeLists.txt
 *  - .h 只放 api 项的声明
 *  - .c 中: API 项用外部链接，非 API 项用 static 链接（避免符号冲突）
 *  - CMakeLists.txt 层级化:
 *      顶层: 收集所有模块源文件
 *      模块级: 处理自己的 .c，通过 add_subdirectory 引入子模块
 *      子模块独立编译，可被多次 im 导入但只编译一次
 *  - compiler 模式:
 *      normal: 普通 .o，被父级收集
 *      exe:    生成可执行文件，模块代码放在 main.c
 *      sl:     静态库
 *      dl:     动态库
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
static void generate_project_cboot(Project *proj);
static void write_im_deps_includes(Project *proj, Domain *mod, FILE *f);
static Domain *find_exe_module(Project *proj);

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

static const char *compiler_mode_str(CompilerMode c) {
    switch (c) {
        case COMPILER_EXE:    return "exe";
        case COMPILER_SL:     return "sl";
        case COMPILER_DL:     return "dl";
        case COMPILER_NORMAL: return "normal";
        default:              return "normal";
    }
}

/* ================================================================== */
/* generate_project - top-level code generator                         */
/* ================================================================== */

int generate_project(Project *proj) {
    if (!proj || !proj->root) return -1;

    /* Project root IS the source root - no src/ directory */

    /* Root module is just a container - generate its child modules directly */
    for (int i = 0; i < proj->root->child_count; i++) {
        Domain *child = proj->root->children[i];
        if (child->type == DOMAIN_MODULE) {
            generate_module(child, ".");
        }
    }

    /* Generate top-level CMakeLists.txt */
    generate_top_cmake(proj);

    /* Generate top-level main.c if no exe module exists */
    generate_top_main(proj);

    /* Generate .cboot for the project */
    generate_project_cboot(proj);

    proj->has_generated = 1;
    return 0;
}

/* ================================================================== */
/* generate_module - recursively generate code for a module            */
/* ================================================================== */

static void generate_module(Domain *mod, const char *parent_dir) {
    if (!mod || mod->type != DOMAIN_MODULE) return;

    char mod_dir[MAX_PATH_LEN];

    if (strcmp(parent_dir, ".") == 0) {
        snprintf(mod_dir, sizeof(mod_dir), "%s", mod->name);
    } else {
        snprintf(mod_dir, sizeof(mod_dir), "%s/%s", parent_dir, mod->name);
    }
    ensure_dir(mod_dir);

    ModuleDomain *md = (ModuleDomain *)mod;
    int is_external = (md->mode == MOD_MODE_EXTERNAL);

    if (is_external) {
        /* External API-reference module (from im command):
         * Only generate .h file (API declarations), no .c, no CMakeLists.txt */
        generate_mod_h(mod, mod_dir);
        generate_mod_cboot(mod, mod_dir);
        generate_module_docs(mod, mod_dir);
    } else {
        /* Generate .c (or main.c for exe mode) and .h */
        generate_mod_c(mod, mod_dir);
        generate_mod_h(mod, mod_dir);

        /* Generate CMakeLists.txt */
        generate_mod_cmake(mod, mod_dir);

        /* Generate minimal .cboot */
        generate_mod_cboot(mod, mod_dir);

        /* Generate markdown docs */
        generate_module_docs(mod, mod_dir);
    }

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

    fprintf(f, "#include \"%s.h\"\n", mod->name);

    /* Include parent module's header */
    if (mod->parent && mod->parent->type == DOMAIN_MODULE && mod->parent->parent != NULL) {
        fprintf(f, "#include \"../%s.h\"\n", mod->parent->name);
    }

    /* Include dependencies */
    for (int i = 0; i < md->dep_count; i++) {
        fprintf(f, "#include \"%s.h\"\n", md->dependencies[i]);
    }

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
            /* Non-API structs: use static to avoid symbol collision */
            if (!is_api(child)) {
                fprintf(f, "/* non-API struct - static to avoid symbol collision */\n");
            }
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
                    fprintf(f, "typedef int %s;\n", child->name);
                }
            } else {
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
    for (int i = 0; i < mod->child_count; i++) {
        Domain *child = mod->children[i];
        if (child->type == DOMAIN_VARIABLE) {
            VariableDomain *v = (VariableDomain *)child;
            if (child->comment) fprintf(f, "// %s\n", child->comment);
            /* Non-API variables: always static to avoid symbol collision */
            if (v->mode == VAR_MODE_STATIC || !is_api(child)) {
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
        int api = is_api(child);

        if (child->comment) fprintf(f, "// %s\n", child->comment);
        if (func->value) fprintf(f, "// 业务逻辑: %s\n", func->value);

        /* Non-API functions: static to prevent linker conflicts with submodule API items */
        if (!api) {
            fprintf(f, "static ");
        }
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

        if (func->code) {
            fprintf(f, "%s\n", func->code);
        } else {
            fprintf(f, "    // TODO: implement\n");
        }

        fprintf(f, "}\n\n");
    }
}

static void generate_mod_c(Domain *mod, const char *dir) {
    ModuleDomain *md = (ModuleDomain *)mod;
    CompilerMode cmode = md->compiler;

    /* For exe mode, the implementation goes into main.c */
    /* For other modes, it goes into <mod>.c */
    char file_path[MAX_PATH_LEN];
    if (cmode == COMPILER_EXE) {
        snprintf(file_path, sizeof(file_path), "%s/main.c", dir);
    } else {
        snprintf(file_path, sizeof(file_path), "%s/%s.c", dir, mod->name);
    }

    FILE *f = fopen(file_path, "w");
    if (!f) {
        fprintf(stderr, "generator: 无法创建 '%s'\n", file_path);
        return;
    }

    fprintf(f, "/* %s.c - CBoot generated (compiler: %s) */\n", mod->name, compiler_mode_str(cmode));
    fprintf(f, "/* Module: %s */\n\n", mod->name);

    /* For dl mode, add export macro */
    if (cmode == COMPILER_DL) {
        fprintf(f, "#ifdef _WIN32\n");
        fprintf(f, "#  ifdef %s_EXPORTS\n", mod->name);
        fprintf(f, "#    define %s_API __declspec(dllexport)\n", mod->name);
        fprintf(f, "#  else\n");
        fprintf(f, "#    define %s_API __declspec(dllimport)\n", mod->name);
        fprintf(f, "#  endif\n");
        fprintf(f, "#else\n");
        fprintf(f, "#  define %s_API\n", mod->name);
        fprintf(f, "#endif\n\n");
    }

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

    char guard[MAX_NAME_LEN * 2];
    snprintf(guard, sizeof(guard), "%s_H", mod->name);
    for (int i = 0; guard[i]; i++)
        guard[i] = (char)toupper((unsigned char)guard[i]);

    fprintf(f, "/* %s.h - CBoot generated (API declarations only) */\n", mod->name);
    fprintf(f, "#ifndef %s\n", guard);
    fprintf(f, "#define %s\n\n", guard);

    /* dllexport/dllimport macro */
    ModuleDomain *md = (ModuleDomain *)mod;
    if (md->compiler == COMPILER_DL) {
        fprintf(f, "#ifdef _WIN32\n");
        fprintf(f, "#  ifdef %s_EXPORTS\n", mod->name);
        fprintf(f, "#    define %s_API __declspec(dllexport)\n", mod->name);
        fprintf(f, "#  else\n");
        fprintf(f, "#    define %s_API __declspec(dllimport)\n", mod->name);
        fprintf(f, "#  endif\n");
        fprintf(f, "#else\n");
        fprintf(f, "#  define %s_API\n", mod->name);
        fprintf(f, "#endif\n\n");
    }

    fprintf(f, "#include <stdio.h>\n");
    fprintf(f, "#include <stdlib.h>\n");
    fprintf(f, "#include <string.h>\n\n");

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

    /* Forward declarations for struct types used in API function params
     * and return types but not defined in this module's API.
     * This must come BEFORE the function declarations to avoid the
     * "struct declared inside parameter list" warning/error. */
    {
        char fwd_decls[32][MAX_NAME_LEN];
        int fwd_count = 0;

        /* Collect candidate struct names from return types and params */
        for (int i = 0; i < mod->child_count; i++) {
            Domain *child = mod->children[i];
            if (child->type != DOMAIN_FUNCTION || !is_api(child)) continue;
            FunctionDomain *func = (FunctionDomain *)child;

            /* Gather candidate type strings for this function */
            const char *cand_types[64];
            int cand_count = 0;
            if (func->return_type && cand_count < 64)
                cand_types[cand_count++] = func->return_type;
            for (int j = 0; j < child->child_count && cand_count < 64; j++) {
                Domain *param = child->children[j];
                if (param->type == DOMAIN_MEMBER)
                    cand_types[cand_count++] = ((MemberDomain *)param)->type;
            }

            for (int c = 0; c < cand_count; c++) {
                const char *ts = cand_types[c];
                if (!ts || strncmp(ts, "struct ", 7) != 0) continue;

                char sname[MAX_NAME_LEN];
                strncpy(sname, ts + 7, MAX_NAME_LEN - 1);
                sname[MAX_NAME_LEN - 1] = '\0';
                int slen = (int)strlen(sname);
                while (slen > 0 && (sname[slen-1] == '*' || isspace((unsigned char)sname[slen-1])))
                    sname[--slen] = '\0';
                if (slen == 0) continue;

                /* Skip if already collected */
                int found = 0;
                for (int k = 0; k < fwd_count; k++) {
                    if (strcmp(fwd_decls[k], sname) == 0) { found = 1; break; }
                }
                if (found) continue;

                /* Skip if defined as an API struct/type in this module */
                int defined_here = 0;
                for (int k = 0; k < mod->child_count; k++) {
                    Domain *tc = mod->children[k];
                    if ((tc->type == DOMAIN_STRUCT || tc->type == DOMAIN_TYPE) &&
                        is_api(tc) && strcmp(tc->name, sname) == 0) {
                        defined_here = 1; break;
                    }
                }
                if (defined_here) continue;

                if (fwd_count < 32) {
                    strncpy(fwd_decls[fwd_count], sname, MAX_NAME_LEN - 1);
                    fwd_decls[fwd_count][MAX_NAME_LEN - 1] = '\0';
                    fwd_count++;
                }
            }
        }

        if (fwd_count > 0) {
            fprintf(f, "/* Forward declarations for struct types used in API signatures */\n");
            for (int k = 0; k < fwd_count; k++) {
                fprintf(f, "typedef struct %s %s;\n", fwd_decls[k], fwd_decls[k]);
            }
            fprintf(f, "\n");
        }
    }

    /* API function declarations */
    for (int i = 0; i < mod->child_count; i++) {
        Domain *child = mod->children[i];
        if (child->type == DOMAIN_FUNCTION && is_api(child)) {
            FunctionDomain *func = (FunctionDomain *)child;
            if (child->comment) fprintf(f, "// %s\n", child->comment);
            if (md->compiler == COMPILER_DL) {
                fprintf(f, "%s_API %s %s(", mod->name, func->return_type, child->name);
            } else {
                fprintf(f, "%s %s(", func->return_type, child->name);
            }
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
/* im dependency helpers                                              */
/* ================================================================== */

static void write_im_deps_includes(Project *proj, Domain *mod, FILE *f)
{
    if (!proj || !mod || !f) return;

    char *mod_path = domain_get_path(mod);
    int wrote_header = 0;

    for (int i = 0; i < proj->dep_count; i++) {
        if (!str_eq(proj->dependencies[i].importer, mod_path)) continue;

        const char *src_path = proj->dependencies[i].source;
        if (!src_path) continue;

        /* src_path is like "/modulename" or "/a/b/modulename"
         * From the current module's directory, relative to source root:
         * need to figure out the relative path */
        char rel_path[MAX_PATH_LEN];

        /* Calculate depth of this module */
        int depth = 0;
        Domain *p = mod;
        while (p && p->parent) { depth++; p = p->parent; }

        /* For top-level modules: depth=1 (parent is root)
         * The source path starts with "/" - we need relative from current dir */
        /* The source module directory is <project_root>/<src_path_without_leading_slash>
         * From <project_root>/<mod_path>, relative path = ../<src_path_without_leading_slash> */

        /* Count how many ../ to go up: depth levels (module's depth in tree) */
        int up_count = depth;
        int ri = 0;
        for (int u = 0; u < up_count; u++) {
            rel_path[ri++] = '.';
            rel_path[ri++] = '.';
            if (u < up_count - 1) rel_path[ri++] = '/';
        }

        /* Append source path (strip leading /) */
        if (src_path[0] == '/') {
            strcpy(rel_path + ri, src_path + 1);
        } else {
            strcpy(rel_path + ri, src_path);
        }

        if (!wrote_header) {
            fprintf(f, "\n# im 导入的 API 依赖 include 目录\n");
            wrote_header = 1;
        }
        fprintf(f, "target_include_directories(%s PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/%s)\n",
                mod->name, rel_path);
    }

    free(mod_path);
}

/* ================================================================== */
/* generate_mod_cmake - write CMakeLists.txt for a module              */
/* ================================================================== */

static void generate_mod_cmake(Domain *mod, const char *dir) {
    ModuleDomain *md = (ModuleDomain *)mod;
    CompilerMode cmode = md->compiler;

    char file_path[MAX_PATH_LEN];
    snprintf(file_path, sizeof(file_path), "%s/CMakeLists.txt", dir);

    FILE *f = fopen(file_path, "w");
    if (!f) {
        fprintf(stderr, "generator: 无法创建 '%s'\n", file_path);
        return;
    }

    fprintf(f, "# CMakeLists.txt for module %s (CBoot generated, compiler: %s)\n\n",
            mod->name, compiler_mode_str(cmode));

    if (cmode == COMPILER_EXE) {
        /* exe mode: module code goes into main.c (not <mod>.c) */
        fprintf(f, "set(%s_SOURCES\n", mod->name);
        fprintf(f, "    ${CMAKE_CURRENT_SOURCE_DIR}/main.c\n");
        fprintf(f, ")\n\n");

        /* Collect sub-module sources */
        int has_subdirs = 0;
        for (int i = 0; i < mod->child_count; i++) {
            if (mod->children[i]->type == DOMAIN_MODULE) {
                ModuleDomain *child_md = (ModuleDomain *)mod->children[i];
                if (child_md->mode == MOD_MODE_EXTERNAL) continue;
                if (!has_subdirs) {
                    fprintf(f, "# Sub-modules\n");
                    has_subdirs = 1;
                }
                fprintf(f, "add_subdirectory(%s)\n", mod->children[i]->name);
                fprintf(f, "list(APPEND %s_SOURCES ${%s_SOURCES})\n",
                        mod->name, mod->children[i]->name);
            }
        }
        fprintf(f, "\n# Executable target\n");
        fprintf(f, "add_executable(%s ${%s_SOURCES})\n\n", mod->name, mod->name);
        fprintf(f, "target_include_directories(%s PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})\n", mod->name);

    } else if (cmode == COMPILER_SL) {
        /* Static library */
        fprintf(f, "set(%s_SOURCES\n", mod->name);
        fprintf(f, "    ${CMAKE_CURRENT_SOURCE_DIR}/%s.c\n", mod->name);
        fprintf(f, ")\n\n");

        int has_subdirs = 0;
        for (int i = 0; i < mod->child_count; i++) {
            if (mod->children[i]->type == DOMAIN_MODULE) {
                ModuleDomain *child_md = (ModuleDomain *)mod->children[i];
                if (child_md->mode == MOD_MODE_EXTERNAL) continue;
                if (!has_subdirs) {
                    fprintf(f, "# Sub-modules\n");
                    has_subdirs = 1;
                }
                fprintf(f, "add_subdirectory(%s)\n", mod->children[i]->name);
                fprintf(f, "list(APPEND %s_SOURCES ${%s_SOURCES})\n",
                        mod->name, mod->children[i]->name);
            }
        }
        fprintf(f, "\n# Static library target\n");
        fprintf(f, "add_library(%s STATIC ${%s_SOURCES})\n\n", mod->name, mod->name);
        fprintf(f, "target_include_directories(%s PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})\n", mod->name);

    } else if (cmode == COMPILER_DL) {
        /* Dynamic library */
        fprintf(f, "set(%s_SOURCES\n", mod->name);
        fprintf(f, "    ${CMAKE_CURRENT_SOURCE_DIR}/%s.c\n", mod->name);
        fprintf(f, ")\n\n");

        int has_subdirs = 0;
        for (int i = 0; i < mod->child_count; i++) {
            if (mod->children[i]->type == DOMAIN_MODULE) {
                ModuleDomain *child_md = (ModuleDomain *)mod->children[i];
                if (child_md->mode == MOD_MODE_EXTERNAL) continue;
                if (!has_subdirs) {
                    fprintf(f, "# Sub-modules\n");
                    has_subdirs = 1;
                }
                fprintf(f, "add_subdirectory(%s)\n", mod->children[i]->name);
                fprintf(f, "list(APPEND %s_SOURCES ${%s_SOURCES})\n",
                        mod->name, mod->children[i]->name);
            }
        }
        fprintf(f, "\n# Dynamic library target\n");
        fprintf(f, "add_library(%s SHARED ${%s_SOURCES})\n", mod->name, mod->name);
        fprintf(f, "target_include_directories(%s PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})\n", mod->name);
        fprintf(f, "target_compile_definitions(%s PRIVATE %s_EXPORTS)\n\n", mod->name, mod->name);

    } else {
        /* COMPILER_NORMAL: just collect sources, no standalone target */
        fprintf(f, "set(%s_SOURCES\n", mod->name);
        fprintf(f, "    ${CMAKE_CURRENT_SOURCE_DIR}/%s.c\n", mod->name);
        fprintf(f, ")\n\n");

        int has_subdirs = 0;
        for (int i = 0; i < mod->child_count; i++) {
            if (mod->children[i]->type == DOMAIN_MODULE) {
                ModuleDomain *child_md = (ModuleDomain *)mod->children[i];
                if (child_md->mode == MOD_MODE_EXTERNAL) continue;
                if (!has_subdirs) {
                    fprintf(f, "# Sub-modules\n");
                    has_subdirs = 1;
                }
                fprintf(f, "add_subdirectory(%s)\n", mod->children[i]->name);
                fprintf(f, "list(APPEND %s_SOURCES ${%s_SOURCES})\n",
                        mod->name, mod->children[i]->name);
            }
        }
        fprintf(f, "\n# Export sources to parent scope\n");
        fprintf(f, "set(%s_SOURCES ${%s_SOURCES} PARENT_SCOPE)\n\n", mod->name, mod->name);
    }

    /* im dependency include paths */
    write_im_deps_includes(g_proj, mod, f);

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

    ModuleDomain *md = (ModuleDomain *)mod;

    /* 模块级 .cboot 假设已在模块作用域内执行（由父级 .cboot 引用创建并 cd 进入）*/
    fprintf(f, "# CBoot minimal file for module %s\n", mod->name);
    fprintf(f, "# 此文件在模块 %s 作用域下执行，不需要 mod %s\n\n", mod->name, mod->name);

    if (mod->comment) {
        fprintf(f, "cmt \"%s\"\n", mod->comment);
    }

    if (md->mode == MOD_MODE_EXTERNAL) {
        fprintf(f, "mode external\n");
    }

    /* 编译器模式始终输出（包括 normal，确保信息完整）*/
    fprintf(f, "cmode %s\n", compiler_mode_str(md->compiler));

    for (int i = 0; i < mod->child_count; i++) {
        Domain *child = mod->children[i];

        switch (child->type) {
            case DOMAIN_MODULE:
                /* 子模块通过 .cboot 引用处理，不在此重复定义 */
                break;
            case DOMAIN_FUNCTION: {
                FunctionDomain *func = (FunctionDomain *)child;
                fprintf(f, "void %s %s\n", child->name, func->return_type);
                fprintf(f, "cd %s\n", child->name);
                if (func->mode == API_MODE_API) fprintf(f, "mode api\n");
                if (child->comment) fprintf(f, "cmt \"%s\"\n", child->comment);
                if (func->value) fprintf(f, "value \"%s\"\n", func->value);
                for (int j = 0; j < child->child_count; j++) {
                    Domain *param = child->children[j];
                    if (param->type == DOMAIN_MEMBER) {
                        MemberDomain *mb = (MemberDomain *)param;
                        fprintf(f, "mem %s %s\n", param->name, mb->type);
                        if (param->comment) fprintf(f, "cmt \"%s\"\n", param->comment);
                    }
                }
                fprintf(f, "cd ..\n");
                break;
            }
            case DOMAIN_STRUCT: {
                fprintf(f, "struct %s\n", child->name);
                fprintf(f, "cd %s\n", child->name);
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
                fprintf(f, "cd ..\n");
                break;
            }
            case DOMAIN_TYPE: {
                TypeDomain *td = (TypeDomain *)child;
                fprintf(f, "type %s\n", child->name);
                fprintf(f, "cd %s\n", child->name);
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
                fprintf(f, "cd ..\n");
                break;
            }
            case DOMAIN_MACRO: {
                MacroDomain *m = (MacroDomain *)child;
                fprintf(f, "def %s\n", child->name);
                fprintf(f, "cd %s\n", child->name);
                if (m->mode == API_MODE_API) fprintf(f, "mode api\n");
                if (child->comment) fprintf(f, "cmt \"%s\"\n", child->comment);
                if (m->value) fprintf(f, "value \"%s\"\n", m->value);
                fprintf(f, "cd ..\n");
                break;
            }
            case DOMAIN_VARIABLE: {
                VariableDomain *v = (VariableDomain *)child;
                fprintf(f, "var %s %s\n", child->name, v->type);
                fprintf(f, "cd %s\n", child->name);
                if (v->mode == VAR_MODE_STATIC) fprintf(f, "mode static\n");
                if (child->comment) fprintf(f, "cmt \"%s\"\n", child->comment);
                if (v->value) fprintf(f, "value \"%s\"\n", v->value);
                fprintf(f, "cd ..\n");
                break;
            }
            case DOMAIN_MEMBER:
                break;
        }
    }

    /* 子模块引用：每个子模块通过 <name>/.cboot 引用 */
    int has_submods = 0;
    for (int i = 0; i < mod->child_count; i++) {
        Domain *child = mod->children[i];
        if (child->type == DOMAIN_MODULE) {
            if (!has_submods) {
                fprintf(f, "\n# 子模块引用\n");
                has_submods = 1;
            }
            fprintf(f, "%s/.cboot\n", child->name);
        }
    }

    fprintf(f, "\n# End of .cboot for %s\n", mod->name);
    fclose(f);
}

/* ================================================================== */
/* find_exe_module - find the first exe-mode module                   */
/* ================================================================== */

static Domain *find_exe_module(Project *proj) {
    for (int i = 0; i < proj->root->child_count; i++) {
        Domain *child = proj->root->children[i];
        if (child->type == DOMAIN_MODULE) {
            ModuleDomain *md = (ModuleDomain *)child;
            if (md->compiler == COMPILER_EXE) return child;
        }
    }
    return NULL;
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

    /* Check if there's an exe module */
    Domain *exe_mod = find_exe_module(proj);

    if (exe_mod) {
        /* exe module handles its own compilation via add_subdirectory */
        fprintf(f, "# Executable module\n");
        fprintf(f, "add_subdirectory(%s)\n", exe_mod->name);

        /* Also add non-executable modules */
        for (int i = 0; i < proj->root->child_count; i++) {
            Domain *child = proj->root->children[i];
            if (child->type == DOMAIN_MODULE && child != exe_mod) {
                ModuleDomain *md = (ModuleDomain *)child;
                if (md->mode == MOD_MODE_EXTERNAL) continue;
                if (md->compiler == COMPILER_EXE) continue; /* already handled */
                fprintf(f, "add_subdirectory(%s)\n", child->name);
            }
        }

        /* Add normal module sources to the executable target */
        fprintf(f, "\n# Add normal module sources to executable\n");
        for (int i = 0; i < proj->root->child_count; i++) {
            Domain *child = proj->root->children[i];
            if (child->type == DOMAIN_MODULE && child != exe_mod) {
                ModuleDomain *md = (ModuleDomain *)child;
                if (md->mode == MOD_MODE_EXTERNAL) continue;
                if (md->compiler == COMPILER_NORMAL) {
                    fprintf(f, "target_sources(%s PRIVATE ${%s_SOURCES})\n",
                            exe_mod->name, child->name);
                    fprintf(f, "target_include_directories(%s PRIVATE ${CMAKE_SOURCE_DIR}/%s)\n",
                            exe_mod->name, child->name);
                }
            }
        }

        /* For sl/dl modules that are not exe, link them into the exe */
        fprintf(f, "\n# Link library modules into executable\n");
        for (int i = 0; i < proj->root->child_count; i++) {
            Domain *child = proj->root->children[i];
            if (child->type == DOMAIN_MODULE && child != exe_mod) {
                ModuleDomain *md = (ModuleDomain *)child;
                if (md->compiler == COMPILER_SL || md->compiler == COMPILER_DL) {
                    fprintf(f, "target_link_libraries(%s PRIVATE %s)\n", exe_mod->name, child->name);
                }
            }
        }

        fprintf(f, "\ntarget_include_directories(%s PRIVATE ${CMAKE_SOURCE_DIR})\n", exe_mod->name);
    } else {
        /* No exe module: collect all sources and build default executable */
        fprintf(f, "# Module subdirectories (hierarchical)\n");
        for (int i = 0; i < proj->root->child_count; i++) {
            Domain *child = proj->root->children[i];
            if (child->type == DOMAIN_MODULE) {
                ModuleDomain *md = (ModuleDomain *)child;
                if (md->mode == MOD_MODE_EXTERNAL) continue;
                fprintf(f, "add_subdirectory(%s)\n", child->name);
            }
        }

        fprintf(f, "\n# Collect all module sources\n");
        fprintf(f, "set(ALL_SOURCES main.c)\n");
        for (int i = 0; i < proj->root->child_count; i++) {
            Domain *child = proj->root->children[i];
            if (child->type == DOMAIN_MODULE) {
                ModuleDomain *md = (ModuleDomain *)child;
                if (md->mode == MOD_MODE_EXTERNAL) continue;
                if (md->compiler == COMPILER_NORMAL) {
                    fprintf(f, "list(APPEND ALL_SOURCES ${%s_SOURCES})\n", child->name);
                }
            }
        }

        fprintf(f, "\n# Main executable\n");
        fprintf(f, "add_executable(%s ${ALL_SOURCES})\n\n", proj->name);
        fprintf(f, "target_include_directories(%s PRIVATE ${CMAKE_SOURCE_DIR})\n", proj->name);
    }

    fprintf(f, "\n# End of CMakeLists.txt\n");
    fclose(f);
}

/* ================================================================== */
/* generate_top_main - write main.c entry point                        */
/* ================================================================== */

static void generate_top_main(Project *proj) {
    /* Check if there's an exe module that already has its own main.c */
    Domain *exe_mod = find_exe_module(proj);
    if (exe_mod) {
        /* The exe module's main.c is generated by generate_module */
        return;
    }

    FILE *f = fopen("main.c", "w");
    if (!f) {
        fprintf(stderr, "generator: 无法创建 main.c\n");
        return;
    }

    fprintf(f, "/* main.c - CBoot generated entry point */\n\n");
    fprintf(f, "#include <stdio.h>\n");
    fprintf(f, "#include <stdlib.h>\n\n");

    for (int i = 0; i < proj->root->child_count; i++) {
        Domain *child = proj->root->children[i];
        if (child->type == DOMAIN_MODULE) {
            fprintf(f, "#include \"%s/%s.h\"\n", child->name, child->name);
        }
    }

    fprintf(f, "\nint main(int argc, char **argv) {\n");
    fprintf(f, "    (void)argc;\n");
    fprintf(f, "    (void)argv;\n");
    if (proj->root->comment) {
        fprintf(f, "    /* %s */\n", proj->root->comment);
    }
    fprintf(f, "    printf(\"Hello from %s!\\n\");\n", proj->name);
    fprintf(f, "    return 0;\n");
    fprintf(f, "}\n");

    fclose(f);
}

/* ================================================================== */
/* generate_project_cboot - write project-level .cboot                 */
/* ================================================================== */

static void generate_project_cboot(Project *proj) {
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s.cboot", proj->name);

    FILE *f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "generator: 无法创建 %s\n", path);
        return;
    }

    fprintf(f, "# CBoot project file for %s\n", proj->name);
    fprintf(f, "# 通过引用各顶级模块的 .cboot 文件来构建整个项目\n\n");
    fprintf(f, "project %s\n\n", proj->name);

    if (proj->dep_count > 0) {
        fprintf(f, "# im 依赖链记录:\n");
        for (int i = 0; i < proj->dep_count; i++) {
            fprintf(f, "# im-dep: %s -> %s",
                    proj->dependencies[i].importer,
                    proj->dependencies[i].source);
            if (proj->dependencies[i].cboot_file)
                fprintf(f, " (%s)", proj->dependencies[i].cboot_file);
            fprintf(f, "\n");
        }
        fprintf(f, "\n");
    }

    if (proj->import_count > 0) {
        fprintf(f, "# in 完整项目导入记录:\n");
        for (int i = 0; i < proj->import_count; i++) {
            fprintf(f, "# in-import: %s\n", proj->imported_projects[i]);
        }
        fprintf(f, "\n");
    }

    /* 顶级模块通过 <mod>/.cboot 引用，不在此重复定义 */
    fprintf(f, "# 顶级模块引用\n");
    for (int i = 0; i < proj->root->child_count; i++) {
        Domain *child = proj->root->children[i];
        if (child->type == DOMAIN_MODULE) {
            fprintf(f, "%s/.cboot\n", child->name);
        }
    }

    fprintf(f, "\ngen\n");
    fclose(f);
}
