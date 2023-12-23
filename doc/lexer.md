## 词法分析器实现(Lexer)

本文将为 PL/0 语言编写词法分析器。 该编译器将能够接受任意自由格式的 PL/0 源代码并输出其所包含的源代码的所有tokens以及一些元数据。 词法分析器应该接受所有有效的token并拒绝无效的token。

## `main()`主函数

基本所有的项目一开始都始于一个单一的功能。 我将从一个简单的 `main()` 函数开始。 在这个函数中，需要确保我们有正确数量的参数，能够打开输入文件，并将整个输入文件读入内存，最后调用解析器开始编译过程。 如果还记得上一篇文章，我们决定了一种由解析器控制整个操作的组织和工作流。 解析器将在需要token时调用词法分析器，并在有足够的信息来决定要写出哪些 C 代码时调用代码生成器。

```c
#include <stdio.h>
#include <stdlib.h>

/*
 * pl0_compiler -- PL/0 compiler.
 * 
 * 
*/

int main(int argc, char **argv)
{
    char *start_position;
    if (argc != 2) {
        fputs("usage: pl0c file.pl0\n", stderr);
        exit(1);
    }

    readin(argv[1]);
    start_position = raw;

    parse();
    free(start_position);

    return 0;
}
```

需要实现一些基础函数比如**readin()**和**error()**。

`error()`是实现错误处理的·函数，一旦遇到错误必须向用户报告然后放弃编译。

而`readin()`函数的功能是读取源文件的字符流到内存中。

实际实现如下这两个函数都是工具函数存放在utils.h中：
```c
void error(const char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, "pl0c: error: %lu: ", line);

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fputc('\n', stderr);

	exit(1);
}

void readin(char *file)
{
	int fd;
	struct stat st;

	if (strrchr(file, '.') == NULL)
		error("file must end in '.pl0'");

	if (!!strcmp(strrchr(file, '.'), ".pl0"))
		error("file must end in '.pl0'");

	if ((fd = open(file, O_RDONLY)) == -1)
		error("couldn't open %s", file);

	if (fstat(fd, &st) == -1)
		error("couldn't get file size");

	if ((raw = malloc(st.st_size + 1)) == NULL)
		error("malloc failed");

	if (read(fd, raw, st.st_size) != st.st_size)
		error("couldn't read %s", file);
	raw[st.st_size] = '\0';

	close(fd);
}
```

该函数首先检查源文件是否是`.pl0`后缀的文件。然后读取整个文件到内存中，该内存通过指针raw来访问。最后`close`掉该文件。现在编译器会将其最终代码输出到标准输出。

词法分析器还将负责为错误处理程序增加行统计。在这个功能上几乎只能由词法分析器来实现。词法分析器是编译器中唯一读取源代码的组件，因此它是唯一知道(程序员)何时在该代码中放置换行符的部分。 行数从1开始统计。

其中变量raw和line很明显是全局变量，放到global_vars.h头文件里。

## 实现词法分析器(Scanner)

为了编写一个词法分析器(lexer)需要知道所有有效的PL/0语言的token。token包括变量名称(variable names)、保留字(reserved words)、数字和所有可能的不同符号(symbols)。

PL/0语言的保留字段有如下这些：`const, var, procedure, call, begin, end, if, then, while, do,`以及`odd`。而符号有这些`.`, `=`, `,`, `;`, `:=`, `#`, `<`, `>`, `+`, `-`, `*`, `/`, `(`, `)`

我通过宏`#define`语法来将所有可能的token标记为整数(每个token有一个唯一的id)。这样就可以对tokens进行简单的比较。这些类型被称为token类型。

将其放在头文件tokens.h中，此时代码如下：

```c
#define TOK_IDENT	'I'
#define TOK_NUMBER	'N'
#define TOK_CONST	'C'
#define TOK_VAR		'V'
#define TOK_PROCEDURE	'P'
#define TOK_CALL	'c'
#define TOK_BEGIN	'B'
#define TOK_END		'E'
#define TOK_IF		'i'
#define TOK_THEN	'T'
#define TOK_WHILE	'W'
#define TOK_DO		'D'
#define TOK_ODD		'O'
#define TOK_DOT		'.'
#define TOK_EQUAL	'='
#define TOK_COMMA	','
#define TOK_SEMICOLON	';'
#define TOK_ASSIGN	':'
#define TOK_HASH	'#'
#define TOK_LESSTHAN	'<'
#define TOK_GREATERTHAN	'>'
#define TOK_PLUS	'+'
#define TOK_MINUS	'-'
#define TOK_MULTIPLY	'*'
#define TOK_DIVIDE	'/'
#define TOK_LPAREN	'('
#define TOK_RPAREN	')'
```

使用`TOK_`前缀可以很容易的识别出上述宏皆为token类型。对于符号，使用符号本身的字符作为符号类型。这样可以简化词法分析器。唯一的例外是赋值运算符`:=`因为它是两个字符，这时使用唯一的字母`:`来定义它的token类型。对于标识符，数字和保留字段使用其第一个唯一字母来定义它们的标记类型。其中存在首字母相同的保留字以大小写来区分。

现在需要一个函数来迭代源代码文件。并根据当前正在查看的ASCII字符来决定这是什么类型的token。它还应该跳过所有空格，并且识别注释跳过所有注释。

下面代码都在lexer.h中。

```c
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
// 标识当前token
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
	case '=':
	case ',':
	case ';':
	case '#':
	case '<':
	case '>':
	case '+':
	case '-':
	case '*':
	case '/':
	case '(':
	case ')':
		return (*raw);
	case ':':
		if (*++raw != '=')
			error("unknown token: ':%c'", *raw);

		return TOK_ASSIGN;
	case '\0':
		return 0;
	default:
		error("unknown token: '%c'", *raw);
	}

	return 0;
}

/*
 * Parser 
*/
void parse(void)
{

	while ((type = lexer()) != 0) {
		++raw;
		fprintf(stdout, "%lu|%d\t", line, type);
		switch (type) {
		case TOK_IDENT:
		case TOK_NUMBER:
		case TOK_CONST:
		case TOK_VAR:
		case TOK_PROCEDURE:
		case TOK_CALL:
		case TOK_BEGIN:
		case TOK_END:
		case TOK_IF:
		case TOK_THEN:
		case TOK_WHILE:
		case TOK_DO:
		case TOK_ODD:
			fprintf(stdout, "%s", token);
			break;
		case TOK_DOT:
		case TOK_EQUAL:
		case TOK_COMMA:
		case TOK_SEMICOLON:
		case TOK_HASH:
		case TOK_LESSTHAN:
		case TOK_GREATERTHAN:
		case TOK_PLUS:
		case TOK_MINUS:
		case TOK_MULTIPLY:
		case TOK_DIVIDE:
		case TOK_LPAREN:
		case TOK_RPAREN:
			fputc(type, stdout);
			break;
		case TOK_ASSIGN:
			fputs(":=", stdout);
		}
		fputc('\n', stdout);
	}
}
```

接着讲解上述词法分析器代码的具体实现：

下面讲解`lexer()`函数。

尝试获取token之前应该越过所有的空格等不需要读取的符号，token可以由任意数量的空格分隔，包括在某些情况下0个空格的情况，因此越过空格是词法分析器所做的第一件事情，代码段如下：

```c
	while (*raw == ' ' || *raw == '\t' || *raw == '\n') {
		if (*raw++ == '\n')
			++line;
	}
```

**`ident()`处理保留字和标识符**

接下来，我们处理标识符和保留字，然后是数字。所有标识符都以区分大小写的字母或下划线开头。并且所有保留字都以小写字母开头，所以在扫描整个token之前，无法区分标识符和保留字相关的歧义。因此因此遇到字母和下划线的情况都会通过`ident()`函数识别当前行的代码其为那种token类型。此函数读取所有字母、数字和下划线，直到遇到不属于其中的字符。 为了简单起见，然后回退到标识符的最后一个字符(`--raw;`语句就是干这件事的)（词法分析器的最后一步总是移动到输入缓冲区中的下一个字符，因此通过在这里回滚避免意外跳过标记）

现在需要确定我们拥有的这个token字符串是否是一个保留字。此时迭代保留字列表，如果匹配，我们返回该保留字的正确token类型。如果到达保留字列表末尾且没有匹配项那么其必为标识符(symbol)并报告token类型。

**`number()`函数处理数字变量**

如果词法分析器看到的不是字母或下划线，而是数字，那么我们一定有一个数字变量。根据语法，所有数字都以数字开头，并且是一个或多个数字的序列。我要在这里添加一点扩展。对于非常大的数字，或者可能遵循标准逻辑分组的数字（如美元和美分），最好为 PL/0 程序员提供一种易于人类阅读的数字分隔方式。 正如在 Ada、Java、D、Python 和其他语言中发现的那样，下划线可用于为人类读者分解这些数字。 我认为这是一个很好的补充，所以我们也会这样做。

在`number()`函数内，扫描所有数字和下划线，直到找到一个不是其中之一的字符，然后将词法分析器倒回到数字的最后一个字符。 然后，我们通过将数字复制到token字符串中来创建当前token的序列。 还有一种特殊的情况，如果数字中有下划线，我们将分配比token字符串必需的空间更多的空间。

在这一点上，还需要验证该字符串是否能够转换为数字。 通过实现一个`strtonum`函数来做到这一点。 此函数采用字符串、最小值和最大值以及字符串位置来报告任何错误。 如果函数运行后该字符串位置为 NULL，则一切正常。 否则，出了点问题。 对我们来说，到底出了什么问题并不重要——可能是数字超出范围或以其他方式无效——但在那种情况下，我们应该报告错误。 通常会将 `strtonum()` 的输出（它返回一个 long long）分配给一个变量。 但我们实际上并不关心这些。 我们只关心这个数字是否有效。 因此，通过将其转换为 void，我们将函数转换为数字验证器。

由于PL/0语法有一些不合理之处。使用上面的方法可能无法检测数字是否越界(负数或者正数的越界情况)，这个项目里不关注负数的情况。可以借用Pascal的一个概念通过增加两个新的保留字：`minint`和`maxint`来判断越界情况。

如果词法分析器没有看到字母、数字或下划线，那么一定是符号的情况。如果词法分析器发现任何不是赋值运算符的符号，它会按原样返回该符号，因为在我们的token类型列表中，我们将符号的值作为其 ASCII 字符的值。 换言之符号与其token类型的值将是一样的。 因为我们这样做了，所以我们不需要像读取标识符、保留字和数字那样费心将这些符号读入标记字符串。 仅token类型就已经为我们提供了我们需要了解的有关该符号的所有信息。

当然为了代码更好看`switch-case`语句可以改为如下:

```c
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
```

如果我们看到 `{` 那么我们知道我们已经进入了注释相关的语句，所以应该进入`comment()`函数。 该函数读取缓冲区的字符，直到到达 `}`，这表示注释结束。 即便如此，我们仍然需要计算行数，所以如果在注释语句中看到换行符`\n`，行数应该增加。

如果找到注释，完全可以使用 goto 返回到 `lexer()` 函数的顶部并再次尝试查找token。

最终，词法分析器将看到一个 NUL 字节，这表示处于原始输入缓冲区的末尾(源文件的末尾)。 此时词法分析器通知解析器我们已经完成了。

如果词法分析器看到任何其他东西，那么我们有一个无效的标记，这是一个错误。

就是这样。 这就是我们的词法分析器需要做的所有工作。 为了能够向我们自己证明我们的词法分析器有效，我们将创建一个 parse() 函数，它在一行中吐出一个标记，旁边是找到该标记的行号以及数字形式的标记类型。

上面就是我们的词法分析器所做的一切了。为了验证词法分析器是否有效，将创建一个parse()函数，它输出一行中的一个token，以及该token的行号以及数字形式的token类型的值。

本文的完整代码可以在这里找到 [lexer](https://github.com/helintongh/pl0_pure_c/commit/5f0c72347481a40a884285cdba79b7d96f28fc70)

## 词法分析器使用

实现了词法分析器后，会输出pl0源文件的代码生成的token。

下面看下如何使用当前应用。

0003.pl0代码如下

```pascal
{ 0003: multiple constants }
const one = 1, two = 2;

.
```

```
./pl0_c tests/0003.pl0
```

输出为：

```
2|67	const
2|73	one
2|61	=
2|78	1
2|44	,
2|73	two
2|61	=
2|78	2
2|59	;
4|46	.
```

可以看到输出了代码token所在行而`|`后代表该token所用的标识类型(其中const是`C`转为ASCII码即为67)。然后`one`是变量名对应的token类型的是`TOK_IDENT`其值是`I`其ASCII码为73。

所有的变量名皆为`TOK_IDENT`类型。