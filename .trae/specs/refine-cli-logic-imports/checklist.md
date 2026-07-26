# Checklist

## 类型严格校验
- [x] 不存在的类型定义变量时直接报错（非 printf 警告）
- [x] 返回类型不存在时 `func` 命令报错
- [x] 参数类型不存在时 `addarg` 命令报错
- [x] 局部变量类型不存在时 `addvar` 命令报错
- [x] 成员类型不存在时 `addmember` 命令报错

## 函数域命令重构
- [x] `func int add` 只创建函数+注释，不提示参数/变量
- [x] `addarg` 在函数域下可添加参数
- [x] `addvar` 在函数域下可添加局部变量
- [x] `addarg` 在非函数域下报错
- [x] `addvar` 在非函数域下报错

## 类型域命令重构
- [x] `type Vec2` 只创建类型+注释，不提示成员
- [x] `addmember` 在类型域下可添加成员
- [x] `addmember` 在非类型域下报错

## C 指针/数组语法
- [x] `int* a` 解析为类型 `int*`，名称 `a`
- [x] `int a[10]` 解析为类型 `int`，名称 `a[10]`
- [x] `int** ptr` 解析为类型 `int**`，名称 `ptr`
- [x] 类型校验时使用基础类型（如 `int*` 的基础类型是 `int`）

## 命令历史回滚
- [x] 交互模式下上箭头键回滚上一条命令
- [x] 上箭头键可多次回滚历史命令
- [x] 回滚后用户可编辑命令再执行

## 项目级 .cboot 导入
- [x] `import project.cboot` 解析 .cboot 文件
- [x] 导入项目文件复制到 `lib/` 目录
- [x] CMakeLists.txt 包含 `add_subdirectory(lib/<name>)`
- [x] 导入文件不存在时报错

## .a/.so 库导入
- [x] `import libfoo.a desc.cboot` 复制 .a 文件到 `lib/`
- [x] `import libbar.so desc.cboot` 复制 .so 文件到 `lib/`
- [x] 解析 .cboot 描述文件获取接口信息
- [x] CMakeLists.txt 包含 `target_link_libraries`

## 代码生成适配
- [x] 自有代码全部在 `src/` 目录
- [x] 导入库在 `lib/` 目录
- [x] CMakeLists.txt 正确链接导入库
- [x] 编译通过零错误零警告