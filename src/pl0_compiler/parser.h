#ifndef PL0_COMPILER_PARSER
#define PL0_COMPILER_PARSER

#include "global_vars.h"
#include "tokens.h"
#include "lexer.h"

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
    switch (type) {
    case TOK_IDENT:
    case TOK_NUMBER:
        next();
        break;
    case TOK_LPAREN:
        expect(TOK_LPAREN);
        expression();
        expect(TOK_RPAREN);
        break;
    }
}

void term()
{
    factor();

    while (type == TOK_MULTIPLY || type == TOK_DIVIDE) {
        next();
        factor();
    }
}

void expression()
{
    if (type == TOK_PLUS || type == TOK_MINUS)
        next();

    term();

    while (type == TOK_PLUS || type == TOK_MINUS) {
        next();
        term();
    }
}

void condition()
{
    if (type == TOK_ODD) {
        expect(TOK_ODD);
        expression();
    } else {
        expression();

        switch (type) {
        case TOK_EQUAL:
        case TOK_HASH:
        case TOK_LESSTHAN:
        case TOK_GREATERTHAN:
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
    switch (type) {
    case TOK_IDENT:
        expect(TOK_IDENT);
        expect(TOK_ASSIGN);
        expression();
        break;
    case TOK_CALL:
        expect(TOK_CALL);
        expect(TOK_IDENT);
        break;
    case TOK_BEGIN:
        expect(TOK_BEGIN);
        statement();
        while (type == TOK_SEMICOLON) {
            expect(TOK_SEMICOLON);
            statement();
        }
        expect(TOK_END);
        break;
    case TOK_IF:
        expect(TOK_IF);
        condition();
        expect(TOK_THEN);
        statement();
        break;
    case TOK_WHILE:
        expect(TOK_WHILE);
        condition();
        expect(TOK_DO);
        statement();
        break;
    }
}

void block()
{
    if (depth++ > 1)
        error("nesting depth exceeded");
    
    if (type == TOK_CONST) {
        expect(TOK_CONST);
        expect(TOK_IDENT);
        expect(TOK_EQUAL);
        expect(TOK_NUMBER);
        while (type == TOK_COMMA) {
            expect(TOK_COMMA);
            expect(TOK_IDENT);
            expect(TOK_EQUAL);
            expect(TOK_NUMBER);
        }
        expect(TOK_SEMICOLON);
    }

    if (type == TOK_VAR) {
        expect(TOK_VAR);
        expect(TOK_IDENT);
        while (type == TOK_COMMA) {
            expect(TOK_COMMA);
            expect(TOK_IDENT);
        }
        expect(TOK_SEMICOLON);
    }

    while (type == TOK_PROCEDURE) {
        expect(TOK_PROCEDURE);
        expect(TOK_IDENT);
        expect(TOK_SEMICOLON);

        block();

        expect(TOK_SEMICOLON);
    }

    statement();

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
}

#endif