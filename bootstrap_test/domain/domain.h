/* domain.h - CBoot generated (API declarations only) */
#ifndef DOMAIN_H
#define DOMAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 域类型枚举: module/function/struct/type/macro/variable/member
typedef int DomainType;

// 模块模式: internal(正常编译)/external(外部API引用)
typedef int ModMode;

// 编译模式: normal(普通.o)/exe(可执行)/sl(静态库)/dl(动态库)
typedef int CompilerMode;

// API模式: api(公开接口)/normal(私有实现)
typedef int ApiMode;

// 类型模式: rename(别名)/struct(结构体)/api rename/api struct
typedef int TypeMode;

// 变量模式: static/normal
typedef int VarMode;

// 域树基础节点，所有域类型的基类
typedef struct Domain Domain;

#endif /* DOMAIN_H */
