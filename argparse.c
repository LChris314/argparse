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

typedef struct ArgparseOpt {
    ArgparseType type;
    char short_opt;
    char *long_opt;
    const char *begin;
    size_t val_strlen;
    union {
        intmax_t int_val;
        double float_val;
    };
    int count, argv_index;
} ArgparseOpt;

struct Argparser {
    char *prog_name;
    size_t num_opts, opts_capacity, num_pos_args, pos_args_capacity;
    intmax_t max_pos_args;
    int *pos_args;
    ArgparseOpt *opts;
};

size_t Argparser_size() { return sizeof(Argparser); }

int Argparser_init(Argparser *const parser, const char *const prog_name,
                   const intmax_t max_pos_args) {
    memset(parser, 0, sizeof *parser);

    const size_t prog_name_strlen = strlen(prog_name);
    if (!(parser->prog_name = malloc(prog_name_strlen + 1)))
        return 1;
    strncpy(parser->prog_name, prog_name, prog_name_strlen + 1);

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
    free(parser->prog_name);
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
            fprintf(stderr, "%s: allocation failed in function %s, line %d\n",
                    parser->prog_name, __func__, __LINE__);
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
        fprintf(stderr, "%s: internal error in function %s, line %d\n",
                parser->prog_name, __func__, __LINE__);
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
    const size_t opt_strlen = strlen(argv[*argv_index]);
    char *opt_name = NULL;
    int has_equal_sign = 0;
    size_t i;
    for (i = 2;; ++i) {
        switch (argv[*argv_index][i]) {
        case '=':
            has_equal_sign = 1;
            break;
        case '\0':
            has_equal_sign = 0;
            break;
        default:
            continue;
        }
        if (!(opt_name = malloc(i - 2 + 1)))
            goto Argparser_recv_long_opt_fail_alloc;
        /* Don't use strncpy because the source may not be NULL-terminated */
        memcpy(opt_name, argv[*argv_index] + 2, i - 2);
        opt_name[i - 2] = '\0';
        break;
    }

    ArgparseOpt *opt;
    const char *begin = NULL;
    size_t val_strlen = 0;
    if (!(opt = Argparser_get_opt_ptr(parser, '\0', opt_name))) {
        fprintf(stderr, "%s: unknown option '--%s'\n", parser->prog_name,
                opt_name);
        goto Argparser_recv_long_opt_fail;
    }

    if (opt->type == ARG_BOOL) {
        if (has_equal_sign) {
            fprintf(stderr, "%s: option '--%s' does not take an argument\n",
                    parser->prog_name, opt_name);
            goto Argparser_recv_long_opt_fail;
        }
    } else {
        if (has_equal_sign) {
            /* Value is after the equal sign */
            ++i;
            val_strlen = opt_strlen - i;
            begin = argv[*argv_index] + i;
        }
        /* Increment the index to look for the argument */
        else if (++(*argv_index) >= argc) {
            /* We're missing an argument */
            fprintf(stderr, "%s: missing argument for option '--%s'\n",
                    parser->prog_name, opt_name);
            goto Argparser_recv_long_opt_fail;
        } else {
            begin = argv[*argv_index];
            val_strlen = strlen(begin);
        }

        /* Handle different argument types */
        if (Argparser_handle_opt(parser, opt, begin, val_strlen, 1))
            goto Argparser_recv_long_opt_fail;
    }
    opt->argv_index = *argv_index;
    opt->begin = begin;
    opt->val_strlen = val_strlen;
    ++opt->count;
    free(opt_name);
    return 0;

Argparser_recv_long_opt_fail_alloc:
    fprintf(stderr, "%s: allocation failed in function %s, line %d\n",
            parser->prog_name, __func__, __LINE__);
    goto Argparser_recv_long_opt_fail;

Argparser_recv_long_opt_fail:
    free(opt_name);
    return 1;
}

int Argparser_parse(Argparser *const parser, const int argc,
                    const char *const *const argv) {
    int pos_args_only = 0;
    for (int i = 1; i < argc; ++i) {
        const size_t len = strlen(argv[i]);
        if (!pos_args_only && len >= 3 && strncmp(argv[i], "--", 2) == 0) {
            Argparser_recv_long_opt(parser, argc, argv, &i);
        } else if (!pos_args_only && len >= 2 && argv[i][0] == '-') {
            if (argv[i][1] == '-')
                pos_args_only = 1;
            else
                Argparser_recv_short_opt(parser, argc, argv, &i, 1);
        } else {
            if (Argparser_recv_pos_arg(parser, i))
                return 1;
        }
    }
    return 0;
}

intmax_t Argparser_int_result(const Argparser *const parser,
                              const char short_opt, const char *const long_opt,
                              int *const count, const char **const begin,
                              size_t *const len, int *const argv_index) {
    ArgparseOpt *opt;
    if (!(opt = Argparser_get_opt_ptr(parser, short_opt, long_opt))) {
        *count = -1;
        return 0;
    }
    if (begin)
        *begin = opt->begin;
    if (len)
        *len = opt->val_strlen;
    if (argv_index)
        *argv_index = opt->argv_index;
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
    if (begin)
        *begin = opt->begin;
    if (len)
        *len = opt->val_strlen;
    if (argv_index)
        *argv_index = opt->argv_index;
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

    if (begin)
        *begin = opt->begin;
    if (len)
        *len = opt->val_strlen;
    if (argv_index)
        *argv_index = opt->argv_index;
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
