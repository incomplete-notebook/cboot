# docgen 开发文档

文档生成器 - 生成README.md/API.md/DEV.md三种文档

## 统计

- 公开 API: **2** 项
- 私有实现: **36** 项

## 函数

### int generate_docs()

> `API` — 公开接口

**业务逻辑**: 生成README.md、DEV.md和DEPENDENCIES.md

**说明**: 输出目录

```c
int generate_docs(Project* proj, const char* output_dir)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |
| `output_dir` | `const char*` | - |

---

### void generate_mod_readme()

```c
void generate_mod_readme(Domain* mod, const char* dir)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `dir` | `const char*` | - |

---

### void generate_mod_api()

```c
void generate_mod_api(Domain* mod, const char* dir)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `dir` | `const char*` | - |

---

### void generate_mod_dev()

```c
void generate_mod_dev(Domain* mod, const char* dir)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `dir` | `const char*` | - |

---

### void generate_project_readme()

```c
void generate_project_readme(Project* proj, const char* output_dir)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |
| `output_dir` | `const char*` | - |

---

### void generate_project_dev()

```c
void generate_project_dev(Project* proj, const char* output_dir)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |
| `output_dir` | `const char*` | - |

---

### void generate_dependencies_doc()

```c
void generate_dependencies_doc(Project* proj, const char* output_dir)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |
| `output_dir` | `const char*` | - |

---

### int is_api()

```c
int is_api(Domain* d)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `d` | `Domain*` | - |

---

### const char* domain_type_str()

```c
const char* domain_type_str(int type)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `type` | `int` | - |

---

### void generate_module_docs()

> `API` — 公开接口

```c
void generate_module_docs(Domain* mod, const char* dir)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `dir` | `const char*` | - |

---

### void write_dev_functions()

```c
void write_dev_functions(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void write_dev_types()

```c
void write_dev_types(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void write_dev_macros()

```c
void write_dev_macros(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void write_api_deps()

```c
void write_api_deps(Project* proj, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |
| `f` | `FILE*` | - |

---

### void proj_readme_write_modules()

```c
void proj_readme_write_modules(Project* proj, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |
| `f` | `FILE*` | - |

---

### void proj_readme_write_mod_entry()

```c
void proj_readme_write_mod_entry(Domain* child, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `child` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void proj_readme_write_tree()

```c
void proj_readme_write_tree(Project* proj, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |
| `f` | `FILE*` | - |

---

### void dev_write_funcs()

```c
void dev_write_funcs(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void dev_write_types()

```c
void dev_write_types(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void dev_write_macros()

```c
void dev_write_macros(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void dev_write_deps()

```c
void dev_write_deps(Project* proj, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |
| `f` | `FILE*` | - |

---

### int is_item()

```c
int is_item(Domain* c)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `c` | `Domain*` | - |

---

### void readme_write_submodules()

```c
void readme_write_submodules(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void readme_write_item_summary()

```c
void readme_write_item_summary(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### int api_write_toc()

```c
int api_write_toc(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void write_func_signature()

```c
void write_func_signature(FunctionDomain* func, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `func` | `FunctionDomain*` | - |
| `f` | `FILE*` | - |

---

### int write_func_params()

```c
int write_func_params(Domain* func_domain, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `func_domain` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### int api_write_functions()

```c
int api_write_functions(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void dev_write_type_def()

```c
void dev_write_type_def(Domain* child, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `child` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void dev_write_members()

```c
void dev_write_members(Domain* child, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `child` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### int api_write_types()

```c
int api_write_types(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### int api_write_macros()

```c
int api_write_macros(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### int api_write_submodules()

```c
int api_write_submodules(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void dev_write_signature()

```c
void dev_write_signature(FunctionDomain* func, Domain* child, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `func` | `FunctionDomain*` | - |
| `child` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void dev_write_params()

```c
void dev_write_params(Domain* child, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `child` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void dev_write_locals()

```c
void dev_write_locals(Domain* child, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `child` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void dev_write_stats()

```c
void dev_write_stats(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void dev_write_child_links()

```c
void dev_write_child_links(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

## 子模块

- [domain](domain/DEV.md): [API 引用] 从项目内模块 domain 导入

*Generated by CBoot v1.1.0*
