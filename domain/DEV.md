# domain 开发文档

域数据模型 - 定义所有域类型、域树结构和项目容器

## 统计

- 公开 API: **67** 项
- 私有实现: **1** 项

## 函数

### Domain* domain_new()

> `API` — 公开接口

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

```c
Domain* domain_find_child(Domain* parent, const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | - |
| `name` | `const char*` | - |

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

### char* domain_get_path()

> `API` — 公开接口

```c
char* domain_get_path(Domain* domain)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |

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

### Domain* domain_find_api_in_submodules()

> `API` — 公开接口

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

```c
Domain* domain_check_api_name_conflict(Domain* scope, const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `scope` | `Domain*` | - |
| `name` | `const char*` | - |

---

### void domain_set_comment()

> `API` — 公开接口

```c
void domain_set_comment(Domain* domain, const char* text)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `text` | `const char*` | - |

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

### void domain_set_mode()

> `API` — 公开接口

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

```c
Project* project_new(const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

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

### void project_add_dependency()

> `API` — 公开接口

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

### void free_type_fields()

```c
void free_type_fields(Domain* domain)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |

---

### int project_would_cycle()

> `API` — 公开接口

```c
int project_would_cycle(Project* proj, const char* importer_path, const char* source_path)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |
| `importer_path` | `const char*` | - |
| `source_path` | `const char*` | - |

---

### Domain* core_domain_new()

> `API` — 公开接口

```c
Domain* core_domain_new(DomainType type, const char* name, size_t struct_size)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `type` | `DomainType` | - |
| `name` | `const char*` | - |
| `struct_size` | `size_t` | - |

---

### ModuleDomain* core_module_domain_new()

> `API` — 公开接口

```c
ModuleDomain* core_module_domain_new(const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

---

### FunctionDomain* core_function_domain_new()

> `API` — 公开接口

```c
FunctionDomain* core_function_domain_new(const char* name, const char* return_type)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `return_type` | `const char*` | - |

---

### StructDomain* core_struct_domain_new()

> `API` — 公开接口

```c
StructDomain* core_struct_domain_new(const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

---

### TypeDomain* core_type_domain_new()

> `API` — 公开接口

```c
TypeDomain* core_type_domain_new(const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

---

### MacroDomain* core_macro_domain_new()

> `API` — 公开接口

```c
MacroDomain* core_macro_domain_new(const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

---

### VariableDomain* core_variable_domain_new()

> `API` — 公开接口

```c
VariableDomain* core_variable_domain_new(const char* name, const char* type)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `type` | `const char*` | - |

---

### MemberDomain* core_member_domain_new()

> `API` — 公开接口

```c
MemberDomain* core_member_domain_new(const char* name, const char* type)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `type` | `const char*` | - |

---

### void core_domain_add_child()

> `API` — 公开接口

```c
void core_domain_add_child(Domain* parent, Domain* child)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | - |
| `child` | `Domain*` | - |

---

### Domain* core_domain_find_child()

> `API` — 公开接口

```c
Domain* core_domain_find_child(Domain* parent, const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | - |
| `name` | `const char*` | - |

---

### Domain* core_domain_find_child_by_type()

> `API` — 公开接口

```c
Domain* core_domain_find_child_by_type(Domain* parent, const char* name, DomainType type)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | - |
| `name` | `const char*` | - |
| `type` | `DomainType` | - |

---

### void core_free_type_fields()

> `API` — 公开接口

```c
void core_free_type_fields(Domain* domain)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |

---

### void core_domain_delete()

> `API` — 公开接口

```c
void core_domain_delete(Domain* domain)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |

---

### void core_domain_remove_child()

> `API` — 公开接口

```c
void core_domain_remove_child(Domain* parent, Domain* child)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | - |
| `child` | `Domain*` | - |

---

### Domain* core_domain_find_nearest_of_type()

> `API` — 公开接口

```c
Domain* core_domain_find_nearest_of_type(Domain* from, DomainType type)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `from` | `Domain*` | - |
| `type` | `DomainType` | - |

---

### Domain* core_domain_find_in_tree()

> `API` — 公开接口

```c
Domain* core_domain_find_in_tree(Domain* root, DomainType type, const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `root` | `Domain*` | - |
| `type` | `DomainType` | - |
| `name` | `const char*` | - |

---

### char* core_domain_get_path()

> `API` — 公开接口

```c
char* core_domain_get_path(Domain* domain)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |

---

### int core_domain_is_api()

> `API` — 公开接口

```c
int core_domain_is_api(Domain* domain)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |

---

### Domain* core_domain_find_api_in_submodules()

> `API` — 公开接口

```c
Domain* core_domain_find_api_in_submodules(Domain* scope, const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `scope` | `Domain*` | - |
| `name` | `const char*` | - |

---

### Domain* core_domain_check_api_name_conflict()

> `API` — 公开接口

```c
Domain* core_domain_check_api_name_conflict(Domain* scope, const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `scope` | `Domain*` | - |
| `name` | `const char*` | - |

---

### void core_domain_set_comment()

> `API` — 公开接口

```c
void core_domain_set_comment(Domain* domain, const char* text)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `text` | `const char*` | - |

---

### void core_domain_set_child_comment()

> `API` — 公开接口

```c
void core_domain_set_child_comment(Domain* domain, const char* target, const char* text)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `target` | `const char*` | - |
| `text` | `const char*` | - |

---

### Comment* core_find_child_comment()

> `API` — 公开接口

```c
Comment* core_find_child_comment(Domain* domain, const char* target)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `target` | `const char*` | - |

---

### void core_domain_set_value()

> `API` — 公开接口

```c
void core_domain_set_value(Domain* domain, const char* value)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `value` | `const char*` | - |

---

### void core_domain_set_code()

> `API` — 公开接口

```c
void core_domain_set_code(Domain* domain, const char* code)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `code` | `const char*` | - |

---

### void core_domain_set_call()

> `API` — 公开接口

```c
void core_domain_set_call(Domain* domain, const char* call)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `call` | `const char*` | - |

---

### const char* core_domain_get_call()

> `API` — 公开接口

```c
const char* core_domain_get_call(Domain* domain)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |

---

### void core_domain_set_mode()

> `API` — 公开接口

```c
void core_domain_set_mode(Domain* domain, int mode)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `domain` | `Domain*` | - |
| `mode` | `int` | - |

---

### Project* core_project_new()

> `API` — 公开接口

```c
Project* core_project_new(const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

---

### void core_project_free()

> `API` — 公开接口

```c
void core_project_free(Project* proj)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |

---

### void core_project_add_dependency()

> `API` — 公开接口

```c
void core_project_add_dependency(Project* proj, const char* importer_path, const char* source_path, const char* cboot_file)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |
| `importer_path` | `const char*` | - |
| `source_path` | `const char*` | - |
| `cboot_file` | `const char*` | - |

---

### int core_project_has_dependency()

> `API` — 公开接口

```c
int core_project_has_dependency(Project* proj, const char* importer_path, const char* source_path)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |
| `importer_path` | `const char*` | - |
| `source_path` | `const char*` | - |

---

### int core_project_would_cycle()

> `API` — 公开接口

```c
int core_project_would_cycle(Project* proj, const char* importer_path, const char* source_path)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |
| `importer_path` | `const char*` | - |
| `source_path` | `const char*` | - |

---

### int core_is_builtin_type()

> `API` — 公开接口

```c
int core_is_builtin_type(const char* type_name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `type_name` | `const char*` | - |

---

*Generated by CBoot v1.0.0*
