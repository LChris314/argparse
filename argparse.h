/*##############################################################################
#                                                                              #
#                        Copyright 2018 TAM, Chun Pang                         #
#                                                                              #
#       The argparse project is covered by the terms of the MIT License.       #
#       See the file "LICENSE" for details.                                    #
#                                                                              #
##############################################################################*/

#ifndef ARGPARSE_H_8F131CC184D441DB9BD6BC9B3A943CD9
#define ARGPARSE_H_8F131CC184D441DB9BD6BC9B3A943CD9

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#ifndef ARGPARSER_INITIAL_CAPACITY
#define ARGPARSER_INITIAL_CAPACITY 10
#endif

typedef enum ArgparseType {
    ARG_INT,
    ARG_FLOAT,
    ARG_STR,
    ARG_BOOL
} ArgparseType;

typedef struct ArgparseArgInfo {
    int count, argv_index;
    char *begin, *end;
} ArgparseArgInfo;

struct Argparser;
typedef struct Argparser Argparser;

size_t Argparser_size();

int Argparser_init(Argparser *const parser, const char *const prog_name,
                   const intmax_t max_pos_args);

void Argparser_deinit(Argparser *const parser);

int Argparser_add_argument(Argparser *const parser, const char short_opt,
                           const char *const long_opt, const ArgparseType type);

int Argparser_parse(Argparser *const parser, const int argc,
                    const char *const *const argv);

intmax_t Argparser_int_result(const Argparser *const parser,
                              const char short_opt, const char *const long_opt,
                              int *const count, const char **const begin,
                              size_t *const len, int *const argv_index);

double Argparser_float_result(const Argparser *const parser,
                              const char short_opt, const char *const long_opt,
                              int *const count, const char **const begin,
                              size_t *const len, int *const argv_index);

int Argparser_str_result(const Argparser *const parser, const char short_opt,
                         const char *const long_opt, const char **const begin,
                         size_t *const len, int *const argv_index);

int Argparser_bool_result(const Argparser *const parser, const char short_opt,
                          const char *const long_opt);

size_t Argparser_num_pos_args(const Argparser *const parser);

int Argparser_get_pos_arg(const Argparser *const parser, const size_t pos,
                          int *const argv_index);

#ifdef __cplusplus
}
#endif

#endif /* ARGPARSE_H_8F131CC184D441DB9BD6BC9B3A943CD9 */
