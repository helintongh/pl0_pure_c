#ifndef PL0_COMPILER_GLOBAL_VARS
#define PL0_COMPILER_GLOBAL_VARS
#include <stddef.h>

char *raw, *token; // raw是指向整个源文件字符的指针,token存放最近一次识别出来的token序列
int type;         // 存放最近一次识别出来的token类型
size_t line = 1; // 行长度
int depth; // procedure的层数限制。允许源文件有无限个procedure块,但不允许procedure中有procedure

#endif