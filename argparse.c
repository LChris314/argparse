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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int Argparser_init(Argparser *const parser, const char *const prog_name,
                   const intmax_t max_pos_args) {
    memset(parser, 0, sizeof *parser);
    parser->prog_name = prog_name;

    parser->opts_capacity = ARGPARSER_INITIAL_CAPACITY;
    if (!(parser->opts = malloc(parser->opts_capacity * sizeof *parser->opts)))
        return 1;

    parser->max_pos_args = max_pos_args;
    parser->pos_args_capacity = ARGPARSER_INITIAL_CAPACITY;
    if (0 <= max_pos_args && max_pos_args < (intmax_t)parser->pos_args_capacity)
        parser->pos_args_capacity = max_pos_args;
    if (!(parser->pos_args =
              malloc(parser->pos_args_capacity * sizeof *parser->pos_args)))
        return 1;

    return 0;
}

static void ArgparseOpt_deinit(ArgparseOpt *const opt) { free(opt->long_opt); }

void Argparser_deinit(Argparser *const parser) {
    free(parser->pos_args);
    for (size_t i = 0; i < parser->num_opts; ++i)
        ArgparseOpt_deinit(parser->opts + i);
    free(parser->opts);
}

int Argparser_add_argument(Argparser *const parser, const char short_opt,
                           const char *const long_opt,
                           const ArgparseType type) {
    /* Grow the opts array if needed */
    if (parser->num_opts >= parser->opts_capacity) {
        ArgparseOpt *new_opts;
        parser->opts_capacity *= 2;
        if (!(new_opts = realloc(parser->opts,
                                 parser->opts_capacity * sizeof *parser->opts)))
            return 1;
        parser->opts = new_opts;
    }

    ArgparseOpt *opt = parser->opts + (parser->num_opts++);
    memset(opt, 0, sizeof *opt);
    opt->short_opt = short_opt;
    opt->type = type;
    if (long_opt) {
        const size_t len = strlen(long_opt);
        if (!(opt->long_opt = malloc(len + 1)))
            return 1;
        strncpy(opt->long_opt, long_opt, len + 1);
    }
    return 0;
}

/* Get a pointer to the ArgparseOpt with matching option, otherwise NULL */
static ArgparseOpt *Argparser_get_opt_ptr(const Argparser *const parser,
                                          const char short_opt,
                                          const char *const long_opt) {
    if (short_opt) {
        for (size_t i = 0; i < parser->num_opts; ++i)
            if (short_opt == parser->opts[i].short_opt)
                return parser->opts + i;
    } else {
        for (size_t i = 0; i < parser->num_opts; ++i)
            if (parser->opts[i].long_opt &&
                strcmp(long_opt, parser->opts[i].long_opt) == 0)
                return parser->opts + i;
    }
    return NULL;
}

/* Process a positional argument */
static int Argparser_recv_pos_arg(Argparser *const parser,
                                  const int argv_index) {
    /* Too many positional arguments */
    if (0 <= parser->max_pos_args &&
        parser->max_pos_args <= (intmax_t)parser->num_pos_args) {
        fprintf(stderr,
                "%s: too many positional arguments (at most %" PRIiMAX ")\n",
                parser->prog_name, parser->max_pos_args);
        return 1;
    }

    /* Grow the pos_args array if needed */
    if (parser->num_pos_args >= parser->pos_args_capacity) {
        int *new_pos_args;
        parser->pos_args_capacity *= 2;
        if (0 <= parser->max_pos_args &&
            parser->max_pos_args < (intmax_t)parser->pos_args_capacity)
            parser->pos_args_capacity = parser->max_pos_args;
        if (!(new_pos_args =
                  realloc(parser->pos_args, parser->pos_args_capacity *
                                                sizeof *parser->pos_args))) {
            fprintf(stderr,
                    "%s: allocation failed in function %s, line %d of %s\n",
                    parser->prog_name, __func__, __LINE__, __FILE__);
            return 1;
        }
        parser->pos_args = new_pos_args;
    }

    parser->pos_args[parser->num_pos_args++] = argv_index;
    return 0;
}

static int Argparser_handle_opt(Argparser *const parser, ArgparseOpt *const opt,
                                const char *const val, const size_t val_strlen,
                                const int is_long_opt) {
    if (opt->type == ARG_STR)
        return 0;

    /* For error messages */
    const char *dashes = is_long_opt ? "--" : "-";
    const char short_opt_str[2] = {opt->short_opt, '\0'};
    const char *opt_str = is_long_opt ? opt->long_opt : short_opt_str;

    char *endptr;
    switch (opt->type) {
    case ARG_INT:
        opt->int_val = strtoimax(val, &endptr, 10);
        if (endptr == val || (size_t)(endptr - val) != val_strlen) {
            /* No conversion, or val is not consumed fully */
            fprintf(stderr,
                    "%s: argument '%s' for option '%s%s' is not a valid "
                    "integer\n",
                    parser->prog_name, val, dashes, opt_str);
            return 1;
        }
        return 0;
    case ARG_FLOAT:
        opt->float_val = strtod(val, &endptr);
        if (endptr == val || (size_t)(endptr - val) != val_strlen) {
            /* No conversion, or val is not consumed fully */
            fprintf(stderr,
                    "%s: argument '%s' for option '%s%s' is not a valid "
                    "floating point number\n",
                    parser->prog_name, val, dashes, opt_str);
            return 1;
        }
        return 0;
    default:
        fprintf(stderr, "%s: internal error in function %s, line %d of %s\n",
                parser->prog_name, __func__, __LINE__, __FILE__);
        return 1;
    }
}

static int Argparser_recv_short_opt(Argparser *const parser, const int argc,
                                    const char *const *const argv,
                                    int *const argv_index, const size_t i) {
    const char short_opt = argv[*argv_index][i];
    const int is_last_char = argv[*argv_index][i + 1] == '\0';
    ArgparseOpt *opt;
    if (!(opt = Argparser_get_opt_ptr(parser, short_opt, NULL))) {
        fprintf(stderr, "%s: unknown option '-%c'\n", parser->prog_name,
                short_opt);
        return 1;
    }

    if (opt->type == ARG_BOOL) {
        /* Option doesn't take an argument */
        ++opt->count;
        opt->argv_index = *argv_index;
        if (is_last_char)
            return 0;
        /* Look at the rest of the characters */
        return Argparser_recv_short_opt(parser, argc, argv, argv_index, i + 1);
    }

    const size_t val_strlen = is_last_char ? strlen(argv[++*argv_index])
                                           : strlen(argv[*argv_index]) - i - 1;
    const char *begin = argv[*argv_index] + (is_last_char ? 0 : i + 1);
    if (Argparser_handle_opt(parser, opt, begin, val_strlen, 0))
        return 1;
    opt->argv_index = *argv_index;
    opt->begin = begin;
    opt->val_strlen = val_strlen;
    ++opt->count;
    return 0;
}

static int Argparser_recv_long_opt(Argparser *const parser, const int argc,
                                   const char *const *const argv,
                                   int *const argv_index) {
    const char *opt_name = argv[*argv_index] + 2;
    int has_equal_sign = 0;
    size_t opt_end_index;
    for (opt_end_index = 2;; ++opt_end_index) {
        if (argv[*argv_index][opt_end_index] == '=') {
            has_equal_sign = 1;
            break;
        } else if (argv[*argv_index][opt_end_index] == '\0') {
            has_equal_sign = 0;
            break;
        }
    }

    ArgparseOpt *opt = NULL;
    if (has_equal_sign) {
        /* opt_name is *not* NULL-terminated */
        for (size_t j = 0; j < parser->num_opts; ++j) {
            const char *long_opt = parser->opts[j].long_opt;
            if (long_opt &&
                strncmp(long_opt, opt_name, opt_end_index - 2) == 0 &&
                long_opt[opt_end_index - 2] == '\0') {
                opt = parser->opts + j;
                break;
            }
        }
    } else {
        opt = Argparser_get_opt_ptr(parser, '\0', opt_name);
    }
    if (!opt) {
        fprintf(stderr, "%s: unknown option '--%.*s'\n", parser->prog_name,
                (int)opt_end_index - 2, opt_name);
        return 1;
    }
    /* opt_name is NULL-terminated from now on */
    opt_name = opt->long_opt;

    const char *begin = NULL;
    size_t val_strlen = 0;
    if (opt->type == ARG_BOOL) {
        if (has_equal_sign) {
            fprintf(stderr, "%s: option '--%s' does not take an argument\n",
                    parser->prog_name, opt_name);
            return 1;
        }
    } else {
        if (has_equal_sign) {
            /* Value is after the equal sign */
            ++opt_end_index;
            val_strlen = strlen(argv[*argv_index]) - opt_end_index;
            begin = argv[*argv_index] + opt_end_index;
        }
        /* Increment the index to look for the argument */
        else if (++(*argv_index) >= argc) {
            /* We're missing an argument */
            fprintf(stderr, "%s: missing argument for option '--%s'\n",
                    parser->prog_name, opt_name);
            return 1;
        } else {
            begin = argv[*argv_index];
            val_strlen = strlen(begin);
        }

        /* Handle different argument types */
        if (Argparser_handle_opt(parser, opt, begin, val_strlen, 1))
            return 1;
    }
    opt->argv_index = *argv_index;
    opt->begin = begin;
    opt->val_strlen = val_strlen;
    ++opt->count;
    return 0;
}

int Argparser_parse(Argparser *const parser, const int argc,
                    const char *const *const argv) {
    int pos_args_only = 0;
    for (int i = 1; i < argc; ++i) {
        const size_t len = strlen(argv[i]);
        if (!pos_args_only && len >= 3 && strncmp(argv[i], "--", 2) == 0) {
            if (Argparser_recv_long_opt(parser, argc, argv, &i))
                return 1;
        } else if (!pos_args_only && len >= 2 && argv[i][0] == '-') {
            if (argv[i][1] == '-')
                pos_args_only = 1;
            else if (Argparser_recv_short_opt(parser, argc, argv, &i, 1))
                return 1;
        } else if (Argparser_recv_pos_arg(parser, i))
            return 1;
    }
    return 0;
}

#define ASSIGN_INFO(_opt, _begin, _len, _argv_index)                           \
    do {                                                                       \
        if (_begin)                                                            \
            *_begin = _opt->begin;                                             \
        if (_len)                                                              \
            *_len = _opt->val_strlen;                                          \
        if (_argv_index)                                                       \
            *_argv_index = _opt->argv_index;                                   \
    } while (0)

intmax_t Argparser_int_result(const Argparser *const parser,
                              const char short_opt, const char *const long_opt,
                              int *const count, const char **const begin,
                              size_t *const len, int *const argv_index) {
    ArgparseOpt *opt;
    if (!(opt = Argparser_get_opt_ptr(parser, short_opt, long_opt))) {
        *count = -1;
        return 0;
    }
    ASSIGN_INFO(opt, begin, len, argv_index);
    if (count)
        *count = opt->count;
    return opt->int_val;
}

double Argparser_float_result(const Argparser *const parser,
                              const char short_opt, const char *const long_opt,
                              int *const count, const char **const begin,
                              size_t *const len, int *const argv_index) {
    ArgparseOpt *opt;
    if (!(opt = Argparser_get_opt_ptr(parser, short_opt, long_opt))) {
        *count = -1;
        return 0;
    }
    ASSIGN_INFO(opt, begin, len, argv_index);
    if (count)
        *count = opt->count;
    return opt->float_val;
}

int Argparser_str_result(const Argparser *const parser, const char short_opt,
                         const char *const long_opt, const char **const begin,
                         size_t *const len, int *const argv_index) {
    ArgparseOpt *opt;
    if (!(opt = Argparser_get_opt_ptr(parser, short_opt, long_opt)))
        return -1;
    if (opt->count == 0)
        return 0;

    ASSIGN_INFO(opt, begin, len, argv_index);
    return opt->count;
}

int Argparser_bool_result(const Argparser *const parser, const char short_opt,
                          const char *const long_opt, int *const argv_index) {
    ArgparseOpt *opt;
    if (!(opt = Argparser_get_opt_ptr(parser, short_opt, long_opt)))
        return -1;
    if (argv_index)
        *argv_index = opt->argv_index;
    return opt->count;
}

size_t Argparser_num_pos_args(const Argparser *const parser) {
    return parser->num_pos_args;
}

int Argparser_get_pos_arg(const Argparser *const parser, const size_t pos,
                          int *const argv_index) {
    if (parser->num_pos_args <= pos)
        return 1;
    if (argv_index)
        *argv_index = parser->pos_args[pos];
    return 0;
}
