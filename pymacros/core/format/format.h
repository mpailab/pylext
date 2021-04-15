#pragma once
#include <string>
#include <utility>
#include <complex>
#include <exception>
#include <type_traits>
#include <sstream>
#include <array>

/** Временная замена стандартной функции std::format,
 * которая на момент начала 2021 года не е реализована в компиляторах.
 *
 * Это не полный эквивалент std::format, но реализует основные возможности.
 */

class FormattedPrint;
FormattedPrint operator ""_fmt(const char *s, size_t n);

class FormattedPrintError : public std::exception {
    std::string _msg;
public:
    explicit FormattedPrintError(std::string msg): _msg(std::move(msg)){}
    const char *what()const noexcept override {
        return _msg.c_str();
    }
};
namespace format_internal_ {
    template<bool is_int>
    struct PrintNum {
        template<class I>
        static void print_formatted_integer(std::string &buf, I i, int width, char fmt, char sign, char sep, bool alt)
        {
            if (fmt == 'c') {
                buf += char(i);
                return;
            }
            constexpr int buflen = (int)sizeof(i) * 16 + 1;
            std::array<char, buflen> bb;
            int pos = buflen - 1;
            bb[pos] = 0;
            bool sgn = i < 0;
            if (i < 0)
                i = I(0) - i;
            int base = 10;
            int lb   = (int)buf.size();
            switch (sign) {
                case '+':
                    buf += (sgn ? '-' : '+');
                    break;
                case '-':
                    if (sgn)
                        buf += '-';
                    break;
                default:
                    buf += (sgn ? '-' : ' ');
                    break;
            }
            const char *digits = "0123456789abcdef";
            int        gg      = 3;
            switch (fmt) {
                case 0:
                case 'd':
                    base = 10;
                    break;
                case 'b':
                    base = 2;
                    gg   = 4;
                    if (alt)
                        buf += "0b";
                    break;
                case 'X':
                    digits = "0123456789ABCDEF";
                case 'x':
                    base   = 16;
                    gg     = 4;
                    if (alt)
                        buf += "0x";
                    break;
                case 'o':
                    base = 8;
                    gg   = 4;
                    if (alt)
                        buf += "0o";
                    break;
                default:
                    throw FormattedPrintError(std::string("invalid integer format ") + fmt);
            }
            int g = 0;
            do {
                if (sep && g == gg) {
                    bb[--pos] = sep;
                    g = 0;
                }
                g++;
                bb[--pos] = digits[int(i % base)];
                i /= base;
            } while (i);

            if (width > (int)buf.size() - lb + buflen - 1 - pos)
                buf.append(width + lb - ((int)buf.size() + buflen - 1 - pos), '0');
            buf += &bb[pos];
        }

        template<class X>
        static void
        print_formatted_float(std::string &buf, X x, int width, int precision, char fmt, char sign, char sep, bool alt)
        {
            buf += std::to_string(x);
        }
    };

    template<>
    struct PrintNum<false> {
        template<class I>
        static void print_formatted_integer(std::string &buf, I i, int width, char fmt, char sign, char sep, bool alt)
        {
            throw FormattedPrintError("Not an integer type");
        }

        template<class X>
        static void
        print_formatted_float(std::string &buf, X x, int width, int precision, char fmt, char sign, char sep, bool alt)
        {
            throw FormattedPrintError("Not a floating-point type");
        }
    };
}
template<class I>
void print_formatted_int(std::string& buf, I i, int width, char fmt, char sign, char sep, bool alt) {
    if(sizeof(I)==1&&!fmt)
        fmt='c';
    format_internal_::PrintNum<std::is_integral<I>::value>::print_formatted_integer(buf, i, width, fmt, sign, sep, alt);
}

template<class X>
void print_formatted_float(std::string& buf, X x, int width, int precision, char fmt, char sign, char sep, bool alt) {
    constexpr bool is_fp = std::is_same<X,float>::value||std::is_same<X,double>::value||std::is_same<X,long double>::value;
    format_internal_::PrintNum<is_fp>::print_formatted_float(buf, x, width, precision, fmt, sign, sep, alt);
}

template<class X>
void print_formatted_float(std::string& buf, std::complex<X> x, int width, int precision, char fmt, char sign, char sep, bool alt) {
    print_formatted_float(buf, x.real(), 0, precision, fmt, sign, sep, alt);
    buf+='+';
    print_formatted_float(buf, x.imag(), 0, precision, fmt, sign, sep, alt);
    buf+='i';
}

template<class T>
void print_formatted(std::string& buf, const T& x, char fmt) {
    if(fmt)throw FormattedPrintError(std::string("Special format option ") + fmt + " not implemented for type " + std::string(typeid(T).name()));
    std::stringstream ss;
    ss << x;
    buf = ss.str();
}

inline void print_formatted(std::string& buf, const std::string& s, char fmt) {
    if (fmt && fmt!='s')
        throw FormattedPrintError(std::string("invalid string format ") + fmt);
    buf += s;
}

inline void print_formatted(std::string& buf, const char* s, char fmt) {
    if (fmt && fmt!='s')
        throw FormattedPrintError(std::string("invalid string format ") + fmt);
    buf += s;
}

template<class T>
void custom_format(std::string& buf, const T& s, const char *fmt, int fmt_len){
    throw FormattedPrintError(std::string("Custom formatting not implemented for type ") + typeid(T).name());
}

template<class T>
constexpr bool is_integer_type(const T*) {
    return std::is_integral<T>::value;
}

template<class T>
constexpr bool is_fp_type(const T*){
    return std::is_same<T,float>::value || std::is_same<T,double>::value || std::is_same<T,long double>::value;
}

template<class T>
constexpr bool is_fp_type(const std::complex<T> *){ return is_fp_type((T*)0); }


class FormattedPrint {
    std::string format;
public:
    explicit FormattedPrint(std::string fmt): format(std::move(fmt)){}
    explicit FormattedPrint(const char *ptr, size_t n): format(ptr, n){}

    template<class ... Args>
    std::string operator()(const Args& ... args){
        std::string res, buf;
        _print(buf, res, 0, 0, args...);
        return res;
    }
private:
    template<class Arg1, class ... Args>
    void _print(std::string& buf, std::string &res, int pos, int arg_pos, const Arg1& arg1, const Args& ... args)const {
        constexpr int default_precision = 6;
        for (; pos < format.size(); pos++){
            if (format[pos] == '{') {
                if (format[pos + 1] != '{') {
                    int width = 0, precision=default_precision;
                    char fill = ' ', fmt0 = 0, sign = '-', align='>', sep = 0;
                    bool alt = false, custom = false;
                    const char *fmt = &format[pos + 1];
                    int fmt_len = 0;
                    for (; format[pos + fmt_len + 1] != '}'; fmt_len++) {
                        if (!format[pos + fmt_len + 1])
                            throw FormattedPrintError("'}' expected in format string");
                    }
                    constexpr bool is_int = is_integer_type((Arg1*)0);
                    constexpr bool is_fp = is_fp_type((Arg1*)0);
                    constexpr bool is_num = is_int || is_fp;
                    _parse_fmt(fmt, fmt_len, &align, &width, is_fp ? &precision : nullptr,
                               is_num ? &sign : nullptr, is_num ? &alt : nullptr, &fill, is_num ? &sep : nullptr,
                               &fmt0, &custom);
                    buf.clear();
                    if(custom){
                        custom_format(buf, arg1, fmt, fmt_len);
                    } else if(is_int) print_formatted_int(buf, arg1, align=='='?width : 0, fmt0, sign, sep, alt);
                    else if(is_fp) print_formatted_float(buf, arg1, align=='='?width : 0, precision, fmt0, sign, sep, alt);
                    else print_formatted(buf, arg1, fmt0);
                    _add_formatted(res, buf, align, width, fill);
                    return _print(buf, res, pos + fmt_len + 2, arg_pos + 1, args...);
                } else res += '{', pos++;
            } else if (format[pos] == '}') {
                if (format[pos + 1] != '}')
                    throw FormattedPrintError("'}}' expected, '}' found");
                else res += '}', pos++;
            } else res += format[pos];
        }
        throw FormattedPrintError("Wrong number of formatted string arguments: " + std::to_string(1 + sizeof...(args)) + " redundant arguments");
    }
    void _print(std::string&, std::string & res, size_t pos, int arg_pos) const{
        for(; pos<format.size(); pos++)
            if(format[pos]=='{') {
                if (format[pos + 1] != '{')
                    throw FormattedPrintError("low format arguments (" + std::to_string(arg_pos) + ") for format string `" + format + "`");
                else res += '{', pos++;
            } else if(format[pos]=='}') {
                if (format[pos + 1] != '}')
                    throw FormattedPrintError("'}}' expected, '}' found");
                else res += '}', pos++;
            } else res += format[pos];
    }

    void  _parse_fmt(const char *&fmt, int &fmt_len, char *align, int *width, int *precision, char *sign,
                     bool *alt_format, char *fill, char *digit_sep, char *format, bool *custom) const;

    static void _add_formatted(std::string &res, const std::string &buf, char align, int width, char fill);
};

template<class ... Args>
std::string sformat(const char *s, const Args& ... args) {
    return FormattedPrint(s)(args...);
}

template<class ... Args>
std::string sformat(const std::string& s, const Args& ... args) {
    return FormattedPrint(s)(args...);
}
