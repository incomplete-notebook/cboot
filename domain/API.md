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
- [domain_find_child_by_type](#domain_find_child_by_type)
- [domain_delete](#domain_delete)
- [domain_remove_child](#domain_remove_child)
- [domain_find_nearest_of_type](#domain_find_nearest_of_type)
- [domain_find_in_tree](#domain_find_in_tree)
- [domain_is_api](#domain_is_api)
- [domain_set_child_comment](#domain_set_child_comment)
- [find_child_comment](#find_child_comment)
- [domain_set_value](#domain_set_value)
- [domain_set_code](#domain_set_code)
- [domain_set_call](#domain_set_call)
- [domain_get_call](#domain_get_call)
- [project_free](#project_free)
- [is_builtin_type](#is_builtin_type)

---

## 函数

### Domain* domain_new()

结构体大小

```c
Domain* domain_new(DomainType type, const char* name, size_t struct_size)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `type` | `DomainType` | - |
| `name` | `const char*` | - |
| `struct_size` | `size_t` | - |

### ModuleDomain* module_domain_new()

模块名称

```c
ModuleDomain* module_domain_new(const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

### FunctionDomain* function_domain_new()

返回类型

```c
FunctionDomain* function_domain_new(const char* name, const char* return_type)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `return_type` | `const char*` | - |

### StructDomain* struct_domain_new()

结构体名称

```c
StructDomain* struct_domain_new(const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

### TypeDomain* type_domain_new()

类型名称

```c
TypeDomain* type_domain_new(const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

### MacroDomain* macro_domain_new()

宏名称

```c
MacroDomain* macro_domain_new(const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

### VariableDomain* variable_domain_new()

变量类型

```c
VariableDomain* variable_domain_new(const char* name, const char* type)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `type` | `const char*` | - |

### MemberDomain* member_domain_new()

成员类型

```c
MemberDomain* member_domain_new(const char* name, const char* type)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `type` | `const char*` | - |

### void domain_add_child()

子域指针

```c
void domain_add_child(Domain* parent, Domain* child)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | - |
| `child` | `Domain*` | - |

### Domain* domain_find_child()

目标名称

```c
Domain* domain_find_child(Domain* parent, const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | - |
| `name` | `const char*` | - |

### Domain* domain_find_api_in_submodules()

目标名称

```c
Domain* domain_find_api_in_submodules(Domain* scope, const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `scope` | `Domain*` | - |
| `name` | `const char*` | - |

### Domain* domain_check_api_name_conflict()

目标名称

```c
Domain* domain_check_api_name_conflict(Domain* scope, const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `scope` | `Domain*` | - |
| `name` | `const char*` | - |

### char* domain_get_path()

目标域

```c
char* domain_get_path(Domain* domain)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |

### void domain_set_comment()

注释文本

```c
void domain_set_comment(Domain* domain, const char* text)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `text` | `const char*` | - |

### void domain_set_mode()

模式值

```c
void domain_set_mode(Domain* domain, int mode)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `mode` | `int` | - |

### Project* project_new()

项目名称

```c
Project* project_new(const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

### void project_add_dependency()

源.cboot文件路径

```c
void project_add_dependency(Project* proj, const char* importer_path, const char* source_path, const char* cboot_file)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |
| `importer_path` | `const char*` | - |
| `source_path` | `const char*` | - |
| `cboot_file` | `const char*` | - |

### int project_has_dependency()

源模块路径

```c
int project_has_dependency(Project* proj, const char* importer_path, const char* source_path)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |
| `importer_path` | `const char*` | - |
| `source_path` | `const char*` | - |

### Domain* domain_find_child_by_type()

```c
Domain* domain_find_child_by_type(Domain* parent, const char* name, DomainType type)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | - |
| `name` | `const char*` | - |
| `type` | `DomainType` | - |

### void domain_delete()

```c
void domain_delete(Domain* domain)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |

### void domain_remove_child()

```c
void domain_remove_child(Domain* parent, Domain* child)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | - |
| `child` | `Domain*` | - |

### Domain* domain_find_nearest_of_type()

```c
Domain* domain_find_nearest_of_type(Domain* from, DomainType type)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `from` | `Domain*` | - |
| `type` | `DomainType` | - |

### Domain* domain_find_in_tree()

```c
Domain* domain_find_in_tree(Domain* root, DomainType type, const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `root` | `Domain*` | - |
| `type` | `DomainType` | - |
| `name` | `const char*` | - |

### int domain_is_api()

```c
int domain_is_api(Domain* domain)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |

### void domain_set_child_comment()

```c
void domain_set_child_comment(Domain* domain, const char* target, const char* text)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `target` | `const char*` | - |
| `text` | `const char*` | - |

### Comment* find_child_comment()

```c
Comment* find_child_comment(Domain* domain, const char* target)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `target` | `const char*` | - |

### void domain_set_value()

```c
void domain_set_value(Domain* domain, const char* value)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `value` | `const char*` | - |

### void domain_set_code()

```c
void domain_set_code(Domain* domain, const char* code)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `code` | `const char*` | - |

### void domain_set_call()

```c
void domain_set_call(Domain* domain, const char* call)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `call` | `const char*` | - |

### const char* domain_get_call()

```c
const char* domain_get_call(Domain* domain)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |

### void project_free()

```c
void project_free(Project* proj)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |

### int is_builtin_type()

```c
int is_builtin_type(const char* type_name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `type_name` | `const char*` | - |

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

子域数组容量

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
| `name` | `char*` | - |
| `type` | `int` | - |
| `comment` | `char*` | - |
| `parent` | `struct Domain*` | - |
| `children` | `struct Domain**` | - |
| `child_count` | `int` | - |
| `child_capacity` | `int` | - |

### ModuleDomain

依赖数量

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
| `base` | `struct Domain` | - |
| `mode` | `int` | - |
| `compiler` | `int` | - |
| `value` | `char*` | - |
| `includes` | `char**` | - |
| `include_count` | `int` | - |
| `dependencies` | `char**` | - |
| `dep_count` | `int` | - |

### FunctionDomain

业务逻辑描述

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
| `base` | `struct Domain` | - |
| `mode` | `int` | - |
| `return_type` | `char*` | - |
| `code` | `char*` | - |
| `value` | `char*` | - |

### StructDomain

API模式(ApiMode枚举)

```c
typedef struct StructDomain {
    struct Domain base;
    int mode;
} StructDomain;
```

**成员**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `base` | `struct Domain` | - |
| `mode` | `int` | - |

### TypeDomain

底层类型(rename模式)或值

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
| `base` | `struct Domain` | - |
| `mode` | `int` | - |
| `value` | `char*` | - |

### MacroDomain

宏值

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
| `base` | `struct Domain` | - |
| `mode` | `int` | - |
| `value` | `char*` | - |

### VariableDomain

变量初始值

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
| `base` | `struct Domain` | - |
| `mode` | `int` | - |
| `type` | `char*` | - |
| `value` | `char*` | - |

### MemberDomain

成员类型

```c
typedef struct MemberDomain {
    struct Domain base;
    char* type;
} MemberDomain;
```

**成员**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `base` | `struct Domain` | - |
| `type` | `char*` | - |

### Dependency

源.cboot文件路径

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
| `importer` | `char*` | - |
| `source` | `char*` | - |
| `cboot_file` | `char*` | - |

### Project

依赖数量

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
| `name` | `char*` | - |
| `root` | `struct Domain*` | - |
| `current` | `struct Domain*` | - |
| `has_generated` | `int` | - |
| `cboot_file` | `char*` | - |
| `dependencies` | `struct Dependency*` | - |
| `dep_count` | `int` | - |

*Generated by CBoot v0.3.1*
