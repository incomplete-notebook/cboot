/* utils.c - CBoot generated (compiler: normal) */
/* Module: utils */

/*
 * CBoot - C Project Bootstrapping Tool v0.5.0
 * Utility functions
 */

#include "cboot.h"

/* ================================================================== */
/* Tokenization                                                        */
/* ================================================================== */

/* 复制 [start, start+len) 为新字符串并存入 tokens[token_count++] */
static void tokenize_emit(char **tokens, int *token_count, const char *start, int len) {
    if (*token_count >= MAX_TOKEN_COUNT) return;
    char *t = (char *)malloc(len + 1);
    if (t) {
        memcpy(t, start, len);
        t[len] = '\0';
    }
    tokens[(*token_count)++] = t;
}

/* 从引号字符串中提取 token：跳过开引号，读到闭引号，返回闭引号位置 */
static const char *tokenize_quoted(const char *p, char **tokens, int *count) {
    p++;  /* skip opening quote */
    const char *start = p;
    while (*p && *p != '"') p++;
    tokenize_emit(tokens, count, start, (int)(p - start));
    return (*p == '"') ? p + 1 : p;
}

/* 从普通 token 中提取：读到空白或引号 */
static const char *tokenize_plain(const char *p, char **tokens, int *count) {
    const char *start = p;
    while (*p && !isspace((unsigned char)*p) && *p != '"') p++;
    tokenize_emit(tokens, count, start, (int)(p - start));
    return p;
}

char **tokenize(const char *line, int *count)
{
    if (!line || !count) return NULL;

    char **tokens = (char **)malloc(sizeof(char *) * MAX_TOKEN_COUNT);
    if (!tokens) { *count = 0; return NULL; }

    int token_count = 0;
    const char *p = line;

    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;
        p = (*p == '"') ? tokenize_quoted(p, tokens, &token_count)
                        : tokenize_plain(p, tokens, &token_count);
    }

    *count = token_count;
    return tokens;
}

void utils_free_tokens(char **tokens, int count)
{
    if (!tokens) return;
    for (int i = 0; i < count; i++) {
        free(tokens[i]);
    }
    free(tokens);
}

/* ================================================================== */
/* String utilities                                                    */
/* ================================================================== */

char *trim(char *str)
{
    if (!str) return NULL;

    /* Trim leading whitespace */
    while (isspace((unsigned char)*str)) str++;

    if (*str == '\0') return str;

    /* Trim trailing whitespace */
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';

    return str;
}

char *utils_str_dup(const char *str)
{
    if (!str) return NULL;

    size_t len = strlen(str);
    char *dup = (char *)malloc(len + 1);
    if (!dup) return NULL;

    strcpy(dup, str);
    return dup;
}

int utils_str_eq(const char *a, const char *b)
{
    if (a == b) return 1;
    if (!a || !b) return 0;
    return strcmp(a, b) == 0;
}

int utils_str_startswith(const char *str, const char *prefix)
{
    if (!str || !prefix) return 0;

    size_t str_len = strlen(str);
    size_t prefix_len = strlen(prefix);

    if (prefix_len > str_len) return 0;

    return strncmp(str, prefix, prefix_len) == 0;
}

/* ================================================================== */
/* Declaration parsing                                                 */
/* ================================================================== */

/* Phase 1-4 helpers for utils_parse_c_decl; each returns next p or NULL on fail. */
static const char *parse_read_base_type(const char *p, char *type_out, int type_size, int *ti) {
    while (isalnum((unsigned char)*p) || *p == '_') {
        if (*ti < type_size - 1) type_out[(*ti)++] = *p;
        p++;
    }
    return p;
}

static const char *parse_read_pointer_stars(const char *p, char *type_out, int type_size, int *ti) {
    while (*p == '*') {
        if (*ti < type_size - 1) type_out[(*ti)++] = *p;
        p++;
    }
    return p;
}

static const char *parse_read_identifier(const char *p, char *name_out, int name_size, int *ni) {
    while (isalnum((unsigned char)*p) || *p == '_') {
        if (*ni < name_size - 1) name_out[(*ni)++] = *p;
        p++;
    }
    return p;
}

static const char *parse_read_array_dims(const char *p, char *name_out, int name_size, int *ni) {
    while (*p == '[') {
        if (*ni < name_size - 1) name_out[(*ni)++] = *p;
        p++;
        while (isdigit((unsigned char)*p)) {
            if (*ni < name_size - 1) name_out[(*ni)++] = *p;
            p++;
        }
        if (*p != ']') return NULL;  /* unclosed bracket */
        if (*ni < name_size - 1) name_out[(*ni)++] = *p;
        p++;
    }
    return p;
}

/* 跳过空格并验证下一个 token 以合法标识符字符开头。
 * 合并"skip ws + 非空检查 + 标识符起始检查"三步，
 * utils_parse_c_decl 中此模式出现两次，抽出降低圈复杂度。
 * 返回指向第一个标识符字符的指针，失败返回 NULL。 */
static const char *parse_skip_ws_to_ident(const char *p) {
    while (*p == ' ') p++;
    if (!*p) return NULL;
    if (!isalpha((unsigned char)*p) && *p != '_') return NULL;
    return p;
}

/*
 * Parse a C declaration: [base_type][*...][whitespace]ident[[N]...]
 * Returns 0 on success, -1 on invalid declaration.
 * When type and name are combined (no space), e.g. "int***a", this
 * correctly splits at the name boundary: type="int***", name="a".
 */
int utils_parse_c_decl(const char *decl, char *type_out, int type_size,
                 char *name_out, int name_size)
{
    const char *p = decl;
    int ti = 0, ni = 0;

    if (!decl || !type_out || !name_out) return -1;
    type_out[0] = '\0';
    name_out[0] = '\0';

    /* Phase 1: skip ws, validate ident start, read base type */
    p = parse_skip_ws_to_ident(p);
    if (!p) return -1;
    p = parse_read_base_type(p, type_out, type_size, &ti);

    /* Phase 2: Read pointer stars */
    p = parse_read_pointer_stars(p, type_out, type_size, &ti);
    type_out[ti] = '\0';

    /* After type, if we see '[' it's invalid ('[' must follow identifier) */
    if (*p == '[') return -1;

    /* Phase 3: skip ws, validate ident start, read identifier */
    p = parse_skip_ws_to_ident(p);
    if (!p) return -1;
    p = parse_read_identifier(p, name_out, name_size, &ni);

    /* Phase 4: Read array dimensions [N] */
    p = parse_read_array_dims(p, name_out, name_size, &ni);
    if (!p) return -1;
    name_out[ni] = '\0';

    /* Should be at end (or trailing whitespace) */
    while (*p == ' ') p++;
    if (*p != '\0') return -1;  /* garbage after declaration */

    return 0;
}

char *extract_base_type(const char *type_decl)
{
    static char buf[MAX_NAME_LEN];
    strncpy(buf, type_decl, MAX_NAME_LEN - 1);
    buf[MAX_NAME_LEN - 1] = '\0';
    /* Strip trailing '*' characters */
    int len = (int)strlen(buf);
    while (len > 0 && buf[len - 1] == '*') {
        buf[len - 1] = '\0';
        len--;
    }
    /* Trim trailing whitespace */
    while (len > 0 && buf[len - 1] == ' ') {
        buf[len - 1] = '\0';
        len--;
    }
    return buf;
}

char *extract_type_from_decl(const char *decl)
{
    static char buf[MAX_NAME_LEN];
    char name_buf[MAX_NAME_LEN];

    if (!decl) return NULL;

    if (utils_parse_c_decl(decl, buf, MAX_NAME_LEN, name_buf, MAX_NAME_LEN) == 0)
        return buf;

    /* Fallback: split on last space for backward compatibility */
    const char *p = decl;
    int len, i, type_len;
    while (*p == ' ') p++;
    len = (int)strlen(p);
    const char *last_space = NULL;
    for (i = 0; i < len; i++) {
        if (p[i] == ' ') last_space = &p[i];
    }
    if (last_space == NULL) {
        strncpy(buf, p, MAX_NAME_LEN - 1);
        buf[MAX_NAME_LEN - 1] = '\0';
        return buf;
    }
    type_len = (int)(last_space - p);
    if (type_len >= MAX_NAME_LEN) type_len = MAX_NAME_LEN - 1;
    memcpy(buf, p, type_len);
    buf[type_len] = '\0';
    return buf;
}

char *extract_name_from_decl(const char *decl)
{
    static char buf[MAX_NAME_LEN];
    char type_buf[MAX_NAME_LEN];

    if (!decl) return NULL;

    if (utils_parse_c_decl(decl, type_buf, MAX_NAME_LEN, buf, MAX_NAME_LEN) == 0)
        return buf;

    /* Fallback: split on last space for backward compatibility */
    const char *p = decl;
    int len, i;
    while (*p == ' ') p++;
    len = (int)strlen(p);
    const char *last_space = NULL;
    for (i = 0; i < len; i++) {
        if (p[i] == ' ') last_space = &p[i];
    }
    if (last_space == NULL) {
        strncpy(buf, p, MAX_NAME_LEN - 1);
        buf[MAX_NAME_LEN - 1] = '\0';
        return buf;
    }
    const char *name_start = last_space + 1;
    strncpy(buf, name_start, MAX_NAME_LEN - 1);
    buf[MAX_NAME_LEN - 1] = '\0';
    return buf;
}

/* ================================================================== */
/* Identifier validation                                               */
/* ================================================================== */

int utils_is_valid_identifier(const char *name)
{
    if (!name || *name == '\0') return 0;

    /* First character must be letter or underscore */
    if (!isalpha((unsigned char)name[0]) && name[0] != '_') return 0;

    /* Remaining characters must be alphanumeric or underscore */
    for (int i = 1; name[i] != '\0'; i++) {
        if (!isalnum((unsigned char)name[i]) && name[i] != '_') return 0;
    }

    return 1;
}

/* ================================================================== */
/* Filesystem utilities                                                */
/* ================================================================== */

void utils_ensure_dir(const char *path)
{
    if (!path) return;

    char tmp[MAX_PATH_LEN];
    strncpy(tmp, path, MAX_PATH_LEN - 1);
    tmp[MAX_PATH_LEN - 1] = '\0';

    /* Create directories recursively */
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

int utils_file_exists(const char *path)
{
    if (!path) return 0;
    return access(path, F_OK) == 0;
}

/* ================================================================== */
/* Quote handling                                                      */
/* ================================================================== */

void utils_strip_quotes(char *str)
{
    if (!str) return;

    int len = (int)strlen(str);
    if (len >= 2 && str[0] == '"' && str[len - 1] == '"') {
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
    }
}
