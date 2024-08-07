# PL/0语言介绍

PL/0是一种简化版本的Pascal语言。

它的语法如下:

```
program		= block "." .
block		= [ "const" ident "=" number { "," ident "=" number } ";" ]
		  [ "var" ident { "," ident } ";" ]
		  { "procedure" ident ";" block ";" } statement .
statement	= [ ident ":=" expression
		  | "call" ident
		  | "begin" statement { ";" statement } "end"
		  | "if" condition "then" statement
		  | "while" condition "do" statement ] .
condition	= "odd" expression
		| expression ( "=" | "#" | "<" | ">" ) expression .
expression	= [ "+" | "-" ] term { ( "+" | "-" ) term } .
term		= factor { ( "*" | "/" ) factor } .
factor		= ident
		| number
		| "(" expression ")" .
ident		= "A-Za-z_" { "A-Za-z0-9_" } .
number		= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" .
```

下面看几个案例，首先要明确pl/0语言变量和常量必须显式声明。

唯一的数据类型是整数。唯一的运算符是算术运算符和比较运算符。有一个`odd`函数可以测试参数是否为奇数。

下面是一个实际程序:

```Pascal
var i, s;
begin
  i := 0; s := 0;
  while i < 5 do
  begin
    i := i + 1;
    s := s + i * i
  end
end.
```

下面代码输出：

```
0
0
1
1
2
5
3
14
4
30
5
55
```

下面是流程控制相关示例:



```Pascal
const max = 100;
var arg, ret;

procedure isprime;
var i;
begin
	ret := 1;
	i := 2;
	while i < arg do
	begin
		if arg / i * i = arg then
		begin
			ret := 0;
			i := arg
		end;
		i := i + 1
	end
end;

procedure primes;
begin
	arg := 2;
	while arg < max do
	begin
		call isprime;
		if ret = 1 then write arg;
		arg := arg + 1
	end
end;

call primes
```

PL/0变量通过`var`声明，而常量通过`const`声明。

函数通过`procedure`声明，而函数中的代码块以及`if-else`和`while`的代码块使用`begin...end`来作为声明(相当于C语言中的括号`{}`)

## 编译器工作流程

![](resource/compiler_process_flow.png)

该编译器主要包含三个主要部分：词法分析器(scanner)，解析器(语法分析和语义分析都包含其中)和代码生成器。

词法分析器的工作是获取任意形式的源代码并将其转换为一系列标记(tokens)，即把源代码转换为各个单元。

解析器的工作是获取该一系列的tokens并确保它们的排序遵循语言语法规则。简单来说它的输入是一系列tokens然后输出是排好序的语法规则(抽象语法树AST)

代码生成器的工作是**生成等价物**(produce equivalent)。

**生成等价物**可以理解为另一种语言(当然也可以是指令)。gcc编译器使用多种中间语言以启用优化技术。

可以选择一个CPU并让我们的编译器为该`CPU`输出等效的汇编语言。 但由于已经在用 `C` 编写初始编译器，所以我认为应该输出C语言。输出到 C 是公认的编译策略。 它将使我们能够拥有我们将编写并提供给我们的编译器的 PL/0 代码，最终通过编生成等价物(C语言)从而可以在任意平台生成本机代码。 事实证明，它还会为我们的代码生成器节省大量工作。

## 分解任务为三个模块

将源代码转换为最终输出代码的方法不止一种。 基于上面的工作流图，将所有三个主要部分——词法分析器(Scanner也叫做Lexer)、解析器(Parser)和代码生成器——都相互交织在一起。 代码中会有明确的分界，哪些功能属于哪些部分，但在实际上还是会交织在一起。 这个编译器是由解析器(Paser)驱动的。 
当解析器需要token时，解析器将调用词法分析器一次一个地移交token。 一旦解析器有足够的信息来决定接下来输出什么 C 代码，它就会调用代码生成器来生成该 C 代码。 代码生成器从不将任何信息返回给解析器； 解析器假定代码生成器将按照它的指示进行操作。 解析器将重复这个循环，直到它到达源代码的末尾。

下图展示了编译器的不同部分如何交互：

```
+-------+     +--------+     +----------------+
| Lexer |<--->| Parser |---->| Code generator |
+-------+     +--------+     +----------------+
```

但这并不是编译器的不同部分可以相互通信的唯一方式。 我们可以改为采用另一种方法，其中独立的词法分析器读取源代码并生成一个临时文件，该文件包含带有一些额外上下文信息的tokens列表。 然后可以用一个独立的解析器，它读取词法分析器生成的临时文件并对其进行解析，生成某种临时数据结构，如抽象语法树(AST)。 然后我们可以有一个独立的代码生成器，它读取解析器生成的临时数据结构并写出最终输出代码。 这有点类似于 `Cowgol` 编译器采用的策略； 每个主要任务都是它自己单独的二进制文件，中间代码被写到文件中，用作调用链中下一个二进制文件的输入。

我们可以走得更远，将事情分解成最小的工作单元，并编写一个单独的通道或单独的独立实用程序来处理这些单独的工作单元中的每一个。

## 补充说明

PL/0缺少注释语法，本项目预计会给该语言添加一个扩展。Pascal中`{...}`和`(*...*)`允许嵌套注释。

而这个编译器只允许`{...}`作为注释并且不允许嵌套注释。

## 下一章:编写词法分析器

在这篇文章中编写任何代码，但已经基本明确了需要做的工作。 接下来，将为 PL/0 语言编写一个词法分析器，这将允许我们向编译器提供任意格式的 PL/0 源代码，并让编译器返回给我们编程语言的第一个重要结构：所有的按顺序排列的tokens。
