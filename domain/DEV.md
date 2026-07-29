# domain 开发文档

域数据模型 - 定义所有域类型、域树结构和项目容器

## 统计

- 公开 API: **48** 项
- 私有实现: **0** 项

## 函数

### Domain* domain_new()

> `API` — 公开接口

**业务逻辑**: 分配内存，设置类型和名称，初始化子域数组，返回新指针

**说明**: 结构体大小

```c
Domain* domain_new(DomainType type, const char* name, size_t struct_size)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `type` | `DomainType` | - |
| `name` | `const char*` | - |
| `struct_size` | `size_t` | - |

---

### ModuleDomain* module_domain_new()

> `API` — 公开接口

**业务逻辑**: 调用domain_new，设置默认模式为INTERNAL，默认编译模式为NORMAL

**说明**: 模块名称

```c
ModuleDomain* module_domain_new(const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

---

### FunctionDomain* function_domain_new()

> `API` — 公开接口

**业务逻辑**: 调用domain_new，设置返回类型，默认API模式为NORMAL

**说明**: 返回类型

```c
FunctionDomain* function_domain_new(const char* name, const char* return_type)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `return_type` | `const char*` | - |

---

### StructDomain* struct_domain_new()

> `API` — 公开接口

**业务逻辑**: 调用domain_new，默认API模式为NORMAL

**说明**: 结构体名称

```c
StructDomain* struct_domain_new(const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

---

### TypeDomain* type_domain_new()

> `API` — 公开接口

**业务逻辑**: 调用domain_new，默认类型模式为RENAME

**说明**: 类型名称

```c
TypeDomain* type_domain_new(const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

---

### MacroDomain* macro_domain_new()

> `API` — 公开接口

**业务逻辑**: 调用domain_new，默认API模式为NORMAL

**说明**: 宏名称

```c
MacroDomain* macro_domain_new(const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

---

### VariableDomain* variable_domain_new()

> `API` — 公开接口

**业务逻辑**: 调用domain_new，设置类型，默认变量模式为NORMAL

**说明**: 变量类型

```c
VariableDomain* variable_domain_new(const char* name, const char* type)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `type` | `const char*` | - |

---

### MemberDomain* member_domain_new()

> `API` — 公开接口

**业务逻辑**: 调用domain_new，设置类型

**说明**: 成员类型

```c
MemberDomain* member_domain_new(const char* name, const char* type)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `type` | `const char*` | - |

---

### void domain_add_child()

> `API` — 公开接口

**业务逻辑**: 检查容量，扩容数组，添加子域，设置parent

**说明**: 子域指针

```c
void domain_add_child(Domain* parent, Domain* child)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | - |
| `child` | `Domain*` | - |

---

### Domain* domain_find_child()

> `API` — 公开接口

**业务逻辑**: 遍历子域数组，返回匹配名称的子域指针，未找到返回NULL

**说明**: 目标名称

```c
Domain* domain_find_child(Domain* parent, const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | - |
| `name` | `const char*` | - |

---

### Domain* domain_find_api_in_submodules()

> `API` — 公开接口

**业务逻辑**: 先在直接子域中查找，再递归进入子模块查找API模式项

**说明**: 目标名称

```c
Domain* domain_find_api_in_submodules(Domain* scope, const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `scope` | `Domain*` | - |
| `name` | `const char*` | - |

---

### Domain* domain_check_api_name_conflict()

> `API` — 公开接口

**业务逻辑**: 调用domain_find_api_in_submodules，返回冲突域或NULL

**说明**: 目标名称

```c
Domain* domain_check_api_name_conflict(Domain* scope, const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `scope` | `Domain*` | - |
| `name` | `const char*` | - |

---

### char* domain_get_path()

> `API` — 公开接口

**业务逻辑**: 从当前域向上遍历到根，拼接路径字符串

**说明**: 目标域

```c
char* domain_get_path(Domain* domain)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |

---

### void domain_set_comment()

> `API` — 公开接口

**业务逻辑**: 复制文本并设置到域的comment字段

**说明**: 注释文本

```c
void domain_set_comment(Domain* domain, const char* text)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `text` | `const char*` | - |

---

### void domain_set_mode()

> `API` — 公开接口

**业务逻辑**: 根据域类型设置对应的模式字段

**说明**: 模式值

```c
void domain_set_mode(Domain* domain, int mode)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `mode` | `int` | - |

---

### Project* project_new()

> `API` — 公开接口

**业务逻辑**: 创建根模块域，初始化项目状态和依赖数组

**说明**: 项目名称

```c
Project* project_new(const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

---

### void project_add_dependency()

> `API` — 公开接口

**业务逻辑**: 检查容量，扩容数组，添加新依赖记录

**说明**: 源.cboot文件路径

```c
void project_add_dependency(Project* proj, const char* importer_path, const char* source_path, const char* cboot_file)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |
| `importer_path` | `const char*` | - |
| `source_path` | `const char*` | - |
| `cboot_file` | `const char*` | - |

---

### int project_has_dependency()

> `API` — 公开接口

**业务逻辑**: 遍历依赖数组，返回1(存在)或0(不存在)

**说明**: 源模块路径

```c
int project_has_dependency(Project* proj, const char* importer_path, const char* source_path)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |
| `importer_path` | `const char*` | - |
| `source_path` | `const char*` | - |

---

### Domain* domain_find_child_by_type()

> `API` — 公开接口

```c
Domain* domain_find_child_by_type(Domain* parent, const char* name, DomainType type)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | - |
| `name` | `const char*` | - |
| `type` | `DomainType` | - |

---

### void domain_delete()

> `API` — 公开接口

```c
void domain_delete(Domain* domain)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |

---

### void domain_remove_child()

> `API` — 公开接口

```c
void domain_remove_child(Domain* parent, Domain* child)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | - |
| `child` | `Domain*` | - |

---

### Domain* domain_find_nearest_of_type()

> `API` — 公开接口

```c
Domain* domain_find_nearest_of_type(Domain* from, DomainType type)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `from` | `Domain*` | - |
| `type` | `DomainType` | - |

---

### Domain* domain_find_in_tree()

> `API` — 公开接口

```c
Domain* domain_find_in_tree(Domain* root, DomainType type, const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `root` | `Domain*` | - |
| `type` | `DomainType` | - |
| `name` | `const char*` | - |

---

### int domain_is_api()

> `API` — 公开接口

```c
int domain_is_api(Domain* domain)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |

---

### void domain_set_child_comment()

> `API` — 公开接口

```c
void domain_set_child_comment(Domain* domain, const char* target, const char* text)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `target` | `const char*` | - |
| `text` | `const char*` | - |

---

### Comment* find_child_comment()

> `API` — 公开接口

```c
Comment* find_child_comment(Domain* domain, const char* target)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `target` | `const char*` | - |

---

### void domain_set_value()

> `API` — 公开接口

```c
void domain_set_value(Domain* domain, const char* value)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `value` | `const char*` | - |

---

### void domain_set_code()

> `API` — 公开接口

```c
void domain_set_code(Domain* domain, const char* code)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `code` | `const char*` | - |

---

### void domain_set_call()

> `API` — 公开接口

```c
void domain_set_call(Domain* domain, const char* call)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `call` | `const char*` | - |

---

### const char* domain_get_call()

> `API` — 公开接口

```c
const char* domain_get_call(Domain* domain)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |

---

### void project_free()

> `API` — 公开接口

```c
void project_free(Project* proj)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |

---

### int is_builtin_type()

> `API` — 公开接口

```c
int is_builtin_type(const char* type_name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `type_name` | `const char*` | - |

---

## 类型

### DomainType

> `API` — 公开类型

**说明**: 域类型枚举: module/function/struct/type/macro/variable/member

```c
typedef int DomainType;
```

---

### ModMode

> `API` — 公开类型

**说明**: 模块模式: internal(正常编译)/external(外部API引用)

```c
typedef int ModMode;
```

---

### CompilerMode

> `API` — 公开类型

**说明**: 编译模式: normal(普通.o)/exe(可执行)/sl(静态库)/dl(动态库)

```c
typedef int CompilerMode;
```

---

### ApiMode

> `API` — 公开类型

**说明**: API模式: api(公开接口)/normal(私有实现)

```c
typedef int ApiMode;
```

---

### TypeMode

> `API` — 公开类型

**说明**: 类型模式: rename(别名)/struct(结构体)/api rename/api struct

```c
typedef int TypeMode;
```

---

### VarMode

> `API` — 公开类型

**说明**: 变量模式: static/normal

```c
typedef int VarMode;
```

---

### Domain

> `API` — 公开类型

**说明**: 子域数组容量

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

---

### ModuleDomain

> `API` — 公开类型

**说明**: 依赖数量

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

---

### FunctionDomain

> `API` — 公开类型

**说明**: 业务逻辑描述

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

---

### StructDomain

> `API` — 公开类型

**说明**: API模式(ApiMode枚举)

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

---

### TypeDomain

> `API` — 公开类型

**说明**: 底层类型(rename模式)或值

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

---

### MacroDomain

> `API` — 公开类型

**说明**: 宏值

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

---

### VariableDomain

> `API` — 公开类型

**说明**: 变量初始值

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

---

### MemberDomain

> `API` — 公开类型

**说明**: 成员类型

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

---

### Dependency

> `API` — 公开类型

**说明**: 源.cboot文件路径

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

---

### Project

> `API` — 公开类型

**说明**: 依赖数量

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

---

*Generated by CBoot v0.3.1*
