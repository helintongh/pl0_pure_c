#ifndef PL0_CODE_GENERATOR
#define PL0_CODE_GENERATOR

#include "global_vars.h"
#include "utils.h"
#include "tokens.h"

#define PL0C_VERSION	"0.0.1"

#define CHECK_LHS	0
#define CHECK_RHS	1
#define CHECK_CALL	2

struct symtab
{
    int depth;
    int type;
    char *name;
    struct symtab *next;
};

struct symtab *g_head;

void cg_out(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
}

void cg_call()
{
    cg_out("%s();\n", token);
}

void cg_const()
{
    cg_out("const long %s=", token);
}

void cg_crlf()
{
    cg_out("\n");
}

void cg_end()
{
    cg_out("/* PL/0 compiler %s */\n", PL0C_VERSION);
}

void cg_epilogue()
{
    cg_out(";");

    if (proc == 0)
        cg_out("return 0;");

    cg_out("\n}\n\n");
}

void cg_odd()
{
    cg_out(")&1");
}

void cg_procedure()
{
    if (proc == 0)
    {
        cg_out("int\n");
        cg_out("main(int argc, char *argv[])\n");
    }
    else
    {
        cg_out("void\n");
        cg_out("%s(void)\n", token);
    }

    cg_out("{\n");
}

void cg_semicolon()
{
    cg_out(";\n");
}
// 根据token生成对应C语言
void cg_symbol()
{
	switch (type)
    {
	case TOK_IDENT:
	case TOK_NUMBER:
		cg_out("%s", token);
		break;
	case TOK_BEGIN:
		cg_out("{\n");
		break;
	case TOK_END:
		cg_out(";\n}\n");
		break;
	case TOK_IF:
		cg_out("if(");
		break;
	case TOK_THEN:
	case TOK_DO:
		cg_out(")");
		break;
	case TOK_ODD:
		cg_out("(");
		break;
	case TOK_WHILE:
		cg_out("while(");
		break;
	case TOK_EQUAL:
		cg_out("==");
		break;
	case TOK_COMMA:
		cg_out(",");
		break;
	case TOK_ASSIGN:
		cg_out("=");
		break;
	case TOK_HASH:
		cg_out("!=");
		break;
	case TOK_LESSTHAN:
		cg_out("<");
		break;
	case TOK_GREATERTHAN:
		cg_out(">");
		break;
	case TOK_PLUS:
		cg_out("+");
		break;
	case TOK_MINUS:
		cg_out("-");
		break;
	case TOK_MULTIPLY:
		cg_out("*");
		break;
	case TOK_DIVIDE:
		cg_out("/");
		break;
	case TOK_LPAREN:
		cg_out("(");
		break;
	case TOK_RPAREN:
		cg_out(")");
        break;
	}
}

void cg_var(void)
{
    cg_out("long %s;\n", token);
}

// 生成符号表并检查
void sym_check(int check)
{
    struct symtab *curr, *ret = NULL;

    curr = g_head;
    while (curr != NULL)
    {
        if (!strcmp(token, curr->name))
            ret = curr;
        curr = curr->next;
    }

    if (ret == NULL)
        error("undefined symbol: %s", token);

    switch (check)
    {
    case CHECK_LHS:
        if (ret->type != TOK_VAR)
            error("must be a variable: %s", token);
        break;
    case CHECK_RHS:
        if (ret->type == TOK_PROCEDURE)
            error("must not be a procedure: %s", token);
        break;
    case CHECK_CALL:
        if (ret->type != TOK_PROCEDURE)
            error("must be a procedure: %s", token);
    }
}
// 符号表里添加符号
void add_symbol(int type)
{
    struct symtab *curr, *new_node;

    curr = g_head;
    while (1) {
        if (!strcmp(curr->name, token)) {
            if (curr->depth == (depth - 1))
                error("duplicate symbol: %s", token);
        }

        if (curr->next == NULL)
            break;

        curr = curr->next;
    }

    if ((new_node = malloc(sizeof(struct symtab))) == NULL)
        error("malloc failed");

    new_node->depth = depth - 1;
    new_node->type = type;
    if ((new_node->name = strdup(token)) == NULL)
        error("malloc failed");
    new_node->next = NULL;

    curr->next = new_node;
}
// 销毁符号表
void destroy_symbols(void)
{
    struct symtab *curr, *prev;
destroy_sym_again:
    curr = g_head;
    while (curr->next != NULL) {
        prev = curr;
        curr = curr->next;
    }

    if (curr->type != TOK_PROCEDURE) {
        free(curr->name);
        free(curr);
        prev->next = NULL;
        goto destroy_sym_again;
    }
}
// 初始化符号表
void init_symtab(void)
{
    struct symtab *new_node;

    if ((new_node = malloc(sizeof(struct symtab))) == NULL)
        error("malloc failed");

    new_node->depth = 0;
    new_node->type = TOK_PROCEDURE;
    new_node->name = "main";
    new_node->next = NULL;

    g_head = new_node;
}

#endif