/* generator.h - CBoot generated (API declarations only) */
#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declarations for struct types used in API signatures */
typedef struct Project Project;
typedef struct Domain Domain;

// 生成完整项目代码
int generate_project(struct Project* proj, char* output_dir);

// 为单个模块生成代码
void generate_module(struct Domain* mod, char* parent_dir);

// 生成模块的.c源文件
void generate_mod_c(struct Domain* mod, char* dir);

// 生成模块的.h头文件(仅API声明)
void generate_mod_h(struct Domain* mod, char* dir);

// 生成模块的CMakeLists.txt
void generate_mod_cmake(struct Domain* mod, char* dir);

// 生成顶层CMakeLists.txt
void generate_top_cmake(struct Project* proj);

// 生成顶层main.c入口
void generate_top_main(struct Project* proj);

#endif /* GENERATOR_H */
