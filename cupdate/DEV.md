# cupdate 开发文档

代码更新模块 - 解析C源码并同步.cboot描述文件

## 统计

- 公开 API: **13** 项
- 私有实现: **12** 项

## 函数

### int run_project()

> `API` — 公开接口

**业务逻辑**: 遍历所有src模式模块，读取.c文件，解析C声明，更新code字段和API定义，重新生成.cboot

**说明**: 输出参数：返回警告数量（可为NULL）

```c
int run_project(Project* proj, int* error_count_out, int* warning_count_out)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |
| `error_count_out` | `int*` | - |
| `warning_count_out` | `int*` | - |

---

### int parse_source()

> `API` — 公开接口

**业务逻辑**: 词法分析+语法分析，提取函数/结构体/typedef/宏/变量声明，检测语法错误

**说明**: 解析结果输出（调用者负责初始化和释放）

```c
int parse_source(const char* source, const char* filename, CUPResult* r)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `source` | `const char*` | - |
| `filename` | `const char*` | - |
| `r` | `CUPResult*` | - |

---

### void result_init()

> `API` — 公开接口

**说明**: 要初始化的结果容器

```c
void result_init(CUPResult* r)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `r` | `CUPResult*` | - |

---

### void result_free()

> `API` — 公开接口

**说明**: 要释放的结果容器

```c
void result_free(CUPResult* r)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `r` | `CUPResult*` | - |

---

### void result_add_error()

> `API` — 公开接口

**说明**: 行号

```c
void result_add_error(CUPResult* r, const char* msg, int line)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `r` | `CUPResult*` | - |
| `msg` | `const char*` | - |
| `line` | `int` | - |

---

### void result_add_warning()

> `API` — 公开接口

**说明**: 行号

```c
void result_add_warning(CUPResult* r, const char* msg, int line)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `r` | `CUPResult*` | - |
| `msg` | `const char*` | - |
| `line` | `int` | - |

---

### CUPDecl* result_add_decl()

> `API` — 公开接口

**说明**: 结果容器

```c
CUPDecl* result_add_decl(CUPResult* r)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `r` | `CUPResult*` | - |

---

### void cup_free_decl()

```c
void cup_free_decl(CUPDecl* d)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `d` | `CUPDecl*` | - |

---

### char* cup_read_file()

```c
char* cup_read_file(const char* path)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `path` | `const char*` | - |

---

### void cup_get_module_prefix()

```c
void cup_get_module_prefix(Domain* mod, char* buf, size_t size)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `buf` | `char*` | - |
| `size` | `size_t` | - |

---

### FunctionDomain* cup_find_function()

```c
FunctionDomain* cup_find_function(ModuleDomain* mod, const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `ModuleDomain*` | - |
| `name` | `const char*` | - |

---

### StructDomain* cup_find_struct()

```c
StructDomain* cup_find_struct(ModuleDomain* mod, const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `ModuleDomain*` | - |
| `name` | `const char*` | - |

---

### TypeDomain* cup_find_type()

```c
TypeDomain* cup_find_type(ModuleDomain* mod, const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `ModuleDomain*` | - |
| `name` | `const char*` | - |

---

### MacroDomain* cup_find_macro()

```c
MacroDomain* cup_find_macro(ModuleDomain* mod, const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `ModuleDomain*` | - |
| `name` | `const char*` | - |

---

### VariableDomain* cup_find_variable()

```c
VariableDomain* cup_find_variable(ModuleDomain* mod, const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `ModuleDomain*` | - |
| `name` | `const char*` | - |

---

### void cup_sync_function_params()

```c
void cup_sync_function_params(FunctionDomain* func, CUPDecl* decl)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `func` | `FunctionDomain*` | - |
| `decl` | `CUPDecl*` | - |

---

### void cup_sync_struct_members()

```c
void cup_sync_struct_members(Domain* sd, CUPDecl* decl)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `sd` | `Domain*` | - |
| `decl` | `CUPDecl*` | - |

---

### void cup_sync_decl()

```c
void cup_sync_decl(ModuleDomain* mod, CUPDecl* decl, int* changes)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `ModuleDomain*` | - |
| `decl` | `CUPDecl*` | - |
| `changes` | `int*` | - |

---

### int run_module()

> `API` — 公开接口

```c
int run_module(ModuleDomain* mod, const char* mod_dir)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `ModuleDomain*` | - |
| `mod_dir` | `const char*` | - |

---

### int cup_update_recursive()

```c
int cup_update_recursive(Domain* dom, const char* dir, int* total_errors, int* total_warnings)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `dom` | `Domain*` | - |
| `dir` | `const char*` | - |
| `total_errors` | `int*` | - |
| `total_warnings` | `int*` | - |

---

## 类型

### CUPDeclKind

> `API` — 公开类型

**说明**: 顶层声明种类枚举: function/struct/enum/typedef/macro/variable/include/other

```c
typedef int CUPDeclKind;
```

---

### CUParParam

> `API` — 公开类型

**说明**: 参数名（可能为空）

```c
typedef struct CUParParam { ... } CUParParam;
```

**成员**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `type` | `char*` | - |
| `name` | `char*` | - |

---

### CUParMember

> `API` — 公开类型

**说明**: 位域宽度或初始值（可选）

```c
typedef struct CUParMember { ... } CUParMember;
```

**成员**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `type` | `char*` | - |
| `name` | `char*` | - |
| `value` | `char*` | - |

---

### CUPDecl

> `API` — 公开类型

**说明**: 是否对外可见（非static）

```c
typedef struct CUPDecl { ... } CUPDecl;
```

**成员**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `kind` | `int` | - |
| `name` | `char*` | - |
| `return_type` | `char*` | - |
| `base_type` | `char*` | - |
| `value` | `char*` | - |
| `body` | `char*` | - |
| `call` | `char*` | - |
| `is_function_def` | `int` | - |
| `is_static` | `int` | - |
| `is_inline` | `int` | - |
| `is_api` | `int` | - |

---

### CUPResult

> `API` — 公开类型

**说明**: 警告数量

```c
typedef struct CUPResult { ... } CUPResult;
```

**成员**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `decls` | `struct CUPDecl` | - |
| `decl_count` | `int` | - |
| `decl_capacity` | `int` | - |
| `error_count` | `int` | - |
| `warning_count` | `int` | - |

---

## 子模块

- [domain](domain/DEV.md): [API 引用] 从项目内模块 domain 导入

*Generated by CBoot v0.3.1*
