/* generator.c - CBoot generated (compiler: normal) */
/* Module: generator */

#include "generator.h"

// 生成完整项目代码
// 业务逻辑: 递归处理所有模块，生成.c/.h/层级化CMake/文档/DEPENDENCIES.md
int generate_project(struct Project* proj, char* output_dir) {
//请在这里输入代码
}

// 为单个模块生成代码
// 业务逻辑: 创建模块目录，生成.c/.h/CMakeLists.txt/.cboot/文档，递归处理子模块
void generate_module(struct Domain* mod, char* parent_dir) {
//请在这里输入代码
}

// 生成模块的.c源文件
// 业务逻辑: 包含头文件、宏、类型、变量、函数实现；非API函数自动static化避免符号冲突
void generate_mod_c(struct Domain* mod, char* dir) {
//请在这里输入代码
}

// 生成模块的.h头文件(仅API声明)
// 业务逻辑: 生成include guard、API宏/类型/函数声明；dl模式添加dll导出宏
void generate_mod_h(struct Domain* mod, char* dir) {
//请在这里输入代码
}

// 生成模块的CMakeLists.txt
// 业务逻辑: 根据compiler模式生成: normal(收集.o)/exe(可执行)/sl(静态库)/dl(动态库)，含子模块add_subdirectory
void generate_mod_cmake(struct Domain* mod, char* dir) {
//请在这里输入代码
}

// 生成顶层CMakeLists.txt
// 业务逻辑: 无exe模块时收集所有normal模块源文件+main.c生成默认可执行；有exe模块时链接库模块
void generate_top_cmake(struct Project* proj) {
//请在这里输入代码
}

// 生成顶层main.c入口
// 业务逻辑: 仅在无exe模块时生成默认main.c，包含所有顶层模块头文件
void generate_top_main(struct Project* proj) {
//请在这里输入代码
}

