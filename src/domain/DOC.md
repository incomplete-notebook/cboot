# domain 完整文档

域数据模型

## 目录

- [domain_new()](#domain_new)
- [domain_add_child()](#domain_add_child)
- [domain_delete()](#domain_delete)
- [domain_remove_child()](#domain_remove_child)
- [domain_find_child()](#domain_find_child)
- [domain_find_nearest_of_type()](#domain_find_nearest_of_type)
- [domain_get_path()](#domain_get_path)
- [domain_set_comment()](#domain_set_comment)
- [domain_set_value()](#domain_set_value)
- [domain_set_mode()](#domain_set_mode)
- [project_new()](#project_new)
- [project_free()](#project_free)
- [DomainType](#DomainType)
- [ModMode](#ModMode)
- [ApiMode](#ApiMode)
- [TypeMode](#TypeMode)
- [VarMode](#VarMode)
- [RunMode](#RunMode)
- [Comment](#Comment)
- [Domain](#Domain)
- [ModuleDomain](#ModuleDomain)
- [FunctionDomain](#FunctionDomain)
- [StructDomain](#StructDomain)
- [TypeDomain](#TypeDomain)
- [MacroDomain](#MacroDomain)
- [VariableDomain](#VariableDomain)
- [MemberDomain](#MemberDomain)
- [Project](#Project)

---

## domain_new()

**声明**: `Domain* domain_new(DomainType type, char* name, size_t struct_size)`

**注释**: 创建新域

| 参数 | 类型 | 说明 |
|------|------|------|
| `type` | `DomainType` | 域类型 |
| `name` | `char*` | 名称 |
| `struct_size` | `size_t` | 结构体大小 |

**调用栈**: (此函数在模块 domain 中定义)

```c
//请在这里输入代码
```

---

## domain_add_child()

**声明**: `void domain_add_child(Domain* parent, Domain* child)`

**注释**: 添加子域

| 参数 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | 父域 |
| `child` | `Domain*` | 子域 |

**调用栈**: (此函数在模块 domain 中定义)

```c
//请在这里输入代码
```

---

## domain_delete()

**声明**: `void domain_delete(Domain* domain)`

**注释**: 删除域

| 参数 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | 域 |

**调用栈**: (此函数在模块 domain 中定义)

```c
//请在这里输入代码
```

---

## domain_remove_child()

**声明**: `void domain_remove_child(Domain* parent, Domain* child)`

**注释**: 移除子域

| 参数 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | 父域 |
| `child` | `Domain*` | 子域 |

**调用栈**: (此函数在模块 domain 中定义)

```c
//请在这里输入代码
```

---

## domain_find_child()

**声明**: `Domain* domain_find_child(Domain* parent, char* name)`

**注释**: 查找子域

| 参数 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | 父域 |
| `name` | `char*` | 名称 |

**调用栈**: (此函数在模块 domain 中定义)

```c
//请在这里输入代码
```

---

## domain_find_nearest_of_type()

**声明**: `Domain* domain_find_nearest_of_type(Domain* from, DomainType type)`

**注释**: 向上查找最近类型域

| 参数 | 类型 | 说明 |
|------|------|------|
| `from` | `Domain*` | 起始域 |
| `type` | `DomainType` | 域类型 |

**调用栈**: (此函数在模块 domain 中定义)

```c
//请在这里输入代码
```

---

## domain_get_path()

**声明**: `char* domain_get_path(Domain* domain)`

**注释**: 获取域路径

| 参数 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | 域 |

**调用栈**: (此函数在模块 domain 中定义)

```c
//请在这里输入代码
```

---

## domain_set_comment()

**声明**: `void domain_set_comment(Domain* domain, char* text)`

**注释**: 设置注释

| 参数 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | 域 |
| `text` | `char*` | 文本 |

**调用栈**: (此函数在模块 domain 中定义)

```c
//请在这里输入代码
```

---

## domain_set_value()

**声明**: `void domain_set_value(Domain* domain, char* value)`

**注释**: 设置值

| 参数 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | 域 |
| `value` | `char*` | 值 |

**调用栈**: (此函数在模块 domain 中定义)

```c
//请在这里输入代码
```

---

## domain_set_mode()

**声明**: `void domain_set_mode(Domain* domain, int mode)`

**注释**: 设置模式

| 参数 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | 域 |
| `mode` | `int` | 模式 |

**调用栈**: (此函数在模块 domain 中定义)

```c
//请在这里输入代码
```

---

## project_new()

**声明**: `Project* project_new(char* name)`

**注释**: 创建项目

| 参数 | 类型 | 说明 |
|------|------|------|
| `name` | `char*` | 项目名 |

**调用栈**: (此函数在模块 domain 中定义)

```c
//请在这里输入代码
```

---

## project_free()

**声明**: `void project_free(Project* proj)`

**注释**: 释放项目

| 参数 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | 项目 |

**调用栈**: (此函数在模块 domain 中定义)

```c
//请在这里输入代码
```

---

## 定义

### typedef DomainType

**注释**: 域类型枚举

```c
typedef int DomainType;
```

### typedef ModMode

**注释**: 模块模式: internal/external

```c
typedef int ModMode;
```

### typedef ApiMode

**注释**: API模式: api/normal

```c
typedef int ApiMode;
```

### typedef TypeMode

**注释**: 类型模式: rename/struct/api rename/api struct

```c
typedef int TypeMode;
```

### typedef VarMode

**注释**: 变量模式: static/normal

```c
typedef int VarMode;
```

### typedef RunMode

**注释**: 运行模式

```c
typedef int RunMode;
```

### struct Comment

**注释**: 注释

```c
typedef struct Comment {
    char* target;
    char* text;
} Comment;
```

### struct Domain

**注释**: 域基类

```c
typedef struct Domain {
    DomainType type;
    char* name;
    Domain* parent;
    Domain** children;
    int child_count;
    int child_capacity;
    Comment* comments;
    int comment_count;
    int comment_capacity;
    char* comment;
} Domain;
```

### struct ModuleDomain

**注释**: 模块域

```c
typedef struct ModuleDomain {
    Domain base;
    ModMode mode;
    char* value;
    char** includes;
    int include_count;
    int include_capacity;
    char** dependencies;
    int dep_count;
    int dep_capacity;
} ModuleDomain;
```

### struct FunctionDomain

**注释**: 函数域

```c
typedef struct FunctionDomain {
    Domain base;
    ApiMode mode;
    char* return_type;
    char* code;
    char* value;
} FunctionDomain;
```

### struct StructDomain

**注释**: 结构体域

```c
typedef struct StructDomain {
    Domain base;
    ApiMode mode;
} StructDomain;
```

### struct TypeDomain

**注释**: 类型域

```c
typedef struct TypeDomain {
    Domain base;
    TypeMode mode;
    char* value;
} TypeDomain;
```

### struct MacroDomain

**注释**: 宏域

```c
typedef struct MacroDomain {
    Domain base;
    ApiMode mode;
    char* value;
} MacroDomain;
```

### struct VariableDomain

**注释**: 变量域

```c
typedef struct VariableDomain {
    Domain base;
    VarMode mode;
    char* type;
    char* value;
} VariableDomain;
```

### struct MemberDomain

**注释**: 成员域

```c
typedef struct MemberDomain {
    Domain base;
    char* type;
} MemberDomain;
```

### struct Project

**注释**: 项目状态

```c
typedef struct Project {
    char* name;
    Domain* root;
    Domain* current;
    int has_generated;
    char* cboot_file;
    int fine_tune_mode;
} Project;
```

*Generated by CBoot v2.0*
