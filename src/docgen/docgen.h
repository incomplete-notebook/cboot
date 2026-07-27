/* docgen.h - CBoot generated (API declarations only) */
#ifndef DOCGEN_H
#define DOCGEN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 生成文档
int generate_docs(Project* proj, char* output_dir);

// 生成模块文档
void generate_module_docs(Domain* mod, char* dir);

#endif /* DOCGEN_H */
