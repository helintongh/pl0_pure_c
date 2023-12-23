# 测试框架

完整的项目还需要编写一些基础设施，以便在测试编译器是否正确工作时更轻松。我将为PL/0编译器编写一个shell脚本和该脚本将作为测试框架。

## 测试脚本实现

自动化测试很棒。希望能够运行make测试并启动一连串的测试。现在，这意味着可以确保编译器前端做正确的事情，接受正确的代码，拒绝不正确的代码。

那能不能实现一个脚本集编译与测试于一身呢？当然是可以的。

把测试放在一个地方，与编译器源代码分开。先创建了一个名为tests/的新目录来保存所有的测试文件。在src的最外层创建一个脚本`build.sh`，这个脚本不光实现pl0编译器的编译同时实现对tests中所有pl0文件进行测试。

先来写脚本:

```shell
#!/bin/sh
if [ ! -d "build" ]; then
    mkdir build
fi
cd ./build
cmake ../pl0_compiler
make

# 测试代码
echo PL/0 compiler test suite
echo ========================

for i in ../tests/*.pl0 ; do
  /usr/bin/printf "%.13s... " $i
  ./pl0_c $i > /dev/null 2>&1
  if [ $? -eq 0 ] ; then
    echo ok
  else
    echo fail
  fi
done
```

进入该项目的src整个文件夹，执行如下命令:

```shell
chmod +x build.sh
./build.sh
```

输出如下:

```shell
# cmake 编译的输出省略
PL/0 compiler test suite
========================
../tests/0000... ok
../tests/0001... ok
../tests/0002... ok
../tests/0003... ok
../tests/0004... ok
../tests/0005... ok
../tests/0006... ok
../tests/0007... ok
../tests/0008... ok
../tests/0009... ok
../tests/0010... ok
```

这个脚本非常好理解，上半部分编译pl0的编译器生成二进制文件`pl0_c`。后半段遍历tests路径下的所有文件输出编译结果。

上面最多可有10,000个测试，命名和编号为0000到9999。如果出于任何原因需要更多，可以将printf行修改为%。做10万次测试。

## 写一个PL/0源码来进行测试

下面将写一个基本的测试作为开始。第一个测试是空程序，最小的合法PL/0程序。

该程序如下

```pascal
{ 0001: Comments }
{
  Multi-line comment
}
. { Inline comment }
{ End of program comment }

```

将该程序放入到tests中，命名为`example.pl0`。此时执行build.sh脚本效果如下:

```shell
# 省略
../tests/0009... ok
../tests/0010... ok
../tests/0011... ok
```

可以看到能够此时我们的编译器工作无误。

所有更改在 [on github](https://github.com/helintongh/pl0_pure_c/commit/53c704823ad59eeae12ea39c75e34d2ccc30a4af) 上可以找到。