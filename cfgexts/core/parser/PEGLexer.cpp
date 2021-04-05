#include "PEGLexer.h"
/*
int PEGLexer::declareCompToken(const vector<pair<bool, string>> &toks, int num)
{
    Assert(toks.size()>1);
    CompositeToken ct;
    for (auto &s : toks) {
        if (!s.first) {
            if (const int *n = cterms(s.second.c_str()))
                ct.t.emplace_back(false, *n);
            else throw Exception("Token `" + s.second + "` (part of composite token) not declared");
        } else {
            if (_ten.has(s.second))
                ct.t.emplace_back(true, _ten[s.second]);
            else throw Exception("Token `" + s.second + "` (part of composite token) not declared");
        }
    }
    string nm=toks[0].second;
    for (int i = 1; i < (int)toks.size(); i++)
        (nm += " + ") += toks[i].second;
    int t = _ten[nm];
    int a = peg._en.num(nm);
    if (t >= (int)tokens.size())
        tokens.resize(t + 1, make_pair(-1, -1));
    tokens[t] = make_pair(a, num);
    if ((int)compTokens.size() <= t)
        compTokens.resize(t + 1);
    compTokens[t] = move(ct);
    composite.add(t);
    return _intnum[num] = t;
}*/

void LexIterator::readToken_p(const NTSet *t)
{
    curr_t.clear();
    bool spec = t && t->intersects(lex->special);
    if (spec && readSpecToken(*t))
        return;
    was_eol = false;

    if (!s[pos]) {
        if(spec && lex->eof.allow(*t))
            outputToken(lex->eof.num, Location{cpos}, emptystr(pos), Token::Special);
        else _at_end = true;
        return;
    }

    int p0 = pos, bpos = pos;
    const int* n = lex->cterms(s, p0);
    int m = 0, end = 0;
    int imax = -1;
    int best = -1, b1 = -1;
    for (int ni : t ? *t & lex->simple : lex->simple) {
        if (lex->special.has(ni))continue;
        //lex->_counter[ni]++;
        if (!packrat.parse(lex->tokens[ni].first, pos, end, nullptr)) continue;
        outputToken(ni, Location{cpos}, substr(bpos, end - bpos), Token::NonConst);
        if (end > imax) {
            best = ni;
            m = 1;
            imax = end;
        } else if (end == imax) {
            m++;
            b1 = ni;
        }
    }
    if (curr_t.size() > 1)
        sort(curr_t.begin(), curr_t.end(), [](const Token& x, const Token& y) {return x.text.len > y.text.len; });
    if (n){
        Token tok{ *n,{ cpos,shifted(p0) }, substr(bpos, p0 - bpos), Token::Const };
        if (p0 >= imax) {
            curr_t.insert(curr_t.begin(), tok);
        } else {
            curr_t.push_back(tok);
            sort(curr_t.begin(), curr_t.end(), [](const Token& x, const Token& y) {return x.text.len > y.text.len; });
        }
    }
    if(!n || p0 < imax) {
        if (best < 0) {
            throw SyntaxError(genErrorMsg(t, bpos));
        } else if (t && m > 1) {
            throw SyntaxError("Lexer conflict at {}: `{}` may be 2 different tokens: {} or {}"_fmt(
                    cpos, substr(bpos, imax - bpos), lex->_ten[best], lex->_ten[b1]));
        }
    }
    rdws = true;
}

void LexIterator::readToken(const NTSet &t)
{
    Assert(_accepted);
    curr_t.clear();
    /*if (t.intersects(lex->composite)) {
        auto st = state();
        Token res, rr;
        int bpos = -1, imax = -1, i1=-1, m = 0;
        for (int i : t & lex->composite) {
            if (readComposite(i, res)) {
                int c = imax < 0 ? 2 : lex->cmpcomp(i, imax);
                if (c == 2 || (c>=0 && pos > bpos-c)) {
                    imax = i;
                    bpos = pos;
                    m = 1;
                    rr = Token(lex->tokens[i].second, Location{ st.cpos,cpos }, substr(st.pos,pos - st.pos), Token::Composite);
                } else if (c == 0 && pos == bpos) {
                    m++;
                    i1 = i;
                }
            }
            undo_all();
            restoreState(st);
        }
        if (imax >= 0) {
            if (m > 1) {
                throw SyntaxError("Lexer conflict at {}: `{}` may be 2 different tokens: {} or {}"_fmt(
                        cpos, substr(bpos, imax - bpos), lex->_ten[imax], lex->_ten[i1]));
            }
            curr_t.push_back(rr); // Составной токен (если прочитан) имеет приоритет перед обычным. Поэтому в случае успеха сразу возвращаем результат
            _accepted = false;
            return;
        }
    }*/
    if (try_first && tryFirstAction(t)) return;
    readWs();

    readToken_p(&t);
    _accepted = false;
}

bool LexIterator::tryFirstAction(const NTSet &t)
{
    int trypos = pos;
    // Substr ss;
    readWs();
    int y = try_first(lex, s, trypos);
    if (y < 0) return false;
    // if(!t.has(y))
    //    throw SyntaxError("Unexpected token `{}`"_fmt(substr(pos, trypos-pos)));
    if(y < len(lex->ctokens) && !lex->ctokens[y].empty()) {
        rdws = true;
        readWs();
        curr_t.emplace_back( y,Location{ cpos,shifted(trypos - pos) }, substr(pos, trypos - pos), Token::Const);
    } else if(!t.has(y) || lex->special.has(y)) {
        auto opos = cpos;
        shift(trypos - pos);
        outputToken(y, Location{opos, cpos}, emptystr(pos), Token::Special);
    } else {
        throw GrammarError("Only special and constant tokens supported in try_first function");
    }
    _accepted = false;
    rdws = true;
    return true;
}

string LexIterator::genErrorMsg(const NTSet *t, int bpos)
{
    Pos ccpos = shifted(packrat.errpos);
    string msg = "Unknown token at {} : '{}',\n"_fmt(cpos, substr(bpos, strcspn(s + bpos, "\n")));
    if (t) {
        //_accepted = true;
        packrat.reseterr();
        readToken_p(nullptr);
        if (!curr_t.empty()) {
            //_accepted = false;
            msg += "it may be ";
            bool fst = true;
            for (auto &w : curr_t) {
                if (!fst) msg += ", "; fst = false;
                msg += lex->_ten[lex->internalNum(w.type)];
            }
            msg += " but expected one of: "; fst = true;
            for (int i : *t) {
                if (!fst) msg += ", "; fst = false;
                msg += lex->_ten[i];
            }
            return msg;
        }
    }
    return msg += "PEG error at {} expected one of: {}"_fmt(ccpos, packrat.err_variants());
}

void LexIterator::acceptToken(Token &tok)
{
    if (_accepted) {
        Assert(curr_t[0].type==tok.type && curr_t[0].text.b == tok.text.b && curr_t[0].text.len == tok.text.len);
        return;
    }
    Assert(s + pos <= tok.text.b);

    /*if (tok.nonconst == Token::Composite) {
        Token r, rr;
        for (auto[nc, tk] : lex->compTokens[lex->internalNum(tok.type)].t) {
            readWs();
            if (nc) {
                Assert(tryNCToken(tk, &r));
            } else {
                Assert(lex->cterms(s,pos));
                rdws = true;
            }
        }
        Assert(tok.text.b + tok.text.len == s + pos);
    } else*/ {
        shift(int(tok.text.b - (s + pos)));
        tok.loc.beg = cpos;
        if (tok.type == lex->eof.num)_at_end = true;
        shift(tok.text.len);
    }
    tok.loc.end = cpos;
    curr_t.resize(1); curr_t[0] = tok;
    _accepted = true;
}

bool LexIterator::readSpecToken(const NTSet &t) { // Чтение специальных токенов
    if (_at_start && lex->sof.num >= 0 && lex->sof.allow(t)) { // начало файла
        outputToken(lex->sof.num, Location{cpos}, emptystr(pos), Token::Special);
        _at_start = false;
        return true;
    }
    if (cpos.line > cprev.line + nlines && lex->eol.allow(t)) { // Переход на новую строку (считается, если после чтения пробелов/комментариев привело к увеличению номера строки
        outputToken(lex->eol.num, Location{cpos}, emptystr(pos), Token::Special);
        nlines++;
        was_eol = true;
        return true;
    }
    // preindent: Если увеличение отступа допустимо, то оно читается вне зависимости от того, что дальше
    if (s[pos] && lex->preindent.allow(t)) {
        // Запрещено читать 2 раза подряд увеличение отступа с одной и той же позиции
        // иначе может произойти зацикливание, если неправильная грамматика, например, есть правило A -> indent A ...
        if (indents.back().col != cprev.col || indents.back().line != cprev.line) {
            indents.push_back(IndentSt{false, cprev.line, cprev.col, indents.back().col + 1 });
            outputToken(lex->preindent.num, Location{cprev}, emptystr(pos), Token::Special);
            return true;
        }
    }
    // indent: Если произошло увеличение отступа, и оно зарегистрировано как токен, то оно читается
    if (was_eol && cpos.col > indents.back().col && s[pos] && lex->indent.allow(t)) {
        // Запрещено читать 2 раза подряд увеличение отступа с одной и той же позиции
        // иначе может произойти зацикливание, если неправильная грамматика, например, есть правило A -> indent A ...
        indents.push_back(IndentSt{true, cpos.line, cpos.col, cpos.col });
        outputToken(lex->indent.num, Location{{cpos.line,1}, cpos}, emptystr(pos), Token::Special);
        return true;
    }
    if (cpos.col < indents.back().col && lex->dedent.allow(t)) { // Уменьшение отступа
        indents.pop_back();
        if (!indents.back().fix) {
            indents.back().fix = true;
            indents.back().col = max(indents.back().col, cpos.col);
        }
        outputToken(lex->dedent.num, Location{cpos}, emptystr(pos), Token::Special);
        return true;
    }
    if (s[pos] && lex->check_indent.allow(t)) { // Проверка отступа
        auto &b       = indents.back();
        bool indented = false;
        if (cpos.col >= b.col) {
            if (cpos.line > b.line) {
                if (!b.fix) {
                    b.fix = true;
                    b.col = cpos.col;
                }
                if (b.col == cpos.col) {
                    indented = true;
                }
            } else {
                indented = (b.start_col == cprev.col);  // Текст начался в той же строке, что и блок с отступами
            }
            if (indented) {
                outputToken(lex->check_indent.num, Location{cpos}, emptystr(pos), Token::Special);
                return true;
            }
        }
    }
    was_eol = false;
    return false;
}
