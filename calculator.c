/*
 * MIT License - Copyright (c) 2026 Pshir
 * calculator.c
 * A clean, interactive command-line calculator in C.
 * Supports: +, -, *, /, %, ^ (power), and parentheses.
 *
 * Build:  gcc -o calculator calculator.c -lm
 * Run:    ./calculator
 */

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

 /* ── Constants ─────────────────────────────────────────────────────────── */
#define INPUT_MAX   256
#define HISTORY_MAX  10

/* ── Result type ────────────────────────────────────────────────────────── */
typedef struct {
    double value;
    int    error;        /* 0 = ok, non-zero = error code */
    char   message[64];
} 
Result;

/* ── History ────────────────────────────────────────────────────────────── */
typedef struct {
    char   expression[INPUT_MAX];
    double value;
} 
HistoryEntry;

static HistoryEntry history[HISTORY_MAX];
static int;
history_count = 0;

/* ── Parser state ───────────────────────────────────────────────────────── */
typedef struct {
    const char* src;
    int         pos;
} 
Parser;

/* ── Forward declarations ───────────────────────────────────────────────── */
static Result parse_expression(Parser* p);
static Result parse_term(Parser* p);
static Result parse_power(Parser* p);
static Result parse_unary(Parser* p);
static Result parse_primary(Parser* p);

/* ── Error helpers ──────────────────────────────────────────────────────── */
static Result make_ok(double v)
{
    Result r;
    r.value = v;
    r.error = 0;
    r.message[0] = '\0';
    return r;
}

static Result make_error(int code, const char* msg)
{
    Result r;
    r.value = 0.0;
    r.error = code;
    strncpy(r.message, msg, sizeof(r.message) - 1);
    r.message[sizeof(r.message) - 1] = '\0';
    return r;
}

/* ── Lexer helpers ──────────────────────────────────────────────────────── */
static void skip_whitespace(Parser* p)
{
    while (p->src[p->pos] != '\0' && isspace((unsigned char)p->src[p->pos])) {
        p->pos++;
    }
}

static char peek(Parser* p)
{
    skip_whitespace(p);
    return p->src[p->pos];
}

static char consume(Parser* p)
{
    skip_whitespace(p);
    return p->src[p->pos++];
}

/* ── Grammar ────────────────────────────────────────────────────────────────
 *
 *   expression  = term { ('+' | '-') term }
 *   term        = power { ('*' | '/' | '%') power }
 *   power       = unary { '^' unary }
 *   unary       = ('-' | '+') unary | primary
 *   primary     = NUMBER | '(' expression ')'
 *
 * ────────────────────────────────────────────────────────────────────────── */

static Result parse_expression(Parser* p)
{
    Result left = parse_term(p);
    if (left.error) {
        return left;
    }

    while (peek(p) == '+' || peek(p) == '-') {
        char   op = consume(p);
        Result right = parse_term(p);
        if (right.error) {
            return right;
        }

        if (op == '+') {
            left = make_ok(left.value + right.value);
        }
        else {
            left = make_ok(left.value - right.value);
        }
    }
    return left;
}

static Result parse_term(Parser* p)
{
    Result left = parse_power(p);
    if (left.error) {
        return left;
    }

    while (peek(p) == '*' || peek(p) == '/' || peek(p) == '%') {
        char   op = consume(p);
        Result right = parse_power(p);
        if (right.error) {
            return right;
        }

        if (op == '*') {
            left = make_ok(left.value * right.value);
        }
        else if (op == '/') {
            if (right.value == 0.0) {
                return make_error(1, "Division by zero");
            }
            left = make_ok(left.value / right.value);
        }
        else {
            if (right.value == 0.0) {
                return make_error(1, "Modulo by zero");
            }
            left = make_ok(fmod(left.value, right.value));
        }
    }
    return left;
}

static Result parse_power(Parser* p)
{
    Result base = parse_unary(p);
    if (base.error) {
        return base;
    }

    if (peek(p) == '^') {
        consume(p);
        Result exp = parse_power(p);   /* right-associative */
        if (exp.error) {
            return exp;
        }
        return make_ok(pow(base.value, exp.value));
    }
    return base;
}

static Result parse_unary(Parser* p)
{
    char c = peek(p);

    if (c == '-') {
        consume(p);
        Result r = parse_unary(p);
        if (r.error) {
            return r;
        }
        return make_ok(-r.value);
    }

    if (c == '+') {
        consume(p);
        return parse_unary(p);
    }

    return parse_primary(p);
}

static Result parse_primary(Parser* p)
{
    char c = peek(p);

    /* Parenthesised sub-expression */
    if (c == '(') {
        consume(p);
        Result inner = parse_expression(p);
        if (inner.error) {
            return inner;
        }
        if (peek(p) != ')') {
            return make_error(2, "Missing closing parenthesis");
        }
        consume(p);
        return inner;
    }

    /* Number literal (integer or floating-point) */
    if (isdigit((unsigned char)c) || c == '.') {
        skip_whitespace(p);
        char* end;
        double val = strtod(p->src + p->pos, &end);
        if (end == p->src + p->pos) {
            return make_error(3, "Expected a number");
        }
        p->pos = (int)(end - p->src);
        return make_ok(val);
    }

    if (c == '\0') {
        return make_error(4, "Unexpected end of input");
    }

    return make_error(5, "Unexpected character");
}

/* ── Public evaluate function ───────────────────────────────────────────── */
static Result evaluate(const char* expr)
{
    Parser p;
    p.src = expr;
    p.pos = 0;

    Result r = parse_expression(&p);
    if (r.error) {
        return r;
    }

    skip_whitespace(&p);
    if (p.src[p.pos] != '\0') {
        return make_error(6, "Unexpected token after expression");
    }

    return r;
}

/* ── History helpers ────────────────────────────────────────────────────── */
static void history_push(const char* expr, double val)
{
    if (history_count < HISTORY_MAX) {
        strncpy(history[history_count].expression, expr, INPUT_MAX - 1);
        history[history_count].expression[INPUT_MAX - 1] = '\0';
        history[history_count].value = val;
        history_count++;
    }
    else {
        /* Shift entries left, drop oldest */
        memmove(&history[0], &history[1],
            sizeof(HistoryEntry) * (HISTORY_MAX - 1));
        strncpy(history[HISTORY_MAX - 1].expression, expr, INPUT_MAX - 1);
        history[HISTORY_MAX - 1].expression[INPUT_MAX - 1] = '\0';
        history[HISTORY_MAX - 1].value = val;
    }
}

static void history_print(void)
{
    if (history_count == 0) {
        printf("  (no history yet)\n");
        return;
    }
    for (int i = 0; i < history_count; i++) {
        printf("  [%d]  %s  =  %g\n",
            i + 1, history[i].expression, history[i].value);
    }
}

/* ── REPL ───────────────────────────────────────────────────────────────── */
static void print_banner(void)
{
    puts("+------------------------------------+");
    puts("|        C Calculator v1.0           |");
    puts("+------------------------------------+");
    puts("|  Operators: + - * / % ^ ( )        |");
    puts("|  Commands : history | clear | quit  |");
    puts("+------------------------------------+");
    putchar('\n');
}

int main(void)
{
    char input[INPUT_MAX];

    print_banner();

    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }

        /* Strip trailing newline */
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[--len] = '\0';
        }

        /* Skip blank lines */
        if (len == 0) {
            continue;
        }

        /* Built-in commands */
        if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
            break;
        }

        if (strcmp(input, "history") == 0) {
            history_print();
            continue;
        }

        if (strcmp(input, "clear") == 0) {
            history_count = 0;
            puts("  History cleared.");
            continue;
        }

        if (strcmp(input, "help") == 0) {
            print_banner();
            continue;
        }

        /* Evaluate expression */
        Result r = evaluate(input);

        if (r.error) {
            fprintf(stderr, "  Error: %s\n", r.message);
        }
        else {
            printf("  = %g\n", r.value);
            history_push(input, r.value);
        }
    }

    puts("\nBye!");
    return EXIT_SUCCESS;
}
