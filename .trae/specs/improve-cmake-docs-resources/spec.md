# 完善 CMake 生成、文档生成、资源导入处理 Spec

## Why
当前 CBoot 在 CMakeLists.txt 生成、API 文档生成和资源文件导入三个模块存在若干缺陷：宏定义丢失值、模块子目录 include 缺失、文档不完整、资源复制不够健壮。

## What Changes
- **CMake 生成修复**: 宏定义保留值、添加模块子目录到 include path、修复资源路径
- **文档生成完善**: api.md 显示 root 模块自身内容、修复 project.man 重复 TYPES 节
- **资源导入强化**: 用 fopen/fread/fwrite 替代 system("cp")、增加文件存在性校验

## Impact
- Affected specs: cboot-cli-implementation
- Affected code: src/generator.c, src/docgen.c, src/commands.c

## MODIFIED Requirements

### Requirement: CMakeLists.txt 生成（完善）
系统 SHALL 生成完整的 CMakeLists.txt，包含：宏定义值、模块子目录的 include path、正确的资源文件路径。

#### Scenario: 宏定义保留值
- **WHEN** 用户执行 `def PI 3.14159265359`
- **THEN** 生成的 .c 文件中输出 `#define PI 3.14159265359`（而非 `#define PI`）

#### Scenario: 模块子目录 include path
- **WHEN** 项目有子模块 `math` 且 `math` 拥有物理目录
- **THEN** CMakeLists.txt 中包含 `target_include_directories(<proj> PRIVATE ${CMAKE_SOURCE_DIR}/src/<proj>/math)` 使子模块头文件可被引用

#### Scenario: 资源文件路径正确
- **WHEN** 资源 `logo.png` 被复制到 `res/` 目录
- **THEN** CMake 中的 `add_custom_command` 使用 `${CMAKE_SOURCE_DIR}/res/logo.png` 作为 DEPENDS 路径

### Requirement: 文档生成（完善）
系统 SHALL 生成完整的 API 文档，包含 root 模块自身的内容，Man page 不重复输出 TYPES 节标题。

#### Scenario: api.md 包含 root 模块内容
- **WHEN** root 模块中定义了函数 `add` 和类型 `Vec2`
- **THEN** api.md 中显示 root 模块的函数和类型（而非仅显示子模块内容）

#### Scenario: project.man 不重复 TYPES 节
- **WHEN** 多个模块中各有一个类型定义
- **THEN** project.man 中只出现一次 `.SH TYPES` 标题，所有类型列在其下

### Requirement: 资源导入（强化）
系统 SHALL 使用 C 标准库函数复制资源文件，并在复制前校验源文件存在。

#### Scenario: 资源文件不存在时报错
- **WHEN** 用户执行 `res logo.png` 但 `logo.png` 不存在
- **THEN** 系统输出 "错误: 资源文件 'logo.png' 不存在" 并返回 -1

#### Scenario: 使用 C 标准库复制文件
- **WHEN** 用户执行 `res logo.png` 且文件存在
- **THEN** 系统使用 fopen/fread/fwrite 将文件复制到 `res/logo.png`（而非 system("cp")）