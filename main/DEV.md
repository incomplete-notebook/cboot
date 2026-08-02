# main 开发文档

可执行入口模块 - CBoot的主程序入口

## 统计

- 公开 API: **2** 项
- 私有实现: **38** 项

## 函数

### int main()

> `API` — 公开接口

**业务逻辑**: 初始化项目结构，处理-f强制标志，进入交互循环或批处理执行

**说明**: 命令行参数数组

```c
int main(int argc, char** argv)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `argc` | `int` | - |
| `argv` | `char**` | - |

---

### void print_usage()

```c
void print_usage(const char* prog)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `prog` | `const char*` | - |

---

### void repl_loop()

```c
void repl_loop()
```

---

### int dispatch_command()

```c
int dispatch_command(char** tokens, int count)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `tokens` | `char**` | - |
| `count` | `int` | - |

---

### int detect_fine_tune_mode()

```c
int detect_fine_tune_mode()
```

---

### int try_complete()

```c
int try_complete(const char* prefix, const char** candidates, int max_matches, const char** matches)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `prefix` | `const char*` | - |
| `candidates` | `const char**` | - |
| `max_matches` | `int` | - |
| `matches` | `const char**` | - |

---

### int common_prefix_len()

```c
int common_prefix_len(const char** strs, int count)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `strs` | `const char**` | - |
| `count` | `int` | - |

---

### int do_tab_complete()

```c
int do_tab_complete(char* line, int* pos, const char* prompt)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `line` | `char*` | - |
| `pos` | `int*` | - |
| `prompt` | `const char*` | - |

---

### void repl_history_up()

```c
void repl_history_up(char* line, int* pos, const char* prompt, char** history, int hist_count, int* hist_pos, int* hist_browsing, char* saved_line)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `line` | `char*` | - |
| `pos` | `int*` | - |
| `prompt` | `const char*` | - |
| `history` | `char**` | - |
| `hist_count` | `int` | - |
| `hist_pos` | `int*` | - |
| `hist_browsing` | `int*` | - |
| `saved_line` | `char*` | - |

---

### void repl_history_down()

```c
void repl_history_down(char* line, int* pos, const char* prompt, char** history, int hist_count, int* hist_pos, int* hist_browsing, char* saved_line)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `line` | `char*` | - |
| `pos` | `int*` | - |
| `prompt` | `const char*` | - |
| `history` | `char**` | - |
| `hist_count` | `int` | - |
| `hist_pos` | `int*` | - |
| `hist_browsing` | `int*` | - |
| `saved_line` | `char*` | - |

---

### void repl_consume_extended_esc()

```c
void repl_consume_extended_esc()
```

---

### void repl_consume_bracketed_paste()

```c
void repl_consume_bracketed_paste()
```

---

### int repl_read_line()

```c
int repl_read_line(char* line, const char* prompt, char** history, int hist_count, int* hist_pos, int* hist_browsing, char* saved_line)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `line` | `char*` | - |
| `prompt` | `const char*` | - |
| `history` | `char**` | - |
| `hist_count` | `int` | - |
| `hist_pos` | `int*` | - |
| `hist_browsing` | `int*` | - |
| `saved_line` | `char*` | - |

---

### void join_rest()

```c
void join_rest(char* buf, size_t size, char** tokens, int count)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `buf` | `char*` | - |
| `size` | `size_t` | - |
| `tokens` | `char**` | - |
| `count` | `int` | - |

---

### int dispatch_build()

```c
int dispatch_build(const char* cmd, char** tokens, int count)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `cmd` | `const char*` | - |
| `tokens` | `char**` | - |
| `count` | `int` | - |

---

### int dispatch_modify()

```c
int dispatch_modify(const char* cmd, char** tokens, int count)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `cmd` | `const char*` | - |
| `tokens` | `char**` | - |
| `count` | `int` | - |

---

### int dispatch_control()

```c
int dispatch_control(const char* cmd, char** tokens, int count)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `cmd` | `const char*` | - |
| `tokens` | `char**` | - |
| `count` | `int` | - |

---

### int dispatch_action()

```c
int dispatch_action(const char* cmd, char** tokens, int count)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `cmd` | `const char*` | - |
| `tokens` | `char**` | - |
| `count` | `int` | - |

---

### int parse_flag()

```c
int parse_flag(const char* arg, int* adjust_mode, int* analyze_mode, int* update_mode)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `arg` | `const char*` | - |
| `adjust_mode` | `int*` | - |
| `analyze_mode` | `int*` | - |
| `update_mode` | `int*` | - |

---

### int parse_info_flag()

```c
int parse_info_flag(const char* arg, const char* prog)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `arg` | `const char*` | - |
| `prog` | `const char*` | - |

---

### void parse_positional()

```c
void parse_positional(const char* arg, const char** proj_name, const char** script_file)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `arg` | `const char*` | - |
| `proj_name` | `const char**` | - |
| `script_file` | `const char**` | - |

---

### int parse_args()

```c
int parse_args(int argc, char** argv, const char** proj_name, const char** script_file, int* adjust_mode, int* analyze_mode, int* update_mode)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `argc` | `int` | - |
| `argv` | `char**` | - |
| `proj_name` | `const char**` | - |
| `script_file` | `const char**` | - |
| `adjust_mode` | `int*` | - |
| `analyze_mode` | `int*` | - |
| `update_mode` | `int*` | - |

---

### int run_update()

```c
int run_update()
```

---

### int run_adjust()

```c
int run_adjust()
```

---

### int run_analyze()

```c
int run_analyze()
```

---

### int run_batch_script()

```c
int run_batch_script(const char* script_file)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `script_file` | `const char*` | - |

---

### int run_default_cboot()

```c
int run_default_cboot()
```

---

### int run_interactive()

```c
int run_interactive(const char* proj_name, const char* prog)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `proj_name` | `const char*` | - |
| `prog` | `const char*` | - |

---

### int tab_collect_child_domains()

```c
int tab_collect_child_domains(const char* prefix, const char** matches, int max_matches)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `prefix` | `const char*` | - |
| `matches` | `const char**` | - |
| `max_matches` | `int` | - |

---

### int tab_base_len()

```c
int tab_base_len(const char* buf)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `buf` | `const char*` | - |

---

### int tab_apply_single()

```c
int tab_apply_single(char* line, int* pos, const char* prompt, const char* buf, const char* completion)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `line` | `char*` | - |
| `pos` | `int*` | - |
| `prompt` | `const char*` | - |
| `buf` | `const char*` | - |
| `completion` | `const char*` | - |

---

### int tab_apply_multi()

```c
int tab_apply_multi(char* line, int* pos, const char* prompt, const char* buf, const char** matches, int match_count)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `line` | `char*` | - |
| `pos` | `int*` | - |
| `prompt` | `const char*` | - |
| `buf` | `const char*` | - |
| `matches` | `const char**` | - |
| `match_count` | `int` | - |

---

### void repl_handle_esc2()

```c
void repl_handle_esc2(char c, char* line, int* pos, const char* prompt, char** history, int hist_count, int* hist_pos, int* hist_browsing, char* saved_line, int* esc_state)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `c` | `char` | - |
| `line` | `char*` | - |
| `pos` | `int*` | - |
| `prompt` | `const char*` | - |
| `history` | `char**` | - |
| `hist_count` | `int` | - |
| `hist_pos` | `int*` | - |
| `hist_browsing` | `int*` | - |
| `saved_line` | `char*` | - |
| `esc_state` | `int*` | - |

---

### int repl_handle_esc()

```c
int repl_handle_esc(int* esc_state, char c, char* line, int* pos, const char* prompt, char** history, int hist_count, int* hist_pos, int* hist_browsing, char* saved_line)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `esc_state` | `int*` | - |
| `c` | `char` | - |
| `line` | `char*` | - |
| `pos` | `int*` | - |
| `prompt` | `const char*` | - |
| `history` | `char**` | - |
| `hist_count` | `int` | - |
| `hist_pos` | `int*` | - |
| `hist_browsing` | `int*` | - |
| `saved_line` | `char*` | - |

---

### int repl_handle_ctrl()

```c
int repl_handle_ctrl(char c, char* line, int* pos, const char* prompt)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `c` | `char` | - |
| `line` | `char*` | - |
| `pos` | `int*` | - |
| `prompt` | `const char*` | - |

---

### void read_code_from_stdin()

```c
void read_code_from_stdin(char* code_buf, size_t size)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `code_buf` | `char*` | - |
| `size` | `size_t` | - |

---

### int parse_rm()

```c
int parse_rm(char** tokens, int count, const char** name, int* force)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `tokens` | `char**` | - |
| `count` | `int` | - |
| `name` | `const char**` | - |
| `force` | `int*` | - |

---

### int parse_find_flags()

```c
int parse_find_flags(char** tokens, int count)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `tokens` | `char**` | - |
| `count` | `int` | - |

---

### int tab_collect_for_cmd()

```c
int tab_collect_for_cmd(const char* cmd, const char* partial, int tc, int is_last_space, const char** matches, int max_matches)
```

**参数列表**:

| 名称 | 类型 | 说明 |
|------|------|------|
| `cmd` | `const char*` | - |
| `partial` | `const char*` | - |
| `tc` | `int` | - |
| `is_last_space` | `int` | - |
| `matches` | `const char**` | - |
| `max_matches` | `int` | - |

---

## 宏

### `CBOOT_HISTORY_MAX`

> `API` — 公开宏

```c
#define CBOOT_HISTORY_MAX 100
```


## 子模块

- [domain](domain/DEV.md): [API 引用] 从项目内模块 domain 导入
- [cupdate](cupdate/DEV.md): [API 引用] 从项目内模块 cupdate 导入

*Generated by CBoot v1.0.0*
