# Tasks

- [x] Task 1: 类型严格校验 — 将警告改为硬错误
  - [x] 修改 `cmd_func` 中返回类型检测：不存在时直接报错返回 -1
  - [x] 修改 `cmd_addarg` (新) 中参数类型检测：不存在时直接报错返回 -1
  - [x] 修改 `cmd_addvar` (新) 中变量类型检测：不存在时直接报错返回 -1
  - [x] 修改 `cmd_addmember` (新) 中成员类型检测：不存在时直接报错返回 -1

- [x] Task 2: 函数域命令重构 — func/addarg/addvar 分离
  - [x] 简化 `cmd_func`：移除参数/局部变量交互式添加流程，仅保留函数名+返回类型+注释
  - [x] 新增 `cmd_addarg`：在函数域下添加参数，校验类型存在性
  - [x] 新增 `cmd_addvar`：在函数域下添加局部变量，校验类型存在性
  - [x] 在 `parser.c` 中注册 `addarg` 和 `addvar` 命令

- [x] Task 3: 类型域命令重构 — type/addmember 分离
  - [x] 简化 `cmd_type`：移除成员交互式添加流程，仅保留类型名+注释
  - [x] 新增 `cmd_addmember`：在类型域下添加成员，校验类型存在性
  - [x] 在 `parser.c` 中注册 `addmember` 命令

- [x] Task 4: C 指针/数组语法解析
  - [x] 实现 `extract_base_type` 函数：剥离指针获取基础类型
  - [x] 重写 `extract_type_from_decl` 和 `extract_name_from_decl` 适配指针/数组语法
  - [x] 在 `addarg`/`addvar`/`addmember` 中集成解析逻辑，类型校验时使用基础类型

- [x] Task 5: 命令历史回滚（上箭头）
  - [x] 修改 `main.c` 的 REPL 循环，使用支持终端原始模式的输入读取
  - [x] 实现历史缓冲区，存储最近 N 条命令
  - [x] 处理上箭头键（`\033[A`）回滚上一条命令并显示

- [x] Task 6: 项目级 .cboot 导入
  - [x] 新增 `cmd_import` 命令：解析 `.cboot` 文件路径
  - [x] 自动创建 `lib/` 目录并复制导入项目文件
  - [x] 修改 `generator.c`：CMakeLists.txt 中为导入项目添加 `add_subdirectory(lib/<name>)`
  - [x] 修改 `generator.c`：CMakeLists.txt 中为导入库添加 `target_link_libraries`

- [x] Task 7: .a/.so 库导入
  - [x] 扩展 `cmd_import`：支持 `import <lib>.a <desc>.cboot` 和 `import <lib>.so <desc>.cboot`
  - [x] 复制 .a/.so 文件到 `lib/` 目录
  - [x] 解析描述 .cboot 文件获取库接口信息
  - [x] 在 CMakeLists.txt 中添加 `find_library` 和 `target_link_libraries`

- [x] Task 8: 代码生成适配新目录结构
  - [x] 确保所有自有代码生成到 `src/` 目录
  - [x] 导入库代码生成到 `lib/` 目录
  - [x] 文档生成在 api.md 中列出导入库信息

# Task Dependencies
- Task 1 依赖 Task 2, Task 3（addarg/addvar/addmember 命令创建后才能改类型校验）
- Task 2, Task 3 可并行
- Task 4 依赖 Task 2, Task 3（解析逻辑在 addarg/addvar/addmember 中调用）
- Task 5 独立（修改 main.c）
- Task 6, Task 7 可并行（均修改 commands.c + generator.c）
- Task 8 依赖 Task 6, Task 7