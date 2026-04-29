/*
* MIT License - Copyright (c) 2026 Pshir
 * calculator_gui.c
 * Windows GUI Calculator using pure WinAPI — no external libraries needed.
 * Supports: + - * / % ^ ( )
 *
 * Project INFINITE
 */

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

 /* ── Constants ─────────────────────────────────────────────────────────── */
#define INPUT_MAX     256
#define HISTORY_MAX    10

#define ID_DISPLAY    100
#define ID_EXPR       101
#define ID_BTN_BASE   200

/* ── Colors ─────────────────────────────────────────────────────────────── */
#define COLOR_BG        RGB(18,  18,  18)
#define COLOR_DISPLAY   RGB(28,  28,  28)
#define COLOR_BTN_NUM   RGB(40,  40,  40)
#define COLOR_BTN_OP    RGB(30,  60,  90)
#define COLOR_BTN_EQ    RGB(24,  95, 165)
#define COLOR_BTN_CLR   RGB(90,  30,  30)
#define COLOR_BTN_SPEC  RGB(35,  70,  45)
#define COLOR_TEXT      RGB(255, 255, 255)
#define COLOR_TEXT_MUTE RGB(160, 160, 160)
#define COLOR_EXPR_TEXT RGB(120, 180, 255)

/* ── Button layout ──────────────────────────────────────────────────────── */
#define COLS         4
#define ROWS         6
#define BTN_W       80
#define BTN_H       55
#define BTN_GAP      8
#define MARGIN      12
#define DISPLAY_H   90
#define EXPR_H      24
#define WIN_W       (COLS * BTN_W + (COLS + 1) * BTN_GAP + MARGIN * 2)
#define WIN_H       (DISPLAY_H + EXPR_H + ROWS * BTN_H + (ROWS + 1) * BTN_GAP + MARGIN * 2 + 40)

/* ── Button definitions ─────────────────────────────────────────────────── */
typedef struct {
    const char* label;
    const char* value;
    COLORREF    color;
    int         span;    /* column span */
} ButtonDef;

static const ButtonDef buttons[ROWS][COLS] = {
    { {"C","C",COLOR_BTN_CLR,1}, {"(","(",COLOR_BTN_SPEC,1}, {")",
")",COLOR_BTN_SPEC,1}, {"<-","BACK",COLOR_BTN_OP,1} },
    { {"7","7",COLOR_BTN_NUM,1}, {"8","8",COLOR_BTN_NUM,1}, {"9","9",COLOR_BTN_NUM,1}, {"/","/",COLOR_BTN_OP,1} },
    { {"4","4",COLOR_BTN_NUM,1}, {"5","5",COLOR_BTN_NUM,1}, {"6","6",COLOR_BTN_NUM,1}, {"x","*",COLOR_BTN_OP,1} },
    { {"1","1",COLOR_BTN_NUM,1}, {"2","2",COLOR_BTN_NUM,1}, {"3","3",COLOR_BTN_NUM,1}, {"-","-",COLOR_BTN_OP,1} },
    { {"0","0",COLOR_BTN_NUM,1}, {".",".",COLOR_BTN_NUM,1}, {"x^y","^",COLOR_BTN_OP,1}, {"+","+",COLOR_BTN_OP,1} },
    { {"mod","%",COLOR_BTN_SPEC,1}, {"+/-","NEG",COLOR_BTN_SPEC,1}, {"=","=",COLOR_BTN_EQ,2}, {"","",COLOR_BTN_EQ,0} },
};

/* ── Result / Parser types ──────────────────────────────────────────────── */
typedef struct {
    double value;
    int    error;
    char   message[64];
} Result;

typedef struct {
    const char* src;
    int         pos;
} Parser;

/* ── State ──────────────────────────────────────────────────────────────── */
static char   g_expr[INPUT_MAX] = "";
static char   g_display[INPUT_MAX] = "0";
static int    g_error = 0;
static HWND   g_hwnd_display;
static HWND   g_hwnd_expr;
static HWND   g_hwnd_btns[ROWS][COLS];
static HFONT  g_font_display;
static HFONT  g_font_expr;
static HFONT  g_font_btn;
static HBRUSH g_brush_bg;
static HBRUSH g_brush_display;

/* ── Parser helpers ─────────────────────────────────────────────────────── */
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

static void skip_whitespace(Parser* p)
{
    while (p->src[p->pos] != '\0' && isspace((unsigned char)p->src[p->pos])) {
        p->pos++;
    }
}

static char pk(Parser* p)
{
    skip_whitespace(p);
    return p->src[p->pos];
}

static char cs(Parser* p)
{
    skip_whitespace(p);
    return p->src[p->pos++];
}

/* ── Forward declarations ───────────────────────────────────────────────── */
static Result parse_expression(Parser* p);
static Result parse_term(Parser* p);
static Result parse_power(Parser* p);
static Result parse_unary(Parser* p);
static Result parse_primary(Parser* p);

static Result parse_expression(Parser* p)
{
    Result left = parse_term(p);
    if (left.error) {
        return left;
    }
    while (pk(p) == '+' || pk(p) == '-') {
        char   op = cs(p);
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
    while (pk(p) == '*' || pk(p) == '/' || pk(p) == '%') {
        char   op = cs(p);
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
    if (pk(p) == '^') {
        cs(p);
        Result exp = parse_power(p);
        if (exp.error) {
            return exp;
        }
        return make_ok(pow(base.value, exp.value));
    }
    return base;
}

static Result parse_unary(Parser* p)
{
    char c = pk(p);
    if (c == '-') {
        cs(p);
        Result r = parse_unary(p);
        if (r.error) {
            return r;
        }
        return make_ok(-r.value);
    }
    if (c == '+') {
        cs(p);
        return parse_unary(p);
    }
    return parse_primary(p);
}

static Result parse_primary(Parser* p)
{
    char c = pk(p);
    if (c == '(') {
        cs(p);
        Result inner = parse_expression(p);
        if (inner.error) {
            return inner;
        }
        if (pk(p) != ')') {
            return make_error(2, "Missing closing )");
        }
        cs(p);
        return inner;
    }
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
        return make_error(6, "Unexpected token");
    }
    return r;
}

/* ── UI helpers ─────────────────────────────────────────────────────────── */
static void update_display(void)
{
    SetWindowTextA(g_hwnd_expr, g_expr);
    SetWindowTextA(g_hwnd_display, g_display);
    InvalidateRect(g_hwnd_display, NULL, TRUE);
    InvalidateRect(g_hwnd_expr, NULL, TRUE);
}

static void do_calculate(void)
{
    if (strlen(g_expr) == 0) {
        return;
    }
    Result r = evaluate(g_expr);
    if (r.error) {
        strncpy(g_display, r.message, INPUT_MAX - 1);
        g_error = 1;
    }
    else {
        /* Format: drop trailing zeros for integers */
        if (r.value == (long long)r.value && fabs(r.value) < 1e15) {
            snprintf(g_display, INPUT_MAX, "%lld", (long long)r.value);
        }
        else {
            snprintf(g_display, INPUT_MAX, "%.10g", r.value);
        }
        g_error = 0;
    }
    update_display();
}

static void handle_button(const char* val)
{
    size_t len = strlen(g_expr);

    if (strcmp(val, "C") == 0) {
        g_expr[0] = '\0';
        g_display[0] = '0';
        g_display[1] = '\0';
        g_error = 0;
        update_display();
        return;
    }

    if (strcmp(val, "BACK") == 0) {
        if (len > 0) {
            g_expr[len - 1] = '\0';
        }
        if (strlen(g_expr) == 0) {
            strcpy(g_display, "0");
        }
        else {
            strcpy(g_display, g_expr);
        }
        g_error = 0;
        update_display();
        return;
    }

    if (strcmp(val, "=") == 0) {
        do_calculate();
        return;
    }

    if (strcmp(val, "NEG") == 0) {
        if (len > 0) {
            /* wrap current expression in -(expr) */
            char tmp[INPUT_MAX];
            snprintf(tmp, INPUT_MAX, "-(%s)", g_expr);
            strncpy(g_expr, tmp, INPUT_MAX - 1);
            strcpy(g_display, g_expr);
            update_display();
        }
        return;
    }

    /* Append character */
    if (len < INPUT_MAX - 2) {
        g_expr[len] = val[0];
        g_expr[len + 1] = '\0';
        strcpy(g_display, g_expr);
        g_error = 0;
        update_display();
    }
}

/* ── Custom button drawing ──────────────────────────────────────────────── */
static LRESULT CALLBACK BtnProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
    UINT_PTR uid, DWORD_PTR data)
{
    static int s_hot = 0;
    static int s_down = 0;

    switch (msg) {
    case WM_MOUSEMOVE:
        if (!s_hot) {
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
            s_hot = 1;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        break;
    case WM_MOUSELEAVE:
        s_hot = 0;
        s_down = 0;
        InvalidateRect(hwnd, NULL, FALSE);
        break;
    case WM_LBUTTONDOWN:
        s_down = 1;
        InvalidateRect(hwnd, NULL, FALSE);
        break;
    case WM_LBUTTONUP:
        s_down = 0;
        InvalidateRect(hwnd, NULL, FALSE);
        break;
    case WM_PAINT: {
        PAINTSTRUCT  ps;
        HDC          hdc = BeginPaint(hwnd, &ps);
        RECT         rc;
        GetClientRect(hwnd, &rc);

        COLORREF base = (COLORREF)data;
        COLORREF color = base;
        if (s_down) {
            /* darken on press */
            color = RGB(
                (int)(GetRValue(base) * 0.7),
                (int)(GetGValue(base) * 0.7),
                (int)(GetBValue(base) * 0.7)
            );
        }
        else if (s_hot) {
            /* lighten on hover */
            color = RGB(
                min(255, (int)(GetRValue(base) * 1.3 + 20)),
                min(255, (int)(GetGValue(base) * 1.3 + 20)),
                min(255, (int)(GetBValue(base) * 1.3 + 20))
            );
        }

        HBRUSH brush = CreateSolidBrush(color);
        HPEN   pen = CreatePen(PS_SOLID, 1, RGB(60, 60, 60));
        SelectObject(hdc, brush);
        SelectObject(hdc, pen);
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 10, 10);
        DeleteObject(brush);
        DeleteObject(pen);

        /* Label */
        char label[16];
        GetWindowTextA(hwnd, label, sizeof(label));
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, COLOR_TEXT);
        SelectObject(hdc, g_font_btn);
        DrawTextA(hdc, label, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        EndPaint(hwnd, &ps);
        return 0;
    }
    }
    return DefSubclassProc(hwnd, msg, wp, lp);
}

/* ── Main window procedure ──────────────────────────────────────────────── */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE: {
        g_brush_bg = CreateSolidBrush(COLOR_BG);
        g_brush_display = CreateSolidBrush(COLOR_DISPLAY);

        g_font_display = CreateFontA(42, 0, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH, "Segoe UI");
        g_font_expr = CreateFontA(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH, "Segoe UI");
        g_font_btn = CreateFontA(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH, "Segoe UI");

        int y_top = MARGIN;

        /* Expression label */
        g_hwnd_expr = CreateWindowExA(
            0, "STATIC", "",
            WS_CHILD | WS_VISIBLE | SS_RIGHT,
            MARGIN, y_top, WIN_W - MARGIN * 2, EXPR_H,
            hwnd, (HMENU)ID_EXPR, NULL, NULL
        );
        SendMessage(g_hwnd_expr, WM_SETFONT, (WPARAM)g_font_expr, TRUE);

        /* Main display */
        g_hwnd_display = CreateWindowExA(
            0, "STATIC", "0",
            WS_CHILD | WS_VISIBLE | SS_RIGHT,
            MARGIN, y_top + EXPR_H, WIN_W - MARGIN * 2, DISPLAY_H,
            hwnd, (HMENU)ID_DISPLAY, NULL, NULL
        );
        SendMessage(g_hwnd_display, WM_SETFONT, (WPARAM)g_font_display, TRUE);

        /* Buttons */
        int y = MARGIN + EXPR_H + DISPLAY_H + BTN_GAP;
        for (int r = 0; r < ROWS; r++) {
            int x = MARGIN + BTN_GAP;
            for (int c = 0; c < COLS; c++) {
                const ButtonDef* b = &buttons[r][c];
                if (b->span == 0) {
                    continue;
                }
                int w = b->span * BTN_W + (b->span - 1) * BTN_GAP;
                g_hwnd_btns[r][c] = CreateWindowExA(
                    0, "BUTTON", b->label,
                    WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                    x, y, w, BTN_H,
                    hwnd, (HMENU)(UINT_PTR)(ID_BTN_BASE + r * COLS + c),
                    NULL, NULL
                );
                SetWindowSubclass(g_hwnd_btns[r][c], BtnProc, 0,
                    (DWORD_PTR)b->color);
                x += w + BTN_GAP;
            }
            y += BTN_H + BTN_GAP;
        }
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wp;
        SetBkColor(hdc, COLOR_DISPLAY);
        if ((HWND)lp == g_hwnd_display) {
            SetTextColor(hdc, g_error ? RGB(255, 100, 100) : COLOR_TEXT);
            SelectObject(hdc, g_font_display);
        }
        else {
            SetTextColor(hdc, COLOR_EXPR_TEXT);
            SelectObject(hdc, g_font_expr);
        }
        return (LRESULT)g_brush_display;
    }

    case WM_ERASEBKGND: {
        HDC  hdc = (HDC)wp;
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, g_brush_bg);
        return 1;
    }

    case WM_COMMAND: {
        int id = LOWORD(wp);
        if (id >= ID_BTN_BASE) {
            int idx = id - ID_BTN_BASE;
            int r = idx / COLS;
            int c = idx % COLS;
            if (r < ROWS && c < COLS) {
                handle_button(buttons[r][c].value);
            }
        }
        return 0;
    }

    case WM_KEYDOWN: {
        char key = (char)wp;
        if (key == VK_RETURN) {
            handle_button("=");
        }
        else if (key == VK_BACK) {
            handle_button("BACK");
        }
        else if (key == VK_ESCAPE) {
            handle_button("C");
        }
        return 0;
    }

    case WM_CHAR: {
        char key = (char)wp;
        char buf[2] = { key, '\0' };
        if (strchr("0123456789.+-*/%^()", key)) {
            handle_button(buf);
        }
        return 0;
    }

    case WM_DESTROY:
        DeleteObject(g_brush_bg);
        DeleteObject(g_brush_display);
        DeleteObject(g_font_display);
        DeleteObject(g_font_expr);
        DeleteObject(g_font_btn);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

/* ── Entry point ────────────────────────────────────────────────────────── */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow)
{
    (void)hPrev;
    (void)lpCmd;

    WNDCLASSEXA wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = "CalcWin";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExA(&wc);

    HWND hwnd = CreateWindowExA(
        0, "CalcWin", "C Calculator v1.0",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WIN_W + 16, WIN_H,
        NULL, NULL, hInst, NULL
    );

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}