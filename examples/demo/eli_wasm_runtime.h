/**
 * @file eli_wasm_runtime.h
 * @brief Minimal freestanding libc glue that lets the elimgui demo link as a
 *        standalone wasm32 module with clang + wasm-ld (no Emscripten).
 *
 * The project builds with `clang --target=wasm32 -nostdlib` and routes libc
 * through JAClibc. JAClibc is header-only, but its full implementation bundle
 * (`<static.h>`, enabled by JACL_MAIN) does not compile for the wasm/jsrun
 * target in this checkout (JACL_FMT is undefined), and its printf/strtod path
 * pulls 128-bit `long double` soft-float builtins (__addtf3, __multf3, ...)
 * that compiler-rt for wasm32 would have to supply — and `-nostdlib` excludes
 * compiler-rt. JAClibc's header-only declarations otherwise link cleanly; with
 * NDEBUG (no assert) the ONLY runtime symbols the elimgui library leaves
 * undefined are the allocator, qsort, and the bounded formatter. This header
 * supplies exactly those, all self-contained (no `long double`), so nothing
 * outside the wasm module is required:
 *
 *   - malloc / free / calloc / realloc  -> free-list allocator over linear mem
 *   - qsort                             -> insertion sort (used once at atlas bake)
 *   - vsnprintf / snprintf              -> compact double-based formatter
 *
 * Include this ONCE, from the demo translation unit, after <eli/elimgui.h>.
 *
 * @status Demo-only build glue. Not part of the elimgui library API.
 * @issues None
 * @todo None
 */
#ifndef ELI_WASM_RUNTIME_H
#define ELI_WASM_RUNTIME_H

/* ---------------------------------------------------------------------------
 * Heap: a first-fit free-list allocator over WebAssembly linear memory.
 *
 * Blocks are laid out in address order, each prefixed by a header. `free`
 * coalesces forward with an adjacent free block; `realloc` grows by copy. New
 * memory is obtained by growing linear memory in whole pages past __heap_base
 * (the base symbol wasm-ld places after static data).
 * ------------------------------------------------------------------------- */

#define ELI_RT_WASM_PAGE 65536u
#define ELI_RT_ALIGN     16u

/* Placed by wasm-ld at the end of static data; start of usable heap. */
extern unsigned char __heap_base;

/** One heap block header; the payload follows immediately after. */
typedef struct eli_rt_block {
    size_t size;                 /* payload bytes (multiple of ELI_RT_ALIGN) */
    struct eli_rt_block *next;   /* next block in ascending address order */
    int is_free;                 /* 1 when available for reuse */
} eli_rt_block;

static eli_rt_block *eli_rt_heap_head = NULL;
static unsigned char *eli_rt_brk = NULL;   /* next unused byte */
static unsigned char *eli_rt_lim = NULL;   /* current linear-memory limit */

/** Round `n` up to the allocator alignment. */
static size_t eli_rt_align_up(size_t n)
{
    return (n + (ELI_RT_ALIGN - 1)) & ~(size_t)(ELI_RT_ALIGN - 1);
}

/** Carve `n` aligned bytes off the break, growing linear memory as needed. */
static void *eli_rt_sbrk(size_t n)
{
    if (eli_rt_brk == NULL) {
        eli_rt_brk = &__heap_base;
        eli_rt_lim = (unsigned char *)((size_t)__builtin_wasm_memory_size(0) * ELI_RT_WASM_PAGE);
    }
    n = eli_rt_align_up(n);
    if (eli_rt_brk + n > eli_rt_lim) {
        size_t need = (size_t)(eli_rt_brk + n - eli_rt_lim);
        size_t pages = (need + ELI_RT_WASM_PAGE - 1) / ELI_RT_WASM_PAGE;
        if (__builtin_wasm_memory_grow(0, pages) == (size_t)-1)
            return NULL;
        eli_rt_lim += pages * ELI_RT_WASM_PAGE;
    }
    void *p = eli_rt_brk;
    eli_rt_brk += n;
    return p;
}

/** Allocate `n` bytes (16-byte aligned); NULL on exhaustion. */
void *malloc(size_t n)
{
    if (n == 0)
        n = 1;
    n = eli_rt_align_up(n);

    eli_rt_block *last = NULL;
    for (eli_rt_block *b = eli_rt_heap_head; b; last = b, b = b->next) {
        if (!b->is_free || b->size < n)
            continue;
        /* Split when the remainder can hold a header plus a minimal payload. */
        if (b->size >= n + sizeof(eli_rt_block) + ELI_RT_ALIGN) {
            eli_rt_block *rest = (eli_rt_block *)((unsigned char *)(b + 1) + n);
            rest->size = b->size - n - sizeof(*rest);
            rest->next = b->next;
            rest->is_free = 1;
            b->next = rest;
            b->size = n;
        }
        b->is_free = 0;
        return b + 1;
    }

    eli_rt_block *nb = (eli_rt_block *)eli_rt_sbrk(sizeof(*nb) + n);
    if (!nb)
        return NULL;
    nb->size = n;
    nb->next = NULL;
    nb->is_free = 0;
    if (last)
        last->next = nb;
    else
        eli_rt_heap_head = nb;
    return nb + 1;
}

/** Release a block and coalesce it with the following block when both free. */
void free(void *p)
{
    if (!p)
        return;
    eli_rt_block *b = (eli_rt_block *)p - 1;
    b->is_free = 1;
    while (b->next && b->next->is_free) {
        b->size += sizeof(*b->next) + b->next->size;
        b->next = b->next->next;
    }
}

/** Allocate zero-initialized storage for `nmemb * size` bytes. */
void *calloc(size_t nmemb, size_t size)
{
    size_t total = nmemb * size;
    if (size != 0 && total / size != nmemb)
        return NULL; /* multiplication overflow */
    void *p = malloc(total);
    if (p)
        memset(p, 0, total);
    return p;
}

/** Resize a block, preserving contents; grows by allocate-copy-free. */
void *realloc(void *p, size_t size)
{
    if (!p)
        return malloc(size);
    if (size == 0) {
        free(p);
        return NULL;
    }
    eli_rt_block *b = (eli_rt_block *)p - 1;
    if (b->size >= eli_rt_align_up(size))
        return p;
    void *np = malloc(size);
    if (!np)
        return NULL;
    memcpy(np, p, b->size);
    free(p);
    return np;
}

/* ---------------------------------------------------------------------------
 * qsort — insertion sort. Called once during font-atlas baking to order a few
 * hundred glyph rects, so O(n^2) on small n is inconsequential and it keeps the
 * glue tiny and allocation-free.
 * ------------------------------------------------------------------------- */

/** Swap `width` bytes between two records. */
static void eli_rt_bswap(unsigned char *a, unsigned char *b, size_t width)
{
    while (width--) {
        unsigned char t = *a;
        *a++ = *b;
        *b++ = t;
    }
}

/** Sort `nmemb` records of `size` bytes in place using `cmp`. */
void qsort(void *base, size_t nmemb, size_t size, int (*cmp)(const void *, const void *))
{
    unsigned char *a = (unsigned char *)base;
    for (size_t i = 1; i < nmemb; i++)
        for (size_t j = i; j > 0 && cmp(a + (j - 1) * size, a + j * size) > 0; j--)
            eli_rt_bswap(a + (j - 1) * size, a + j * size, size);
}

/* ---------------------------------------------------------------------------
 * strtod / atof — double-based number parsing for the numeric-input widgets.
 * Handles optional sign, integer/fraction digits, and a decimal exponent.
 * ------------------------------------------------------------------------- */

/** Parse a floating-point value from `s`; on return `*endptr` (if non-NULL)
 *  points past the consumed characters. No `long double` is used. */
double strtod(const char *s, char **endptr)
{
    const char *p = s;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == '\f' || *p == '\v')
        p++;

    int sign = 1;
    if (*p == '+' || *p == '-') {
        if (*p == '-')
            sign = -1;
        p++;
    }

    double value = 0.0;
    int any_digits = 0;
    while (*p >= '0' && *p <= '9') {
        value = value * 10.0 + (double)(*p - '0');
        p++;
        any_digits = 1;
    }
    if (*p == '.') {
        p++;
        double scale = 0.1;
        while (*p >= '0' && *p <= '9') {
            value += (double)(*p - '0') * scale;
            scale *= 0.1;
            p++;
            any_digits = 1;
        }
    }

    if (any_digits && (*p == 'e' || *p == 'E')) {
        const char *e = p + 1;
        int esign = 1;
        if (*e == '+' || *e == '-') {
            if (*e == '-')
                esign = -1;
            e++;
        }
        if (*e >= '0' && *e <= '9') {
            int exp = 0;
            while (*e >= '0' && *e <= '9') {
                exp = exp * 10 + (*e - '0');
                e++;
            }
            double factor = 1.0;
            for (int i = 0; i < exp; i++)
                factor *= 10.0;
            value = esign < 0 ? value / factor : value * factor;
            p = e;
        }
    }

    if (endptr)
        *endptr = (char *)(any_digits ? p : s);
    return sign < 0 ? -value : value;
}

/** Parse a floating-point value from `s`, ignoring trailing characters. */
double atof(const char *s)
{
    return strtod(s, NULL);
}

/** strtoll — parse a signed integer (base 0/10/16/8) for the numeric-input
 *  widgets. Base 0 auto-detects 0x (hex) / 0 (octal) prefixes. */
long long strtoll(const char *s, char **endptr, int base)
{
    const char *p = s;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;

    int sign = 1;
    if (*p == '+' || *p == '-') {
        if (*p == '-')
            sign = -1;
        p++;
    }

    if ((base == 0 || base == 16) && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
        base = 16;
    } else if (base == 0 && p[0] == '0') {
        base = 8;
    } else if (base == 0) {
        base = 10;
    }

    long long value = 0;
    int any = 0;
    for (;;) {
        int c = *p, digit;
        if (c >= '0' && c <= '9')
            digit = c - '0';
        else if (c >= 'a' && c <= 'z')
            digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z')
            digit = c - 'A' + 10;
        else
            break;
        if (digit >= base)
            break;
        value = value * base + digit;
        any = 1;
        p++;
    }

    if (endptr)
        *endptr = (char *)(any ? p : s);
    return sign < 0 ? -value : value;
}

/* ---------------------------------------------------------------------------
 * Compact vsnprintf / snprintf
 *
 * A bounded, double-based formatter covering the conversions elimgui uses for
 * widget labels and value readouts: %d %i %u %x %X %p %c %s %f %e %g %%, with
 * flag/width/precision/length parsing (- + space 0 #, width, .prec, h/l/z).
 * Deliberately avoids `long double` so no soft-float builtins are needed.
 * ------------------------------------------------------------------------- */

/** Bounded output cursor: writes up to `cap-1` bytes, always counts would-be length. */
typedef struct eli_rt_sink {
    char *buf;
    size_t cap;   /* total buffer capacity (including NUL slot) */
    size_t len;   /* bytes that WOULD be written (may exceed cap-1) */
} eli_rt_sink;

/** Emit one byte to the sink, respecting capacity but always advancing len. */
static void eli_rt_putc(eli_rt_sink *s, char c)
{
    if (s->buf && s->len + 1 < s->cap)
        s->buf[s->len] = c;
    s->len++;
}

/** Emit `n` copies of `c` (used for field padding). */
static void eli_rt_pad(eli_rt_sink *s, char c, int n)
{
    for (int i = 0; i < n; i++)
        eli_rt_putc(s, c);
}

/** Parsed conversion-spec state threaded through the integer/float emitters. */
typedef struct eli_rt_spec {
    int left;       /* '-' : left-justify */
    int zero;       /* '0' : zero-pad */
    int plus;       /* '+' : always show sign */
    int space;      /* ' ' : space before positive */
    int alt;        /* '#' : alternate form */
    int width;      /* minimum field width */
    int prec;       /* precision, or -1 if unspecified */
} eli_rt_spec;

/** Write a prepared body with the requested sign/prefix, honoring width/justify. */
static void eli_rt_emit_padded(eli_rt_sink *s, const eli_rt_spec *sp, const char *sign,
                               const char *prefix, const char *body, int body_len)
{
    int sign_len = (int)strlen(sign);
    int prefix_len = (int)strlen(prefix);
    int total = sign_len + prefix_len + body_len;
    int pad = sp->width > total ? sp->width - total : 0;

    if (!sp->left && !sp->zero)
        eli_rt_pad(s, ' ', pad);
    for (int i = 0; i < sign_len; i++)
        eli_rt_putc(s, sign[i]);
    for (int i = 0; i < prefix_len; i++)
        eli_rt_putc(s, prefix[i]);
    if (!sp->left && sp->zero)
        eli_rt_pad(s, '0', pad);
    for (int i = 0; i < body_len; i++)
        eli_rt_putc(s, body[i]);
    if (sp->left)
        eli_rt_pad(s, ' ', pad);
}

/** Format an unsigned magnitude into `out` (reversed then corrected); returns length. */
static int eli_rt_utoa(unsigned long long v, int base, int upper, char *out)
{
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char tmp[24];
    int n = 0;
    do {
        tmp[n++] = digits[v % (unsigned)base];
        v /= (unsigned)base;
    } while (v);
    for (int i = 0; i < n; i++)
        out[i] = tmp[n - 1 - i];
    out[n] = '\0';
    return n;
}

/** Emit a signed/unsigned integer conversion. */
static void eli_rt_emit_int(eli_rt_sink *s, const eli_rt_spec *sp, unsigned long long mag,
                            int negative, int base, int upper)
{
    char body[24];
    int body_len = eli_rt_utoa(mag, base, upper, body);

    /* Precision on integers sets a minimum digit count (and disables zero-pad). */
    char prec_body[40];
    if (sp->prec >= 0) {
        int lead = sp->prec > body_len ? sp->prec - body_len : 0;
        int k = 0;
        for (int i = 0; i < lead; i++)
            prec_body[k++] = '0';
        for (int i = 0; i < body_len; i++)
            prec_body[k++] = body[i];
        prec_body[k] = '\0';
        body_len = k;
    } else {
        strcpy(prec_body, body);
    }

    const char *sign = negative ? "-" : (sp->plus ? "+" : (sp->space ? " " : ""));
    const char *prefix = "";
    if (sp->alt && base == 16 && mag != 0)
        prefix = upper ? "0X" : "0x";

    eli_rt_spec eff = *sp;
    if (sp->prec >= 0)
        eff.zero = 0; /* explicit precision overrides zero-pad for integers */
    eli_rt_emit_padded(s, &eff, sign, prefix, prec_body, body_len);
}

/** Round-half-away scale helper: 10^p as a double (small p only). */
static double eli_rt_pow10(int p)
{
    double r = 1.0;
    while (p-- > 0)
        r *= 10.0;
    return r;
}

/** Emit a %f fixed-point conversion of a finite double. */
static void eli_rt_emit_fixed(eli_rt_sink *s, const eli_rt_spec *sp, double v)
{
    int prec = sp->prec < 0 ? 6 : sp->prec;
    int negative = 0;
    if (v < 0.0 || (v == 0.0 && 1.0 / v < 0.0)) {
        negative = 1;
        v = -v;
    }

    double scale = eli_rt_pow10(prec);
    double scaled = v * scale + 0.5; /* round half up */
    unsigned long long whole_scaled = (unsigned long long)scaled;
    unsigned long long int_part = prec > 0 ? whole_scaled / (unsigned long long)scale : whole_scaled;
    unsigned long long frac_part = prec > 0 ? whole_scaled % (unsigned long long)scale : 0;

    char body[64];
    int n = eli_rt_utoa(int_part, 10, 0, body);
    if (prec > 0 || sp->alt) {
        body[n++] = '.';
        char frac[24];
        int fn = eli_rt_utoa(frac_part, 10, 0, frac);
        for (int i = 0; i < prec - fn; i++)
            body[n++] = '0';
        for (int i = 0; i < fn && i < prec; i++)
            body[n++] = frac[i];
    }
    body[n] = '\0';

    const char *sign = negative ? "-" : (sp->plus ? "+" : (sp->space ? " " : ""));
    eli_rt_emit_padded(s, sp, sign, "", body, n);
}

/** Emit a %e scientific conversion of a finite double. */
static void eli_rt_emit_sci(eli_rt_sink *s, const eli_rt_spec *sp, double v, int upper)
{
    int prec = sp->prec < 0 ? 6 : sp->prec;
    int negative = 0;
    if (v < 0.0) {
        negative = 1;
        v = -v;
    }
    int exp = 0;
    if (v != 0.0) {
        while (v >= 10.0) {
            v /= 10.0;
            exp++;
        }
        while (v < 1.0) {
            v *= 10.0;
            exp--;
        }
    }
    eli_rt_spec mant_sp = *sp;
    mant_sp.width = 0;
    mant_sp.plus = 0;
    mant_sp.space = 0;
    mant_sp.zero = 0;

    char mant[64];
    eli_rt_sink ms = {mant, sizeof(mant), 0};
    eli_rt_emit_fixed(&ms, &mant_sp, v);
    mant[ms.len < sizeof(mant) ? ms.len : sizeof(mant) - 1] = '\0';

    char body[96];
    int n = 0;
    for (size_t i = 0; mant[i] && n < (int)sizeof(body) - 6; i++)
        body[n++] = mant[i];
    body[n++] = upper ? 'E' : 'e';
    body[n++] = exp < 0 ? '-' : '+';
    int ae = exp < 0 ? -exp : exp;
    char ebuf[8];
    int en = eli_rt_utoa((unsigned long long)ae, 10, 0, ebuf);
    if (en < 2)
        body[n++] = '0';
    for (int i = 0; i < en; i++)
        body[n++] = ebuf[i];
    body[n] = '\0';

    const char *sign = negative ? "-" : (sp->plus ? "+" : (sp->space ? " " : ""));
    eli_rt_emit_padded(s, sp, sign, "", body, n);
    (void)prec;
}

/** Emit a %g conversion (chooses %f or %e, trims trailing zeros). */
static void eli_rt_emit_general(eli_rt_sink *s, const eli_rt_spec *sp, double v, int upper)
{
    int prec = sp->prec < 0 ? 6 : (sp->prec == 0 ? 1 : sp->prec);
    double av = v < 0.0 ? -v : v;
    int exp = 0;
    if (av != 0.0) {
        while (av >= 10.0) {
            av /= 10.0;
            exp++;
        }
        while (av < 1.0) {
            av *= 10.0;
            exp--;
        }
    }
    eli_rt_spec g = *sp;
    if (exp < -4 || exp >= prec) {
        g.prec = prec - 1;
        eli_rt_emit_sci(s, &g, v, upper);
    } else {
        g.prec = prec - 1 - exp;
        if (g.prec < 0)
            g.prec = 0;
        eli_rt_emit_fixed(s, &g, v);
    }
}

/** Parse flags/width/precision/length from `fmt`, advancing it; fills `sp`. */
static const char *eli_rt_parse_spec(const char *fmt, eli_rt_spec *sp, va_list *ap)
{
    sp->left = sp->zero = sp->plus = sp->space = sp->alt = 0;
    sp->width = 0;
    sp->prec = -1;

    for (;; fmt++) {
        if (*fmt == '-') sp->left = 1;
        else if (*fmt == '0') sp->zero = 1;
        else if (*fmt == '+') sp->plus = 1;
        else if (*fmt == ' ') sp->space = 1;
        else if (*fmt == '#') sp->alt = 1;
        else break;
    }
    if (*fmt == '*') {
        sp->width = va_arg(*ap, int);
        if (sp->width < 0) {
            sp->left = 1;
            sp->width = -sp->width;
        }
        fmt++;
    } else {
        while (*fmt >= '0' && *fmt <= '9')
            sp->width = sp->width * 10 + (*fmt++ - '0');
    }
    if (*fmt == '.') {
        fmt++;
        sp->prec = 0;
        if (*fmt == '*') {
            sp->prec = va_arg(*ap, int);
            fmt++;
        } else {
            while (*fmt >= '0' && *fmt <= '9')
                sp->prec = sp->prec * 10 + (*fmt++ - '0');
        }
    }
    return fmt;
}

/** Length-modifier rank: 0=int, 1=long, 2=long long, and z (size_t) maps to long. */
static const char *eli_rt_parse_length(const char *fmt, int *rank)
{
    *rank = 0;
    if (*fmt == 'h') {
        fmt++;
        if (*fmt == 'h')
            fmt++;
    } else if (*fmt == 'l') {
        fmt++;
        *rank = 1;
        if (*fmt == 'l') {
            fmt++;
            *rank = 2;
        }
    } else if (*fmt == 'z' || *fmt == 't' || *fmt == 'j') {
        fmt++;
        *rank = 2;
    }
    return fmt;
}

/** Pull a signed integer arg of the given length rank, split into sign+magnitude. */
static unsigned long long eli_rt_signed_arg(va_list *ap, int rank, int *negative)
{
    long long v = rank >= 2 ? va_arg(*ap, long long) : (rank == 1 ? va_arg(*ap, long) : va_arg(*ap, int));
    if (v < 0) {
        *negative = 1;
        return (unsigned long long)(-(v + 1)) + 1ULL;
    }
    *negative = 0;
    return (unsigned long long)v;
}

/** Pull an unsigned integer arg of the given length rank. */
static unsigned long long eli_rt_unsigned_arg(va_list *ap, int rank)
{
    if (rank >= 2)
        return va_arg(*ap, unsigned long long);
    if (rank == 1)
        return va_arg(*ap, unsigned long);
    return va_arg(*ap, unsigned int);
}

/** Emit a %s conversion, honoring precision (max chars) and width. */
static void eli_rt_emit_str(eli_rt_sink *s, const eli_rt_spec *sp, const char *str)
{
    if (!str)
        str = "(null)";
    int len = 0;
    while (str[len] && (sp->prec < 0 || len < sp->prec))
        len++;
    int pad = sp->width > len ? sp->width - len : 0;
    if (!sp->left)
        eli_rt_pad(s, ' ', pad);
    for (int i = 0; i < len; i++)
        eli_rt_putc(s, str[i]);
    if (sp->left)
        eli_rt_pad(s, ' ', pad);
}

/**
 * Bounded formatted print (freestanding subset). Writes at most `n-1` chars
 * plus a NUL when `n > 0`, and returns the length that would have been written.
 *
 * @param out  Destination buffer (may be NULL when `n` is 0).
 * @param n    Buffer capacity in bytes.
 * @param fmt  printf-style format string.
 * @param ap   Variadic argument cursor.
 * @return     Number of characters that a full write would produce.
 *
 * Thread-safe: yes (no shared state)  Reentrant: yes
 */
int vsnprintf(char *out, size_t n, const char *fmt, va_list ap)
{
    eli_rt_sink s = {out, n, 0};

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            eli_rt_putc(&s, *p);
            continue;
        }
        p++;
        if (*p == '%') {
            eli_rt_putc(&s, '%');
            continue;
        }

        eli_rt_spec sp;
        p = eli_rt_parse_spec(p, &sp, &ap);
        int rank;
        p = eli_rt_parse_length(p, &rank);

        switch (*p) {
        case 'd':
        case 'i': {
            int neg;
            unsigned long long mag = eli_rt_signed_arg(&ap, rank, &neg);
            eli_rt_emit_int(&s, &sp, mag, neg, 10, 0);
            break;
        }
        case 'u':
            eli_rt_emit_int(&s, &sp, eli_rt_unsigned_arg(&ap, rank), 0, 10, 0);
            break;
        case 'x':
            eli_rt_emit_int(&s, &sp, eli_rt_unsigned_arg(&ap, rank), 0, 16, 0);
            break;
        case 'X':
            eli_rt_emit_int(&s, &sp, eli_rt_unsigned_arg(&ap, rank), 0, 16, 1);
            break;
        case 'o':
            eli_rt_emit_int(&s, &sp, eli_rt_unsigned_arg(&ap, rank), 0, 8, 0);
            break;
        case 'p': {
            sp.alt = 1;
            eli_rt_emit_int(&s, &sp, (unsigned long long)(uintptr_t)va_arg(ap, void *), 0, 16, 0);
            break;
        }
        case 'c': {
            char ch = (char)va_arg(ap, int);
            char body[1] = {ch};
            eli_rt_emit_padded(&s, &sp, "", "", body, 1);
            break;
        }
        case 's':
            eli_rt_emit_str(&s, &sp, va_arg(ap, const char *));
            break;
        case 'f':
        case 'F':
            eli_rt_emit_fixed(&s, &sp, va_arg(ap, double));
            break;
        case 'e':
            eli_rt_emit_sci(&s, &sp, va_arg(ap, double), 0);
            break;
        case 'E':
            eli_rt_emit_sci(&s, &sp, va_arg(ap, double), 1);
            break;
        case 'g':
            eli_rt_emit_general(&s, &sp, va_arg(ap, double), 0);
            break;
        case 'G':
            eli_rt_emit_general(&s, &sp, va_arg(ap, double), 1);
            break;
        case '\0':
            p--; /* trailing '%' — stop */
            break;
        default:
            eli_rt_putc(&s, '%');
            eli_rt_putc(&s, *p);
            break;
        }
    }

    if (s.buf && s.cap > 0)
        s.buf[s.len < s.cap ? s.len : s.cap - 1] = '\0';
    return (int)s.len;
}

/** Bounded formatted print wrapper (see vsnprintf). */
int snprintf(char *out, size_t n, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(out, n, fmt, ap);
    va_end(ap);
    return r;
}

#endif /* ELI_WASM_RUNTIME_H */
