# generator 开发文档

代码生成器 - 将域树转换为C源代码、CMake构建系统和文档

## 统计

- 公开 API: **2** 项
- 私有实现: **55** 项

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

### void cboot_write_header()

```c
void cboot_write_header(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void cboot_write_children()

```c
void cboot_write_children(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void cboot_write_submodule_refs()

```c
void cboot_write_submodule_refs(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void cmake_write_prebuilt_lib()

```c
void cmake_write_prebuilt_lib(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void cmake_write_submodule_sources()

```c
void cmake_write_submodule_sources(Domain* mod, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void write_struct_members()

```c
void write_struct_members(Domain* parent, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void write_func_signature()

```c
void write_func_signature(FunctionDomain* func, const char* abs_name, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `func` | `FunctionDomain*` | - |
| `abs_name` | `const char*` | - |
| `f` | `FILE*` | - |

---

### void write_func_vars()

```c
void write_func_vars(Domain* child, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `child` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void write_api_rename_type()

```c
void write_api_rename_type(TypeDomain* td, Domain* child, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `td` | `TypeDomain*` | - |
| `child` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### int extract_struct_name()

```c
int extract_struct_name(const char* ts, char* sname, size_t size)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `ts` | `const char*` | - |
| `sname` | `char*` | - |
| `size` | `size_t` | - |

---

### int fwd_list_contains()

```c
int fwd_list_contains(char[][MAX_NAME_LEN] arr, int count, const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `arr` | `char[][MAX_NAME_LEN]` | - |
| `count` | `int` | - |
| `name` | `const char*` | - |

---

### int defined_in_module()

```c
int defined_in_module(Domain* mod, const char* sname)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod` | `Domain*` | - |
| `sname` | `const char*` | - |

---

### int collect_func_types()

```c
int collect_func_types(FunctionDomain* func, Domain* child, const char*[] out, int cap)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `func` | `FunctionDomain*` | - |
| `child` | `Domain*` | - |
| `out` | `const char*[]` | - |
| `cap` | `int` | - |

---

### void fwd_try_add()

```c
void fwd_try_add(const char* type, Domain* mod, char[][MAX_NAME_LEN] fwd_decls, int* fwd_count, int cap)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `type` | `const char*` | - |
| `mod` | `Domain*` | - |
| `fwd_decls` | `char[][MAX_NAME_LEN]` | - |
| `fwd_count` | `int*` | - |
| `cap` | `int` | - |

---

### void write_mod_mode()

```c
void write_mod_mode(ModuleDomain* md, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `md` | `ModuleDomain*` | - |
| `f` | `FILE*` | - |

---

### void cboot_write_members()

```c
void cboot_write_members(Domain* parent, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `parent` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void cboot_write_function()

```c
void cboot_write_function(Domain* child, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `child` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void cboot_write_struct()

```c
void cboot_write_struct(Domain* child, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `child` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void cboot_write_type()

```c
void cboot_write_type(Domain* child, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `child` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void cboot_write_macro()

```c
void cboot_write_macro(Domain* child, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `child` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void cboot_write_variable()

```c
void cboot_write_variable(Domain* child, FILE* f)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `child` | `Domain*` | - |
| `f` | `FILE*` | - |

---

### void top_cmake_add_subdirs()

```c
void top_cmake_add_subdirs(FILE* f, Project* proj, Domain* skip)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `f` | `FILE*` | - |
| `proj` | `Project*` | - |
| `skip` | `Domain*` | - |

---

### void top_cmake_emit_exe_sources()

```c
void top_cmake_emit_exe_sources(FILE* f, Project* proj, Domain* exe_mod)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `f` | `FILE*` | - |
| `proj` | `Project*` | - |
| `exe_mod` | `Domain*` | - |

---

### void top_cmake_emit_exe_libs()

```c
void top_cmake_emit_exe_libs(FILE* f, Project* proj, Domain* exe_mod)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `f` | `FILE*` | - |
| `proj` | `Project*` | - |
| `exe_mod` | `Domain*` | - |

---

### void top_cmake_collect_all_sources()

```c
void top_cmake_collect_all_sources(FILE* f, Project* proj)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `f` | `FILE*` | - |
| `proj` | `Project*` | - |

---

### void top_cmake_link_prebuilt()

```c
void top_cmake_link_prebuilt(FILE* f, Project* proj, const char* target_name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `f` | `FILE*` | - |
| `proj` | `Project*` | - |
| `target_name` | `const char*` | - |

---

### void top_cmake_with_exe()

```c
void top_cmake_with_exe(FILE* f, Project* proj, Domain* exe_mod)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `f` | `FILE*` | - |
| `proj` | `Project*` | - |
| `exe_mod` | `Domain*` | - |

---

### void top_cmake_without_exe()

```c
void top_cmake_without_exe(FILE* f, Project* proj)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `f` | `FILE*` | - |
| `proj` | `Project*` | - |

---

## 子模块

- [domain](domain/DEV.md): [API 引用] 从项目内模块 domain 导入

*Generated by CBoot v1.0.0*
