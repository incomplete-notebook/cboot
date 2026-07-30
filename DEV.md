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

脚本解析器 - 解析.cboot脚本文件为项目定义

**函数:**

- `int dispatch_script_line()` — 分词数量  
  逻辑: 根据首个分词(命令名)调用对应cmd_函数
- `int is_cboot_ref()`
- `int exec_cboot_ref()`
- `int try_cboot_ref()`
- `int parse_cboot_script()`

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
- `char* trim()`
- `int str_startswith()`
- `int parse_c_decl()`
- `char* extract_base_type()`
- `char* extract_type_from_decl()`
- `char* extract_name_from_decl()`
- `int is_valid_identifier()`
- `int file_exists()`

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

**宏:**

- `CBOOT_HISTORY_MAX` = `100`

详细文档见 [main](main/DEV.md)。

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

*Generated by CBoot v0.4.0*
