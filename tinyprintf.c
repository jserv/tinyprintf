/*
 * Copyright (C) 2004  Kustaa Nyholm
 *
 * This library can be licensed either under MIT, BSD or LGPL license.
 */

#include <stddef.h>

#include "tinyprintf.h"

/*
 * Configuration
 */

/* Enable long int support */
#define PRINTF_LONG_SUPPORT

/* Enable long long int support (implies long int support) */
#define PRINTF_LONG_LONG_SUPPORT

/* Enable %z (size_t) support */
#define PRINTF_SIZE_T_SUPPORT

/*
 * Configuration adjustments
 */
#ifdef PRINTF_LONG_LONG_SUPPORT
# define PRINTF_LONG_SUPPORT
#endif

/* __SIZEOF_<type>__ defined at least by gcc */
#ifdef __SIZEOF_POINTER__
# define SIZEOF_POINTER __SIZEOF_POINTER__
#endif
#ifdef __SIZEOF_LONG_LONG__
# define SIZEOF_LONG_LONG __SIZEOF_LONG_LONG__
#endif
#ifdef __SIZEOF_LONG__
# define SIZEOF_LONG __SIZEOF_LONG__
#endif
#ifdef __SIZEOF_INT__
# define SIZEOF_INT __SIZEOF_INT__
#endif

#ifdef __GNUC__
# define _TFP_GCC_NO_INLINE_  __attribute__ ((noinline))
#else
# define _TFP_GCC_NO_INLINE_
#endif

#define IS_DIGIT(x) ((x) >= '0' && (x) <= '9')

#ifdef PRINTF_LONG_SUPPORT
#define BF_MAX 20 /* long = 64b on some architectures */
#else
#define BF_MAX 10 /* int = 32b on some architectures */
#endif

/* Boilerplate for compiler compatibility */
#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

/* since C23 */
#ifndef __has_c_attribute
#define __has_c_attribute(x) 0
#endif

#if defined(__clang__) || defined(__GNUC__)
#define maybe_unused __attribute__((unused))
#else
#define maybe_unused
#endif

/* suppress warnings when intentional switch case fall-through is desired. */
#if defined(__has_c_attribute) && __has_c_attribute(fallthrough) /* C23+ */
#define fallthrough [[fallthrough]]
#elif defined(__has_attribute) && __has_attribute(fallthrough)
/* GNU-style attribute. The __has_attribute() macro was first available in Clang
 * 2.9 and incorporated into GCC since GCC 9.
 */
#define fallthrough __attribute__((fallthrough))
#elif defined(__GNUC__) && __GNUC__ >= 7
#define fallthrough __attribute__((__fallthrough__))
#else
#define fallthrough ((void) 0)
#endif

/*
 * Implementation
 */
struct param {
    unsigned char lz:1;          /**<  Leading zeros */
    unsigned char alt:1;         /**<  alternate form */
    unsigned char uc:1;          /**<  Upper case (for base16 only) */
    unsigned char align_left:1;  /**<  0 == align right (default), 1 == align left */
    int width;                   /**<  field width */
    int prec;                    /**<  precision */
    char sign;                   /**<  The sign to display (if any) */
    unsigned int base;           /**<  number base (e.g.: 8, 10, 16) */
    char *bf;                    /**<  Buffer to output */
    size_t bf_len;               /**<  Buffer length */
};


#ifdef PRINTF_LONG_LONG_SUPPORT
static void _TFP_GCC_NO_INLINE_ ulli2a(
    unsigned long long int num, struct param *p)
{
    unsigned long long int d = 1;
    char *bf = p->bf;
    if ((p->prec == 0) && (num == 0))
            return;
    while (num / d >= p->base) {
        d *= p->base;
    }
    while (d != 0) {
        int dgt = num / d;
        num %= d;
        d /= p->base;
        *bf++ = dgt + (dgt < 10 ? '0' : (p->uc ? 'A' : 'a') - 10);
    }
    p->bf_len = bf - p->bf;
}

static void lli2a(long long int num, struct param *p)
{
    if (num < 0) {
        num = -num;
        p->sign = '-';
    }
    ulli2a(num, p);
}
#endif

#ifdef PRINTF_LONG_SUPPORT
static void uli2a(unsigned long int num, struct param *p)
{
    unsigned long int d = 1;
    char *bf = p->bf;
    if ((p->prec == 0) && (num == 0))
            return;
    while (num / d >= p->base) {
        d *= p->base;
    }
    while (d != 0) {
        int dgt = num / d;
        num %= d;
        d /= p->base;
        *bf++ = dgt + (dgt < 10 ? '0' : (p->uc ? 'A' : 'a') - 10);
    }
    p->bf_len = bf - p->bf;
}

static void li2a(long num, struct param *p)
{
    if (num < 0) {
        num = -num;
        p->sign = '-';
    }
    uli2a(num, p);
}
#endif

static void ui2a(unsigned int num, struct param *p)
{
    unsigned int d = 1;
    char *bf = p->bf;
    if ((p->prec == 0) && (num == 0))
            return;
    while (num / d >= p->base) {
        d *= p->base;
    }
    while (d != 0) {
        int dgt = num / d;
        num %= d;
        d /= p->base;
        *bf++ = dgt + (dgt < 10 ? '0' : (p->uc ? 'A' : 'a') - 10);
    }
    p->bf_len = bf - p->bf;
}

static void i2a(int num, struct param *p)
{
    if (num < 0) {
        num = -num;
        p->sign = '-';
    }
    ui2a(num, p);
}

static int a2d(char ch)
{
    if (IS_DIGIT(ch))
        return ch - '0';
    else if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    else
        return -1;
}

static char a2u(char ch, const char **src, int base, unsigned int *nump)
{
    const char *p = *src;
    unsigned int num = 0;
    int digit;
    while ((digit = a2d(ch)) >= 0) {
        if (digit > base)
            break;
        num = num * base + digit;
        ch = *p++;
    }
    *src = p;
    *nump = num;
    return ch;
}

static void putchw(void *putp, putcf putf, struct param *p)
{
    char ch;
    int width = p->width;
    int prec = p->prec;
    char *bf = p->bf;
    size_t bf_len = p->bf_len;

    /* Number of filling characters */
    width -= bf_len;
    prec -= bf_len;
    if (p->sign)
        width--;
    if (p->alt && p->base == 16)
        width -= 2;
    else if (p->alt && p->base == 8)
        width--;
    if (prec > 0)
        width -= prec;

    /* Fill with space to align to the right, before alternate or sign */
    if (!p->lz && !p->align_left) {
        while (width-- > 0)
            putf(putp, ' ');
    }

    /* print sign */
    if (p->sign)
        putf(putp, p->sign);

    /* Alternate */
    if (p->alt && p->base == 16) {
        putf(putp, '0');
        putf(putp, (p->uc ? 'X' : 'x'));
    } else if (p->alt && p->base == 8) {
        putf(putp, '0');
    }

    /* Fill with zeros, after alternate or sign */
    while (prec-- > 0)
        putf(putp, '0');
    if (p->lz) {
        while (width-- > 0)
            putf(putp, '0');
    }

    /* Put actual buffer */
    while ((bf_len-- > 0) && (ch = *bf++))
        putf(putp, ch);

    /* Fill with space to align to the left, after string */
    if (!p->lz && p->align_left) {
        while (width-- > 0)
            putf(putp, ' ');
    }
}

void tfp_format(void *putp, putcf putf, const char *fmt, va_list va)
{
    struct param p;
    char bf[BF_MAX];
    char ch;

    while ((ch = *(fmt++))) {
        if (ch != '%') {
            putf(putp, ch);
        } else {
#ifdef PRINTF_LONG_SUPPORT
            char lng = 0;  /* 1 for long, 2 for long long */
#endif
            /* Init parameter struct */
            p.lz = 0;
            p.alt = 0;
            p.uc = 0;
            p.align_left = 0;
            p.width = 0;
            p.prec = -1;
            p.sign = 0;
            p.bf = bf;
            p.bf_len = 0;

            /* Flags */
            while ((ch = *(fmt++))) {
                switch (ch) {
                case '-':
                    p.align_left = 1;
                    continue;
                case '0':
                    p.lz = 1;
                    continue;
                case '#':
                    p.alt = 1;
                    continue;
                default:
                    break;
                }
                break;
            }

            if (p.align_left)
                p.lz = 0;

            /* Width */
            if (ch == '*') {
                ch = *(fmt++);
                p.width = va_arg(va, int);
                if (p.width < 0) {
                    p.align_left = 1;
                    p.width = -p.width;
                }
            } else if (IS_DIGIT(ch)) {
                unsigned int width;
                ch = a2u(ch, &fmt, 10, &(width));
                p.width = width;
            }

            /* Precision */
            if (ch == '.') {
                ch = *(fmt++);
                if (ch == '*') {
                    int prec;
                    ch = *(fmt++);
                    prec = va_arg(va, int);
                    if (prec < 0)
                        /* act as if precision was omitted */
                        p.prec = -1;
                    else
                        p.prec = prec;
                } else if (IS_DIGIT(ch)) {
                    unsigned int prec;
                    ch = a2u(ch, &fmt, 10, &(prec));
                    p.prec = prec;
                } else {
                    p.prec = 0;
                }
            }
            if (p.prec >= 0)
                /* precision causes zero pad to be ignored */
                p.lz = 0;

#ifdef PRINTF_SIZE_T_SUPPORT
# ifdef PRINTF_LONG_SUPPORT
            if (ch == 'z') {
                ch = *(fmt++);
                if (sizeof(size_t) == sizeof(unsigned long int))
                    lng = 1;
#  ifdef PRINTF_LONG_LONG_SUPPORT
                else if (sizeof(size_t) == sizeof(unsigned long long int))
                    lng = 2;
#  endif
            } else
# endif
#endif

#ifdef PRINTF_LONG_SUPPORT
            if (ch == 'l') {
                ch = *(fmt++);
                lng = 1;
#ifdef PRINTF_LONG_LONG_SUPPORT
                if (ch == 'l') {
                  ch = *(fmt++);
                  lng = 2;
                }
#endif
            }
#endif
            switch (ch) {
            case 0:
                goto abort;
            case 'u':
                p.base = 10;
                if (p.prec < 0)
                    p.prec = 1;
#ifdef PRINTF_LONG_SUPPORT
#ifdef PRINTF_LONG_LONG_SUPPORT
                if (2 == lng)
                    ulli2a(va_arg(va, unsigned long long int), &p);
                else
#endif
                  if (1 == lng)
                    uli2a(va_arg(va, unsigned long int), &p);
                else
#endif
                    ui2a(va_arg(va, unsigned int), &p);
                putchw(putp, putf, &p);
                break;
            case 'd':
            case 'i':
                p.base = 10;
                if (p.prec < 0)
                    p.prec = 1;
#ifdef PRINTF_LONG_SUPPORT
#ifdef PRINTF_LONG_LONG_SUPPORT
                if (2 == lng)
                    lli2a(va_arg(va, long long int), &p);
                else
#endif
                  if (1 == lng)
                    li2a(va_arg(va, long int), &p);
                else
#endif
                    i2a(va_arg(va, int), &p);
                putchw(putp, putf, &p);
                break;
#ifdef SIZEOF_POINTER
            case 'p':
                p.alt = 1;
# if defined(SIZEOF_INT) && SIZEOF_POINTER <= SIZEOF_INT
                lng = 0;
# elif defined(SIZEOF_LONG) && SIZEOF_POINTER <= SIZEOF_LONG
                lng = 1;
# elif defined(SIZEOF_LONG_LONG) && SIZEOF_POINTER <= SIZEOF_LONG_LONG
                lng = 2;
# endif
#endif
                fallthrough;
            case 'x':
            case 'X':
                p.base = 16;
                p.uc = (ch == 'X')?1:0;
                if (p.prec < 0)
                    p.prec = 1;
#ifdef PRINTF_LONG_SUPPORT
#ifdef PRINTF_LONG_LONG_SUPPORT
                if (2 == lng)
                    ulli2a(va_arg(va, unsigned long long int), &p);
                else
#endif
                  if (1 == lng)
                    uli2a(va_arg(va, unsigned long int), &p);
                else
#endif
                    ui2a(va_arg(va, unsigned int), &p);
                putchw(putp, putf, &p);
                break;
            case 'o':
                p.base = 8;
                if (p.prec < 0)
                    p.prec = 1;
                ui2a(va_arg(va, unsigned int), &p);
                putchw(putp, putf, &p);
                break;
            case 'c':
                putf(putp, (char)(va_arg(va, int)));
                break;
            case 's':
            {
                unsigned int prec = p.prec;
                const char *b;
                p.bf = va_arg(va, char *);
                b = p.bf;
                while ((prec-- != 0) && *b++) {
                    p.bf_len++;
                }
                p.prec = -1;
                putchw(putp, putf, &p);
            }
                break;
            case '%':
                putf(putp, ch);
            default:
                break;
            }
        }
    }
 abort:;
}

#if TINYPRINTF_DEFINE_TFP_PRINTF
static putcf stdout_putf;
static void *stdout_putp;

void init_printf(void *putp, putcf putf)
{
    stdout_putf = putf;
    stdout_putp = putp;
}

void tfp_printf(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    tfp_format(stdout_putp, stdout_putf, fmt, va);
    va_end(va);
}
#endif

#if TINYPRINTF_DEFINE_TFP_SPRINTF
struct _vsnprintf_putcf_data
{
  size_t dest_capacity;
  char *dest;
  size_t num_chars;
};

static void _vsnprintf_putcf(void *p, char c)
{
  struct _vsnprintf_putcf_data *data = (struct _vsnprintf_putcf_data*)p;
  if (data->num_chars < data->dest_capacity)
    data->dest[data->num_chars] = c;
  data->num_chars ++;
}

int tfp_vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
  struct _vsnprintf_putcf_data data;

  if (size < 1)
    return 0;

  data.dest = str;
  data.dest_capacity = size-1;
  data.num_chars = 0;
  tfp_format(&data, _vsnprintf_putcf, format, ap);

  if (data.num_chars < data.dest_capacity)
    data.dest[data.num_chars] = '\0';
  else
    data.dest[data.dest_capacity] = '\0';

  return data.num_chars;
}

int tfp_snprintf(char *str, size_t size, const char *format, ...)
{
  va_list ap;
  int retval;

  va_start(ap, format);
  retval = tfp_vsnprintf(str, size, format, ap);
  va_end(ap);
  return retval;
}

struct _vsprintf_putcf_data
{
  char *dest;
  size_t num_chars;
};

static void _vsprintf_putcf(void *p, char c)
{
  struct _vsprintf_putcf_data *data = (struct _vsprintf_putcf_data*)p;
  data->dest[data->num_chars++] = c;
}

int tfp_vsprintf(char *str, const char *format, va_list ap)
{
  struct _vsprintf_putcf_data data;
  data.dest = str;
  data.num_chars = 0;
  tfp_format(&data, _vsprintf_putcf, format, ap);
  data.dest[data.num_chars] = '\0';
  return data.num_chars;
}

int tfp_sprintf(char *str, const char *format, ...)
{
  va_list ap;
  int retval;

  va_start(ap, format);
  retval = tfp_vsprintf(str, format, ap);
  va_end(ap);
  return retval;
}
#endif
