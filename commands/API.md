# commands API 文档

本文档列出 `commands` 模块的所有公开 API。

## 目录

- [cmd_mod](#cmd_mod)
- [cmd_struct](#cmd_struct)
- [cmd_type](#cmd_type)
- [cmd_def](#cmd_def)
- [cmd_void](#cmd_void)
- [cmd_var](#cmd_var)
- [cmd_mem](#cmd_mem)
- [cmd_enum](#cmd_enum)
- [cmd_cmt](#cmd_cmt)
- [cmd_value](#cmd_value)
- [cmd_call](#cmd_call)
- [cmd_mode](#cmd_mode)
- [cmd_cmode](#cmd_cmode)
- [cmd_cd](#cmd_cd)
- [cmd_rm](#cmd_rm)
- [cmd_find](#cmd_find)
- [cmd_ls](#cmd_ls)
- [cmd_mv](#cmd_mv)
- [cmd_exit](#cmd_exit)
- [cmd_gen](#cmd_gen)
- [cmd_update](#cmd_update)
- [ANALYZE_NGRAM_SIZE](#ANALYZE_NGRAM_SIZE)
- [ANALYZE_MAX_MODS](#ANALYZE_MAX_MODS)
- [ANALYZE_MAX_FUNCS](#ANALYZE_MAX_FUNCS)
- [ANALYZE_MAX_TOKENS](#ANALYZE_MAX_TOKENS)
- [AnalyzeMod](#AnalyzeMod)
- [AnalyzeFunc](#AnalyzeFunc)
- [AnalyzeToken](#AnalyzeToken)
- [cmd_analyze](#cmd_analyze)
- [cmd_adjust](#cmd_adjust)
- [cmd_im](#cmd_im)
- [cmd_in](#cmd_in)
- [cmd_res](#cmd_res)

---

## 函数

### int cmd_mod()

```c
int cmd_mod(const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

### int cmd_struct()

```c
int cmd_struct(const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

### int cmd_type()

```c
int cmd_type(const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

### int cmd_def()

```c
int cmd_def(const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

### int cmd_void()

```c
int cmd_void(const char* name, const char* return_type)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `return_type` | `const char*` | - |

### int cmd_var()

```c
int cmd_var(const char* name, const char* type)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `type` | `const char*` | - |

### int cmd_mem()

```c
int cmd_mem(const char* name, const char* type)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `type` | `const char*` | - |

### int cmd_enum()

```c
int cmd_enum(const char* defs, const char* start_num_str)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `defs` | `const char*` | - |
| `start_num_str` | `const char*` | - |

### int cmd_cmt()

```c
int cmd_cmt(const char* text)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `text` | `const char*` | - |

### int cmd_value()

```c
int cmd_value(const char* text)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `text` | `const char*` | - |

### int cmd_call()

```c
int cmd_call(const char* call_conv)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `call_conv` | `const char*` | - |

### int cmd_mode()

```c
int cmd_mode(const char* text)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `text` | `const char*` | - |

### int cmd_cmode()

```c
int cmd_cmode(const char* text)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `text` | `const char*` | - |

### int cmd_cd()

```c
int cmd_cd(const char* path)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `path` | `const char*` | - |

### int cmd_rm()

```c
int cmd_rm(const char* name, int force)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `force` | `int` | - |

### int cmd_find()

```c
int cmd_find(const char* type_filter, const char* pattern, int flags)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `type_filter` | `const char*` | - |
| `pattern` | `const char*` | - |
| `flags` | `int` | - |

### int cmd_ls()

```c
int cmd_ls(const char* name)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

### int cmd_mv()

```c
int cmd_mv(const char* src, const char* target)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `src` | `const char*` | - |
| `target` | `const char*` | - |

### int cmd_exit()

```c
int cmd_exit()
```

### int cmd_gen()

```c
int cmd_gen()
```

### int cmd_update()

```c
int cmd_update()
```

### int cmd_analyze()

```c
int cmd_analyze()
```

### int cmd_adjust()

```c
int cmd_adjust()
```

### int cmd_im()

```c
int cmd_im(const char* path)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `path` | `const char*` | - |

### int cmd_in()

```c
int cmd_in(const char* path)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `path` | `const char*` | - |

### int cmd_res()

```c
int cmd_res(const char* file_path)
```

**参数**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `file_path` | `const char*` | - |

## 类型

### AnalyzeMod

```c
typedef struct AnalyzeMod;
```

### AnalyzeFunc

```c
typedef struct AnalyzeFunc;
```

### AnalyzeToken

```c
typedef struct AnalyzeToken;
```

## 宏

### `ANALYZE_NGRAM_SIZE`

```c
#define ANALYZE_NGRAM_SIZE 8
```

### `ANALYZE_MAX_MODS`

```c
#define ANALYZE_MAX_MODS 256
```

### `ANALYZE_MAX_FUNCS`

```c
#define ANALYZE_MAX_FUNCS 2048
```

### `ANALYZE_MAX_TOKENS`

```c
#define ANALYZE_MAX_TOKENS 8192
```

*Generated by CBoot v0.3.1*
