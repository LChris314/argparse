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

enum { RESULT_NOT_FOUND = -1, RESULT_ALLOC_FAILURE = -2 };

struct Argparser;
typedef struct Argparser Argparser;

size_t Argparser_size();

int Argparser_init(Argparser *const parser, const char *const prog_name);
void Argparser_deinit(Argparser *const parser);

int Argparser_add_argument(Argparser *const parser, const char short_opt,
                           const char *const long_opt, const ArgparseType type);

int Argparser_parse(Argparser *const parser, const int argc,
                    const char *const *const argv);

intmax_t Argparser_int_result(const Argparser *const parser,
                              const char short_opt, const char *const long_opt,
                              int *const count);

double Argparser_float_result(const Argparser *const parser,
                              const char short_opt, const char *const long_opt,
                              int *const count);

char *Argparser_str_result(const Argparser *const parser, const char short_opt,
                           const char *const long_opt, int *const count);

int Argparser_bool_result(const Argparser *const parser, const char short_opt,
                          const char *const long_opt);

size_t Argparser_num_pos_args(const Argparser *const parser);

char *Argparser_get_pos_arg(const Argparser *const parser, const size_t pos);

#ifdef __cplusplus
}
#endif

#endif /* ARGPARSE_H_8F131CC184D441DB9BD6BC9B3A943CD9 */
