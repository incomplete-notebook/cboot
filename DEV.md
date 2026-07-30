# cboot 开发文档

本项目主要代码仓是gitee，另有github和gitcode两个镜像，其中github实时同步，gitcode定期同步。

## 模块概览

| 模块 | 说明 | 路径 |
|------|------|------|
| `domain` | 域数据模型 - 定义所有域类型、域树结构和项目容器 | domain/ |
| `cupdate` | 代码更新模块 - 解析C源码并同步.cboot描述文件 | cupdate/ |
| `commands` | - | commands/ |
| `parser` | - | parser/ |
| `generator` | - | generator/ |
| `docgen` | - | docgen/ |
| `typecheck` | - | typecheck/ |
| `utils` | - | utils/ |
| `main` | - | main/ |

## 模块详细内容

### domain

域数据模型 - 定义所有域类型、域树结构和项目容器

**函数:**

- `Domain* domain_new()`
- `ModuleDomain* module_domain_new()`
- `FunctionDomain* function_domain_new()`
- `StructDomain* struct_domain_new()`
- `TypeDomain* type_domain_new()`
- `MacroDomain* macro_domain_new()`
- `VariableDomain* variable_domain_new()`
- `MemberDomain* member_domain_new()`
- `void domain_add_child()`
- `Domain* domain_find_child()`
- `Domain* domain_find_child_by_type()`
- `void domain_delete()`
- `void domain_remove_child()`
- `Domain* domain_find_nearest_of_type()`
- `Domain* domain_find_in_tree()`
- `char* domain_get_path()`
- `int domain_is_api()`
- `Domain* domain_find_api_in_submodules()`
- `Domain* domain_check_api_name_conflict()`
- `void domain_set_comment()`
- `void domain_set_child_comment()`
- `Comment* find_child_comment()`
- `void domain_set_value()`
- `void domain_set_code()`
- `void domain_set_call()`
- `const char* domain_get_call()`
- `void domain_set_mode()`
- `Project* project_new()`
- `void project_free()`
- `void project_add_dependency()`
- `int project_has_dependency()`
- `int is_builtin_type()`

详细文档见 [domain](domain/DEV.md)。

### cupdate

代码更新模块 - 解析C源码并同步.cboot描述文件

**函数:**

- `int run_project()` — 输出参数：返回警告数量（可为NULL）  
  逻辑: 遍历所有src模式模块，读取.c文件，解析C声明，更新code字段和API定义，重新生成.cboot
- `void result_init()`
- `void cup_free_decl()`
- `void result_free()`
- `void result_add_error()`
- `void result_add_warning()`
- `CUPDecl* result_add_decl()`
- `int parse_source()`
- `char* cup_read_file()`
- `void cup_get_module_prefix()`
- `FunctionDomain* cup_find_function()`
- `StructDomain* cup_find_struct()`
- `TypeDomain* cup_find_type()`
- `MacroDomain* cup_find_macro()`
- `VariableDomain* cup_find_variable()`
- `void cup_sync_function_params()`
- `void cup_sync_struct_members()`
- `void cup_sync_decl()`
- `int run_module()`
- `int cup_update_recursive()`

详细文档见 [cupdate](cupdate/DEV.md)。

### commands

**函数:**

- `ModuleDomain* get_current_module()`
- `int is_in_domain_type()`
- `const char* domain_type_name()`
- `int check_type()`
- `int check_name_dup()`
- `const char* mode_str()`
- `int cmd_mod()`
- `int cmd_struct()`
- `int cmd_type()`
- `int cmd_def()`
- `int cmd_void()`
- `int cmd_var()`
- `int cmd_mem()`
- `int cmd_enum()`
- `int cmd_cmt()`
- `int cmd_value()`
- `int cmd_call()`
- `int cmd_mode()`
- `int cmd_cmode()`
- `int cmd_cd()`
- `int cmd_rm()`
- `void find_recursive()`
- `int cmd_find()`
- `int cmd_ls()`
- `int cmd_mv()`
- `int cmd_exit()`
- `int cmd_gen()`
- `int cmd_update()`
- `void analyze_collect_modules()`
- `int analyze_count_code_lines()`
- `void analyze_extract_functions()`
- `int analyze_tokenize()`
- `int analyze_cyclomatic_complexity()`
- `int analyze_ngram_equal()`
- `int cmd_analyze()`
- `int cmd_adjust()`
- `void copy_api_items()`
- `void copy_api_items_recursive()`
- `int cmd_im()`
- `int cmd_in()`
- `int cmd_res()`

**类型:**

- `AnalyzeMod`
- `AnalyzeFunc`
- `AnalyzeToken`

**宏:**

- `ANALYZE_NGRAM_SIZE` = `8`
- `ANALYZE_MAX_MODS` = `256`
- `ANALYZE_MAX_FUNCS` = `2048`
- `ANALYZE_MAX_TOKENS` = `8192`

详细文档见 [commands](commands/DEV.md)。

### parser

**函数:**

- `int is_cboot_ref()`
- `int exec_cboot_ref()`
- `int try_cboot_ref()`
- `int dispatch_script_line()`
- `int parse_cboot_script()`

详细文档见 [parser](parser/DEV.md)。

### generator

**函数:**

- `void generate_module()`
- `void generate_mod_c()`
- `void generate_mod_h()`
- `void generate_mod_cmake()`
- `void generate_mod_cboot()`
- `void generate_top_cmake()`
- `void generate_top_main()`
- `void generate_project_cboot()`
- `Domain* find_exe_module()`
- `int docgen_is_api()`
- `const char* compiler_mode_str()`
- `void get_module_prefix()`
- `void make_abs_name()`
- `int generate_project()`
- `void write_c_includes()`
- `void write_c_defs()`
- `void write_c_types()`
- `void write_c_variables()`
- `void write_c_functions()`

详细文档见 [generator](generator/DEV.md)。

### docgen

**函数:**

- `void generate_mod_readme()`
- `void generate_mod_api()`
- `void generate_mod_dev()`
- `void generate_project_readme()`
- `void generate_project_dev()`
- `void generate_dependencies_doc()`
- `int is_api()`
- `const char* domain_type_str()`
- `int generate_docs()`
- `void generate_module_docs()`
- `void write_dev_functions()`
- `void write_dev_types()`
- `void write_dev_macros()`

详细文档见 [docgen](docgen/DEV.md)。

### typecheck

**函数:**

- `char* strip_pointer()`
- `char* strip_qualifiers()`
- `int type_checker_is_builtin()`
- `int is_integer_type()`
- `int is_float_type()`
- `int is_accept_any_value()`
- `void type_checker_init()`
- `Domain* type_checker_find_api_type_in_submodules()`
- `int type_checker_validate()`
- `const char* type_checker_resolve_typedef()`
- `int type_checker_validate_value()`

详细文档见 [typecheck](typecheck/DEV.md)。

### utils

**函数:**

- `char** tokenize()`
- `void free_tokens()`
- `char* trim()`
- `char* str_dup()`
- `int str_eq()`
- `int str_startswith()`
- `int parse_c_decl()`
- `char* extract_base_type()`
- `char* extract_type_from_decl()`
- `char* extract_name_from_decl()`
- `int is_valid_identifier()`
- `void ensure_dir()`
- `int file_exists()`
- `void strip_quotes()`

详细文档见 [utils](utils/DEV.md)。

### main

**函数:**

- `void print_usage()`
- `void repl_loop()`
- `int dispatch_command()`
- `int detect_fine_tune_mode()`
- `int main()`
- `int try_complete()`
- `int common_prefix_len()`
- `int do_tab_complete()`

**宏:**

- `CBOOT_HISTORY_MAX` = `100`

详细文档见 [main](main/DEV.md)。

## 依赖链

详见 [DEPENDENCIES.md](DEPENDENCIES.md)。

### API 依赖摘要

```
/cupdate --> /domain
/cupdate/commands --> /cupdate/domain
/cupdate/commands --> /cupdate
/cupdate/parser --> /cupdate/domain
/cupdate/generator --> /cupdate/domain
/cupdate/generator/docgen --> /cupdate/generator/domain
/cupdate/generator/docgen/typecheck --> /cupdate/generator/docgen/domain
```

*Generated by CBoot v0.3.1*
