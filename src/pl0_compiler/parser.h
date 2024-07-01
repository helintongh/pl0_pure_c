#ifndef PL0_COMPILER_PARSER
#define PL0_COMPILER_PARSER

#include "global_vars.h"
#include "tokens.h"
#include "lexer.h"
#include "code_generator.h"

/*
 * Parser 
*/
void next()
{
    type = lexer();
    ++raw;
}

void expect(int match)
{
    if (match != type)
        error("syntax error");
    next();
}

void expression();

void factor()
{
    switch (type)
    {
    case TOK_IDENT:
        sym_check(CHECK_RHS);
    case TOK_NUMBER:
        cg_symbol();
        next();
        break;
    case TOK_LPAREN:
        cg_symbol();
        expect(TOK_LPAREN);
        expression();
        if (type == TOK_RPAREN)
            cg_symbol();
        expect(TOK_RPAREN);
        break;
    }
}

void term()
{
    factor();

    while (type == TOK_MULTIPLY || type == TOK_DIVIDE) {
        cg_symbol();
        next();
        factor();
    }
}

void expression()
{
    if (type == TOK_PLUS || type == TOK_MINUS)
    {
        cg_symbol();
        next();
    }

    term();

    while (type == TOK_PLUS || type == TOK_MINUS)
    {
        cg_symbol();
        next();
        term();
    }
}

void condition()
{
    if (type == TOK_ODD)
    {
        cg_symbol();
        expect(TOK_ODD);
        expression();
        cg_odd();
    }
    else
    {
        expression();

        switch (type)
        {
        case TOK_EQUAL:
        case TOK_HASH:
        case TOK_LESSTHAN:
        case TOK_GREATERTHAN:
            cg_symbol();
            next();
            break;
        default:
            error("invalid conditional");
        }

        expression();
    }
}

void statement()
{
    switch (type)
    {
    case TOK_IDENT:
        sym_check(CHECK_LHS);
        cg_symbol();
        expect(TOK_IDENT);
        if (type == TOK_ASSIGN)
            cg_symbol();
        expect(TOK_ASSIGN);
        expression();
        break;
    case TOK_CALL:
        expect(TOK_CALL);
        if (type == TOK_IDENT)
        {
            sym_check(CHECK_CALL);
            cg_call();
        }
        expect(TOK_IDENT);
        break;
    case TOK_BEGIN:
        cg_symbol();
        expect(TOK_BEGIN);
        statement();
        while (type == TOK_SEMICOLON)
        {
            cg_semicolon();
            expect(TOK_SEMICOLON);
            statement();
        }
        if (type == TOK_END)
            cg_symbol();
        expect(TOK_END);
        break;
    case TOK_IF:
        cg_symbol();
        expect(TOK_IF);
        condition();
        if (type == TOK_THEN)
            cg_symbol();
        expect(TOK_THEN);
        statement();
        break;
    case TOK_WHILE:
        cg_symbol();
        expect(TOK_WHILE);
        condition();
        if (type == TOK_DO)
            cg_symbol();
        expect(TOK_DO);
        statement();
        break;
    }
}

void block()
{
    if (depth++ > 1)
        error("nesting depth exceeded");
    
    if (type == TOK_CONST)
    {
        expect(TOK_CONST);
        if (type == TOK_IDENT)
        {
            add_symbol(TOK_CONST);
            cg_const();
        }
        expect(TOK_IDENT);
        expect(TOK_EQUAL);
        if (type == TOK_NUMBER)
        {
            cg_symbol();
            cg_semicolon();
        }
        expect(TOK_NUMBER);
        while (type == TOK_COMMA)
        {
            expect(TOK_COMMA);
            if (type == TOK_IDENT)
            {
                add_symbol(TOK_CONST);
                cg_const();
            }
            expect(TOK_IDENT);
            expect(TOK_EQUAL);
            if (type == TOK_NUMBER)
            {
                cg_symbol();
                cg_semicolon();
            }
            expect(TOK_NUMBER);
        }
        expect(TOK_SEMICOLON);
    }

    if (type == TOK_VAR)
    {
        expect(TOK_VAR);
        if (type == TOK_IDENT)
        {
            add_symbol(TOK_VAR);
            cg_var();
        }
        expect(TOK_IDENT);
        while (type == TOK_COMMA)
        {
            expect(TOK_COMMA);
            if (type == TOK_IDENT)
            {
                add_symbol(TOK_VAR);
                cg_var();
            }
            expect(TOK_IDENT);
        }
        expect(TOK_SEMICOLON);
        cg_crlf();
    }

    while (type == TOK_PROCEDURE)
    {
        proc = 1;
        expect(TOK_PROCEDURE);
        if (type == TOK_PROCEDURE)
        {
            add_symbol(TOK_PROCEDURE);
            cg_procedure();
        }
        expect(TOK_IDENT);
        expect(TOK_SEMICOLON);

        block();

        expect(TOK_SEMICOLON);

        proc = 0;
        destroy_symbols();
    }

    if (proc == 0)
        cg_procedure();

    statement();

    cg_epilogue();

    if (--depth < 0)
        error("nesting depth fell below 0");
}

void parse()
{
    next();
    block();
    expect(TOK_DOT);

    if (type != 0)
        error("extra tokens at end of file");
    
    cg_end();
}

#endif