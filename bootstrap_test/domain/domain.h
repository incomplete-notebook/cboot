/* domain.h - CBoot generated (API declarations only) */
#ifndef DOMAIN_H
#define DOMAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 域类型枚举: module/function/struct/type/macro/variable/member
typedef int DomainType;

// 模块模式: internal(正常编译)/external(外部API引用)
typedef int ModMode;

// 编译模式: normal(普通.o)/exe(可执行)/sl(静态库)/dl(动态库)
typedef int CompilerMode;

// API模式: api(公开接口)/normal(私有实现)
typedef int ApiMode;

// 类型模式: rename(别名)/struct(结构体)/api rename/api struct
typedef int TypeMode;

// 变量模式: static/normal
typedef int VarMode;

// 域树基础节点，所有域类型的基类
typedef struct Domain Domain;

// 模块域 - 项目中的一个模块节点，含编译模式和依赖管理
typedef struct ModuleDomain ModuleDomain;

// 函数域 - 项目中的函数定义
typedef struct FunctionDomain FunctionDomain;

// 结构体域 - 项目中的结构体类型定义
typedef struct StructDomain StructDomain;

// 类型域 - typedef定义
typedef struct TypeDomain TypeDomain;

// 宏域 - #define定义
typedef struct MacroDomain MacroDomain;

// 变量域 - 变量定义
typedef struct VariableDomain VariableDomain;

// 成员域 - 结构体成员或函数参数
typedef struct MemberDomain MemberDomain;

// 依赖记录 - im命令建立的API依赖关系
typedef struct Dependency Dependency;

// 项目容器 - 整个项目的根节点和全局状态
typedef struct Project Project;

// 创建新域节点，分配内存并初始化所有字段
struct Domain* domain_new(int type, char* name, size_t size);

// 创建新模块域
struct ModuleDomain* module_domain_new(char* name);

// 创建新函数域
struct FunctionDomain* function_domain_new(char* name, char* return_type);

// 创建新结构体域
struct StructDomain* struct_domain_new(char* name);

// 创建新类型域
struct TypeDomain* type_domain_new(char* name);

// 创建新宏域
struct MacroDomain* macro_domain_new(char* name);

// 创建新变量域
struct VariableDomain* variable_domain_new(char* name, char* type);

// 创建新成员域(结构体成员或函数参数)
struct MemberDomain* member_domain_new(char* name, char* type);

// 添加子域到父域，自动设置parent指针
void domain_add_child(struct Domain* parent, struct Domain* child);

// 按名称查找直接子域
struct Domain* domain_find_child(struct Domain* parent, char* name);

// 在子模块中递归查找API项，用于名称冲突检测和类型解析
struct Domain* domain_find_api_in_submodules(struct Domain* scope, char* name);

// 检查名称是否与子模块API项冲突
struct Domain* domain_check_api_name_conflict(struct Domain* scope, char* name);

// 获取域的完整路径字符串(如 /module/submodule)
char* domain_get_path(struct Domain* domain);

// 设置域的注释文本
void domain_set_comment(struct Domain* domain, char* text);

// 设置域的模式字段
void domain_set_mode(struct Domain* domain, int mode);

// 创建新项目
struct Project* project_new(char* name);

// 添加依赖记录(im命令使用)
void project_add_dependency(struct Project* proj, char* importer_path, char* source_path, char* cboot_file);

// 检查依赖是否已存在
int project_has_dependency(struct Project* proj, char* importer_path, char* source_path);

#endif /* DOMAIN_H */
