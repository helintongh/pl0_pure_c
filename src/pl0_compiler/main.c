

#include "utils.h"
#include "global_vars.h"
#include "lexer.h"
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