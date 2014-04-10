
    int ret = 0;
    
    double a_dbl = 0.0; long double a_ldbl = 0.0L;
    double b_dbl = 0.0; long double b_ldbl = 0.0L;
    void *a_ptr;
    int *a_iptr;
    bool is_replaced;

    int chars;
    int flag_parse;
    char flag[256], *fi;
    //int width;
    char size;

    va_list args;
    va_start(args, fmt);
    while (*fmt) {
        flag_parse = 1; flag[0] = '%'; fi = flag; fi++;
        //width = 0;
        size = '\0';

        // ignore non-format characters
        if ((*fmt != '%') || (*++fmt == '%')) {
            chars = PRINT_ORIG(PRINT_OUT, "%c", *fmt++);
#ifdef PRINT_INC
            PRINT_INC += chars;
#endif
            ret += chars;
            //fputc(*fmt++, fd); ret++;
            continue;
        }

        // parse flag (save and ignore)
        while (*fmt && flag_parse) {
            switch (*fmt) {
                case '0':
                case '#':
                case '-':
                case '+':
                case ' ':
                    *fi++ = *fmt++; break;
                default:
                    flag_parse = 0; break;
            }
        }
        // width
        if (*fmt == '*') {
            // IGNORE ADDITIONAL WIDTH PARAMETERS FOR NOW
            //*fi++ = *fmt++;
            //width = va_arg(args, int);
            fmt++;
        } else {
            while (*fmt && isdigit(*fmt)) {
                *fi++ = *fmt++;
            }
        }
        // precision
        if (*fmt && *fmt == '.') {
            *fi++ = *fmt++;
            if (*fmt == '*') {
                // IGNORE ADDITIONAL WIDTH PARAMETERS FOR NOW
                //*fi++ = *fmt++;
                //width = va_arg(args, int);
                fmt++;
            } else {
                while (*fmt && isdigit(*fmt)) {
                    *fi++ = *fmt++;
                }
            }
        }
        // length
        if (*fmt && ((*fmt == 'h') || (*fmt == 'l') || (*fmt == 'L'))) {
            size = *fmt;
            *fi++ = *fmt++;
        }
        if (*fmt && (*fmt == 'l')) {    // handle "ll"
            size = 'L';
            *fi++ = *fmt++;
        }
        *fi++ = *fmt;
        *fi = '\0';

#if 0
        fputc('|', stdout); ret++;
        fputc(*fmt, stdout); ret++;
        fputc('|', stdout); ret++;
#ifdef PRINT_INC
        PRINT_INC += 3;
#endif
        fi = flag;
        while (*fi) {
            fputc(*fi++, stdout); ret++;
#ifdef PRINT_INC
            PRINT_INC += 1;
#endif
        }
        fputc('|', stdout); ret++;
        fputc(size, stdout); ret++;
        fputc('|', stdout); ret++;
#ifdef PRINT_INC
        PRINT_INC += 3;
#endif
#endif

        // delegate to original printf for actual printing
        switch (*fmt) {
            case 'c':
                //a_char = (char)va_arg(args, int);
                //chars = PRINT_ORIG(PRINT_OUT, flag, a_char);
                chars = PRINT_VORIG(PRINT_OUT, flag, args);
#ifdef PRINT_INC
                PRINT_INC += chars;
#endif
                ret += chars;
                break;
            case 'e': case 'E':
            case 'f': case 'F':
            case 'g': case 'G':

                is_replaced = false;
                if (size == 'L') {

                    a_ldbl = va_arg(args, long double);

                    if (FPFLAG(a_ldbl)) {
                        b_ldbl = FPCONV(a_ldbl);
                        is_replaced = true;
                    }

                    if (is_replaced) {
                        /*chars = PRINT_ORIG(PRINT_OUT, "^^");*/
#ifdef PRINT_INC
                        /*PRINT_INC += chars;*/
#endif
                        /*ret += chars;*/
                        chars = PRINT_ORIG(PRINT_OUT, flag, b_ldbl);
#ifdef PRINT_INC
                        PRINT_INC += chars;
#endif
                        ret += chars;
                    } else {
                        chars = PRINT_ORIG(PRINT_OUT, flag, a_ldbl);
#ifdef PRINT_INC
                        PRINT_INC += chars;
#endif
                        ret += chars;
                    }

                } else {

                    a_dbl = va_arg(args, double);

                    if (FPFLAG(a_dbl)) {
                        b_dbl = FPCONV(a_dbl);
                        is_replaced = true;
                    }

                    if (is_replaced) {
                        /*chars = PRINT_ORIG(PRINT_OUT, "^");*/
#ifdef PRINT_INC
                        /*PRINT_INC += chars;*/
#endif
                        /*ret += chars;*/
                        chars = PRINT_ORIG(PRINT_OUT, flag, b_dbl);
#ifdef PRINT_INC
                        PRINT_INC += chars;
#endif
                        ret += chars;
                    } else {
                        chars = PRINT_ORIG(PRINT_OUT, flag, a_dbl);
#ifdef PRINT_INC
                        PRINT_INC += chars;
#endif
                        ret += chars;
                    }

                }
                break;

            case 'd': case 'i': case 'o':
                switch(size) {
                    case 'h': //a_sint = (short int)va_arg(args, int);
                              //chars = PRINT_ORIG(PRINT_OUT, flag, a_sint); break;
                              chars = PRINT_VORIG(PRINT_OUT, flag, args); break;
                    case 'l': //a_lint = va_arg(args, long int);
                              //chars = PRINT_ORIG(PRINT_OUT, flag, a_lint); break;
                              chars = PRINT_VORIG(PRINT_OUT, flag, args); break;
                    default:  //a_int = va_arg(args, int);
                              //chars = PRINT_ORIG(PRINT_OUT, flag, a_int); break;
                              chars = PRINT_VORIG(PRINT_OUT, flag, args); break;
                }
#ifdef PRINT_INC
                PRINT_INC += chars;
#endif
                ret += chars;
                break;
            case 'u': case 'x': case 'X':
                switch(size) {
                    case 'h': //a_suint = (unsigned short int)va_arg(args, unsigned int);
                              //chars = PRINT_ORIG(PRINT_OUT, flag, a_suint); break;
                              chars = PRINT_VORIG(PRINT_OUT, flag, args); break;
                    case 'l': //a_luint = va_arg(args, unsigned long int);
                              //chars = PRINT_ORIG(PRINT_OUT, flag, a_luint); break;
                              chars = PRINT_VORIG(PRINT_OUT, flag, args); break;
                    default:  //a_uint = va_arg(args, unsigned int);
                              //chars = PRINT_ORIG(PRINT_OUT, flag, a_uint); break;
                              chars = PRINT_VORIG(PRINT_OUT, flag, args); break;
                }
#ifdef PRINT_INC
                PRINT_INC += chars;
#endif
                ret += chars;
                break;
            case 's':
                //a_str = va_arg(args, char*);
                //chars = PRINT_ORIG(PRINT_OUT, flag, a_str);
                chars = PRINT_VORIG(PRINT_OUT, flag, args);
#ifdef PRINT_INC
                PRINT_INC += chars;
#endif
                ret += chars;
                break;
            case 'p':
                //a_ptr = va_arg(args, void*);
                //chars = PRINT_ORIG(PRINT_OUT, flag, a_ptr);
                chars = PRINT_VORIG(PRINT_OUT, flag, args);
#ifdef PRINT_INC
                PRINT_INC += chars;
#endif
                ret += chars;
                break;
            case 'n':
                a_iptr = va_arg(args, int*);
                *a_iptr = ret;
                break;
            default:
                chars = PRINT_ORIG(PRINT_OUT, "?%c%s?", *fmt, flag);
#ifdef PRINT_INC
                PRINT_INC += chars;
#endif
                ret += chars;
                /*
                 *fputc('?', fd); ret++;
                 *fputc(*fmt, fd); ret++;
                 *fi = flag;
                 *while (*fi) {
                 *  fputc(*fi++, fd); ret++;
                 *}
                 *fputc('?', fd); ret++;
                 */
                break;
        }
        fmt++;
    }

    va_end(args);

