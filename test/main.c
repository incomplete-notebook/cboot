/* main.c - CBoot generated entry point */

#include <stdio.h>
#include <stdlib.h>

#include "domain/domain.h"
#include "cupdate/cupdate.h"
#include "commands/commands.h"
#include "parser/parser.h"
#include "generator/generator.h"
#include "docgen/docgen.h"
#include "typecheck/typecheck.h"
#include "utils/utils.h"
#include "main/main.h"

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf("Hello from cboot!\n");
    return 0;
}
