# utils 开发文档

工具函数 - 通用辅助功能

## 统计

- 公开 API: **14** 项
- 私有实现: **8** 项

## 函数

### void ensure_dir()

> `API` — 公开接口

**业务逻辑**: 递归创建目录结构，使用mkdir系统调用

**说明**: 目录路径

```c
void ensure_dir(const char* path)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `path` | `const char*` | - |

---

### int str_eq()

> `API` — 公开接口

**业务逻辑**: 使用strcmp比较，返回1(相等)或0(不等)

**说明**: 字符串B

```c
int str_eq(const char* a, const char* b)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `a` | `const char*` | - |
| `b` | `const char*` | - |

---

### char* str_dup()

> `API` — 公开接口

**业务逻辑**: 分配新内存并复制字符串，返回新指针

**说明**: 源字符串

```c
char* str_dup(const char* str)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `str` | `const char*` | - |

---

### void strip_quotes()

> `API` — 公开接口

**业务逻辑**: 原地修改，去除首尾的单引号或双引号

**说明**: 待处理字符串

```c
void strip_quotes(char* str)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `str` | `char*` | - |

---

### char** tokenize()

> `API` — 公开接口

```c
char** tokenize(const char* line, int* count)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `line` | `const char*` | - |
| `count` | `int*` | - |

---

### void free_tokens()

> `API` — 公开接口

```c
void free_tokens(char** tokens, int count)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `tokens` | `char**` | - |
| `count` | `int` | - |

---

### int str_startswith()

> `API` — 公开接口

```c
int str_startswith(const char* str, const char* prefix)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `str` | `const char*` | - |
| `prefix` | `const char*` | - |

---

### int parse_c_decl()

> `API` — 公开接口

```c
int parse_c_decl(const char* decl, char* type_out, int type_size, char* name_out, int name_size)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `decl` | `const char*` | - |
| `type_out` | `char*` | - |
| `type_size` | `int` | - |
| `name_out` | `char*` | - |
| `name_size` | `int` | - |

---

### char* extract_base_type()

> `API` — 公开接口

```c
char* extract_base_type(const char* type_decl)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `type_decl` | `const char*` | - |

---

### char* extract_type_from_decl()

> `API` — 公开接口

```c
char* extract_type_from_decl(const char* decl)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `decl` | `const char*` | - |

---

### char* extract_name_from_decl()

> `API` — 公开接口

```c
char* extract_name_from_decl(const char* decl)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `decl` | `const char*` | - |

---

### int is_valid_identifier()

> `API` — 公开接口

```c
int is_valid_identifier(const char* name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |

---

### int file_exists()

> `API` — 公开接口

```c
int file_exists(const char* path)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `path` | `const char*` | - |

---

### void tokenize_emit()

```c
void tokenize_emit(char** tokens, int* token_count, const char* start, int len)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `tokens` | `char**` | - |
| `token_count` | `int*` | - |
| `start` | `const char*` | - |
| `len` | `int` | - |

---

### const char* tokenize_quoted()

```c
const char* tokenize_quoted(const char* p, char** tokens, int* count)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `p` | `const char*` | - |
| `tokens` | `char**` | - |
| `count` | `int*` | - |

---

### const char* tokenize_plain()

```c
const char* tokenize_plain(const char* p, char** tokens, int* count)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `p` | `const char*` | - |
| `tokens` | `char**` | - |
| `count` | `int*` | - |

---

### const char* parse_read_base_type()

```c
const char* parse_read_base_type(const char* p, char* type_out, int type_size, int* ti)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `p` | `const char*` | - |
| `type_out` | `char*` | - |
| `type_size` | `int` | - |
| `ti` | `int*` | - |

---

### const char* parse_read_pointer_stars()

```c
const char* parse_read_pointer_stars(const char* p, char* type_out, int type_size, int* ti)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `p` | `const char*` | - |
| `type_out` | `char*` | - |
| `type_size` | `int` | - |
| `ti` | `int*` | - |

---

### const char* parse_read_identifier()

```c
const char* parse_read_identifier(const char* p, char* name_out, int name_size, int* ni)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `p` | `const char*` | - |
| `name_out` | `char*` | - |
| `name_size` | `int` | - |
| `ni` | `int*` | - |

---

### const char* parse_read_array_dims()

```c
const char* parse_read_array_dims(const char* p, char* name_out, int name_size, int* ni)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `p` | `const char*` | - |
| `name_out` | `char*` | - |
| `name_size` | `int` | - |
| `ni` | `int*` | - |

---

### const char* parse_skip_ws_to_ident()

```c
const char* parse_skip_ws_to_ident(const char* p)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `p` | `const char*` | - |

---

### char* trim()

> `API` — 公开接口

```c
char* trim(char* str)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `str` | `char*` | - |

---

## 子模块

- [domain](domain/DEV.md): [API 引用] 从项目内模块 domain 导入

*Generated by CBoot v1.1.0*
