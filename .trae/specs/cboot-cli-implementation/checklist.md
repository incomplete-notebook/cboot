# Checklist

## 数据结构
- [x] Scope 结构体定义正确（type, name, parent, children, comment, comments）
- [x] Module 结构体包含 base Scope + has_directory, forced_subdir, macros, includes, dependencies, asset_refs
- [x] Function 结构体包含 base Scope + return_type, params, 局部变量（children）
- [x] Type 结构体包含 base Scope + members
- [x] AssetRef 结构体包含 asset_name, mode_flags
- [x] Project 结构体包含 name, root, current, copied_assets
- [x] scope_new 正确创建所有类型的作用域
- [x] scope_add_child 正确维护父子关系
- [x] scope_find 支持按名称在当前作用域查找
- [x] scope_delete 递归删除整棵子树
- [x] 内置类型白名单包含 int, float, double, char, void, short, long, unsigned, signed, size_t 等
- [x] scope_find_type 沿 parent 链向上查找类型定义

## 命令行解析
- [x] 交互模式 `cboot <项目名>` 正确解析
- [x] 批处理模式 `cboot`（无参数）正确解析
- [x] `-f` 标志正确解析
- [x] `-h`/`--help` 输出帮助信息
- [x] 项目名合法性校验

## REPL 交互
- [x] 提示符显示当前作用域路径（如 CBoot/core/add>）
- [x] 输入读取支持空格和引号
- [x] 命令分词正确
- [x] 命令分发器映射正确
- [x] 未知命令错误提示

## cd/ls 导航
- [x] cd 支持相对路径（子作用域名）
- [x] cd 支持 .. 返回上级
- [x] cd 支持 / 绝对路径
- [x] ls 列出所有子项（含类型标注）
- [x] 导航到不存在的路径时报错

## 创建命令
- [x] mod 向导完整（注释 → 自动 cd）
- [x] func 向导完整（注释 → 参数循环 → 局部变量循环 → 自动 cd）
- [x] type 向导完整（注释 → 成员循环 → 自动 cd）
- [x] add 根据当前作用域类型正确添加
- [x] def 宏定义向导正确
- [x] im 模块依赖和头文件导入正确，自动去重

## 类型检测
- [x] 使用已定义的自定义类型不报错
- [x] 使用未定义类型给出警告但不阻断
- [x] 内置类型（int/float/char/double/void 等）直接通过
- [x] 沿作用域链向上查找类型（父模块中定义的类型在子模块可见）
- [x] func 向导添加参数时触发类型检测
- [x] type 向导添加成员时触发类型检测
- [x] add 命令添加变量时触发类型检测

## rm 删除命令
- [x] rm 在当前作用域查找匹配项
- [x] 匹配子作用域时递归删除整棵子树
- [x] 匹配参数/局部变量/成员时从列表移除
- [x] 匹配宏/依赖/资源时从列表移除
- [x] 歧义消除向导列出所有匹配项
- [x] rm -f 歧义时报错退出
- [x] 删除不存在的项时报错

## 注释命令
- [x] cmt 正确设置当前作用域注释
- [x] arg 正确设置函数参数注释（仅限函数作用域）
- [x] mem 正确设置类型成员注释（仅限类型作用域）
- [x] 非对应作用域使用 arg/mem 时报错

## 资源管理
- [x] res 向导选择目标模块
- [x] 位掩码组合模式（HEX | BITMAP）
- [ ] 资源文件复制到 /res/
- [ ] CMake 去重生成 add_custom_command

## gen 代码生成
- [x] 规则A：子模块或强制下沉 → 拥有物理目录
- [x] 规则B：has_directory → 模块目录/模块名.c，否则父目录
- [x] 规则C：冲突时 X.c 迁移为 X/X.c
- [x] .c 文件内容正确（#include、函数体、结构体）
- [x] .h 文件内容正确（声明、类型定义、extern）
- [x] CMakeLists.txt 正确（target、include、链接）
- [x] main.c 生成正确

## 文档生成
- [x] api.md 格式正确（Markdown）
- [x] project.man 格式正确（troff）
- [ ] CMake 注入 pandoc 规则
- [ ] CMake 注入 groff 规则
- [x] docs 自定义目标正确

## .cboot 脚本解析
- [x] .cboot 文件读取正确
- [x] 行内标志 --comment 解析正确
- [x] 行内标志 --arg 解析正确
- [x] 行内标志 --member 解析正确
- [x] 行内标志 --targets 解析正确
- [x] 行内标志 --mode 解析正确
- [x] project 命令设置项目名
- [x] 脚本模式命令与交互模式复用

## 依赖管理
- [x] im 模块依赖自动插入 #include
- [x] 系统头文件全局去重
- [ ] 依赖图循环检测
- [x] CMake 包含目录和链接顺序正确

## exit 与安全检查
- [x] exit 未 gen 时警告
- [x] 确认提示正确

## 编译与测试
- [x] 项目可通过 make 编译
- [ ] 交互模式端到端测试通过
- [ ] 批处理模式测试通过
- [ ] gen 生成结果可编译
- [ ] 文档生成内容正确