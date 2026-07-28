# domain API 文档

本文档列出 `domain` 模块的所有公开 API。

## 目录

- [DomainType](#DomainType)
- [ModMode](#ModMode)
- [CompilerMode](#CompilerMode)
- [ApiMode](#ApiMode)
- [TypeMode](#TypeMode)
- [VarMode](#VarMode)
- [Domain](#Domain)
- [ModuleDomain](#ModuleDomain)
- [FunctionDomain](#FunctionDomain)
- [StructDomain](#StructDomain)
- [TypeDomain](#TypeDomain)
- [MacroDomain](#MacroDomain)
- [VariableDomain](#VariableDomain)
- [MemberDomain](#MemberDomain)
- [Dependency](#Dependency)
- [Project](#Project)
- [domain_new](#domain_new)
- [module_domain_new](#module_domain_new)
- [function_domain_new](#function_domain_new)
- [struct_domain_new](#struct_domain_new)
- [type_domain_new](#type_domain_new)
- [macro_domain_new](#macro_domain_new)
- [variable_domain_new](#variable_domain_new)
- [member_domain_new](#member_domain_new)
- [domain_add_child](#domain_add_child)
- [domain_find_child](#domain_find_child)
- [domain_find_api_in_submodules](#domain_find_api_in_submodules)
- [domain_check_api_name_conflict](#domain_check_api_name_conflict)
- [domain_get_path](#domain_get_path)
- [domain_set_comment](#domain_set_comment)
- [domain_set_mode](#domain_set_mode)
- [project_new](#project_new)
- [project_add_dependency](#project_add_dependency)
- [project_has_dependency](#project_has_dependency)

---

## 函数

### struct Domain* domain_new()

创建新域节点，分配内存并初始化所有字段

```c
struct Domain* domain_new(int type, char* name, size_t size)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `type` | `int` | 域类型(DomainType枚举) |
| `name` | `char*` | 域名称 |
| `size` | `size_t` | 结构体大小 |

### struct ModuleDomain* module_domain_new()

创建新模块域

```c
struct ModuleDomain* module_domain_new(char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `char*` | 模块名称 |

### struct FunctionDomain* function_domain_new()

创建新函数域

```c
struct FunctionDomain* function_domain_new(char* name, char* return_type)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `char*` | 函数名称 |
| `return_type` | `char*` | 返回类型 |

### struct StructDomain* struct_domain_new()

创建新结构体域

```c
struct StructDomain* struct_domain_new(char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `char*` | 结构体名称 |

### struct TypeDomain* type_domain_new()

创建新类型域

```c
struct TypeDomain* type_domain_new(char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `char*` | 类型名称 |

### struct MacroDomain* macro_domain_new()

创建新宏域

```c
struct MacroDomain* macro_domain_new(char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `char*` | 宏名称 |

### struct VariableDomain* variable_domain_new()

创建新变量域

```c
struct VariableDomain* variable_domain_new(char* name, char* type)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `char*` | 变量名称 |
| `type` | `char*` | 变量类型 |

### struct MemberDomain* member_domain_new()

创建新成员域(结构体成员或函数参数)

```c
struct MemberDomain* member_domain_new(char* name, char* type)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `char*` | 成员名称 |
| `type` | `char*` | 成员类型 |

### void domain_add_child()

添加子域到父域，自动设置parent指针

```c
void domain_add_child(struct Domain* parent, struct Domain* child)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `parent` | `struct Domain*` | 父域指针 |
| `child` | `struct Domain*` | 子域指针 |

### struct Domain* domain_find_child()

按名称查找直接子域

```c
struct Domain* domain_find_child(struct Domain* parent, char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `parent` | `struct Domain*` | 父域指针 |
| `name` | `char*` | 目标名称 |

### struct Domain* domain_find_api_in_submodules()

在子模块中递归查找API项，用于名称冲突检测和类型解析

```c
struct Domain* domain_find_api_in_submodules(struct Domain* scope, char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `scope` | `struct Domain*` | 搜索起始域 |
| `name` | `char*` | 目标名称 |

### struct Domain* domain_check_api_name_conflict()

检查名称是否与子模块API项冲突

```c
struct Domain* domain_check_api_name_conflict(struct Domain* scope, char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `scope` | `struct Domain*` | 检查起始域 |
| `name` | `char*` | 目标名称 |

### char* domain_get_path()

获取域的完整路径字符串(如 /module/submodule)

```c
char* domain_get_path(struct Domain* domain)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `struct Domain*` | 目标域 |

### void domain_set_comment()

设置域的注释文本

```c
void domain_set_comment(struct Domain* domain, char* text)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `struct Domain*` | 目标域 |
| `text` | `char*` | 注释文本 |

### void domain_set_mode()

设置域的模式字段

```c
void domain_set_mode(struct Domain* domain, int mode)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `struct Domain*` | 目标域 |
| `mode` | `int` | 模式值 |

### struct Project* project_new()

创建新项目

```c
struct Project* project_new(char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `char*` | 项目名称 |

### void project_add_dependency()

添加依赖记录(im命令使用)

```c
void project_add_dependency(struct Project* proj, char* importer_path, char* source_path, char* cboot_file)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `struct Project*` | 项目指针 |
| `importer_path` | `char*` | 导入方路径 |
| `source_path` | `char*` | 源模块路径 |
| `cboot_file` | `char*` | 源.cboot文件路径 |

### int project_has_dependency()

检查依赖是否已存在

```c
int project_has_dependency(struct Project* proj, char* importer_path, char* source_path)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `struct Project*` | 项目指针 |
| `importer_path` | `char*` | 导入方路径 |
| `source_path` | `char*` | 源模块路径 |

## 类型

### DomainType

域类型枚举: module/function/struct/type/macro/variable/member

```c
typedef int DomainType;
```

### ModMode

模块模式: internal(正常编译)/external(外部API引用)

```c
typedef int ModMode;
```

### CompilerMode

编译模式: normal(普通.o)/exe(可执行)/sl(静态库)/dl(动态库)

```c
typedef int CompilerMode;
```

### ApiMode

API模式: api(公开接口)/normal(私有实现)

```c
typedef int ApiMode;
```

### TypeMode

类型模式: rename(别名)/struct(结构体)/api rename/api struct

```c
typedef int TypeMode;
```

### VarMode

变量模式: static/normal

```c
typedef int VarMode;
```

### Domain

域树基础节点，所有域类型的基类

```c
typedef struct Domain {
    char* name;
    int type;
    char* comment;
    struct Domain* parent;
    struct Domain** children;
    int child_count;
    int child_capacity;
} Domain;
```

**成员**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `char*` | 域名称 |
| `type` | `int` | 域类型(DomainType枚举) |
| `comment` | `char*` | 域注释文本 |
| `parent` | `struct Domain*` | 父域指针 |
| `children` | `struct Domain**` | 子域数组 |
| `child_count` | `int` | 子域数量 |
| `child_capacity` | `int` | 子域数组容量 |

### ModuleDomain

模块域 - 项目中的一个模块节点，含编译模式和依赖管理

```c
typedef struct ModuleDomain {
    struct Domain base;
    int mode;
    int compiler;
    char* value;
    char** includes;
    int include_count;
    char** dependencies;
    int dep_count;
} ModuleDomain;
```

**成员**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `base` | `struct Domain` | 基础域 |
| `mode` | `int` | 模块模式(ModMode枚举) |
| `compiler` | `int` | 编译模式(CompilerMode枚举): exe/sl/dl/normal |
| `value` | `char*` | 模块值 |
| `includes` | `char**` | 头文件包含列表 |
| `include_count` | `int` | 包含数量 |
| `dependencies` | `char**` | 依赖模块名称列表 |
| `dep_count` | `int` | 依赖数量 |

### FunctionDomain

函数域 - 项目中的函数定义

```c
typedef struct FunctionDomain {
    struct Domain base;
    int mode;
    char* return_type;
    char* code;
    char* value;
} FunctionDomain;
```

**成员**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `base` | `struct Domain` | 基础域 |
| `mode` | `int` | API模式(ApiMode枚举) |
| `return_type` | `char*` | 返回类型 |
| `code` | `char*` | 函数实现代码 |
| `value` | `char*` | 业务逻辑描述 |

### StructDomain

结构体域 - 项目中的结构体类型定义

```c
typedef struct StructDomain {
    struct Domain base;
    int mode;
} StructDomain;
```

**成员**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `base` | `struct Domain` | 基础域 |
| `mode` | `int` | API模式(ApiMode枚举) |

### TypeDomain

类型域 - typedef定义

```c
typedef struct TypeDomain {
    struct Domain base;
    int mode;
    char* value;
} TypeDomain;
```

**成员**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `base` | `struct Domain` | 基础域 |
| `mode` | `int` | 类型模式(TypeMode枚举) |
| `value` | `char*` | 底层类型(rename模式)或值 |

### MacroDomain

宏域 - #define定义

```c
typedef struct MacroDomain {
    struct Domain base;
    int mode;
    char* value;
} MacroDomain;
```

**成员**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `base` | `struct Domain` | 基础域 |
| `mode` | `int` | API模式(ApiMode枚举) |
| `value` | `char*` | 宏值 |

### VariableDomain

变量域 - 变量定义

```c
typedef struct VariableDomain {
    struct Domain base;
    int mode;
    char* type;
    char* value;
} VariableDomain;
```

**成员**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `base` | `struct Domain` | 基础域 |
| `mode` | `int` | 变量模式(VarMode枚举) |
| `type` | `char*` | 变量类型 |
| `value` | `char*` | 变量初始值 |

### MemberDomain

成员域 - 结构体成员或函数参数

```c
typedef struct MemberDomain {
    struct Domain base;
    char* type;
} MemberDomain;
```

**成员**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `base` | `struct Domain` | 基础域 |
| `type` | `char*` | 成员类型 |

### Dependency

依赖记录 - im命令建立的API依赖关系

```c
typedef struct Dependency {
    char* importer;
    char* source;
    char* cboot_file;
} Dependency;
```

**成员**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `importer` | `char*` | 导入方模块路径 |
| `source` | `char*` | 源模块路径 |
| `cboot_file` | `char*` | 源.cboot文件路径 |

### Project

项目容器 - 整个项目的根节点和全局状态

```c
typedef struct Project {
    char* name;
    struct Domain* root;
    struct Domain* current;
    int has_generated;
    char* cboot_file;
    struct Dependency* dependencies;
    int dep_count;
} Project;
```

**成员**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `char*` | 项目名称 |
| `root` | `struct Domain*` | 根域(包含所有模块) |
| `current` | `struct Domain*` | 当前作用域指针 |
| `has_generated` | `int` | 是否已生成代码 |
| `cboot_file` | `char*` | .cboot文件路径 |
| `dependencies` | `struct Dependency*` | API依赖数组 |
| `dep_count` | `int` | 依赖数量 |

*Generated by CBoot v0.3.1*
