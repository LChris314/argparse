/*##############################################################################
#                                                                              #
#                           Copyright 2018 C. P. Tam                           #
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

    ArgparseOpt opts[] = {{'n', "int", ARG_INT},
                          {'f', "float", ARG_FLOAT},
                          {'v', "verbose", ARG_BOOL},
                          {'s', "str", ARG_STR}};
    int pos_args[10] = {0};
    const size_t num_opts = sizeof opts / sizeof opts[0];
    const size_t max_pos_args = sizeof pos_args / sizeof pos_args[0];
    Argparser parser =
        Argparser_struct(argv[0], num_opts, opts, max_pos_args, pos_args);

    /*Argparser parser;
    Argparser_init(&parser, argv[0], -1);
    Argparser_add_argument(&parser, 'n', "int", ARG_INT);
    Argparser_add_argument(&parser, 'f', "float", ARG_FLOAT);
    Argparser_add_argument(&parser, 'v', "verbose", ARG_BOOL);
    Argparser_add_argument(&parser, 's', "str", ARG_STR);*/

    if (Argparser_parse(&parser, argc, argv)) {
        exit_code = 1;
        goto main_exit;
    }

    int argv_index = -1;
    const int vlevel = Argparser_bool_result(&parser, 'v', NULL, &argv_index);
    printf("Verbosity level: %d, last at index %d\n", vlevel, argv_index);

    const char *begin;
    int count =
        Argparser_str_result(&parser, 's', "str", &begin, NULL, &argv_index);
    printf(
        "Option '--str' specified %d times, last value is '%s' at index %d.\n",
        count, count ? begin : "(null)", argv_index);
    const intmax_t int_ = Argparser_int_result(&parser, 'n', "int", &count,
                                               NULL, NULL, &argv_index);
    printf("Option '--int' specified %d times, last value is '%" PRIiMAX
           "' at index %d.\n",
           count, int_, argv_index);
    const double d = Argparser_float_result(&parser, 'f', "float", &count, NULL,
                                            NULL, &argv_index);
    printf("Option '--float' specified %d times, last value is '%f' at index "
           "%d.\n",
           count, d, argv_index);

    const size_t num_pos_args = Argparser_num_pos_args(&parser);
    for (size_t i = 0; i < num_pos_args; ++i) {
        if (!Argparser_get_pos_arg(&parser, i, &argv_index))
            printf("Positional argument #%zu is '%s' at index %d.\n", i,
                   argv[argv_index], argv_index);
    }

main_exit:
    /*Argparser_deinit(&parser);*/
    return exit_code;
}
