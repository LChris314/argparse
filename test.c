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
    Argparser_add_argument(parser, 'v', NULL, ARG_BOOL);
    Argparser_add_argument(parser, 'n', "name", ARG_STR);

    Argparser_parse(parser, argc, argv);

    printf("Verbosity level: %d\n", Argparser_bool_result(parser, 'v', NULL));

    int count;
    char *name = Argparser_str_result(parser, '\0', "name", &count);
    printf("Option '--name' specified %d times, last value is '%s'.\n", count,
           name ? name : "(null)");
    free(name);

    const size_t num_pos_args = Argparser_num_pos_args(parser);
    for (size_t i = 0; i < num_pos_args; ++i) {
        char *pos_arg = Argparser_get_pos_arg(parser, i);
        printf("Positional argument #%zu is '%s'.\n", i, pos_arg);
        free(pos_arg);
    }

    Argparser_deinit(parser);
    free(parser);
    return 0;
}
