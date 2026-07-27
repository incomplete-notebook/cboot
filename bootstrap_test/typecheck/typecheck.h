/* typecheck.h - CBoot generated (API declarations only) */
#ifndef TYPECHECK_H
#define TYPECHECK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declarations for struct types used in API signatures */
typedef struct Domain Domain;
typedef struct Project Project;

// 检查单个域的类型一致性
int typecheck_domain(struct Domain* domain);

// 检查整个项目的类型一致性
int typecheck_project(struct Project* proj);

#endif /* TYPECHECK_H */
