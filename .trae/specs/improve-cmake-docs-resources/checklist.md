# Checklist

## CMake 生成修复
- [x] `#define PI 3.14159265359` 输出完整宏定义（含值）而非 `#define PI`
- [x] 有子模块时，`target_include_directories` 包含子模块目录路径
- [x] 资源 `add_custom_command` 的 DEPENDS 路径使用 `${CMAKE_SOURCE_DIR}/res/`

## 文档生成完善
- [x] api.md 包含 root 模块自身的函数列表
- [x] api.md 包含 root 模块自身的类型列表
- [x] api.md 包含 root 模块自身的宏列表
- [x] project.man 中 `.SH TYPES` 只出现一次
- [x] project.man 中所有模块的类型汇总在同一个 TYPES 节下

## 资源导入强化
- [x] 资源文件不存在时输出错误并返回 -1
- [x] 使用 fopen/fread/fwrite 复制文件，不用 system("cp")
- [x] 复制失败时输出错误提示
- [x] 资源文件去重逻辑正确