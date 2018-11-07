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

typedef struct ArgparseArg {
    char short_opt;
    union {
        intmax_t int_val;
        double float_val;
        char *str_val;
    };
    ArgparseType type;
    int count;
    char *long_opt;
} ArgparseArg;

struct Argparser {
    char *prog_name;
    size_t num_args, args_capacity, num_pos_args, pos_args_capacity;
    intmax_t max_pos_args;
    char **pos_args;
    ArgparseArg *args;
};

size_t Argparser_size() { return sizeof(Argparser); }

int Argparser_init(Argparser *const parser, const char *const prog_name,
                   const intmax_t max_pos_args) {
    memset(parser, 0, sizeof *parser);

    const size_t prog_name_strlen = strlen(prog_name);
    if (!(parser->prog_name = malloc(prog_name_strlen + 1)))
        return 1;
    strncpy(parser->prog_name, prog_name, prog_name_strlen + 1);

    parser->args_capacity = ARGPARSER_INITIAL_CAPACITY;
    if (!(parser->args = malloc(parser->args_capacity * sizeof *parser->args)))
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

static void ArgparseArg_deinit(ArgparseArg *const arg) {
    free(arg->long_opt);
    if (arg->type == ARG_STR)
        free(arg->str_val);
}

void Argparser_deinit(Argparser *const parser) {
    free(parser->prog_name);

    for (size_t i = 0; i < parser->num_pos_args; ++i)
        free(parser->pos_args[i]);
    free(parser->pos_args);

    for (size_t i = 0; i < parser->num_args; ++i)
        ArgparseArg_deinit(parser->args + i);
    free(parser->args);
}

int Argparser_add_argument(Argparser *const parser, const char short_opt,
                           const char *const long_opt,
                           const ArgparseType type) {
    /* Grow the args array if needed */
    if (parser->num_args >= parser->args_capacity) {
        ArgparseArg *new_args;
        parser->args_capacity *= 2;
        if (!(new_args = realloc(parser->args,
                                 parser->args_capacity * sizeof *parser->args)))
            return 1;
        parser->args = new_args;
    }

    ArgparseArg *arg = parser->args + (parser->num_args++);
    memset(arg, 0, sizeof *arg);
    arg->short_opt = short_opt;
    arg->type = type;
    if (long_opt) {
        const size_t len = strlen(long_opt);
        if (!(arg->long_opt = malloc(len + 1)))
            return 1;
        strncpy(arg->long_opt, long_opt, len + 1);
    }
    return 0;
}

/* Get a pointer to the ArgparseArg with matching option, otherwise NULL */
static ArgparseArg *Argparser_get_arg_ptr(const Argparser *const parser,
                                          const char short_opt,
                                          const char *const long_opt) {
    if (short_opt) {
        for (size_t i = 0; i < parser->num_args; ++i)
            if (short_opt == parser->args[i].short_opt)
                return parser->args + i;
    } else {
        for (size_t i = 0; i < parser->num_args; ++i)
            if (parser->args[i].long_opt &&
                strcmp(long_opt, parser->args[i].long_opt) == 0)
                return parser->args + i;
    }
    return NULL;
}

/* Process a positional argument */
static int Argparser_recv_pos_arg(Argparser *const parser,
                                  const char *const val,
                                  const size_t val_strlen) {
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
        char **new_pos_args;
        parser->pos_args_capacity *= 2;
        if (0 <= parser->max_pos_args &&
            parser->max_pos_args < (intmax_t)parser->pos_args_capacity)
            parser->pos_args_capacity = parser->max_pos_args;
        if (!(new_pos_args =
                  realloc(parser->pos_args, parser->pos_args_capacity *
                                                sizeof *parser->pos_args))) {
            goto Argparser_recv_pos_arg_fail_alloc;
        }
        parser->pos_args = new_pos_args;
    }

    if (!(parser->pos_args[parser->num_pos_args] = malloc(val_strlen + 1)))
        goto Argparser_recv_pos_arg_fail_alloc;
    strncpy(parser->pos_args[parser->num_pos_args], val, val_strlen + 1);
    ++parser->num_pos_args;
    return 0;

Argparser_recv_pos_arg_fail_alloc:
    fprintf(stderr, "%s: allocation failed in function %s, line %d\n",
            parser->prog_name, __func__, __LINE__);
    return 1;
}

static int Argparser_handle_arg(Argparser *const parser, ArgparseArg *const arg,
                                const char *const val, const size_t val_strlen,
                                const int is_long_opt) {
    /* For error messages */
    const char *dashes = is_long_opt ? "--" : "-";
    const char short_opt_str[2] = {arg->short_opt, '\0'};
    const char *opt_str = is_long_opt ? arg->long_opt : short_opt_str;

    char *endptr;
    switch (arg->type) {
    case ARG_INT: {
        arg->int_val = strtoimax(val, &endptr, 10);
        if (endptr == val || (size_t)(endptr - val) != val_strlen) {
            /* No conversion, or val is not consumed fully */
            fprintf(stderr,
                    "%s: argument '%s' for option '%s%s' is not a valid "
                    "integer\n",
                    parser->prog_name, val, dashes, opt_str);
            return 1;
        }
        return 0;
    }
    case ARG_FLOAT: {
        arg->float_val = strtod(val, &endptr);
        if (endptr == val || (size_t)(endptr - val) != val_strlen) {
            /* No conversion, or val is not consumed fully */
            fprintf(stderr,
                    "%s: argument '%s' for option '%s%s' is not a valid "
                    "floating point number\n",
                    parser->prog_name, val, dashes, opt_str);
            return 1;
        }
        return 0;
    }
    case ARG_STR: {
        char *new_str_val;
        if (!(new_str_val = realloc(arg->str_val, val_strlen + 1))) {
            fprintf(stderr, "%s: allocation failed in function %s, line %d\n",
                    parser->prog_name, __func__, __LINE__);
            return 1;
        }
        arg->str_val = new_str_val;
        strncpy(arg->str_val, val, val_strlen + 1);
        return 0;
    }
    default:
        fprintf(stderr, "%s: internal error in function %s, line %d\n",
                parser->prog_name, __func__, __LINE__);
        return 1;
    }
}

static int Argparser_recv_short_opt(Argparser *const parser, const int argc,
                                    const char *const *const argv,
                                    int *const arg_index, const size_t i) {
    const char opt = argv[*arg_index][i];
    const int is_last_char = argv[*arg_index][i + 1] == '\0';
    char *val = NULL;
    ArgparseArg *arg;
    if (!(arg = Argparser_get_arg_ptr(parser, opt, NULL))) {
        fprintf(stderr, "%s: unknown option '-%c'\n", parser->prog_name, opt);
        return 1;
    }

    if (arg->type == ARG_BOOL) {
        /* Option doesn't take an argument */
        ++arg->count;
        if (is_last_char)
            return 0;
        /* Look at the rest of the characters */
        return Argparser_recv_short_opt(parser, argc, argv, arg_index, i + 1);
    }

    const size_t val_strlen = strlen(argv[*arg_index]) - i - 1;
    if (!(val = malloc(val_strlen + 1)))
        goto Argparser_recv_short_opt_fail_alloc;
    strncpy(val, argv[*arg_index] + i + 1, val_strlen + 1);
    if (Argparser_handle_arg(parser, arg, val, val_strlen, 0))
        goto Argparser_recv_short_opt_fail;
    ++arg->count;

    free(val);
    return 0;

Argparser_recv_short_opt_fail_alloc:
    fprintf(stderr, "%s: allocation failed in function %s, line %d\n",
            parser->prog_name, __func__, __LINE__);
    goto Argparser_recv_short_opt_fail;

Argparser_recv_short_opt_fail:
    free(val);
    return 1;
}

static int Argparser_recv_long_opt(Argparser *const parser, const int argc,
                                   const char *const *const argv,
                                   int *const arg_index) {
    const size_t arg_strlen = strlen(argv[*arg_index]);
    char *arg_name = NULL, *val = NULL;
    int has_equal_sign = 0;
    size_t i;
    for (i = 2;; ++i) {
        switch (argv[*arg_index][i]) {
        case '=':
            has_equal_sign = 1;
            break;
        case '\0':
            has_equal_sign = 0;
            break;
        default:
            continue;
        }
        if (!(arg_name = malloc(i - 2 + 1)))
            goto Argparser_recv_long_opt_fail_alloc;
        /* Don't use strncpy because the source may not be NULL-terminated */
        memcpy(arg_name, argv[*arg_index] + 2, i - 2);
        arg_name[i - 2] = '\0';
        break;
    }

    ArgparseArg *arg;
    if (!(arg = Argparser_get_arg_ptr(parser, '\0', arg_name))) {
        fprintf(stderr, "%s: unknown option '--%s'\n", parser->prog_name,
                arg_name);
        goto Argparser_recv_long_opt_fail;
    }

    if (arg->type == ARG_BOOL) {
        if (has_equal_sign) {
            fprintf(stderr, "%s: option '--%s' does not take an argument\n",
                    parser->prog_name, arg_name);
            goto Argparser_recv_long_opt_fail;
        }
    } else {
        size_t val_strlen;
        if (has_equal_sign) {
            /* Copy everything after the equal sign */
            ++i;
            val_strlen = arg_strlen - i;
            if (!(val = malloc(val_strlen + 1)))
                goto Argparser_recv_long_opt_fail_alloc;
            strncpy(val, argv[*arg_index] + i, val_strlen + 1);
        }
        /* Increment the index to look for the argument */
        else if (++(*arg_index) >= argc) {
            /* We're missing an argument */
            fprintf(stderr, "%s: missing argument for option '--%s'\n",
                    parser->prog_name, arg_name);
            goto Argparser_recv_long_opt_fail;
        } else {
            val_strlen = strlen(argv[*arg_index]);
            if (!(val = malloc(val_strlen + 1)))
                goto Argparser_recv_long_opt_fail_alloc;
            strncpy(val, argv[*arg_index], val_strlen + 1);
        }

        /* Handle different argument types */
        if (Argparser_handle_arg(parser, arg, val, val_strlen, 1))
            goto Argparser_recv_long_opt_fail;
    }
    ++arg->count;
    free(val);
    free(arg_name);
    return 0;

Argparser_recv_long_opt_fail_alloc:
    fprintf(stderr, "%s: allocation failed in function %s, line %d\n",
            parser->prog_name, __func__, __LINE__);
    goto Argparser_recv_long_opt_fail;

Argparser_recv_long_opt_fail:
    free(val);
    free(arg_name);
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
            if (Argparser_recv_pos_arg(parser, argv[i], len))
                return 1;
        }
    }
    return 0;
}

intmax_t Argparser_int_result(const Argparser *const parser,
                              const char short_opt, const char *const long_opt,
                              int *const count) {
    ArgparseArg *arg;
    if (!(arg = Argparser_get_arg_ptr(parser, short_opt, long_opt))) {
        *count = RESULT_NOT_FOUND;
        return 0;
    }
    *count = arg->count;
    return arg->int_val;
}

double Argparser_float_result(const Argparser *const parser,
                              const char short_opt, const char *const long_opt,
                              int *const count) {
    ArgparseArg *arg;
    if (!(arg = Argparser_get_arg_ptr(parser, short_opt, long_opt))) {
        *count = RESULT_NOT_FOUND;
        return 0;
    }
    *count = arg->count;
    return arg->float_val;
}

char *Argparser_str_result(const Argparser *const parser, const char short_opt,
                           const char *const long_opt, int *const count) {
    ArgparseArg *arg;
    if (!(arg = Argparser_get_arg_ptr(parser, short_opt, long_opt))) {
        *count = RESULT_NOT_FOUND;
        return NULL;
    }
    if (arg->count == 0) {
        *count = 0;
        return NULL;
    }

    const size_t len = strlen(arg->str_val);
    char *result;
    if (!(result = malloc(len + 1))) {
        *count = RESULT_ALLOC_FAILURE;
        return NULL;
    }
    strncpy(result, arg->str_val, len + 1);
    *count = arg->count;
    return result;
}

int Argparser_bool_result(const Argparser *const parser, const char short_opt,
                          const char *const long_opt) {
    ArgparseArg *arg;
    if (!(arg = Argparser_get_arg_ptr(parser, short_opt, long_opt)))
        return RESULT_NOT_FOUND;
    return arg->count;
}

size_t Argparser_num_pos_args(const Argparser *const parser) {
    return parser->num_pos_args;
}

char *Argparser_get_pos_arg(const Argparser *const parser, const size_t pos) {
    if (parser->num_pos_args <= pos)
        return NULL;

    const size_t len = strlen(parser->pos_args[pos]);
    char *result;
    if (!(result = malloc(len + 1)))
        return NULL;
    strncpy(result, parser->pos_args[pos], len + 1);
    return result;
}
