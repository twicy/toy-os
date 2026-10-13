#include <stdint.h>
#include <stdio.h>

/*
 * Supported: %d %i %u %o %x %X %c %s %p %%
 * flags - + space # 0, width and precision (both may be *),
 * length modifiers hh h l z. No floating point, no 64-bit (ll) values.
 */

/* Flags, width and precision of one conversion */
struct spec {
    int left;   /* '-' */
    int plus;   /* '+' */
    int space;  /* ' ' */
    int alt;    /* '#' */
    int zero;   /* '0' */
    int width;
    int prec;   /* -1 when not given */
};

enum length { LEN_HH, LEN_H, LEN_INT, LEN_L, LEN_Z };

struct out {
    putc_fn putc;
    void *arg;
    int len;
};

static void emit(struct out *o, char c) {
    o->putc(c, o->arg);
    o->len++;
}

static void emit_repeat(struct out *o, char c, int n) {
    while (n-- > 0) {
        emit(o, c);
    }
}

/* Write n chars of str padded with spaces to the field width */
static void format_chars(struct out *o, const struct spec *s, const char *str, int n) {
    int pad = s->width - n;

    if (!s->left) {
        emit_repeat(o, ' ', pad);
    }
    for (int i = 0; i < n; i++) {
        emit(o, str[i]);
    }
    if (s->left) {
        emit_repeat(o, ' ', pad);
    }
}

static void format_str(struct out *o, const struct spec *s, const char *str) {
    int n = 0;

    if (str == NULL) {
        str = "(null)";
    }
    // with a precision, str need not be NUL-terminated, so never read past it
    while ((s->prec < 0 || n < s->prec) && str[n] != '\0') {
        n++;
    }
    format_chars(o, s, str, n);
}

/* conv is one of d i u o x X p, value is the magnitude for d and i */
static void format_int(struct out *o, const struct spec *s, char conv,
                       uint32_t value, int negative) {
    const char *digits = conv == 'X' ? "0123456789ABCDEF" : "0123456789abcdef";
    uint32_t base = 10;
    const char *prefix = "";
    int prefix_len;
    char buf[11];   // 32-bit octal has 11 digits
    int n = 0;
    int nonzero = value != 0;
    int zeros;
    int pad;

    if (conv == 'o') {
        base = 8;
    } else if (conv == 'x' || conv == 'X' || conv == 'p') {
        base = 16;
    }

    // digits in reverse order, a zero value with precision 0 prints none
    while (value != 0 || (n == 0 && s->prec != 0)) {
        buf[n++] = digits[value % base];
        value /= base;
    }

    // precision is the minimum number of digits
    zeros = s->prec > n ? s->prec - n : 0;

    if (conv == 'd' || conv == 'i') {
        if (negative) {
            prefix = "-";
        } else if (s->plus) {
            prefix = "+";
        } else if (s->space) {
            prefix = " ";
        }
    } else if (conv == 'p') {
        prefix = "0x";
    } else if (s->alt && nonzero && conv == 'x') {
        prefix = "0x";
    } else if (s->alt && nonzero && conv == 'X') {
        prefix = "0X";
    } else if (s->alt && conv == 'o' && zeros == 0 && (n == 0 || buf[n - 1] != '0')) {
        // '#' with octal makes the first digit a 0
        zeros = 1;
    }

    prefix_len = 0;
    while (prefix[prefix_len] != '\0') {
        prefix_len++;
    }

    pad = s->width - prefix_len - zeros - n;
    // '0' pads after the sign or 0x, but '-' or a precision disable it
    if (s->zero && !s->left && s->prec < 0 && pad > 0) {
        zeros += pad;
        pad = 0;
    }

    if (!s->left) {
        emit_repeat(o, ' ', pad);
    }
    for (int i = 0; i < prefix_len; i++) {
        emit(o, prefix[i]);
    }
    emit_repeat(o, '0', zeros);
    while (n > 0) {
        emit(o, buf[--n]);
    }
    if (s->left) {
        emit_repeat(o, ' ', pad);
    }
}

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

int vformat(putc_fn putc, void *arg, const char *format, va_list ap) {
    struct out o = { putc, arg, 0 };
    const char *p = format;

    while (*p != '\0') {
        if (*p != '%') {
            emit(&o, *p++);
            continue;
        }

        const char *start = p++;
        struct spec s = { 0, 0, 0, 0, 0, 0, -1 };
        enum length len = LEN_INT;

        for (;; p++) {
            if (*p == '-') {
                s.left = 1;
            } else if (*p == '+') {
                s.plus = 1;
            } else if (*p == ' ') {
                s.space = 1;
            } else if (*p == '#') {
                s.alt = 1;
            } else if (*p == '0') {
                s.zero = 1;
            } else {
                break;
            }
        }

        if (*p == '*') {
            p++;
            s.width = va_arg(ap, int);
            // a negative * width means '-' with the absolute value
            if (s.width < 0) {
                s.left = 1;
                s.width = -s.width;
            }
        } else {
            while (is_digit(*p)) {
                s.width = s.width * 10 + (*p++ - '0');
            }
        }

        if (*p == '.') {
            p++;
            s.prec = 0;
            if (*p == '*') {
                p++;
                s.prec = va_arg(ap, int);
                // a negative * precision counts as not given
                if (s.prec < 0) {
                    s.prec = -1;
                }
            } else {
                while (is_digit(*p)) {
                    s.prec = s.prec * 10 + (*p++ - '0');
                }
            }
        }

        if (*p == 'h') {
            p++;
            len = LEN_H;
            if (*p == 'h') {
                p++;
                len = LEN_HH;
            }
        } else if (*p == 'l') {
            p++;
            len = LEN_L;
        } else if (*p == 'z') {
            p++;
            len = LEN_Z;
        }

        switch (*p) {
        case 'd':
        case 'i': {
            int32_t v;
            // varargs promote char and short to int, so read an int and narrow it
            switch (len) {
            case LEN_HH: v = (signed char)va_arg(ap, int); break;
            case LEN_H:  v = (short)va_arg(ap, int); break;
            case LEN_L:  v = va_arg(ap, long); break;
            case LEN_Z:  v = va_arg(ap, ptrdiff_t); break;
            default:     v = va_arg(ap, int); break;
            }
            // negate as unsigned so the most negative value does not overflow
            format_int(&o, &s, *p, v < 0 ? -(uint32_t)v : (uint32_t)v, v < 0);
            break;
        }
        case 'u':
        case 'o':
        case 'x':
        case 'X': {
            uint32_t v;
            switch (len) {
            case LEN_HH: v = (unsigned char)va_arg(ap, unsigned int); break;
            case LEN_H:  v = (unsigned short)va_arg(ap, unsigned int); break;
            case LEN_L:  v = va_arg(ap, unsigned long); break;
            case LEN_Z:  v = va_arg(ap, size_t); break;
            default:     v = va_arg(ap, unsigned int); break;
            }
            format_int(&o, &s, *p, v, 0);
            break;
        }
        case 'p':
            // always 0x plus every hex digit of an address, e.g. 0xc0001500
            if (s.prec < 0) {
                s.prec = 2 * sizeof(void *);
            }
            format_int(&o, &s, 'p', (uintptr_t)va_arg(ap, void *), 0);
            break;
        case 'c': {
            char c = (char)va_arg(ap, int);
            format_chars(&o, &s, &c, 1);
            break;
        }
        case 's':
            format_str(&o, &s, va_arg(ap, const char *));
            break;
        case '%':
            emit(&o, '%');
            break;
        default:
            // unknown conversion, print the specifier as written
            while (start < p) {
                emit(&o, *start++);
            }
            // format ended inside the specifier, let the loop condition stop
            if (*p == '\0') {
                continue;
            }
            emit(&o, *p);
            break;
        }
        p++;
    }

    return o.len;
}

struct str_out {
    char *p;
    size_t room;    // chars left before the NUL terminator
};

static void str_putc(char c, void *arg) {
    struct str_out *so = arg;

    if (so->room > 0) {
        *so->p++ = c;
        so->room--;
    }
}

int vsnprintf(char *buf, size_t size, const char *format, va_list ap) {
    struct str_out so = { buf, size > 0 ? size - 1 : 0 };
    int len = vformat(str_putc, &so, format, ap);

    if (size > 0) {
        *so.p = '\0';
    }
    return len;
}

int snprintf(char *buf, size_t size, const char *format, ...) {
    va_list ap;
    int len;

    va_start(ap, format);
    len = vsnprintf(buf, size, format, ap);
    va_end(ap);
    return len;
}
