# cboot 开发文档

本项目主要代码仓是gitee，另有github和gitcode两个镜像，其中github实时同步，gitcode定期同步。

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

- `Domain* domain_new()` — 结构体大小  
  逻辑: 分配内存，设置类型和名称，初始化子域数组，返回新指针
- `ModuleDomain* module_domain_new()` — 模块名称  
  逻辑: 调用domain_new，设置默认模式为INTERNAL，默认编译模式为NORMAL
- `FunctionDomain* function_domain_new()` — 返回类型  
  逻辑: 调用domain_new，设置返回类型，默认API模式为NORMAL
- `StructDomain* struct_domain_new()` — 结构体名称  
  逻辑: 调用domain_new，默认API模式为NORMAL
- `TypeDomain* type_domain_new()` — 类型名称  
  逻辑: 调用domain_new，默认类型模式为RENAME
- `MacroDomain* macro_domain_new()` — 宏名称  
  逻辑: 调用domain_new，默认API模式为NORMAL
- `VariableDomain* variable_domain_new()` — 变量类型  
  逻辑: 调用domain_new，设置类型，默认变量模式为NORMAL
- `MemberDomain* member_domain_new()` — 成员类型  
  逻辑: 调用domain_new，设置类型
- `void domain_add_child()` — 子域指针  
  逻辑: 检查容量，扩容数组，添加子域，设置parent
- `Domain* domain_find_child()` — 目标名称  
  逻辑: 遍历子域数组，返回匹配名称的子域指针，未找到返回NULL
- `Domain* domain_find_api_in_submodules()` — 目标名称  
  逻辑: 先在直接子域中查找，再递归进入子模块查找API模式项
- `Domain* domain_check_api_name_conflict()` — 目标名称  
  逻辑: 调用domain_find_api_in_submodules，返回冲突域或NULL
- `char* domain_get_path()` — 目标域  
  逻辑: 从当前域向上遍历到根，拼接路径字符串
- `void domain_set_comment()` — 注释文本  
  逻辑: 复制文本并设置到域的comment字段
- `void domain_set_mode()` — 模式值  
  逻辑: 根据域类型设置对应的模式字段
- `Project* project_new()` — 项目名称  
  逻辑: 创建根模块域，初始化项目状态和依赖数组
- `void project_add_dependency()` — 源.cboot文件路径  
  逻辑: 检查容量，扩容数组，添加新依赖记录
- `int project_has_dependency()` — 源模块路径  
  逻辑: 遍历依赖数组，返回1(存在)或0(不存在)
- `Domain* domain_find_child_by_type()`
- `void domain_delete()`
- `void domain_remove_child()`
- `Domain* domain_find_nearest_of_type()`
- `Domain* domain_find_in_tree()`
- `int domain_is_api()`
- `void domain_set_child_comment()`
- `Comment* find_child_comment()`
- `void domain_set_value()`
- `void domain_set_code()`
- `void domain_set_call()`
- `const char* domain_get_call()`
- `void project_free()`
- `int is_builtin_type()`

**类型:**

- `DomainType` — 域类型枚举: module/function/struct/type/macro/variable/member
- `ModMode` — 模块模式: internal(正常编译)/external(外部API引用)
- `CompilerMode` — 编译模式: normal(普通.o)/exe(可执行)/sl(静态库)/dl(动态库)
- `ApiMode` — API模式: api(公开接口)/normal(私有实现)
- `TypeMode` — 类型模式: rename(别名)/struct(结构体)/api rename/api struct
- `VarMode` — 变量模式: static/normal
- `Domain` — 子域数组容量
- `ModuleDomain` — 依赖数量
- `FunctionDomain` — 业务逻辑描述
- `StructDomain` — API模式(ApiMode枚举)
- `TypeDomain` — 底层类型(rename模式)或值
- `MacroDomain` — 宏值
- `VariableDomain` — 变量初始值
- `MemberDomain` — 成员类型
- `Dependency` — 源.cboot文件路径
- `Project` — 依赖数量

详细文档见 [domain](domain/DEV.md)。

### cupdate

代码更新模块 - 解析C源码并同步.cboot描述文件

**函数:**

- `int run_project()` — 输出参数：返回警告数量（可为NULL）  
  逻辑: 遍历所有src模式模块，读取.c文件，解析C声明，更新code字段和API定义，重新生成.cboot
- `int parse_source()` — 解析结果输出（调用者负责初始化和释放）  
  逻辑: 词法分析+语法分析，提取函数/结构体/typedef/宏/变量声明，检测语法错误
- `void result_init()` — 要初始化的结果容器
- `void result_free()` — 要释放的结果容器
- `void result_add_error()` — 行号
- `void result_add_warning()` — 行号
- `CUPDecl* result_add_decl()` — 结果容器
- `void cup_free_decl()`
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

**类型:**

- `CUPDeclKind` — 顶层声明种类枚举: function/struct/enum/typedef/macro/variable/include/other
- `CUParParam` — 参数名（可能为空）
- `CUParMember` — 位域宽度或初始值（可选）
- `CUPDecl` — 是否对外可见（非static）
- `CUPResult` — 警告数量

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
- `int cmd_help()` — 帮助命令  
  逻辑: 打印所有可用命令的帮助信息
- `int cmd_quit()` — 退出命令  
  逻辑: 设置运行标志为0，退出交互/批处理模式
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
- `void copy_api_items()`
- `void copy_api_items_recursive()`
- `int cmd_res()`

详细文档见 [commands](commands/DEV.md)。

### parser

脚本解析器 - 解析.cboot脚本文件为项目定义

**函数:**

- `int parse_cboot()` — 输出项目  
  逻辑: 逐行读取脚本，分派命令到对应处理函数，支持#注释
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
- `void generate_module()` — 父目录  
  逻辑: 创建模块目录，生成.c/.h/CMakeLists.txt/.cboot/文档，递归处理子模块
- `void generate_mod_c()` — 模块目录路径  
  逻辑: 包含头文件、宏、类型、变量、函数实现；非API函数自动static化避免符号冲突
- `void generate_mod_h()` — 模块目录路径  
  逻辑: 生成include guard、API宏/类型/函数声明；dl模式添加dll导出宏
- `void generate_mod_cmake()` — 模块目录路径  
  逻辑: 根据compiler模式生成: normal(收集.o)/exe(可执行)/sl(静态库)/dl(动态库)，含子模块add_subdirectory
- `void generate_top_cmake()` — 项目定义  
  逻辑: 无exe模块时收集所有normal模块源文件+main.c生成默认可执行；有exe模块时链接库模块
- `void generate_top_main()` — 项目定义  
  逻辑: 仅在无exe模块时生成默认main.c，包含所有顶层模块头文件
- `void generate_mod_cboot()`
- `void generate_project_cboot()`
- `Domain* find_exe_module()`
- `int docgen_is_api()`
- `const char* compiler_mode_str()`
- `void get_module_prefix()`
- `void make_abs_name()`
- `void write_c_includes()`
- `void write_c_defs()`
- `void write_c_types()`
- `void write_c_variables()`
- `void write_c_functions()`

详细文档见 [generator](generator/DEV.md)。

### docgen

文档生成器 - 生成README.md/API.md/DEV.md三种文档

**函数:**

- `int generate_docs()` — 输出目录  
  逻辑: 生成README.md、DEV.md和DEPENDENCIES.md
- `void generate_module_docs()` — 模块目录路径  
  逻辑: 生成README.md(模块说明+子模块链接)、API.md(仅API项)、DEV.md(所有项+业务逻辑)
- `void generate_mod_readme()`
- `void generate_mod_api()`
- `void generate_mod_dev()`
- `void generate_project_readme()`
- `void generate_project_dev()`
- `void generate_dependencies_doc()`
- `int is_api()`
- `const char* domain_type_str()`
- `void write_dev_functions()`
- `void write_dev_types()`
- `void write_dev_macros()`

详细文档见 [docgen](docgen/DEV.md)。

### typecheck

类型检查器 - 检查域类型一致性和符号冲突

**函数:**

- `int typecheck_domain()` — 待检查域  
  逻辑: 检查类型引用是否有效、API名称是否与子模块冲突
- `int typecheck_project()` — 项目定义  
  逻辑: 递归遍历所有域，调用typecheck_domain，输出错误列表
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

*Generated by CBoot v0.3.1*
