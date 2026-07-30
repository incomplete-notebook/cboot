# commands 开发文档

## 统计

- 公开 API: **33** 项
- 私有实现: **15** 项

## 函数

### ModuleDomain* get_current_module()

```c
ModuleDomain* get_current_module()
```

---

### int is_in_domain_type()

```c
int is_in_domain_type(DomainType type)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `type` | `DomainType` | - |

---

### const char* domain_type_name()

```c
const char* domain_type_name(DomainType type)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `type` | `DomainType` | - |

---

### int check_type()

```c
int check_type(const char* type_name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `type_name` | `const char*` | - |

---

### int check_name_dup()

```c
int check_name_dup(const char* name, Domain* scope)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `scope` | `Domain*` | - |

---

### const char* mode_str()

```c
const char* mode_str(Domain* d)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `d` | `Domain*` | - |

---

### int cmd_mod()

> `API` — 公开接口

```c
int cmd_mod(const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

---

### int cmd_struct()

> `API` — 公开接口

```c
int cmd_struct(const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

---

### int cmd_type()

> `API` — 公开接口

```c
int cmd_type(const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

---

### int cmd_def()

> `API` — 公开接口

```c
int cmd_def(const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

---

### int cmd_void()

> `API` — 公开接口

```c
int cmd_void(const char* name, const char* return_type)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `return_type` | `const char*` | - |

---

### int cmd_var()

> `API` — 公开接口

```c
int cmd_var(const char* name, const char* type)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `type` | `const char*` | - |

---

### int cmd_mem()

> `API` — 公开接口

```c
int cmd_mem(const char* name, const char* type)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `type` | `const char*` | - |

---

### int cmd_enum()

> `API` — 公开接口

```c
int cmd_enum(const char* defs, const char* start_num_str)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `defs` | `const char*` | - |
| `start_num_str` | `const char*` | - |

---

### int cmd_cmt()

> `API` — 公开接口

```c
int cmd_cmt(const char* text)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `text` | `const char*` | - |

---

### int cmd_value()

> `API` — 公开接口

```c
int cmd_value(const char* text)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `text` | `const char*` | - |

---

### int cmd_call()

> `API` — 公开接口

```c
int cmd_call(const char* call_conv)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `call_conv` | `const char*` | - |

---

### int cmd_mode()

> `API` — 公开接口

```c
int cmd_mode(const char* text)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `text` | `const char*` | - |

---

### int cmd_cmode()

> `API` — 公开接口

```c
int cmd_cmode(const char* text)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `text` | `const char*` | - |

---

### int cmd_cd()

> `API` — 公开接口

```c
int cmd_cd(const char* path)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `path` | `const char*` | - |

---

### int cmd_rm()

> `API` — 公开接口

```c
int cmd_rm(const char* name, int force)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `force` | `int` | - |

---

### void find_recursive()

```c
void find_recursive(Domain* root, DomainType type_filter, const char* pattern, int max_depth, int cur_depth, int* found_count)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `root` | `Domain*` | - |
| `type_filter` | `DomainType` | - |
| `pattern` | `const char*` | - |
| `max_depth` | `int` | - |
| `cur_depth` | `int` | - |
| `found_count` | `int*` | - |

---

### int cmd_find()

> `API` — 公开接口

```c
int cmd_find(const char* type_filter, const char* pattern, int flags)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `type_filter` | `const char*` | - |
| `pattern` | `const char*` | - |
| `flags` | `int` | - |

---

### int cmd_ls()

> `API` — 公开接口

```c
int cmd_ls(const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

---

### int cmd_mv()

> `API` — 公开接口

```c
int cmd_mv(const char* src, const char* target)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `src` | `const char*` | - |
| `target` | `const char*` | - |

---

### int cmd_exit()

> `API` — 公开接口

```c
int cmd_exit()
```

---

### int cmd_gen()

> `API` — 公开接口

```c
int cmd_gen()
```

---

### int cmd_update()

> `API` — 公开接口

```c
int cmd_update()
```

---

### void analyze_collect_modules()

```c
void analyze_collect_modules(Domain* d, AnalyzeMod* mods, int* count, int cap)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `d` | `Domain*` | - |
| `mods` | `AnalyzeMod*` | - |
| `count` | `int*` | - |
| `cap` | `int` | - |

---

### int analyze_count_code_lines()

```c
int analyze_count_code_lines(const char* code)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `code` | `const char*` | - |

---

### void analyze_extract_functions()

```c
void analyze_extract_functions(const char* mod_name, const char* code, AnalyzeFunc* funcs, int* count, int cap)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `mod_name` | `const char*` | - |
| `code` | `const char*` | - |
| `funcs` | `AnalyzeFunc*` | - |
| `count` | `int*` | - |
| `cap` | `int` | - |

---

### int analyze_tokenize()

```c
int analyze_tokenize(const char* code, AnalyzeToken* toks, int cap)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `code` | `const char*` | - |
| `toks` | `AnalyzeToken*` | - |
| `cap` | `int` | - |

---

### int analyze_cyclomatic_complexity()

```c
int analyze_cyclomatic_complexity(const char* code)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `code` | `const char*` | - |

---

### int analyze_ngram_equal()

```c
int analyze_ngram_equal(AnalyzeToken* a, AnalyzeToken* b)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `a` | `AnalyzeToken*` | - |
| `b` | `AnalyzeToken*` | - |

---

### int cmd_analyze()

> `API` — 公开接口

```c
int cmd_analyze()
```

---

### int cmd_adjust()

> `API` — 公开接口

```c
int cmd_adjust()
```

---

### void copy_api_items()

```c
void copy_api_items(Domain* src_mod, Domain* dst_mod)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `src_mod` | `Domain*` | - |
| `dst_mod` | `Domain*` | - |

---

### void copy_api_items_recursive()

```c
void copy_api_items_recursive(Domain* src_mod, Domain* dst_mod)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `src_mod` | `Domain*` | - |
| `dst_mod` | `Domain*` | - |

---

### int cmd_im()

> `API` — 公开接口

```c
int cmd_im(const char* path)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `path` | `const char*` | - |

---

### int cmd_in()

> `API` — 公开接口

```c
int cmd_in(const char* path)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `path` | `const char*` | - |

---

### int cmd_res()

> `API` — 公开接口

```c
int cmd_res(const char* file_path)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `file_path` | `const char*` | - |

---

## 类型

### AnalyzeMod

> `API` — 公开类型

```c
typedef struct AnalyzeMod;
```

---

### AnalyzeFunc

> `API` — 公开类型

```c
typedef struct AnalyzeFunc;
```

---

### AnalyzeToken

> `API` — 公开类型

```c
typedef struct AnalyzeToken;
```

---

## 宏

### `ANALYZE_NGRAM_SIZE`

> `API` — 公开宏

```c
#define ANALYZE_NGRAM_SIZE 8
```

### `ANALYZE_MAX_MODS`

> `API` — 公开宏

```c
#define ANALYZE_MAX_MODS 256
```

### `ANALYZE_MAX_FUNCS`

> `API` — 公开宏

```c
#define ANALYZE_MAX_FUNCS 2048
```

### `ANALYZE_MAX_TOKENS`

> `API` — 公开宏

```c
#define ANALYZE_MAX_TOKENS 8192
```


*Generated by CBoot v0.3.1*
