/* docgen.h - CBoot generated (API declarations only) */
#ifndef DOCGEN_H
#define DOCGEN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 输出目录
int docgen_generate_docs(Project* proj, const char* output_dir);

void docgen_generate_module_docs(Domain* mod, const char* dir);

#endif /* DOCGEN_H */
