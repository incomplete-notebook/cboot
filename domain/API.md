# domain API 文档

本文档列出 `domain` 模块的所有公开 API。

## 目录

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
- [domain_find_child_by_type](#domain_find_child_by_type)
- [domain_delete](#domain_delete)
- [domain_remove_child](#domain_remove_child)
- [domain_find_nearest_of_type](#domain_find_nearest_of_type)
- [domain_find_in_tree](#domain_find_in_tree)
- [domain_get_path](#domain_get_path)
- [domain_is_api](#domain_is_api)
- [domain_find_api_in_submodules](#domain_find_api_in_submodules)
- [domain_check_api_name_conflict](#domain_check_api_name_conflict)
- [domain_set_comment](#domain_set_comment)
- [domain_set_child_comment](#domain_set_child_comment)
- [find_child_comment](#find_child_comment)
- [domain_set_value](#domain_set_value)
- [domain_set_code](#domain_set_code)
- [domain_set_call](#domain_set_call)
- [domain_get_call](#domain_get_call)
- [domain_set_mode](#domain_set_mode)
- [project_new](#project_new)
- [project_free](#project_free)
- [project_add_dependency](#project_add_dependency)
- [project_has_dependency](#project_has_dependency)
- [is_builtin_type](#is_builtin_type)

---

## 函数

### Domain* domain_new()

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

```c
ModuleDomain* module_domain_new(const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

### FunctionDomain* function_domain_new()

```c
FunctionDomain* function_domain_new(const char* name, const char* return_type)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `return_type` | `const char*` | - |

### StructDomain* struct_domain_new()

```c
StructDomain* struct_domain_new(const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

### TypeDomain* type_domain_new()

```c
TypeDomain* type_domain_new(const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

### MacroDomain* macro_domain_new()

```c
MacroDomain* macro_domain_new(const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

### VariableDomain* variable_domain_new()

```c
VariableDomain* variable_domain_new(const char* name, const char* type)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `type` | `const char*` | - |

### MemberDomain* member_domain_new()

```c
MemberDomain* member_domain_new(const char* name, const char* type)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `type` | `const char*` | - |

### void domain_add_child()

```c
void domain_add_child(Domain* parent, Domain* child)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | - |
| `child` | `Domain*` | - |

### Domain* domain_find_child()

```c
Domain* domain_find_child(Domain* parent, const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | - |
| `name` | `const char*` | - |

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

### char* domain_get_path()

```c
char* domain_get_path(Domain* domain)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |

### int domain_is_api()

```c
int domain_is_api(Domain* domain)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |

### Domain* domain_find_api_in_submodules()

```c
Domain* domain_find_api_in_submodules(Domain* scope, const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `scope` | `Domain*` | - |
| `name` | `const char*` | - |

### Domain* domain_check_api_name_conflict()

```c
Domain* domain_check_api_name_conflict(Domain* scope, const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `scope` | `Domain*` | - |
| `name` | `const char*` | - |

### void domain_set_comment()

```c
void domain_set_comment(Domain* domain, const char* text)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `text` | `const char*` | - |

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

### void domain_set_mode()

```c
void domain_set_mode(Domain* domain, int mode)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `mode` | `int` | - |

### Project* project_new()

```c
Project* project_new(const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

### void project_free()

```c
void project_free(Project* proj)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |

### void project_add_dependency()

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

```c
int project_has_dependency(Project* proj, const char* importer_path, const char* source_path)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |
| `importer_path` | `const char*` | - |
| `source_path` | `const char*` | - |

### int is_builtin_type()

```c
int is_builtin_type(const char* type_name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `type_name` | `const char*` | - |

*Generated by CBoot v0.4.0*
