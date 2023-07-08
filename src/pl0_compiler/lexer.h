#ifndef PL0_COMPILER_LEXER
#define PL0_COMPILER_LEXER


#include "utils.h"
#include "tokens.h"
/*
 * Lexer.
*/
// 源文件注释处理,直接跳过
void comment()
{
    int ch;
    while ((ch = *raw++) != '}')
    {
        if (ch == '\0')
            error("unterminated comment");

        if (ch == '\n')
            ++line;
    }
}
// 标识符的处理
int ident()
{
    char *p;
    size_t i, len;

    p = raw;
    while (isalnum(*raw) || *raw == '_')
        ++raw;
    
    len = raw - p;
    --raw;
    if (token != NULL)
        free(token);

    if ((token = malloc(len + 1)) == NULL)
        error("malloc failed");

    for (i = 0; i < len; i++)
        token[i] = *p++;
    token[i] = '\0';

    if (!strcmp(token, "const"))
        return TOK_CONST;
    else if (!strcmp(token, "var"))
        return TOK_VAR;
    else if (!strcmp(token, "procedure"))
        return TOK_PROCEDURE;
    else if (!strcmp(token, "call"))
        return TOK_CALL;
    else if (!strcmp(token, "begin"))
        return TOK_BEGIN;
    else if (!strcmp(token, "end"))
        return TOK_END;
    else if (!strcmp(token, "if"))
        return TOK_IF;
    else if (!strcmp(token, "then"))
        return TOK_THEN;
    else if (!strcmp(token, "while"))
        return TOK_WHILE;
    else if (!strcmp(token, "do"))
        return TOK_DO;
    else if (!strcmp(token, "odd"))
        return TOK_ODD;

    return TOK_IDENT;
}
// 数字变量的处理
int number()
{
    const char *errstr;
    char *p;
    size_t i, j = 0, len;
    
    p = raw;
    while (isdigit(*raw) || *raw == '_')
        ++raw;
    len = raw - p;
    --raw;

    free(token);

    if ((token = malloc(len + 1)) == NULL)
        error("malloc failed");

    for (i = 0; i < len; i++)
    {
        if (isdigit(*p))
            token[j++] = *p;
        ++p;
    }

    token[j] = '\0';
    strtonum(token, 0, LONG_MAX, &errstr);
    if (errstr != NULL)
        error("invalid number: %s", token);

    return TOK_NUMBER;
}

int lexer()
{
again:
    /* 忽略空格符号  */
    while (*raw == ' ' || *raw == '\t' || *raw == '\n') {
        if (*raw++ == '\n')
            ++line;
    }

    if (isalpha(*raw) || *raw == '_')
        return ident();

    if (isdigit(*raw))
        return number();

    switch (*raw) {
    case '{':
        comment();
        goto again;
    case '.':
        return TOK_DOT;
    case '=':
        return TOK_EQUAL;
    case ',':
        return TOK_COMMA;
    case ';':
        return TOK_SEMICOLON;
    case '#':
        return TOK_HASH;
    case '<':
        return TOK_LESSTHAN;
    case '>':
        return TOK_GREATERTHAN;
    case '+':
        return TOK_PLUS;
    case '-':
        return TOK_MINUS;
    case '*':
        return TOK_MULTIPLY;
    case '/':
        return TOK_DIVIDE;
    case '(':
        return TOK_LPAREN;
    case ')':
        return TOK_RPAREN;
    case ':':
        if (*++raw != '=') // 下一位字符必须是=
            error("unknown token: ':%c'", *raw);

        return TOK_ASSIGN;
    case '\0':
        return 0;
    default:
        error("unknown token: '%c'", *raw);
    }

    return 0;
}

#endif