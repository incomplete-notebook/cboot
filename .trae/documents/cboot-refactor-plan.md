# CBoot 全面重构计划

## Context
当前 CBoot v2.0 的数据模型、命令系统和生成器与新规范 `cboot.txt` 存在根本性差异。新规范定义了 7 种域类型、统一的 mode/value/comment 字段、每模块独立文件生成、4 种文档、to_cboot 反向工程和微调模式。需要全面重构，最终通过自举和 tcc 转化测试验证。

## 新增文件
| 文件 | 说明 |
|------|------|
| `include/domain.h` | 新数据模型 (7种域 + 统一基类) |
| `include/typecheck.h` | 类型检查器 |
| `include/to_cboot.h` | to_cboot 反向工程 |
| `include/fine_tune.h` | 微调模式 |
| `src/domain.c` | 域操作实现 |
| `src/typecheck.c` | 类型检查器实现 |
| `src/to_cboot.c` | to_cboot 实现 |
| `src/fine_tune.c` | 微调模式实现 |

## 修改文件
| 文件 | 变更 |
|------|------|
| `include/cboot.h` | 移除旧数据模型，保留常量和工具函数声明 |
| `src/main.c` | 新命令分发、-to_cboot 参数、微调模式检测 |
| `src/commands.c` | 完全重写：新命令体系 (mod/void/struct/type/def/var/mem/enum/cmt/value/mode/cd/rm/mv/find/ls/gen/im/exit) |
| `src/generator.c` | 完全重写：每模块 .c/.h + CMake + 4 种 .md + 最小 .cboot |
| `src/docgen.c` | 完全重写：README.md/API.md/DEV.md/DOC.md |
| `src/parser.c` | 更新：新命令脚本语法 |
| `src/utils.c` | 新增 C 值合法性验证 |
| `Makefile` | 添加新源文件 |

## 删除文件
| 文件 | 替代 |
|------|------|
| `src/scope.c` | `src/domain.c` |

## 实现阶段 (10 个阶段)

### P1: 数据模型重构
- 创建 `domain.h` / `domain.c`：统一 Domain 基类 + 7 种特定域 struct (ModuleDomain, FunctionDomain, StructDomain, TypeDomain, MacroDomain, VariableDomain, MemberDomain)
- 每个域有 mode/value/comment 字段
- 域操作：创建、添加子域、查找、删除、路径、递归遍历
- 更新 `cboot.h` 移除旧 Scope/Module/Function/Type 定义

### P2: 类型检查器 (可与 P1 并行)
- 创建 `typecheck.h` / `typecheck.c`
- 内置类型表 + 域树类型查找 + typedef 解析
- 支持指针/数组类型

### P3: 命令系统重构
- 完全重写 `commands.c`：18 个命令处理器
- 命令上下文验证矩阵
- 名称冲突 + 类型检查 + 警告系统

### P4: 代码生成器重构
- 完全重写 `generator.c`
- 每模块 .c/.h (API 分离) + 每模块 CMakeLists.txt + 顶层 CMake + 最小 .cboot

### P5: 文档生成器重构
- 完全重写 `docgen.c`
- 每模块 README.md + API.md + DEV.md + DOC.md (含 TOC)

### P6: .cboot 脚本解析器更新
- 更新 `parser.c` 支持新命令语法

### P7: REPL 与主入口更新
- 更新 `main.c`：新命令分发、-to_cboot、微调模式检测

### P8: to_cboot 实现
- 创建 `to_cboot.c`：扫描目录、解析 Makefile/CMake、解析 C 源文件、生成 .cboot
- 参考 tcc 代码实现 C 解析器

### P9: 微调模式实现
- 创建 `fine_tune.c`：检测微调模式、还原域模型、更新 code 字段、重新生成

### P10: 自举与测试
- 自举脚本、矩阵乘法微调测试、tcc 转化测试

## 验证
- 每阶段编译通过
- 最终：cboot to_cboot 自举 → 编译新 cboot → 用自举 cboot 再次 to_cboot → 结果一致
- tcc 终极测试：cboot to_cboot tcc → 编译 tcc → 用 tcc 编译 cboot