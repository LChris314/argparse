/*##############################################################################
#                                                                              #
#                        Copyright 2018 TAM, Chun Pang                         #
#                                                                              #
#       The argparse project is covered by the terms of the MIT License.       #
#       See the file "LICENSE" for details.                                    #
#                                                                              #
##############################################################################*/

#include "argparse.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[]) {
    int exit_code = 0;

    Argparser *parser = malloc(Argparser_size());
    Argparser_init(parser, argc >= 1 ? argv[0] : "argparse", 3);
    Argparser_add_argument(parser, '\0', "lag", ARG_INT);
    Argparser_add_argument(parser, 'v', NULL, ARG_BOOL);
    Argparser_add_argument(parser, 'n', "name", ARG_STR);

    if (Argparser_parse(parser, argc, argv)) {
        exit_code = 1;
        goto main_exit;
    }

    printf("Verbosity level: %d\n", Argparser_bool_result(parser, 'v', NULL));

    int count;
    char *name = Argparser_str_result(parser, '\0', "name", &count);
    printf("Option '--name' specified %d times, last value is '%s'.\n", count,
           name ? name : "(null)");
    free(name);
    const intmax_t lag = Argparser_int_result(parser, '\0', "lag", &count);
    printf("Option '--lag' specified %d times, last value is '%" PRIiMAX "'.\n",
           count, lag);

    const size_t num_pos_args = Argparser_num_pos_args(parser);
    for (size_t i = 0; i < num_pos_args; ++i) {
        int argv_index = -1;
        if (!Argparser_get_pos_arg(parser, i, &argv_index))
            printf("Positional argument #%zu is '%s' at index %d.\n", i,
                   argv[argv_index], argv_index);
    }

main_exit:
    Argparser_deinit(parser);
    free(parser);
    return exit_code;
}
