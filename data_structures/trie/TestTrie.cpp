#include "TestTrie.h"

std::vector<std::string> read_words(const std::string &filename)
{
    std::ifstream f(filename, std::ios::in);
    std::string buf;
    std::vector<std::string> res;
    while(f >> buf)
        res.push_back(buf);
    return res;
}


std::string read_file(const std::string &filename)
{
    std::ifstream f(filename, std::ios::in);
    return std::string(std::istream_iterator<char>(f), std::istream_iterator<char>());
}

/// Пропускает текущий токен и переходит к следующему
void next_tok(const char *text, int &pos)
{
    if (isalnum(text[pos]) || text[pos]=='_') { // Пропускаем число или идентификатор
        while (isalnum(text[pos]) || text[pos]=='_')
            pos++;
    } else pos++; // Если не число и не буква, то скорее всего это знак операции или скобка, пропускаем один символ

    while(isspace(text[pos])) // Пропускаем все пробелы
        pos++;
}
