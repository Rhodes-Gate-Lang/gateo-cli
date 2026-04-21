/**
 * gate_rt.c — Stable runtime for gate-codegen'd executables (C11).
 *
 * Compiled together with <stem>_design.c (emitted by `gate codegen`).
 * This file never changes between designs; all design-specific data arrives
 * through the ABI symbols declared in gate_rt.h.
 *
 * Compilation (invoked automatically by `gate codegen`):
 *   cc -std=c11 gate_rt.c <stem>_design.c -o <stem>
 */

#include "gate_rt.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdnoreturn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Internal helpers ────────────────────────────────────────────────────── */

noreturn static void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}

static uint64_t mask_width(uint32_t width) {
    if (width == 0)  return 0;
    if (width >= 64) return ~(uint64_t)0;
    return ((uint64_t)1 << width) - 1;
}

/* ── Number parsing ──────────────────────────────────────────────────────── */

uint64_t rt_parse_number(const char *s, uint32_t width) {
    while (isspace((unsigned char)*s)) ++s;
    if (*s == '\0') die("empty number string");

    uint64_t raw = 0;

    if (s[0] == '0' && (s[1] == 'b' || s[1] == 'B')) {
        /* Binary: 0b... */
        const char *p = s + 2;
        if (*p == '\0') die("binary literal '%s' has no digits after '0b'", s);
        for (; *p; ++p) {
            if (*p != '0' && *p != '1')
                die("binary literal '%s' contains non-binary character '%c'", s, *p);
            raw = (raw << 1) | (uint64_t)(*p - '0');
        }
    } else if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        /* Hexadecimal: 0x... */
        const char *p = s + 2;
        if (*p == '\0') die("hex literal '%s' has no digits after '0x'", s);
        char *end = NULL;
        raw = strtoull(p, &end, 16);
        if (*end != '\0')
            die("hex literal '%s' contains invalid character '%c'", s, *end);
    } else {
        /* Decimal (bare digits only) */
        char *end = NULL;
        raw = strtoull(s, &end, 10);
        if (end == s || *end != '\0')
            die("'%s' is not a valid decimal number", s);
    }

    const uint64_t mask    = mask_width(width);
    const uint64_t clipped = raw & mask;
    if (clipped != raw) {
        fprintf(stderr, "warning: value 0x%" PRIx64 " truncated to %u bits\n",
                raw, width);
    }
    return clipped;
}

/* ── Number formatting ───────────────────────────────────────────────────── */

void rt_print_number(uint64_t val, uint32_t width, int fmt) {
    const uint64_t w = val & mask_width(width);

    switch (fmt) {
        case GATE_FMT_BIN: {
            const int bits = (width == 0) ? 1
                           : (width > 64) ? 64
                           :                (int)width;
            printf("0b");
            for (int i = bits - 1; i >= 0; --i)
                putchar(((w >> i) & 1u) ? '1' : '0');
            break;
        }
        case GATE_FMT_DEC:
            printf("%" PRIu64, w);
            break;
        case GATE_FMT_HEX:
        default:
            printf("0x%" PRIx64, w);
            break;
    }
}

/* ── --output-format option ──────────────────────────────────────────────── */

static const char *fmt_name(int fmt) {
    switch (fmt) {
        case GATE_FMT_DEC: return "dec";
        case GATE_FMT_BIN: return "bin";
        default:           return "hex";
    }
}

static int parse_fmt(const char *s) {
    if (strcmp(s, "hex") == 0 || strcmp(s, "hexadecimal") == 0) return GATE_FMT_HEX;
    if (strcmp(s, "dec") == 0 || strcmp(s, "decimal")     == 0) return GATE_FMT_DEC;
    if (strcmp(s, "int") == 0)                                   return GATE_FMT_DEC;
    if (strcmp(s, "bin") == 0 || strcmp(s, "binary")      == 0) return GATE_FMT_BIN;
    die("unknown --output-format '%s' (accepted: hex, dec, bin)", s);
}

/* ── --help ──────────────────────────────────────────────────────────────── */

static void print_help(const char *prog) {
    printf("Usage: %s [OPTIONS] INPUTS...\n\n", prog);
    printf("Evaluate a compiled Gate-Lang design and print its output ports.\n\n");
    printf("Options:\n");
    printf("  --output-format FMT   Output number format (default: %s).\n"
           "                        Accepted: hex, decimal, binary\n",
           fmt_name(GATE_DEFAULT_OUTPUT_FORMAT));
    printf("  -h, --help            Print this help and exit\n\n");

    printf("Required inputs (%d port%s):\n",
           GATE_INPUT_COUNT, GATE_INPUT_COUNT == 1 ? "" : "s");
    for (int i = 0; i < GATE_INPUT_COUNT; ++i) {
        printf("  %s=<value>  [%u-bit]\n",
               GATE_INPUT_NAMES[i], GATE_INPUT_WIDTHS[i]);
    }

    printf("\nOutputs (%d port%s):\n",
           GATE_OUTPUT_COUNT, GATE_OUTPUT_COUNT == 1 ? "" : "s");
    for (int i = 0; i < GATE_OUTPUT_COUNT; ++i) {
        printf("  %s  [%u-bit]\n",
               GATE_OUTPUT_NAMES[i], GATE_OUTPUT_WIDTHS[i]);
    }

    printf("\nValue formats: 42  0x2a  0b101010\n");
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(int argc, char **argv) {
    int output_fmt = GATE_DEFAULT_OUTPUT_FORMAT;

    /* Collect positional NAME=VALUE tokens; at most argc-1 of them. */
    const char **assignments = malloc((size_t)argc * sizeof(char *));
    int          n_assign    = 0;
    if (!assignments) die("out of memory");

    /* ── Scan argv: consume flags, collect positionals ───────────────────── */
    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];

        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            print_help(argv[0]);
            free(assignments);
            return 0;
        }

        if (strncmp(arg, "--output-format=", 16) == 0) {
            output_fmt = parse_fmt(arg + 16);
            continue;
        }

        if (strcmp(arg, "--output-format") == 0) {
            if (++i >= argc) die("--output-format requires an argument");
            output_fmt = parse_fmt(argv[i]);
            continue;
        }

        if (arg[0] == '-') {
            die("unknown option '%s' (try --help)", arg);
        }

        assignments[n_assign++] = arg;
    }

    /* ── Allocate evaluation buffers ─────────────────────────────────────── */
    /*
     * Allocate at least 1 element so the NULL check is always safe; loops
     * are guarded by *_COUNT so the placeholder element is never accessed.
     */
    const int in_alloc  = GATE_INPUT_COUNT  > 0 ? GATE_INPUT_COUNT  : 1;
    const int out_alloc = GATE_OUTPUT_COUNT > 0 ? GATE_OUTPUT_COUNT : 1;

    uint64_t *inputs    = calloc((size_t)in_alloc,  sizeof(uint64_t));
    uint64_t *outputs   = calloc((size_t)out_alloc, sizeof(uint64_t));
    int      *satisfied = calloc((size_t)in_alloc,  sizeof(int));
    if (!inputs || !outputs || !satisfied) die("out of memory");

    /* ── Bind NAME=VALUE tokens to input slots ───────────────────────────── */
    for (int a = 0; a < n_assign; ++a) {
        const char *tok = assignments[a];
        const char *eq  = strchr(tok, '=');
        if (!eq) die("expected NAME=VALUE, got '%s' (try --help)", tok);

        const int name_len = (int)(eq - tok);
        if (name_len == 0) die("empty port name in assignment '%s'", tok);

        int slot = -1;
        for (int i = 0; i < GATE_INPUT_COUNT; ++i) {
            if ((int)strlen(GATE_INPUT_NAMES[i]) == name_len &&
                strncmp(GATE_INPUT_NAMES[i], tok, (size_t)name_len) == 0) {
                slot = i;
                break;
            }
        }
        if (slot < 0)
            die("unknown input port '%.*s' (try --help)", name_len, tok);
        if (satisfied[slot])
            die("duplicate assignment for port '%s'", GATE_INPUT_NAMES[slot]);

        inputs[slot]    = rt_parse_number(eq + 1, GATE_INPUT_WIDTHS[slot]);
        satisfied[slot] = 1;
    }

    /* ── Verify every required input was provided ────────────────────────── */
    for (int i = 0; i < GATE_INPUT_COUNT; ++i) {
        if (!satisfied[i]) {
            die("missing value for required input port '%s' [%u-bit] (try --help)",
                GATE_INPUT_NAMES[i], GATE_INPUT_WIDTHS[i]);
        }
    }

    /* ── Evaluate & print ────────────────────────────────────────────────── */
    design_eval(inputs, outputs);

    for (int i = 0; i < GATE_OUTPUT_COUNT; ++i) {
        printf("%s [%u]: ", GATE_OUTPUT_NAMES[i], GATE_OUTPUT_WIDTHS[i]);
        rt_print_number(outputs[i], GATE_OUTPUT_WIDTHS[i], output_fmt);
        putchar('\n');
    }

    free(assignments);
    free(inputs);
    free(outputs);
    free(satisfied);
    return 0;
}
