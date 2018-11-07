/*##############################################################################
#                                                                              #
#                        Copyright 2018 TAM, Chun Pang                         #
#                                                                              #
#       The argparse project is covered by the terms of the MIT License.       #
#       See the file "LICENSE" for details.                                    #
#                                                                              #
##############################################################################*/

#include "argparse.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[]) {
    Argparser *parser = malloc(Argparser_size());
    Argparser_init(parser, argc >= 1 ? argv[0] : "argparse");
    Argparser_add_argument(parser, '\0', "lag", ARG_INT);
    Argparser_add_argument(parser, 'a', NULL, ARG_BOOL);
    Argparser_add_argument(parser, 'b', NULL, ARG_BOOL);
    Argparser_add_argument(parser, 'c', NULL, ARG_INT);
    Argparser_add_argument(parser, 'n', "name", ARG_STR);

    Argparser_parse(parser, argc, argv);

    int count;
    char *name = Argparser_str_result(parser, '\0', "name", &count);
    printf("Option '--name' specified %d times, last value is %s.\n", count,
           name ? name : "(null)");
    free(name);

    Argparser_deinit(parser);
    free(parser);
    return 0;
}
