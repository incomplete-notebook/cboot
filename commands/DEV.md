# commands 开发文档

命令处理器 - 实现交互式和批处理模式的所有命令

## 统计

- 公开 API: **29** 项
- 私有实现: **27** 项

## 函数

### int cmd_mod()

> `API` — 公开接口

**业务逻辑**: 验证名称唯一性，创建ModuleDomain并添加到当前作用域

**说明**: 模块名称

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

**业务逻辑**: 在当前模块作用域中创建StructDomain

**说明**: 结构体名称

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

**业务逻辑**: 在当前模块作用域中创建TypeDomain

**说明**: 类型名称

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

**业务逻辑**: 在当前模块作用域中创建MacroDomain

**说明**: 宏名称

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

**业务逻辑**: 验证名称唯一性，创建FunctionDomain并设为当前

**说明**: 返回类型

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

**业务逻辑**: 在当前作用域创建VariableDomain

**说明**: 变量类型

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

**业务逻辑**: 根据当前作用域(函数/结构体/类型)创建MemberDomain，设为当前

**说明**: 类型

```c
int cmd_mem(const char* name, const char* type)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `type` | `const char*` | - |

---

### int cmd_cmt()

> `API` — 公开接口

**业务逻辑**: 设置当前域的注释；若当前是成员/参数，设置后恢复到父作用域

**说明**: 注释文本

```c
int cmd_cmt(const char* text)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `text` | `const char*` | - |

---

### int cmd_mode()

> `API` — 公开接口

**业务逻辑**: 根据当前域类型设置对应模式；模块支持internal/external，函数/结构体/宏支持api/normal

**说明**: 模式值(api/normal/internal/external/static/rename/struct等)

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

**业务逻辑**: 仅对模块有效：exe生成可执行文件(代码放main.c)，sl静态库，dl动态库，normal普通.o

**说明**: 编译器模式值(exe/sl/dl/normal)

```c
int cmd_cmode(const char* text)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `text` | `const char*` | - |

---

### int cmd_test()

> `API` — 公开接口

**业务逻辑**: 添加测试用例: test <输入> => <预期> (纯函数) 或 tcode <<EOF (复杂场景)

**说明**: 测试用例文本 (输入=>预期 或 自定义代码)

```c
int cmd_test(int cov, int pass)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `cov` | `int` | - |
| `pass` | `int` | - |

---

### int cmd_cd()

> `API` — 公开接口

**业务逻辑**: 解析路径段，逐级在域树中导航

**说明**: 目标路径(支持..和/前缀)

```c
int cmd_cd(const char* path)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `path` | `const char*` | - |

---

### int cmd_gen()

> `API` — 公开接口

**业务逻辑**: 遍历域树，生成.c/.h/CMakeLists.txt/文档，项目根为源码根

**说明**: 生成项目代码命令

```c
int cmd_gen()
```

---

### int cmd_im()

> `API` — 公开接口

**业务逻辑**: 解析源文件，提取API模式项，创建external模块，记录依赖链

**说明**: 源.cboot文件路径

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

**业务逻辑**: 完整导入源项目作为子模块，包括所有非API项

**说明**: 源.cboot文件路径

```c
int cmd_in(const char* path)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `path` | `const char*` | - |

---

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

### int cmd_update()

> `API` — 公开接口

```c
int cmd_update()
```

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

### int is_binary_api_domain()

```c
int is_binary_api_domain(DomainType t)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `t` | `DomainType` | - |

---

### int binary_api_mode()

```c
int binary_api_mode(Domain* d)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `d` | `Domain*` | - |

---

### int add_member()

```c
int add_member(const char* name, const char* type, const char* label)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | - |
| `type` | `const char*` | - |
| `label` | `const char*` | - |

---

### int mode_find()

```c
int mode_find(const ModeMap* map, int n, const char* text, int* out)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `map` | `const ModeMap*` | - |
| `n` | `int` | - |
| `text` | `const char*` | - |
| `out` | `int*` | - |

---

### int mode_becoming_api()

```c
int mode_becoming_api(DomainType type, const char* text)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `type` | `DomainType` | - |
| `text` | `const char*` | - |

---

### int mode_check_api_conflict()

```c
int mode_check_api_conflict(Domain* cur)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `cur` | `Domain*` | - |

---

### int mode_apply()

```c
int mode_apply(Domain* cur, const char* text)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `cur` | `Domain*` | - |
| `text` | `const char*` | - |

---

### int cd_walk()

```c
int cd_walk(Domain* start, const char* p)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `start` | `Domain*` | - |
| `p` | `const char*` | - |

---

### DomainType parse_type_filter()

```c
DomainType parse_type_filter(const char* type_filter)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `type_filter` | `const char*` | - |

---

### void ls_print_type_info()

```c
void ls_print_type_info(Domain* target)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `target` | `Domain*` | - |

---

### void ls_print_child()

```c
void ls_print_child(Domain* child)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `child` | `Domain*` | - |

---

### int mv_resolve_target()

```c
int mv_resolve_target(const char* target, Domain** out_parent, const char** out_name)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `target` | `const char*` | - |
| `out_parent` | `Domain**` | - |
| `out_name` | `const char**` | - |

---

### int mv_check_target()

```c
int mv_check_target(Domain* target_parent, const char* target_name, Domain* src_domain)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `target_parent` | `Domain*` | - |
| `target_name` | `const char*` | - |
| `src_domain` | `Domain*` | - |

---

### void copy_members()

```c
void copy_members(Domain* src, Domain* dst)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `src` | `Domain*` | - |
| `dst` | `Domain*` | - |

---

### Domain* clone_api_child()

```c
Domain* clone_api_child(Domain* child)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `child` | `Domain*` | - |

---

### Domain* im_find_sibling()

```c
Domain* im_find_sibling(Domain* importer, const char* path)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `importer` | `Domain*` | - |
| `path` | `const char*` | - |

---

### int im_internal()

```c
int im_internal(Domain* importer, const char* path, char* importer_path)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `importer` | `Domain*` | - |
| `path` | `const char*` | - |
| `importer_path` | `char*` | - |

---

### int im_external()

```c
int im_external(Domain* importer, const char* path, char* importer_path)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `importer` | `Domain*` | - |
| `path` | `const char*` | - |
| `importer_path` | `char*` | - |

---

### int cmd_analyze_impl()

> `API` — 公开接口

```c
int cmd_analyze_impl()
```

---

## 类型

### ModeMap

> `API` — 公开类型

```c
typedef struct ModeMap;
```

---

## 子模块

- [domain](domain/DEV.md): [API 引用] 从项目内模块 domain 导入
- [cupdate](cupdate/DEV.md): [API 引用] 从项目内模块 cupdate 导入

*Generated by CBoot v1.0.0*
