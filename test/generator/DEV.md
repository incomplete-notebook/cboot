# generator 开发文档

代码生成器 - 将域树转换为C源代码、CMake构建系统和文档

## 统计

- 公开 API: **2** 项
- 私有实现: **27** 项

## 函数

### int generate_project()

> `API` — 公开接口

**业务逻辑**: 递归处理所有模块，生成.c/.h/层级化CMake/文档/DEPENDENCIES.md

**说明**: 输出目录(项目根即为源码根)

```c
int generate_project(Project* proj)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |

---

### void generate_module()

```c
void generate_module(Domain* mod, const char* parent_dir)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `parent_dir` | `const char*` | - |

---

### void generate_mod_c()

```c
void generate_mod_c(Domain* mod, const char* dir)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `dir` | `const char*` | - |

---

### void generate_mod_h()

```c
void generate_mod_h(Domain* mod, const char* dir)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `dir` | `const char*` | - |

---

### void generate_mod_cmake()

```c
void generate_mod_cmake(Domain* mod, const char* dir)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `dir` | `const char*` | - |

---

### void generate_mod_cboot()

```c
void generate_mod_cboot(Domain* mod, const char* dir)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `dir` | `const char*` | - |

---

### void generate_top_cmake()

```c
void generate_top_cmake(Project* proj)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |

---

### void generate_top_main()

```c
void generate_top_main(Project* proj)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |

---

### void generate_project_cboot()

```c
void generate_project_cboot(Project* proj)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |

---

### Domain* find_exe_module()

```c
Domain* find_exe_module(Project* proj)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |

---

### int docgen_is_api()

```c
int docgen_is_api(Domain* d)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `d` | `Domain*` | - |

---

### const char* compiler_mode_str()

```c
const char* compiler_mode_str(CompilerMode c)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `c` | `CompilerMode` | - |

---

### void get_module_prefix()

```c
void get_module_prefix(Domain* mod, char* buf, size_t size)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `buf` | `char*` | - |
| `size` | `size_t` | - |

---

### void make_abs_name()

```c
void make_abs_name(const char* prefix, const char* name, char* buf, size_t size)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `prefix` | `const char*` | - |
| `name` | `const char*` | - |
| `buf` | `char*` | - |
| `size` | `size_t` | - |

---

### void generate_cboot_only_recursive()

```c
void generate_cboot_only_recursive(Domain* mod, const char* parent_dir)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `parent_dir` | `const char*` | - |

---

### int generate_cboot_only()

> `API` — 公开接口

```c
int generate_cboot_only(Project* proj)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj` | `Project*` | - |

---

### void write_c_includes()

```c
void write_c_includes(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void write_c_defs()

```c
void write_c_defs(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void write_c_types()

```c
void write_c_types(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void write_c_variables()

```c
void write_c_variables(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void write_c_functions()

```c
void write_c_functions(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void write_dllexport_macro()

```c
void write_dllexport_macro(FILE* f, const char* mod_name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `f` | `FILE*` | - |
| `mod_name` | `const char*` | - |

---

### void write_function_params()

```c
void write_function_params(Domain* func_domain, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `func_domain` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void write_h_guard_begin()

```c
void write_h_guard_begin(FILE* f, const char* mod_name, char* guard_out, size_t guard_size)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `f` | `FILE*` | - |
| `mod_name` | `const char*` | - |
| `guard_out` | `char*` | - |
| `guard_size` | `size_t` | - |

---

### void write_h_guard_end()

```c
void write_h_guard_end(FILE* f, const char* guard)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `f` | `FILE*` | - |
| `guard` | `const char*` | - |

---

### void write_h_api_macros()

```c
void write_h_api_macros(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void write_h_api_types()

```c
void write_h_api_types(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void write_h_fwd_decls()

```c
void write_h_fwd_decls(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void write_h_api_functions()

```c
void write_h_api_functions(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

## 子模块

- [domain](domain/DEV.md): [API 引用] 从项目内模块 domain 导入

*Generated by CBoot v0.3.1*
