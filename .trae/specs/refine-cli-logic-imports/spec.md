# 完善 CLI 逻辑与导入机制 Spec

## Why
当前 CBoot CLI 存在若干逻辑瑕疵：类型检测仅为警告而非硬错误；函数定义过于臃肿（包含参数/变量创建）；缺少 C 指针/数组语法支持；导入机制不支持项目级 .cboot 导入和 .a/.so 库导入；缺少命令历史回滚功能。

## What Changes
- **类型严格化**: 不存在的类型定义变量时直接报错（非警告）
- **函数域命令重构**: `func` 只创建函数+注释，参数/变量通过 `addarg`/`addvar` 单独添加；同样 `type` 只创建类型+注释，成员通过 `addmember` 添加
- **C 指针/数组语法支持**: 解析 `int* a`, `int a[10]`, `int** a` 等声明
- **导入系统重设计**: 项目级导入 .cboot 文件到 `lib/` 目录；支持 `.a/.so` + `.cboot` 组合导入库
- **交互改进**: 支持上箭头回滚历史命令
- **BREAKING**: `func` 命令不再接受参数/变量；原有 `cmd_func` 的交互式参数/变量添加流程移除

## Impact
- Affected specs: cboot-cli-implementation, improve-cmake-docs-resources
- Affected code: src/commands.c, src/parser.c, src/scope.c, src/generator.c, src/docgen.c, src/main.c, include/cboot.h

## ADDED Requirements

### Requirement: 类型严格校验
系统 SHALL 在变量定义、参数定义、成员定义时检查类型是否存在，不存在时直接报错并拒绝创建。

#### Scenario: 不存在的类型定义变量报错
- **WHEN** 用户执行 `addvar FooBar x` 且 `FooBar` 既不是内置类型也不是已定义的类型
- **THEN** 系统输出 "错误: 类型 'FooBar' 未定义" 并返回 -1，不创建变量

#### Scenario: 内置类型可正常使用
- **WHEN** 用户执行 `addvar int x` 且当前在函数域下
- **THEN** 系统正常创建局部变量

#### Scenario: 用户自定义类型可正常使用
- **WHEN** 用户已定义 `struct Vec2`，然后执行 `addvar Vec2 v`
- **THEN** 系统正常创建变量

### Requirement: 函数域命令专一化
系统 SHALL 将函数创建与参数/变量添加分离为独立命令：`func` 仅创建函数+注释，`addarg` 添加参数，`addvar` 添加局部变量。

#### Scenario: func 只创建函数
- **WHEN** 用户执行 `func int add`
- **THEN** 系统创建函数 `int add()`，提示输入注释，完成后进入函数域。不提示输入参数或变量。

#### Scenario: addarg 在函数域下添加参数
- **WHEN** 用户在函数域下执行 `addarg int a`
- **THEN** 系统检查类型 `int` 存在，添加参数 `int a` 到当前函数

#### Scenario: addvar 在函数域下添加局部变量
- **WHEN** 用户在函数域下执行 `addvar int result`
- **THEN** 系统检查类型 `int` 存在，添加局部变量 `int result` 到当前函数

#### Scenario: addarg 在非函数域下报错
- **WHEN** 用户在模块域下执行 `addarg int x`
- **THEN** 系统输出 "错误: 当前不在函数作用域中" 并返回 -1

### Requirement: 类型域命令专一化
系统 SHALL 将类型创建与成员添加分离：`type` 仅创建类型+注释，`addmember` 添加成员。

#### Scenario: type 只创建类型
- **WHEN** 用户执行 `type Vec2`
- **THEN** 系统创建 `struct Vec2`，提示输入注释，完成后进入类型域。不提示输入成员。

#### Scenario: addmember 在类型域下添加成员
- **WHEN** 用户在类型域下执行 `addmember float x`
- **THEN** 系统检查类型 `float` 存在，添加成员 `float x` 到当前类型

### Requirement: C 指针/数组语法解析
系统 SHALL 支持解析 C 语言指针和数组声明语法，包括 `int* a`, `int *a`, `int a[10]`, `int** a`, `int* a[10]` 等形式。

#### Scenario: 解析指针声明
- **WHEN** 用户执行 `addarg int* a`
- **THEN** 系统解析出类型为 `int*`，名称为 `a`

#### Scenario: 解析数组声明
- **WHEN** 用户执行 `addvar int a[10]`
- **THEN** 系统解析出类型为 `int`，名称为 `a[10]`

#### Scenario: 解析多级指针
- **WHEN** 用户执行 `addarg int** ptr`
- **THEN** 系统解析出类型为 `int**`，名称为 `ptr`

### Requirement: 项目级 .cboot 导入
系统 SHALL 支持 `import <file>.cboot` 命令，将导入项目的所有文件复制到 `lib/<project_name>/` 目录下，并在 CMake 中自动添加为子目录。

#### Scenario: 导入 .cboot 项目
- **WHEN** 用户执行 `import ../mathlib/mathlib.cboot` 且该文件存在
- **THEN** 系统解析 .cboot 文件，将导入项目的 src/ 内容复制到 `lib/mathlib/`，并在 CMakeLists.txt 中添加 `add_subdirectory(lib/mathlib)`

#### Scenario: 导入文件不存在报错
- **WHEN** 用户执行 `import missing.cboot` 且文件不存在
- **THEN** 系统输出错误并返回 -1

### Requirement: .a/.so 库导入
系统 SHALL 支持 `import <lib>.a <desc>.cboot` 或 `import <lib>.so <desc>.cboot` 命令，将库文件复制到 `lib/` 目录，通过 .cboot 描述文件获取接口信息。

#### Scenario: 导入静态库
- **WHEN** 用户执行 `import /usr/lib/libfoo.a libfoo.cboot`
- **THEN** 系统将 `libfoo.a` 复制到 `lib/`，解析 `libfoo.cboot` 获取接口信息，在 CMake 中添加 `target_link_libraries`

#### Scenario: 导入动态库
- **WHEN** 用户执行 `import /usr/lib/libbar.so libbar.cboot`
- **THEN** 系统将 `libbar.so` 复制到 `lib/`，解析 `libbar.cboot` 获取接口信息

### Requirement: 命令历史回滚
系统 SHALL 在交互模式下支持通过上箭头键回滚上一个命令，行为与标准 shell 一致。

#### Scenario: 上箭头回滚命令
- **WHEN** 用户在交互模式下按下上箭头键
- **THEN** 系统显示上一次输入的命令，用户可编辑后重新执行

## MODIFIED Requirements

### Requirement: 代码生成适配新目录结构
系统 SHALL 在生成代码时适配 `src/`（自有代码）和 `lib/`（导入库）的目录结构。

#### Scenario: CMakeLists.txt 包含 lib 子目录
- **WHEN** 项目导入了 `mathlib.cboot`
- **THEN** CMakeLists.txt 中包含 `add_subdirectory(lib/mathlib)` 和 `target_link_libraries(<proj> mathlib)`

### Requirement: 文档生成适配导入库
系统 SHALL 在 api.md 中列出导入的库及其接口信息。

#### Scenario: api.md 包含导入库信息
- **WHEN** 项目导入了 `libfoo.a` + `libfoo.cboot`
- **THEN** api.md 的 Resources 或新增 Dependencies 节中包含 `libfoo` 的接口描述