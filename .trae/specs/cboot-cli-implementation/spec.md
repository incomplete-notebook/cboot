# CBoot-CLI v2.0 实现规范

## Why
CBoot-CLI 是一个 C 项目引导工具，通过交互式 REPL 和批处理脚本两种模式，帮助开发者快速搭建 C 项目骨架：模块、函数、类型定义、依赖管理、资源管理和文档生成。

## What Changes
- 实现命令行参数解析（argc/argv），支持交互模式、批处理模式、-f 强制标志
- 实现统一作用域树数据结构（Scope/Module/Function/Type），支持增删改查
- 实现 cd/ls 导航系统，支持路径解析、..、绝对路径
- 实现 mod/func/type/add/def/im 创建命令的向导模式
- 实现 rm 智能删除命令（含歧义消除向导）
- 实现 cmt/arg/mem 注释命令
- 实现 res 资源管理命令（位掩码组合模式）
- 实现 gen 代码生成器（目录创建、C文件、CMakeLists.txt）
- 实现文档生成器（.md / .man）
- 实现 .cboot 脚本解析器（行内标志支持）
- 实现依赖管理（im 命令 + 循环检测）
- 实现类型检测（变量定义时校验类型名在作用域树中是否存在）

## Impact
- Affected specs: 无（新项目）
- Affected code: 新建全部源文件（cboot.c, scope.c, commands.c, generator.c, parser.c, docgen.c, utils.c）
- 参考代码: tcc-0.9.27（tinycc 编译器，用于语法解析和 C 语言处理参考）

## ADDED Requirements

### Requirement: 命令行解析
系统 SHALL 解析命令行参数，支持三种运行模式：交互模式（`cboot <项目名>`）、批处理模式（`cboot` 读取 .cboot）、强制模式（`-f` 标志）。

#### Scenario: 交互模式启动
- **WHEN** 用户执行 `cboot MyProject`
- **THEN** 系统创建 MyProject 目录，进入交互式 REPL 向导

#### Scenario: 批处理模式启动
- **WHEN** 用户在当前目录执行 `cboot`（无参数）
- **THEN** 系统读取当前目录的 .cboot 脚本，逐行执行

#### Scenario: 强制模式
- **WHEN** 用户执行 `cboot MyProject -f`
- **THEN** 交互模式：删除重建项目目录；批处理模式：静默覆盖冲突文件

### Requirement: 作用域树数据结构
系统 SHALL 实现统一作用域树，包含 Scope、Module、Function、Type 结构体，支持父子关系、子项管理、注释存储。

#### Scenario: 创建模块作用域
- **WHEN** 系统创建模块 "core"
- **THEN** 创建 Scope 节点，type=SCOPE_MODULE，name="core"，可容纳子模块、函数、类型、宏、依赖、资源

#### Scenario: 作用域父子关系
- **WHEN** 在模块 "core" 下创建函数 "add"
- **THEN** 函数 Scope 的 parent 指向模块 Scope，模块 Scope 的 children 包含函数 Scope

### Requirement: cd/ls 导航系统
系统 SHALL 支持 cd 命令在作用域间导航，支持相对路径、..、绝对路径（如 /core/add）。ls 命令列出当前作用域下所有子项。

#### Scenario: cd 导航到子作用域
- **WHEN** 用户在模块 "core" 下执行 `cd add`
- **THEN** current 指针指向函数 "add" 的作用域

#### Scenario: cd .. 返回上级
- **WHEN** 用户在函数 "add" 下执行 `cd ..`
- **THEN** current 指针指向父模块 "core"

#### Scenario: ls 列出子项
- **WHEN** 用户在模块 "core" 下执行 `ls`
- **THEN** 列出所有子模块、函数、类型、宏、依赖、资源（含类型标注）

### Requirement: 创建命令向导模式
系统 SHALL 为 mod/func/type/add/def/im 命令提供交互式向导，逐步收集所需信息。

#### Scenario: func 向导
- **WHEN** 用户执行 `func int add`
- **THEN** 系统依次询问：函数注释 → 循环添加参数（含注释）→ 循环添加局部变量（含注释），创建后自动 cd 进入

#### Scenario: type 向导
- **WHEN** 用户执行 `type Point`
- **THEN** 系统依次询问：类型注释 → 循环添加成员（含注释），创建后自动 cd 进入

### Requirement: rm 智能删除命令
系统 SHALL 实现 rm 命令，在当前作用域查找并删除匹配的项，支持歧义消除向导和 -f 强制模式。

#### Scenario: 删除无歧义项
- **WHEN** 用户在函数作用域执行 `rm a`，且只有一个参数名为 "a"
- **THEN** 直接删除参数 int a

#### Scenario: 歧义消除
- **WHEN** 用户执行 `rm a`，且同时存在参数 int a 和局部变量 int a
- **THEN** 列出所有匹配项，提示用户选择编号删除

#### Scenario: 强制删除
- **WHEN** 用户执行 `rm -f a`，且存在歧义
- **THEN** 报错退出，不询问

### Requirement: 注释命令
系统 SHALL 实现 cmt/arg/mem 短命令，为作用域、参数、成员添加注释。

#### Scenario: cmt 为作用域添加注释
- **WHEN** 用户在函数作用域执行 `cmt "加法函数"`
- **THEN** 当前作用域的 comment 字段设为 "加法函数"

#### Scenario: arg 为参数添加注释
- **WHEN** 用户在函数作用域执行 `arg a "左值"`
- **THEN** 参数 a 的注释设为 "左值"

### Requirement: 资源管理
系统 SHALL 实现 res 命令，支持资源文件的位掩码组合模式（-h 转 .h 头文件、-bitmap 转 .bmp）。

#### Scenario: 资源勾选多模式
- **WHEN** 用户执行 `res logo.png`，在向导中同时勾选 -h 和 -bitmap
- **THEN** 原文件复制到 /res/，CMake 同时生成 xxd 和 ffmpeg 命令

### Requirement: gen 代码生成器
系统 SHALL 实现 gen 命令，遍历作用域树生成所有 C 代码文件、CMakeLists.txt。

#### Scenario: 生成 C 代码
- **WHEN** 用户执行 `gen`
- **THEN** 根据三大铁律（规则A/B/C）创建目录结构，生成 .c/.h 文件，生成 CMakeLists.txt

#### Scenario: 冲突自动下沉
- **WHEN** 创建子模块 X 时，父目录已存在 X.c/X.h
- **THEN** 子模块 X 强制拥有目录，父模块的 X.c 迁移为 X/X.c

### Requirement: 文档生成器
系统 SHALL 在 gen 时生成 API 文档（docs/api.md）和手册页（docs/project.man），并在 CMake 中注入 pandoc/groff 构建规则。

#### Scenario: 生成文档
- **WHEN** 执行 gen
- **THEN** 遍历作用域树注释，生成 api.md 和 project.man，CMake 注入文档构建规则

### Requirement: .cboot 脚本解析器
系统 SHALL 解析 .cboot 脚本文件，支持行内标志（--comment、--arg、--member、--targets、--mode）。

#### Scenario: 脚本逐行执行
- **WHEN** 系统读取 .cboot 脚本
- **THEN** 逐行解析命令，支持 project、mod、func、type、im、res、rm、gen 等完整命令，使用行内标志替代交互式问答

### Requirement: 类型检测
系统 SHALL 在变量定义时校验所使用的类型名在当前作用域或父作用域中是否已定义（内置类型或用户自定义类型）。

#### Scenario: 使用已定义类型
- **WHEN** 用户在 func 向导中添加参数 `Point p`，且当前模块或其父模块中已通过 type 定义了 `Point`
- **THEN** 系统接受该参数，不报错

#### Scenario: 使用未定义类型
- **WHEN** 用户在 func 向导中添加参数 `Foo f`，但 `Foo` 在作用域树中从未定义
- **THEN** 系统提示警告 "类型 Foo 未定义"，但允许继续（不阻断流程）

#### Scenario: 内置类型直接通过
- **WHEN** 用户添加参数 `int a`、`float b`、`char c`、`double d`、`void *p` 等内置类型
- **THEN** 系统识别为内置类型，直接通过，不检查作用域树

#### Scenario: 类型查找沿作用域链向上
- **WHEN** 当前模块为 /core/utils，且父模块 /core 中定义了类型 `Point`
- **THEN** 在 /core/utils 中定义变量 `Point p`，系统沿作用域链向上找到 /core 中的 Point，通过

### Requirement: 依赖管理
系统 SHALL 通过 im 命令管理模块依赖和系统头文件，gen 时检测循环依赖。

#### Scenario: 模块依赖
- **WHEN** 用户执行 `im core`
- **THEN** 当前模块记录依赖 core，gen 时自动插入 #include "core.h"

#### Scenario: 循环依赖检测
- **WHEN** gen 时检测到模块 A 依赖 B，B 依赖 A
- **THEN** 报错并提示循环依赖路径