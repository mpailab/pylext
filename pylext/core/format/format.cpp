#include <cstring>
#include "format.h"
using namespace std;

void FormattedPrint::_parse_fmt(const char *&fmt, int &fmt_len, char *align, int *width, int *precision,
                                char*sign, bool* alt_format, char *fill, char* digit_sep, char *format, bool *custom) const {
    if(fmt_len<=0)return;
    int align_pos = strchr("<>=^", fmt[1]) ? 1 : strchr("<>=^", fmt[0]) ? 0 : -1;
    if(align_pos>=0)
        *align = fmt[align_pos];
    if (align_pos==1)
        *fill = fmt[0];
    fmt+=align_pos+1;
    fmt_len -=align_pos+1;
    if(fmt[0] == '!'){
        *custom = true;
        fmt++, fmt_len--;
    }
    if(strchr("+- ",fmt[0])) {
        if(!sign)
            throw FormattedPrintError("unexpected sign specification for non-numeric format: " + string(fmt, fmt_len));
        *sign = *fmt;
        fmt++, fmt_len--;
        if(!fmt_len)return;
    }
    if(fmt[0] == '#'){
        if(!alt_format)
            throw FormattedPrintError("unexpected '#' specification for non-numeric format: " + string(fmt, fmt_len));
        *alt_format = true;
        fmt++, fmt_len--;
        if(!fmt_len)return;
    }
    if(fmt[0] == '0' && alt_format && align_pos <= 0) {
        *fill = '0';
        if(align_pos<0)
            *align = '=';
    }
    *width = 0;
    for(;isdigit(*fmt); fmt++, fmt_len--)
        *width = *width*10+(*fmt-'0');
    if(!fmt_len)return;
    if(strchr(",_ n", fmt[0])){
        if(!digit_sep)
            throw FormattedPrintError("unexpected digit separator for non-numeric format: " + string(fmt, fmt_len));
        if(fmt[0]=='n') *digit_sep = ',';
        else *digit_sep = fmt[0];
        fmt++, fmt_len--;
        if(!fmt_len)return;
    }
    if (fmt[0]=='.') {
        if(!precision)
            throw FormattedPrintError("unexpected precision for non-numeric format: " + string(fmt, fmt_len));
        *precision = 0;
        if(!isdigit(fmt[1]))
            throw FormattedPrintError("error in format string: number expected after '.'");
        for(fmt++, fmt_len--; isdigit(*fmt); fmt++, fmt_len--)
            *precision = *precision*10 + (fmt[0]-'0');
        if(!fmt_len)
            return;
    }

    if (fmt_len>1)
        throw FormattedPrintError("invalid format " + string(fmt, fmt_len));
    *format = fmt[0];
}

void FormattedPrint::_add_formatted(std::string& res, const std::string& buf, char align, int width, char fill) {
    int rst = width-(int)buf.size(), r=0, l=0;
    if(align=='<')r=rst;
    else if(align=='>')l=rst;
    else r=rst/2, l=rst-r;
    if(l>0)res.append(l,fill);
    res+=buf;
    if(r>0)res.append(r,fill);
}

FormattedPrint operator ""_fmt(const char *s, size_t n) {
    return FormattedPrint(s, n);
}
