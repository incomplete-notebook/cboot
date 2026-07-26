# Tasks

- [x] Task 1: 修复 CMakeLists.txt 生成的宏定义值
  - [x] 修改 `generate_c_file` 中宏输出，保留 `#define MACRO VALUE` 完整形式
  - [x] 修复 `generate_h_file` 中可能存在的相同问题

- [x] Task 2: CMakeLists.txt 添加模块子目录 include path
  - [x] 遍历所有模块，对有 `has_directory` 的模块，在 CMake 中添加对应的 `target_include_directories`
  - [x] 确保资源文件路径使用 `${CMAKE_SOURCE_DIR}/res/` 前缀

- [x] Task 3: 完善 api.md 文档生成，显示 root 模块内容
  - [x] 修改 `generate_api_md`，在遍历子模块前先输出 root 模块自身的函数、类型、宏
  - [x] 递归处理 root 模块的直接子模块

- [x] Task 4: 修复 project.man 重复 TYPES 节
  - [x] 将 `.SH TYPES` 移到 `write_module_man` 外部，在 `generate_man_page` 中统一输出一次
  - [x] 所有模块的类型定义汇总到同一个 TYPES 节下

- [x] Task 5: 强化资源文件复制
  - [x] 替换 `cmd_res` 中的 `system("cp ...")` 为 fopen/fread/fwrite 实现
  - [x] 添加源文件存在性校验，不存在时报错
  - [x] 添加复制失败时的错误提示

# Task Dependencies
- Task 1, Task 2 可并行（均修改 generator.c）
- Task 3, Task 4 可并行（均修改 docgen.c）
- Task 5 独立（修改 commands.c）