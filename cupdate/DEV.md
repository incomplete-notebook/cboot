# cupdate 开发文档

代码更新模块 - 解析C源码并同步.cboot描述文件

## 统计

- 公开 API: **8** 项
- 私有实现: **38** 项

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

### void result_init()

> `API` — 公开接口

```c
void result_init(CUPResult* r)
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

### void result_free()

> `API` — 公开接口

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

```c
CUPDecl* result_add_decl(CUPResult* r)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `r` | `CUPResult*` | - |

---

### int parse_source()

> `API` — 公开接口

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

### FunctionDomain* cup_create_new_function()

```c
FunctionDomain* cup_create_new_function(ModuleDomain* mod, CUPDecl* decl)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `ModuleDomain*` | - |
| `decl` | `CUPDecl*` | - |

---

### int cup_replace_str_field()

```c
int cup_replace_str_field(char** field, const char* new_val)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `field` | `char**` | - |
| `new_val` | `const char*` | - |

---

### int cup_sync_func_body()

```c
int cup_sync_func_body(FunctionDomain* func, CUPDecl* decl)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `func` | `FunctionDomain*` | - |
| `decl` | `CUPDecl*` | - |

---

### int cup_update_existing_function()

```c
int cup_update_existing_function(FunctionDomain* func, CUPDecl* decl)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `func` | `FunctionDomain*` | - |
| `decl` | `CUPDecl*` | - |

---

### void cup_sync_function_decl()

```c
void cup_sync_function_decl(ModuleDomain* mod, CUPDecl* decl, int* changes)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `ModuleDomain*` | - |
| `decl` | `CUPDecl*` | - |
| `changes` | `int*` | - |

---

### void cup_sync_struct_decl()

```c
void cup_sync_struct_decl(ModuleDomain* mod, CUPDecl* decl, int* changes)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `ModuleDomain*` | - |
| `decl` | `CUPDecl*` | - |
| `changes` | `int*` | - |

---

### void cup_sync_typedef_decl()

```c
void cup_sync_typedef_decl(ModuleDomain* mod, CUPDecl* decl, int* changes)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `ModuleDomain*` | - |
| `decl` | `CUPDecl*` | - |
| `changes` | `int*` | - |

---

### void cup_sync_macro_decl()

```c
void cup_sync_macro_decl(ModuleDomain* mod, CUPDecl* decl, int* changes)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `ModuleDomain*` | - |
| `decl` | `CUPDecl*` | - |
| `changes` | `int*` | - |

---

### void cup_sync_var_fields()

```c
void cup_sync_var_fields(VariableDomain* vd, CUPDecl* decl, int* changes)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `vd` | `VariableDomain*` | - |
| `decl` | `CUPDecl*` | - |
| `changes` | `int*` | - |

---

### void cup_sync_variable_decl()

```c
void cup_sync_variable_decl(ModuleDomain* mod, CUPDecl* decl, int* changes)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `ModuleDomain*` | - |
| `decl` | `CUPDecl*` | - |
| `changes` | `int*` | - |

---

### void cup_sync_enum_decl()

```c
void cup_sync_enum_decl(ModuleDomain* mod, CUPDecl* decl, int* changes)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `ModuleDomain*` | - |
| `decl` | `CUPDecl*` | - |
| `changes` | `int*` | - |

---

### int cup_decl_type_matches()

```c
int cup_decl_type_matches(DomainType dtype, CUPDeclKind dkind)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `dtype` | `DomainType` | - |
| `dkind` | `CUPDeclKind` | - |

---

### int cup_find_matching_decl()

```c
int cup_find_matching_decl(Domain* c, CUPResult* result, const char* prefix, size_t plen)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `c` | `Domain*` | - |
| `result` | `CUPResult*` | - |
| `prefix` | `const char*` | - |
| `plen` | `size_t` | - |

---

### void cup_detect_and_remove_deleted()

```c
void cup_detect_and_remove_deleted(ModuleDomain* mod, CUPResult* result, const char* prefix, size_t plen, int* changes)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `ModuleDomain*` | - |
| `result` | `CUPResult*` | - |
| `prefix` | `const char*` | - |
| `plen` | `size_t` | - |
| `changes` | `int*` | - |

---

### const char* cup_skip_generated_header()

```c
const char* cup_skip_generated_header(const char* p, const char* mod_name, size_t name_len)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `p` | `const char*` | - |
| `mod_name` | `const char*` | - |
| `name_len` | `size_t` | - |

---

### void cup_set_module_code()

```c
void cup_set_module_code(ModuleDomain* mod, const char* source)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `ModuleDomain*` | - |
| `source` | `const char*` | - |

---

### void cup_print_diagnostics()

```c
void cup_print_diagnostics(CUPResult* result)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `result` | `CUPResult*` | - |

---

### void cup_strip_decl_prefix()

```c
void cup_strip_decl_prefix(CUPResult* result, const char* prefix, size_t plen)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `result` | `CUPResult*` | - |
| `prefix` | `const char*` | - |
| `plen` | `size_t` | - |

---

### void cup_print_summary()

```c
void cup_print_summary(ModuleDomain* mod, CUPResult* result, int changes)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `ModuleDomain*` | - |
| `result` | `CUPResult*` | - |
| `changes` | `int` | - |

---

### void cup_free_param_array()

```c
void cup_free_param_array(CUParParam* params, int count)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `params` | `CUParParam*` | - |
| `count` | `int` | - |

---

### void cup_free_member_array()

```c
void cup_free_member_array(CUParMember* members, int count)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `members` | `CUParMember*` | - |
| `count` | `int` | - |

---

### void cup_free_str_array()

```c
void cup_free_str_array(char*** arr_ptr, int* count, int* cap)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `arr_ptr` | `char***` | - |
| `count` | `int*` | - |
| `cap` | `int*` | - |

---

### void cup_str_array_push()

```c
void cup_str_array_push(char*** arr_ptr, int* count, int* cap, const char* msg)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `arr_ptr` | `char***` | - |
| `count` | `int*` | - |
| `cap` | `int*` | - |
| `msg` | `const char*` | - |

---

### void cup_remove_all_member_children()

```c
void cup_remove_all_member_children(Domain* parent)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | - |

---

### void cup_zero_str_slot()

```c
void cup_zero_str_slot(char*** arr_ptr, int* count, int* cap)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `arr_ptr` | `char***` | - |
| `count` | `int*` | - |
| `cap` | `int*` | - |

---

### void cup_print_str_array()

```c
void cup_print_str_array(FILE* fp, char** arr, int count)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `fp` | `FILE*` | - |
| `arr` | `char**` | - |
| `count` | `int` | - |

---

## 子模块

- [domain](domain/DEV.md): [API 引用] 从项目内模块 domain 导入

*Generated by CBoot v1.0.0*
