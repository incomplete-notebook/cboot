/* docgen.h - CBoot generated (API declarations only) */
#ifndef DOCGEN_H
#define DOCGEN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declarations for struct types used in API signatures */
typedef struct Project Project;
typedef struct Domain Domain;

// 生成项目级文档
int generate_docs(struct Project* proj, char* output_dir);

// 为单个模块生成文档
void generate_module_docs(struct Domain* mod, char* dir);

#endif /* DOCGEN_H */
