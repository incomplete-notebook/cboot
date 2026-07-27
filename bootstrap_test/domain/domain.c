/* domain.c - CBoot generated (compiler: normal) */
/* Module: domain */

#include "domain.h"

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
typedef struct Domain {
    char* name;  // 域名称
    int type;  // 域类型(DomainType枚举)
    char* comment;  // 域注释文本
    struct Domain* parent;  // 父域指针
    struct Domain** children;  // 子域数组
    int child_count;  // 子域数量
    int child_capacity;  // 子域数组容量
} Domain;

// 模块域 - 项目中的一个模块节点，含编译模式和依赖管理
typedef struct ModuleDomain {
    struct Domain base;  // 基础域
    int mode;  // 模块模式(ModMode枚举)
    int compiler;  // 编译模式(CompilerMode枚举): exe/sl/dl/normal
    char* value;  // 模块值
    char** includes;  // 头文件包含列表
    int include_count;  // 包含数量
    char** dependencies;  // 依赖模块名称列表
    int dep_count;  // 依赖数量
} ModuleDomain;

// 函数域 - 项目中的函数定义
typedef struct FunctionDomain {
    struct Domain base;  // 基础域
    int mode;  // API模式(ApiMode枚举)
    char* return_type;  // 返回类型
    char* code;  // 函数实现代码
    char* value;  // 业务逻辑描述
} FunctionDomain;

// 结构体域 - 项目中的结构体类型定义
typedef struct StructDomain {
    struct Domain base;  // 基础域
    int mode;  // API模式(ApiMode枚举)
} StructDomain;

// 类型域 - typedef定义
typedef struct TypeDomain {
    struct Domain base;  // 基础域
    int mode;  // 类型模式(TypeMode枚举)
    char* value;  // 底层类型(rename模式)或值
} TypeDomain;

// 宏域 - #define定义
typedef struct MacroDomain {
    struct Domain base;  // 基础域
    int mode;  // API模式(ApiMode枚举)
    char* value;  // 宏值
} MacroDomain;

// 变量域 - 变量定义
typedef struct VariableDomain {
    struct Domain base;  // 基础域
    int mode;  // 变量模式(VarMode枚举)
    char* type;  // 变量类型
    char* value;  // 变量初始值
} VariableDomain;

// 成员域 - 结构体成员或函数参数
typedef struct MemberDomain {
    struct Domain base;  // 基础域
    char* type;  // 成员类型
} MemberDomain;

// 依赖记录 - im命令建立的API依赖关系
typedef struct Dependency {
    char* importer;  // 导入方模块路径
    char* source;  // 源模块路径
    char* cboot_file;  // 源.cboot文件路径
} Dependency;

// 项目容器 - 整个项目的根节点和全局状态
typedef struct Project {
    char* name;  // 项目名称
    struct Domain* root;  // 根域(包含所有模块)
    struct Domain* current;  // 当前作用域指针
    int has_generated;  // 是否已生成代码
    char* cboot_file;  // .cboot文件路径
    struct Dependency* dependencies;  // API依赖数组
    int dep_count;  // 依赖数量
} Project;

// 创建新域节点，分配内存并初始化所有字段
// 业务逻辑: 分配内存，设置类型和名称，初始化子域数组，返回新指针
struct Domain* domain_new(int type, char* name, size_t size) {
//请在这里输入代码
}

// 创建新模块域
// 业务逻辑: 调用domain_new，设置默认模式为INTERNAL，默认编译模式为NORMAL
struct ModuleDomain* module_domain_new(char* name) {
//请在这里输入代码
}

// 创建新函数域
// 业务逻辑: 调用domain_new，设置返回类型，默认API模式为NORMAL
struct FunctionDomain* function_domain_new(char* name, char* return_type) {
//请在这里输入代码
}

// 创建新结构体域
// 业务逻辑: 调用domain_new，默认API模式为NORMAL
struct StructDomain* struct_domain_new(char* name) {
//请在这里输入代码
}

// 创建新类型域
// 业务逻辑: 调用domain_new，默认类型模式为RENAME
struct TypeDomain* type_domain_new(char* name) {
//请在这里输入代码
}

// 创建新宏域
// 业务逻辑: 调用domain_new，默认API模式为NORMAL
struct MacroDomain* macro_domain_new(char* name) {
//请在这里输入代码
}

// 创建新变量域
// 业务逻辑: 调用domain_new，设置类型，默认变量模式为NORMAL
struct VariableDomain* variable_domain_new(char* name, char* type) {
//请在这里输入代码
}

// 创建新成员域(结构体成员或函数参数)
// 业务逻辑: 调用domain_new，设置类型
struct MemberDomain* member_domain_new(char* name, char* type) {
//请在这里输入代码
}

// 添加子域到父域，自动设置parent指针
// 业务逻辑: 检查容量，扩容数组，添加子域，设置parent
void domain_add_child(struct Domain* parent, struct Domain* child) {
//请在这里输入代码
}

// 按名称查找直接子域
// 业务逻辑: 遍历子域数组，返回匹配名称的子域指针，未找到返回NULL
struct Domain* domain_find_child(struct Domain* parent, char* name) {
//请在这里输入代码
}

// 在子模块中递归查找API项，用于名称冲突检测和类型解析
// 业务逻辑: 先在直接子域中查找，再递归进入子模块查找API模式项
struct Domain* domain_find_api_in_submodules(struct Domain* scope, char* name) {
//请在这里输入代码
}

// 检查名称是否与子模块API项冲突
// 业务逻辑: 调用domain_find_api_in_submodules，返回冲突域或NULL
struct Domain* domain_check_api_name_conflict(struct Domain* scope, char* name) {
//请在这里输入代码
}

// 获取域的完整路径字符串(如 /module/submodule)
// 业务逻辑: 从当前域向上遍历到根，拼接路径字符串
char* domain_get_path(struct Domain* domain) {
//请在这里输入代码
}

// 设置域的注释文本
// 业务逻辑: 复制文本并设置到域的comment字段
void domain_set_comment(struct Domain* domain, char* text) {
//请在这里输入代码
}

// 设置域的模式字段
// 业务逻辑: 根据域类型设置对应的模式字段
void domain_set_mode(struct Domain* domain, int mode) {
//请在这里输入代码
}

// 创建新项目
// 业务逻辑: 创建根模块域，初始化项目状态和依赖数组
struct Project* project_new(char* name) {
//请在这里输入代码
}

// 添加依赖记录(im命令使用)
// 业务逻辑: 检查容量，扩容数组，添加新依赖记录
void project_add_dependency(struct Project* proj, char* importer_path, char* source_path, char* cboot_file) {
//请在这里输入代码
}

// 检查依赖是否已存在
// 业务逻辑: 遍历依赖数组，返回1(存在)或0(不存在)
int project_has_dependency(struct Project* proj, char* importer_path, char* source_path) {
//请在这里输入代码
}

