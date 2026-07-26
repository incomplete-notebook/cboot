# Tasks

- [x] Task 1: 项目骨架搭建 - 创建 Makefile、主入口 cboot.c、头文件结构
  - [x] 创建项目目录结构（src/、include/、tests/）
  - [x] 编写 Makefile（使用 clang-22，参考 tcc 编译方式）
  - [x] 创建 cboot.h 主头文件（包含所有数据结构定义）
  - [x] 创建 cboot.c 主入口（main 函数框架）

- [x] Task 2: 数据结构实现 - 作用域树 + 增删改查 API + 类型检测
  - [x] 实现 Scope/Module/Function/Type/Project 结构体
  - [x] 实现 scope_new、scope_add_child、scope_find、scope_delete
  - [x] 实现模块宏列表、includes 列表、依赖列表、资源引用列表的增删查
  - [x] 实现函数参数/局部变量的增删查
  - [x] 实现类型成员的增删查
  - [x] 内置类型白名单（int, float, double, char, void, short, long, unsigned, signed, size_t, uint8_t 等）
  - [x] 实现 scope_find_type（从当前作用域沿 parent 链向上查找类型定义）

- [x] Task 3: 命令行解析 - argc/argv 处理
  - [x] 解析交互模式（cboot <项目名>）
  - [x] 解析批处理模式（cboot 无参数）
  - [x] 解析 -f 强制标志
  - [x] 解析 -h/--help 帮助信息
  - [x] 校验项目名合法性

- [x] Task 4: REPL 交互循环 - 命令读取与分发
  - [x] 实现 readline 风格的输入读取（支持提示符显示当前路径）
  - [x] 实现命令分词（参考 tcc 的词法分析逻辑）
  - [x] 实现命令分发器（命令名 → 处理函数映射）
  - [x] 实现未知命令错误提示

- [x] Task 5: cd/ls 导航命令
  - [x] 实现 cd 路径解析（支持相对路径、..、/ 绝对路径）
  - [x] 实现 cd 作用域切换（模块/函数/类型间导航）
  - [x] 实现 ls 列出当前作用域子项（含类型标注）
  - [x] 实现提示符路径显示（如 CBoot/core/add>）

- [x] Task 6: 创建命令向导模式（mod/func/type/add/def/im）
  - [x] 实现 mod 向导（询问模块注释，自动 cd 进入）
  - [x] 实现 func 向导（询问注释 → 循环添加参数 → 循环添加局部变量，自动 cd 进入）
  - [x] 实现 type 向导（询问注释 → 循环添加成员，自动 cd 进入）
  - [x] 实现 add 通用添加（根据当前作用域类型添加参数/成员）
  - [x] 实现 def 宏定义向导（询问宏注释）
  - [x] 实现 im 依赖导入（模块依赖/系统头文件，自动去重）
  - [x] 在 func/add/type 中添加变量时调用 scope_find_type 检测类型是否存在

- [x] Task 7: rm 智能删除命令
  - [x] 实现 rm 在当前作用域查找匹配项（子作用域/参数/变量/成员/宏/依赖/资源）
  - [x] 实现歧义消除向导（列出所有匹配项，用户选择编号）
  - [x] 实现 rm -f 强制模式（歧义时报错退出）
  - [x] 实现递归删除子作用域整棵子树

- [x] Task 8: 注释命令（cmt/arg/mem）
  - [x] 实现 cmt 为当前作用域添加/修改注释
  - [x] 实现 arg 为函数参数添加注释（仅限函数作用域）
  - [x] 实现 mem 为类型成员添加注释（仅限类型作用域）

- [x] Task 9: 资源管理（res 命令）
  - [x] 实现 res 向导（选择目标模块、勾选模式）
  - [x] 实现位掩码组合模式（HEX | BITMAP）
  - [x] 实现资源文件复制到 /res/ 目录
  - [x] 实现 CMake 去重逻辑（add_custom_command）

- [x] Task 10: gen 代码生成器
  - [x] 实现规则A：目录判定（子模块或被强制下沉 → 拥有物理目录）
  - [x] 实现规则B：文件落位（has_directory → 模块目录/模块名.c，否则父目录）
  - [x] 实现规则C：冲突自动下沉（X.c 与子模块 X 冲突时迁移）
  - [x] 实现 C 代码生成（.c 文件含 #include、函数体、结构体定义）
  - [x] 实现头文件生成（.h 文件含函数声明、类型定义、extern）
  - [x] 实现 CMakeLists.txt 生成（含 target、include 目录、链接依赖）
  - [x] 实现 main.c 生成

- [x] Task 11: 文档生成器
  - [x] 实现 api.md 生成（Markdown 格式 API 文档）
  - [x] 实现 project.man 生成（troff 格式手册页）
  - [x] 实现 CMake 文档构建规则注入（pandoc → .html/.pdf，groff → .1）
  - [x] 实现 docs 自定义 CMake 目标

- [x] Task 12: .cboot 脚本解析器
  - [x] 实现 .cboot 文件读取与逐行解析
  - [x] 实现脚本行内标志解析（--comment、--arg、--member、--targets、--mode）
  - [x] 实现脚本模式 project 命令（设置 CMake 项目名）
  - [x] 实现脚本模式与交互模式的命令复用

- [x] Task 13: 依赖管理增强
  - [x] 实现模块级依赖的 #include 自动插入
  - [x] 实现系统头文件全局去重
  - [x] 实现依赖图循环检测（gen 时执行）
  - [x] 实现 CMake 包含目录顺序和链接顺序

- [x] Task 14: exit 命令与安全检查
  - [x] 实现 exit 命令（未 gen 时警告）
  - [x] 实现未保存检查与确认提示

- [x] Task 15: 集成测试与验证
  - [x] 编写交互模式端到端测试脚本
  - [x] 编写批处理模式测试（.cboot 脚本示例）
  - [x] 验证 gen 生成结果编译通过
  - [x] 验证文档生成正确性

# Task Dependencies
- Task 2 依赖 Task 1（数据结构需要项目骨架）
- Task 3 依赖 Task 1（命令行解析需要主入口）
- Task 4 依赖 Task 3（REPL 需要命令行解析结果）
- Task 5 依赖 Task 2, Task 4（导航需要数据结构和 REPL）
- Task 6 依赖 Task 2, Task 4（创建命令需要数据结构和 REPL）
- Task 7 依赖 Task 2, Task 4（删除命令需要数据结构和 REPL）
- Task 8 依赖 Task 2, Task 4（注释命令需要数据结构和 REPL）
- Task 9 依赖 Task 2, Task 4（资源管理需要数据结构和 REPL）
- Task 10 依赖 Task 2, Task 6, Task 7, Task 8, Task 9（生成需要完整数据）
- Task 11 依赖 Task 10（文档生成需要代码生成完成）
- Task 12 依赖 Task 3, Task 5, Task 6, Task 7, Task 8, Task 9（脚本模式复用交互命令）
- Task 13 依赖 Task 6, Task 10（依赖管理需要 im 和 gen）
- Task 14 依赖 Task 4, Task 10（exit 需要 REPL 和 gen 状态）
- Task 15 依赖 Task 1-14（测试需要全部功能完成）