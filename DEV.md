# cboot 开发文档

## 模块概览

| 模块 | 说明 | 路径 |
|------|------|------|
| `domain` | 域数据模型 - 定义所有域类型、域树结构和项目容器 | domain/ |
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

- `struct Domain* domain_new()` — 创建新域节点，分配内存并初始化所有字段  
  逻辑: 分配内存，设置类型和名称，初始化子域数组，返回新指针
- `struct ModuleDomain* module_domain_new()` — 创建新模块域  
  逻辑: 调用domain_new，设置默认模式为INTERNAL，默认编译模式为NORMAL
- `struct FunctionDomain* function_domain_new()` — 创建新函数域  
  逻辑: 调用domain_new，设置返回类型，默认API模式为NORMAL
- `struct StructDomain* struct_domain_new()` — 创建新结构体域  
  逻辑: 调用domain_new，默认API模式为NORMAL
- `struct TypeDomain* type_domain_new()` — 创建新类型域  
  逻辑: 调用domain_new，默认类型模式为RENAME
- `struct MacroDomain* macro_domain_new()` — 创建新宏域  
  逻辑: 调用domain_new，默认API模式为NORMAL
- `struct VariableDomain* variable_domain_new()` — 创建新变量域  
  逻辑: 调用domain_new，设置类型，默认变量模式为NORMAL
- `struct MemberDomain* member_domain_new()` — 创建新成员域(结构体成员或函数参数)  
  逻辑: 调用domain_new，设置类型
- `void domain_add_child()` — 添加子域到父域，自动设置parent指针  
  逻辑: 检查容量，扩容数组，添加子域，设置parent
- `struct Domain* domain_find_child()` — 按名称查找直接子域  
  逻辑: 遍历子域数组，返回匹配名称的子域指针，未找到返回NULL
- `struct Domain* domain_find_api_in_submodules()` — 在子模块中递归查找API项，用于名称冲突检测和类型解析  
  逻辑: 先在直接子域中查找，再递归进入子模块查找API模式项
- `struct Domain* domain_check_api_name_conflict()` — 检查名称是否与子模块API项冲突  
  逻辑: 调用domain_find_api_in_submodules，返回冲突域或NULL
- `char* domain_get_path()` — 获取域的完整路径字符串(如 /module/submodule)  
  逻辑: 从当前域向上遍历到根，拼接路径字符串
- `void domain_set_comment()` — 设置域的注释文本  
  逻辑: 复制文本并设置到域的comment字段
- `void domain_set_mode()` — 设置域的模式字段  
  逻辑: 根据域类型设置对应的模式字段
- `struct Project* project_new()` — 创建新项目  
  逻辑: 创建根模块域，初始化项目状态和依赖数组
- `void project_add_dependency()` — 添加依赖记录(im命令使用)  
  逻辑: 检查容量，扩容数组，添加新依赖记录
- `int project_has_dependency()` — 检查依赖是否已存在  
  逻辑: 遍历依赖数组，返回1(存在)或0(不存在)

**类型:**

- `DomainType` — 域类型枚举: module/function/struct/type/macro/variable/member
- `ModMode` — 模块模式: internal(正常编译)/external(外部API引用)
- `CompilerMode` — 编译模式: normal(普通.o)/exe(可执行)/sl(静态库)/dl(动态库)
- `ApiMode` — API模式: api(公开接口)/normal(私有实现)
- `TypeMode` — 类型模式: rename(别名)/struct(结构体)/api rename/api struct
- `VarMode` — 变量模式: static/normal
- `Domain` — 域树基础节点，所有域类型的基类
- `ModuleDomain` — 模块域 - 项目中的一个模块节点，含编译模式和依赖管理
- `FunctionDomain` — 函数域 - 项目中的函数定义
- `StructDomain` — 结构体域 - 项目中的结构体类型定义
- `TypeDomain` — 类型域 - typedef定义
- `MacroDomain` — 宏域 - #define定义
- `VariableDomain` — 变量域 - 变量定义
- `MemberDomain` — 成员域 - 结构体成员或函数参数
- `Dependency` — 依赖记录 - im命令建立的API依赖关系
- `Project` — 项目容器 - 整个项目的根节点和全局状态

详细文档见 [domain](domain/DEV.md)。

### commands

命令处理器 - 实现交互式和批处理模式的所有命令

**函数:**

- `int cmd_mod()` — 创建新模块命令  
  逻辑: 验证名称唯一性，创建ModuleDomain并添加到当前作用域
- `int cmd_struct()` — 创建新结构体命令  
  逻辑: 在当前模块作用域中创建StructDomain
- `int cmd_type()` — 创建新类型(typedef)命令  
  逻辑: 在当前模块作用域中创建TypeDomain
- `int cmd_def()` — 创建新宏(#define)命令  
  逻辑: 在当前模块作用域中创建MacroDomain
- `int cmd_void()` — 创建新函数命令  
  逻辑: 验证名称唯一性，创建FunctionDomain并设为当前
- `int cmd_var()` — 创建新变量命令  
  逻辑: 在当前作用域创建VariableDomain
- `int cmd_mem()` — 添加成员/参数命令  
  逻辑: 根据当前作用域(函数/结构体/类型)创建MemberDomain，设为当前
- `int cmd_cmt()` — 设置注释命令  
  逻辑: 设置当前域的注释；若当前是成员/参数，设置后恢复到父作用域
- `int cmd_mode()` — 设置模式命令  
  逻辑: 根据当前域类型设置对应模式；模块支持internal/external，函数/结构体/宏支持api/normal
- `int cmd_cmode()` — 设置编译器模式命令  
  逻辑: 仅对模块有效：exe生成可执行文件(代码放main.c)，sl静态库，dl动态库，normal普通.o
- `int cmd_cd()` — 导航命令 - 进入子域或返回父域  
  逻辑: 解析路径段，逐级在域树中导航
- `int cmd_gen()` — 生成项目代码命令  
  逻辑: 遍历域树，生成.c/.h/CMakeLists.txt/文档，项目根为源码根
- `int cmd_im()` — API导入命令 - 从.cboot文件仅导入API项  
  逻辑: 解析源文件，提取API模式项，创建external模块，记录依赖链
- `int cmd_in()` — 完整项目导入命令  
  逻辑: 完整导入源项目作为子模块，包括所有非API项
- `int cmd_help()` — 帮助命令  
  逻辑: 打印所有可用命令的帮助信息
- `int cmd_quit()` — 退出命令  
  逻辑: 设置运行标志为0，退出交互/批处理模式

详细文档见 [commands](commands/DEV.md)。

### parser

脚本解析器 - 解析.cboot脚本文件为项目定义

**函数:**

- `int parse_cboot()` — 解析.cboot脚本文件  
  逻辑: 逐行读取脚本，分派命令到对应处理函数，支持#注释
- `int dispatch_script_line()` — 分派单行脚本命令  
  逻辑: 根据首个分词(命令名)调用对应cmd_函数

详细文档见 [parser](parser/DEV.md)。

### generator

代码生成器 - 将域树转换为C源代码、CMake构建系统和文档

**函数:**

- `int generate_project()` — 生成完整项目代码  
  逻辑: 递归处理所有模块，生成.c/.h/层级化CMake/文档/DEPENDENCIES.md
- `void generate_module()` — 为单个模块生成代码  
  逻辑: 创建模块目录，生成.c/.h/CMakeLists.txt/.cboot/文档，递归处理子模块
- `void generate_mod_c()` — 生成模块的.c源文件  
  逻辑: 包含头文件、宏、类型、变量、函数实现；非API函数自动static化避免符号冲突
- `void generate_mod_h()` — 生成模块的.h头文件(仅API声明)  
  逻辑: 生成include guard、API宏/类型/函数声明；dl模式添加dll导出宏
- `void generate_mod_cmake()` — 生成模块的CMakeLists.txt  
  逻辑: 根据compiler模式生成: normal(收集.o)/exe(可执行)/sl(静态库)/dl(动态库)，含子模块add_subdirectory
- `void generate_top_cmake()` — 生成顶层CMakeLists.txt  
  逻辑: 无exe模块时收集所有normal模块源文件+main.c生成默认可执行；有exe模块时链接库模块
- `void generate_top_main()` — 生成顶层main.c入口  
  逻辑: 仅在无exe模块时生成默认main.c，包含所有顶层模块头文件

详细文档见 [generator](generator/DEV.md)。

### docgen

文档生成器 - 生成README.md/API.md/DEV.md三种文档

**函数:**

- `int generate_docs()` — 生成项目级文档  
  逻辑: 生成README.md、DEV.md和DEPENDENCIES.md
- `void generate_module_docs()` — 为单个模块生成文档  
  逻辑: 生成README.md(模块说明+子模块链接)、API.md(仅API项)、DEV.md(所有项+业务逻辑)

详细文档见 [docgen](docgen/DEV.md)。

### typecheck

类型检查器 - 检查域类型一致性和符号冲突

**函数:**

- `int typecheck_domain()` — 检查单个域的类型一致性  
  逻辑: 检查类型引用是否有效、API名称是否与子模块冲突
- `int typecheck_project()` — 检查整个项目的类型一致性  
  逻辑: 递归遍历所有域，调用typecheck_domain，输出错误列表

详细文档见 [typecheck](typecheck/DEV.md)。

### utils

工具函数 - 通用辅助功能

**函数:**

- `int ensure_dir()` — 确保目录存在，不存在则创建  
  逻辑: 递归创建目录结构，使用mkdir系统调用
- `int str_eq()` — 字符串相等比较  
  逻辑: 使用strcmp比较，返回1(相等)或0(不等)
- `char* str_dup()` — 字符串复制  
  逻辑: 分配新内存并复制字符串，返回新指针
- `void strip_quotes()` — 去除字符串首尾的引号  
  逻辑: 原地修改，去除首尾的单引号或双引号

详细文档见 [utils](utils/DEV.md)。

### main

可执行入口模块 - CBoot的主程序入口

**函数:**

- `int main()` — 程序入口函数 - 解析命令行参数，进入交互或批处理模式  
  逻辑: 初始化项目结构，处理-f强制标志，进入交互循环或批处理执行

详细文档见 [main](main/DEV.md)。

*Generated by CBoot v2.0*
