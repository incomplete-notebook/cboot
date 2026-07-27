/* domain.c - CBoot generated (compiler: normal) */
/* Module: domain */

#include "domain.h"

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
typedef struct Domain {
    char* name;  // 域名称
    int type;  // 域类型(DomainType枚举)
    char* comment;  // 域注释文本
    struct Domain* parent;  // 父域指针
    struct Domain** children;  // 子域数组
    int child_count;  // 子域数量
    int child_capacity;  // 子域数组容量
} Domain;

