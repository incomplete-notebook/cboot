# cboot 开发文档

## 模块概览

| 模块 | 说明 | 路径 |
|------|------|------|
| `domain` | 域数据模型 - 定义所有域类型、域树结构和项目容器 | domain/ |
| `cupdate` | 代码更新模块 - 解析C源码并同步.cboot描述文件 | cupdate/ |
| `commands` | 命令处理器 - 实现交互式和批处理模式的所有命令 | commands/ |
| `parser` | 脚本解析器 - 解析.cboot脚本文件为项目定义 | parser/ |
| `generator` | 代码生成器 - 将域树转换为C源代码、CMake构建系统和文档 | generator/ |
| `docgen` | 文档生成器 - 生成README.md/API.md/DEV.md三种文档 | docgen/ |
| `typecheck` | 类型检查器 - 检查域类型一致性和符号冲突 | typecheck/ |
| `utils` | 工具函数 - 通用辅助功能 | utils/ |
| `main` | 可执行入口模块 - CBoot的主程序入口 | main/ |
| `k` | - | k/ |

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
- `void function_add_test_case()` — 添加测试用例到函数域 (链表尾插)
- `void function_clear_test_cases()` — 清除函数域的所有测试用例
- `Project* project_new()`
- `void project_free()`
- `void project_add_dependency()`
- `int project_has_dependency()`
- `int is_builtin_type()`
- `void free_type_fields()`
- `int project_would_cycle()`
- `Domain* core_domain_new()`
- `ModuleDomain* core_module_domain_new()`
- `FunctionDomain* core_function_domain_new()`
- `StructDomain* core_struct_domain_new()`
- `TypeDomain* core_type_domain_new()`
- `MacroDomain* core_macro_domain_new()`
- `VariableDomain* core_variable_domain_new()`
- `MemberDomain* core_member_domain_new()`
- `void core_domain_add_child()`
- `Domain* core_domain_find_child()`
- `Domain* core_domain_find_child_by_type()`
- `void core_free_type_fields()`
- `void core_domain_delete()`
- `void core_domain_remove_child()`
- `Domain* core_domain_find_nearest_of_type()`
- `Domain* core_domain_find_in_tree()`
- `char* core_domain_get_path()`
- `int core_domain_is_api()`
- `Domain* core_domain_find_api_in_submodules()`
- `Domain* core_domain_check_api_name_conflict()`
- `void core_domain_set_comment()`
- `void core_domain_set_child_comment()`
- `Comment* core_find_child_comment()`
- `void core_domain_set_value()`
- `void core_domain_set_code()`
- `void core_domain_set_call()`
- `const char* core_domain_get_call()`
- `void core_domain_set_mode()`
- `Project* core_project_new()`
- `void core_project_free()`
- `void core_project_add_dependency()`
- `int core_project_has_dependency()`
- `int core_project_would_cycle()`
- `int core_is_builtin_type()`

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
- `FunctionDomain* cup_create_new_function()`
- `int cup_replace_str_field()`
- `int cup_sync_func_body()`
- `int cup_update_existing_function()`
- `void cup_sync_function_decl()`
- `void cup_sync_struct_decl()`
- `void cup_sync_typedef_decl()`
- `void cup_sync_macro_decl()`
- `void cup_sync_var_fields()`
- `void cup_sync_variable_decl()`
- `void cup_sync_enum_decl()`
- `int cup_decl_type_matches()`
- `int cup_find_matching_decl()`
- `void cup_detect_and_remove_deleted()`
- `const char* cup_skip_generated_header()`
- `void cup_set_module_code()`
- `void cup_print_diagnostics()`
- `void cup_strip_decl_prefix()`
- `void cup_print_summary()`
- `void cup_free_param_array()`
- `void cup_free_member_array()`
- `void cup_free_str_array()`
- `void cup_str_array_push()`
- `void cup_remove_all_member_children()`
- `void cup_zero_str_slot()`
- `void cup_print_str_array()`

详细文档见 [cupdate](cupdate/DEV.md)。

### commands

命令处理器 - 实现交互式和批处理模式的所有命令

**函数:**

- `int cmd_mod()` — 模块名称  
  逻辑: 验证名称唯一性，创建ModuleDomain并添加到当前作用域
- `int cmd_struct()` — 结构体名称  
  逻辑: 在当前模块作用域中创建StructDomain
- `int cmd_type()` — 类型名称  
  逻辑: 在当前模块作用域中创建TypeDomain
- `int cmd_def()` — 宏名称  
  逻辑: 在当前模块作用域中创建MacroDomain
- `int cmd_void()` — 返回类型  
  逻辑: 验证名称唯一性，创建FunctionDomain并设为当前
- `int cmd_var()` — 变量类型  
  逻辑: 在当前作用域创建VariableDomain
- `int cmd_mem()` — 类型  
  逻辑: 根据当前作用域(函数/结构体/类型)创建MemberDomain，设为当前
- `int cmd_cmt()` — 注释文本  
  逻辑: 设置当前域的注释；若当前是成员/参数，设置后恢复到父作用域
- `int cmd_mode()` — 模式值(api/normal/internal/external/static/rename/struct等)  
  逻辑: 根据当前域类型设置对应模式；模块支持internal/external，函数/结构体/宏支持api/normal
- `int cmd_cmode()` — 编译器模式值(exe/sl/dl/normal)  
  逻辑: 仅对模块有效：exe生成可执行文件(代码放main.c)，sl静态库，dl动态库，normal普通.o
- `int cmd_test()` — 测试用例文本 (输入=>预期 或 自定义代码)  
  逻辑: 添加测试用例: test <输入> => <预期> (纯函数) 或 tcode <<EOF (复杂场景)
- `int cmd_cd()` — 目标路径(支持..和/前缀)  
  逻辑: 解析路径段，逐级在域树中导航
- `int cmd_gen()` — 生成项目代码命令  
  逻辑: 遍历域树，生成.c/.h/CMakeLists.txt/文档，项目根为源码根
- `int cmd_im()` — 源.cboot文件路径  
  逻辑: 解析源文件，提取API模式项，创建external模块，记录依赖链
- `int cmd_in()` — 源.cboot文件路径  
  逻辑: 完整导入源项目作为子模块，包括所有非API项
- `ModuleDomain* get_current_module()`
- `int is_in_domain_type()`
- `const char* domain_type_name()`
- `int check_type()`
- `int check_name_dup()`
- `const char* mode_str()`
- `int cmd_enum()`
- `int cmd_value()`
- `int cmd_call()`
- `int cmd_rm()`
- `void find_recursive()`
- `int cmd_find()`
- `int cmd_ls()`
- `int cmd_mv()`
- `int cmd_exit()`
- `int cmd_update()`
- `int cmd_analyze()`
- `int cmd_adjust()`
- `void copy_api_items()`
- `void copy_api_items_recursive()`
- `int cmd_res()`
- `int is_binary_api_domain()`
- `int binary_api_mode()`
- `int add_member()`
- `int mode_find()`
- `int mode_becoming_api()`
- `int mode_check_api_conflict()`
- `int mode_apply()`
- `int cd_walk()`
- `DomainType parse_type_filter()`
- `void ls_print_type_info()`
- `void ls_print_child()`
- `int mv_resolve_target()`
- `int mv_check_target()`
- `void copy_members()`
- `Domain* clone_api_child()`
- `Domain* im_find_sibling()`
- `int im_internal()`
- `int im_external()`
- `int cmd_analyze_impl()`
- `int cmd_tcode()`

**类型:**

- `ModeMap`

详细文档见 [commands](commands/DEV.md)。

### parser

脚本解析器 - 解析.cboot脚本文件为项目定义

**函数:**

- `int dispatch_script_line()` — 分词数量  
  逻辑: 根据首个分词(命令名)调用对应cmd_函数
- `int is_cboot_ref()`
- `int exec_cboot_ref()`
- `int try_cboot_ref()`
- `int parse_cboot_script()`
- `void join_tokens_from()`
- `int require_args()`
- `int dispatch_build()`
- `int dispatch_modify()`
- `int parse_rm()`
- `int parse_find_flags()`
- `int dispatch_control()`
- `int dispatch_action()`
- `void set_script_dir()`
- `int read_heredoc()`
- `int read_tcode_heredoc()`

**宏:**

- `PARSER_NOT_HANDLED` = ` ( - 1000 )`

详细文档见 [parser](parser/DEV.md)。

### generator

代码生成器 - 将域树转换为C源代码、CMake构建系统和文档

**函数:**

- `int generate_project()` — 输出目录(项目根即为源码根)  
  逻辑: 递归处理所有模块，生成.c/.h/层级化CMake/文档/DEPENDENCIES.md
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
- `void generate_cboot_only_recursive()`
- `int generate_cboot_only()`
- `void write_c_includes()`
- `void write_c_defs()`
- `void write_c_types()`
- `void write_c_variables()`
- `void write_c_functions()`
- `void write_dllexport_macro()`
- `void write_function_params()`
- `void write_h_guard_begin()`
- `void write_h_guard_end()`
- `void write_h_api_macros()`
- `void write_h_api_types()`
- `void write_h_fwd_decls()`
- `void write_h_api_functions()`
- `void cboot_write_header()`
- `void cboot_write_children()`
- `void cboot_write_submodule_refs()`
- `void cmake_write_prebuilt_lib()`
- `void cmake_write_submodule_sources()`
- `void write_struct_members()`
- `void write_func_signature()`
- `void write_func_vars()`
- `void write_api_rename_type()`
- `int extract_struct_name()`
- `int fwd_list_contains()`
- `int defined_in_module()`
- `int collect_func_types()`
- `void fwd_try_add()`
- `void write_mod_mode()`
- `void cboot_write_members()`
- `void cboot_write_function()`
- `void cboot_write_struct()`
- `void cboot_write_type()`
- `void cboot_write_macro()`
- `void cboot_write_variable()`
- `void top_cmake_add_subdirs()`
- `void top_cmake_emit_exe_sources()`
- `void top_cmake_emit_exe_libs()`
- `void top_cmake_collect_all_sources()`
- `void top_cmake_link_prebuilt()`
- `void top_cmake_with_exe()`
- `void top_cmake_without_exe()`
- `void generate_mod_h_external()`
- `void generate_mod_test()`
- `int test_is_string_type()`
- `const char* test_fmt()`
- `void test_emit_expected()`

详细文档见 [generator](generator/DEV.md)。

### docgen

文档生成器 - 生成README.md/API.md/DEV.md三种文档

**函数:**

- `int generate_docs()` — 输出目录  
  逻辑: 生成README.md、DEV.md和DEPENDENCIES.md
- `void generate_mod_readme()`
- `void generate_mod_api()`
- `void generate_mod_dev()`
- `void generate_project_readme()`
- `void generate_project_dev()`
- `void generate_dependencies_doc()`
- `int is_api()`
- `const char* domain_type_str()`
- `void generate_module_docs()`
- `void write_dev_functions()`
- `void write_dev_types()`
- `void write_dev_macros()`
- `void write_api_deps()`
- `void proj_readme_write_modules()`
- `void proj_readme_write_mod_entry()`
- `void proj_readme_write_tree()`
- `void dev_write_funcs()`
- `void dev_write_types()`
- `void dev_write_macros()`
- `void dev_write_deps()`
- `int is_item()`
- `void readme_write_submodules()`
- `void readme_write_item_summary()`
- `int api_write_toc()`
- `void write_func_signature()`
- `int write_func_params()`
- `int api_write_functions()`
- `void dev_write_type_def()`
- `void dev_write_members()`
- `int api_write_types()`
- `int api_write_macros()`
- `int api_write_submodules()`
- `void dev_write_signature()`
- `void dev_write_params()`
- `void dev_write_locals()`
- `void dev_write_stats()`
- `void dev_write_child_links()`

详细文档见 [docgen](docgen/DEV.md)。

### typecheck

类型检查器 - 检查域类型一致性和符号冲突

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
- `int validate_integer_value()`
- `int validate_float_value()`
- `size_t match_qualifier()`
- `void strip_array_dims()`
- `int find_named_type_in_scope()`
- `const char* typedef_value()`
- `Domain* find_type_in_scope()`
- `int check_end()`
- `int skip_digits()`
- `int skip_exponent()`

详细文档见 [typecheck](typecheck/DEV.md)。

### utils

工具函数 - 通用辅助功能

**函数:**

- `void ensure_dir()` — 目录路径  
  逻辑: 递归创建目录结构，使用mkdir系统调用
- `int str_eq()` — 字符串B  
  逻辑: 使用strcmp比较，返回1(相等)或0(不等)
- `char* str_dup()` — 源字符串  
  逻辑: 分配新内存并复制字符串，返回新指针
- `void strip_quotes()` — 待处理字符串  
  逻辑: 原地修改，去除首尾的单引号或双引号
- `char** tokenize()`
- `void free_tokens()`
- `int str_startswith()`
- `int parse_c_decl()`
- `char* extract_base_type()`
- `char* extract_type_from_decl()`
- `char* extract_name_from_decl()`
- `int is_valid_identifier()`
- `int file_exists()`
- `void tokenize_emit()`
- `const char* tokenize_quoted()`
- `const char* tokenize_plain()`
- `const char* parse_read_base_type()`
- `const char* parse_read_pointer_stars()`
- `const char* parse_read_identifier()`
- `const char* parse_read_array_dims()`
- `const char* parse_skip_ws_to_ident()`
- `char* trim()`

详细文档见 [utils](utils/DEV.md)。

### main

可执行入口模块 - CBoot的主程序入口

**函数:**

- `int main()` — 命令行参数数组  
  逻辑: 初始化项目结构，处理-f强制标志，进入交互循环或批处理执行
- `void print_usage()`
- `void repl_loop()`
- `int dispatch_command()`
- `int detect_fine_tune_mode()`
- `int try_complete()`
- `int common_prefix_len()`
- `int do_tab_complete()`
- `void repl_history_up()`
- `void repl_history_down()`
- `void repl_consume_extended_esc()`
- `void repl_consume_bracketed_paste()`
- `int repl_read_line()`
- `void join_rest()`
- `int dispatch_build()`
- `int dispatch_modify()`
- `int dispatch_control()`
- `int dispatch_action()`
- `int parse_flag()`
- `int parse_info_flag()`
- `void parse_positional()`
- `int parse_args()`
- `int run_update()`
- `int run_adjust()`
- `int run_analyze()`
- `int run_batch_script()`
- `int run_default_cboot()`
- `int run_interactive()`
- `int tab_collect_child_domains()`
- `int tab_base_len()`
- `int tab_apply_single()`
- `int tab_apply_multi()`
- `void repl_handle_esc2()`
- `int repl_handle_esc()`
- `int repl_handle_ctrl()`
- `void read_code_from_stdin()`
- `int parse_rm()`
- `int parse_find_flags()`
- `int tab_collect_for_cmd()`

**宏:**

- `CBOOT_HISTORY_MAX` = `100`

详细文档见 [main](main/DEV.md)。

### k

详细文档见 [k](k/DEV.md)。

## 依赖链

详见 [DEPENDENCIES.md](DEPENDENCIES.md)。

### API 依赖摘要

```
/cupdate --> /domain
/commands --> /domain
/commands --> /cupdate
/parser --> /domain
/generator --> /domain
/docgen --> /domain
/typecheck --> /domain
/utils --> /domain
/main --> /domain
/main --> /cupdate
```

*Generated by CBoot v1.1.0*
