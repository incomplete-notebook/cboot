/* generator.h - CBoot generated (API declarations only) */
#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int generator_generate_project(Project* proj);

/* 仅重新生成 .cboot 文件（不覆盖 .c/.h/CMake），
 * 用于 cboot update 后同步 .cboot 描述文件。 */
int generator_generate_cboot_only(Project *proj);

#endif /* GENERATOR_H */
