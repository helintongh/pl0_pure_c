## 解析器

上一章实现了词法分析器。词法分析器只是整个编译器前端的开始，接下来转向解析PL/0的语法。一旦完成了解析器那么编译器的前端就完成了。

前端完成后将拥有一个PL/0验证器。这是一个可以读取源代码并通知用户源代码是否是有效的PL/0代码的程序。

编写解析器有多种办法。上一章我手动编写了一个词法分析器，当然也可以使用flex工具来为你生成一个词法分析器。同样对于解析器来说，可以使用yacc等工具来生成解析器。

但是本章将手写一个**递归下降的解析器(recursive-descent parser)**。递归下降的解析器是自上而下(top-down)的解析器。

自上而下意味着该解析器是从PL/0源代码的第一个token开始，并按照顺序执行，不断取token来执行操作，直到对PL/0源代码中的最后一个token执行有意义的操作后解析结束。

## 一些辅助函数

照例先实现一些parser需要的辅助函数，函数名为`next()`和`except()`。其中`next()`函数只做一件事情:调用词法分析器(lexer)。因为解析器需要从PL/0源代码获取下一个token并记录该token的所有有用信息：token类型甚至是该token声明的变量字符串(如果有需要的话)。这正是lexer所做的，所以这个函数很简单，调用`lexer()`函数即可。

`expect()` 函数用于判断当前代码是否是正确语法。该函数将采用一个参数，该参数类型是token类型，它将检查当前的token类型是否与我们在语法中此时期望(expect)的token类型相匹配。如果没有，将通过现有的错误机制报告语法错误。如果确实匹配，则让词法分析器获取下一个token。

这两个辅助函数如下所示：

```c
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
```

## PL/0语法详解


实现一门语言需要能够阅读扩展巴科斯范式 (EBNF) 样式的语法，以便能够编写我们的解析器。在不涉及大量正式理论或细节的情况下，需要记住以下阅读语法的规则：

下面是pl/0语言的EBNF以及对应解读:

```
program = block "." ;

block = [ "const" ident "=" number {"," ident "=" number} ";"]
        [ "var" ident {"," ident} ";"]
        { "procedure" ident ";" block ";" } statement ;

statement = [ ident ":=" expression | "call" ident 
              | "?" ident | "!" expression 
              | "begin" statement {";" statement } "end" 
              | "if" condition "then" statement 
              | "while" condition "do" statement ];

condition = "odd" expression |
            expression ("="|"#"|"<"|"<="|">"|">=") expression ;

expression = [ "+"|"-"] term { ("+"|"-") term};

term = factor {("*"|"/") factor};

factor = ident | number | "(" expression ")";
```

- `=` 号左边的所有内容都将成为我们的解析器中的一个函数。
- 一个`|`符号代表“或”。
- 任何用“双引号”包裹的都是token(或者说关键字)。比如在block规则里的`"const"`当你遇到了该字符就必须将其视为下一个token即`TOK_CONST`这一个token。
- 而`[]`方括号中的内容都意味着该种规则必须恰好有零个或其中一个`[]`方括号所包含的token类型。
- 任何用`{}`花括号包含的意味着可以有零个或多个没有任意上限。在block规则中你可以拥有任意个数的`procedure`声明。
- 任何包含在 `()` 括号中的东西都意味着你必须恰好拥有其中之一。这仅出现在condition、expression和术语规则(term和factor)中。
- 每条规则末尾的点(`.`dot)只是象征着该规则的结束，没有任何意义。
- 不需要担心标识符或数字是什么。词法分析器知道如何确定这些东西并为我们提供所有token、保留字(reserved words,理解为**keyword**即可其实就是其他编程语言中的关键字)和数字的token。词法分析器还为我们验证数字是否合法。为了保险起见，在我们的 PL/0 语言版本中，标识符以 A-Za-z_ 字符开头，后跟零个或多个 A-Za-z0-9_ 字符。数字以 0-9 字符开头，包含一个或多个 0-9 字符以及可选的`_`字符以供阅读。保留字全部小写。

## 递归下降

传统上，编写语法分析器有两种方法，一种是自顶向下，一种是自底向上。自顶向下是从起始非终结符开始，不断地对非终结符进行分解，直到匹配输入的终结符；自底向上是不断地将终结符进行合并，直到合并成起始的非终结符。

其中的自顶向下方法就是我们所说的递归下降。

## 终结符与非终结符
没有学过编译原理的话可能并不知道什么是“终结符”，“非终结符”。这里简单介绍一下。首先是 BNF 范式，就是一种用来描述语法的语言，例如，四则运算的规则可以表示如下：

```
<expr> ::= <expr> + <term>
         | <expr> - <term>
         | <term>

<term> ::= <term> * <factor>
         | <term> / <factor>
         | <factor>

<factor> ::= ( <expr> )
           | Num
```

用尖括号 `<>` 括起来的就称作 非终结符 ，因为它们可以用 `::=` 右侧的式子代替。`|` 表示选择，如 `<expr>` 可以是 `<expr> + <term>`、`<expr> - <term>`或 `<term>` 中的一种。而没有出现在`::=`左边的就称作 终结符 ，一般终结符对应于词法分析器输出的标记。

## 四则运算的递归下降

例如，我们对 3 * (4 + 2) 进行语法分析。我们假设词法分析器已经正确地将其中的数字识别成了标记 Num。

递归下降是从起始的非终结符开始（顶），本例中是 `<expr>`，实际中可以自己指定，不指定的话一般认为是第一个出现的非终结符。

```
1. <expr> => <expr>
2.           => <term>        * <factor>
3.              => <factor>     |
4.                 => Num (3)   |
5.                              => ( <expr> )
6.                                   => <expr>           + <term>
7.                                      => <term>          |
8.                                         => <factor>     |
9.                                            => Num (4)   |
10.                                                        => <factor>
11.                                                           => Num (2)
```
可以看到，整个解析的过程是在不断对非终结符进行替换（向下），直到遇见了终结符（底）。而我们可以从解析的过程中看出，一些非终结符如`<expr>`被递归地使用了。

## 为什么选择递归下降

从上小节对四则运算的递归下降解析可以看出，整个解析的过程和语法的 BNF 表示是十分接近的，更为重要的是，我们可以很容易地直接将 BNF 表示转换成实际的代码。方法是为每个产生式（即 非终结符 `::=` ...）生成一个同名的函数。

这里会有一个疑问，就是上例中，当一个终结符有多个选择时，如何确定具体选择哪一个？如为什么用 `<expr> ::= <term> * <factor>` 而不是 `<expr> ::= <term> / <factor>` ？这就用到了上一章中提到的“向前看 k 个标记”的概念了。我们向前看一个标记，发现是 `*`，而这个标记足够让我们确定用哪个表达式了。

另外，递归下下降方法对 BNF 方法本身有一定的要求，否则会有一些问题，如经典的“左递归”问题。

## 左递归

上面的四则运算的文法就是左递归的，而左递归的语法是没法直接使用递归下降的方法实现的。因此我们要消除左递归，消除后的文法如下：

```
<expr> ::= <term> <expr_tail>
<expr_tail> ::= + <term> <expr_tail>
              | - <term> <expr_tail>
              | <empty>

<term> ::= <factor> <term_tail>
<term_tail> ::= * <factor> <term_tail>
              | / <factor> <term_tail>
              | <empty>

<factor> ::= ( <expr> )
           | Num
```

消除左递归的相关方法，这里不再多说，请自行查阅相关的资料。

## 四则运算的实现
本节中我们专注语法分析器部分的实现，具体实现很容易，我们直接贴上代码，就是上述的消除左递归后的文法直接转换而来的：

```c
int expr();

int factor() {
    int value = 0;
    if (token == '(') {
        match('(');
        value = expr();
        match(')');
    } else {
        value = token_val;
        match(Num);
    }
    return value;
}

int term_tail(int lvalue) {
    if (token == '*') {
        match('*');
        int value = lvalue * factor();
        return term_tail(value);
    } else if (token == '/') {
        match('/');
        int value = lvalue / factor();
        return term_tail(value);
    } else {
        return lvalue;
    }
}

int term() {
    int lvalue = factor();
    return term_tail(lvalue);
}

int expr_tail(int lvalue) {
    if (token == '+') {
        match('+');
        int value = lvalue + term();
        return expr_tail(value);
    } else if (token == '-') {
        match('-');
        int value = lvalue - term();
        return expr_tail(value);
    } else {
        return lvalue;
    }
}

int expr() {
    int lvalue = term();
    return expr_tail(lvalue);
}
```

可以看到，有了BNF方法后，采用递归向下的方法来实现编译器是很直观的。

我们把词法分析器的代码一并贴上：

```c
#include <stdio.h>
#include <stdlib.h>

enum {Num};
int token;
int token_val;
char *line = NULL;
char *src = NULL;

void next() {
    // skip white space
    while (*src == ' ' || *src == '\t') {
        src ++;
    }

    token = *src++;

    if (token >= '0' && token <= '9' ) {
        token_val = token - '0';
        token = Num;

        while (*src >= '0' && *src <= '9') {
            token_val = token_val*10 + *src - '0';
            src ++;
        }
        return;
    }
}

void match(int tk) {
    if (token != tk) {
        printf("expected token: %d(%c), got: %d(%c)\n", tk, tk, token, token);
        exit(-1);
    }
    next();
}
```

最后是main函数：

```c
int main(int argc, char *argv[])
{
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, stdin)) > 0) {
        src = line;
        next();
        printf("%d\n", expr());
    }
    return 0;
}
```

## parser初步实现

有了上面的理论基础就可以来实现自己的PL/0解析器了。

从顶部开始。语法中的第一条规则是program。program规则被定义为block规则后跟一个(`.`)点。

```c
void parse()
{
    next();
    block();
    expect(TOK_DOT);

    if (type != 0)
        error("extra tokens at end of file");
}
```

代码的逻辑:

1. 调用`next()`获取token，没有token解析器没办法做任何事情。
2. 调用`block()`，这个函数是block规则的处理函数。即是代码块的处理函数。后面会详细介绍。
3. 调用`expect(TOK_DOT)`,expect函数会自动获取下一个token。这一段代码的逻辑是检查遇到`.`符号后是否就不存在其他的token了。如果有那么就出现语法错误了(`.`是pl/0源文件的末尾)。注意注释语句不会被标记为token，因此注释语句可以放到点号之后。

这里可以看到关键在于实现`block()`函数完成对代码块的解析。

## 处理块规则中可选部分

块规则有几个可选的部分。第一个是`const`部分。由于它被包装在方括号中，这意味着你可以有零个或一个const声明的变量。我们可以按照编写前一个函数的相同规则将其转换为代码，只不过我们将其包装在`if`语句中，该语句检查第一个token是否为`TOK_CONST`。如果是，我们将在一个`const`部分中执行块中的所有内容。如果不是就跳过`const`语法的处理逻辑。

block()函数的开头部分是这样的:

```c
void block()
{
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
    ...
}
```

通过阅读上面的代码，就明白了如何处理EBNF规则中花括号包装的语法部分。花括号`{}`表示零或更多，没有上限。所以处理大括号和处理方括号`[]`是完全一样的需要用到`while`语句，有一点不同`[]`方括号的语法通过`if`加`while`来实现，而大括号的`while`通过循环来实现(`{}`大括号包裹的语法规则可能出现无限次)，要保证可以进行任意次数的迭代或者零迭代。

由语法规则可以写出`block()`函数:

```c
void block()
{
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
}
```

const和var语法规则都用方括号括起来，而procedure语法则是用大括号括起来。这意味这它不是可选的。还需注意，procedure只不过是一个命名块，它递归地调用`block()`来完成它的工作。这意味着可以根据语法拥有无限嵌套的procedure。这是类pascal语言的一个共同特征。但是要在这里做一点限制，并且禁止嵌套过程。可以有任意多的procedure，但它们不能相互嵌套。Pascal编译器对嵌套过程的深度同样进行了限制，我这里做限制只是为了编写简单。

同样不应该为了禁止嵌套procedure而修改语法。我希望实现的PL/0解析器与上文编写的语法完全一样。将使用一些变量来强制procedure过程不存在嵌套情况。创建一个名为`depth`的新全局变量，我们将在每个块的开始处增加深度级别，并在每个块的结束处减少深度级别。我们可以用它来检查没有嵌套procedure。

这使得我们完成的block()函数看起来像这样

```c
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
```

虽然有procedure的规则，但procedure语法不接受任何参数，也不返回任何值。它们相当于C语言中不接受参数的void函数`void procedure()`。

## 处理逻辑或(or)语法

语句规则有多种可能性。每种可能性都用一个“或”隔开。为了弄清楚该怎么做，我们必须查看每种可能性中的第一个标记。我们正在寻找的是，每一种可能性都是从不同的标记开始的。如果不是这种情况，我们可能不得不重写受影响的可能性，直到我们达到每个可能性以不同的标记开始的点。我们是幸运的，或者可能是沃思博士故意这样做的(这是故意的)，因为每种可能性都是从不同的符号开始的。这很适合使用switch语句。这里的处理|意味着在第一个令牌上执行一个switch语句，每种可能性都有自己的情况。