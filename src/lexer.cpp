#include "lexer.h"
#include <cctype>
#include <unordered_map>
#include <utility>

std::unordered_map<std::string, TokenType> keywords = {
    {"let", TokenType::LET},
    {"const", TokenType::CONST},
    {"fn", TokenType::FN},
    {"return", TokenType::RETURN},
    {"true", TokenType::TRUE},
    {"false", TokenType::FALSE},
    {"and", TokenType::AND},
    {"or", TokenType::OR},
    {"if", TokenType::IF},
    {"else", TokenType::ELSE},
    {"for", TokenType::FOR},
    {"break", TokenType::BREAK},
    {"continue", TokenType::CONTINUE},
    {"as", TokenType::AS},
    {"struct", TokenType::STRUCT},
    {"enum", TokenType::ENUM},
    {"trait", TokenType::TRAIT},
    {"impl", TokenType::IMPL},
    {"pub", TokenType::PUB},
    {"static", TokenType::STATIC},
    {"constructor", TokenType::CONSTRUCTOR}
};

Lexer::Lexer(std::string  source) : source(std::move(source)) {
}

Token Lexer::getNextToken() {
    skipWhitespace();
    
    if (position >= source.length()) {
        return {TokenType::EOF_TOKEN, "", line, column};
    }
    
    char c = currentChar();
    
    if (c == '"') {
        return processString();
    }
    
    if (isalpha(c) || c == '_') {
        return processIdentifier();
    }
    
    if (isdigit(c)) {
        return processNumber();
    }
    
    switch (c) {
        case '+':
            advance();
            return {TokenType::PLUS, "+", line, column - 1};
        case '-':
            advance();
            if (position < source.length() && currentChar() == '>') {
                advance();
                return {TokenType::ARROW, "->", line, column - 2};
            }
            return {TokenType::MINUS, "-", line, column - 1};
        case '*':
            advance();
            return {TokenType::MULTIPLY, "*", line, column - 1};
        case '/':
            advance();
            return {TokenType::DIVIDE, "/", line, column - 1};
        case '%':
            advance();
            return {TokenType::MOD, "%", line, column - 1};
        case '=':
            advance();
            if (position < source.length() && currentChar() == '=') {
                advance();
                return {TokenType::EQ, "==", line, column - 2};
            }
            return {TokenType::ASSIGN, "=", line, column - 1};
        case '!':
            advance();
            if (position < source.length() && currentChar() == '=') {
                advance();
                return {TokenType::NE, "!=", line, column - 2};
            }
            return {TokenType::NOT, "!", line, column - 1};
            break;
        case '<':
            advance();
            if (position < source.length() && currentChar() == '=') {
                advance();
                return {TokenType::LE, "<=", line, column - 2};
            }
            return {TokenType::LT, "<", line, column - 1};
        case '>':
            advance();
            if (position < source.length() && currentChar() == '=') {
                advance();
                return {TokenType::GE, ">=", line, column - 2};
            }
            return {TokenType::GT, ">", line, column - 1};
        case '(':
            advance();
            return {TokenType::LPAREN, "(", line, column - 1};
        case ')':
            advance();
            return {TokenType::RPAREN, ")", line, column - 1};
        case '{':
            advance();
            return {TokenType::LBRACE, "{", line, column - 1};
        case '}':
            advance();
            return {TokenType::RBRACE, "}", line, column - 1};
        case '[':
            advance();
            return {TokenType::LBRACKET, "[", line, column - 1};
        case ']':
            advance();
            return {TokenType::RBRACKET, "]", line, column - 1};
        case ',':
            advance();
            return {TokenType::COMMA, ",", line, column - 1};
        case ';':
            advance();
            return {TokenType::SEMICOLON, ";", line, column - 1};
        case '.':
            advance();
            return {TokenType::DOT, ".", line, column - 1};
        case ':':
            advance();
            if (position < source.length() && currentChar() == ':') {
                advance();
                return {TokenType::COL_COLON, "::", line, column - 1};
            }
            return {TokenType::COLON, ":", line, column - 1};
        case '#': {
            advance();
            return {TokenType::GRID, "#", line, column - 1};
        }
        case '&': {
            advance();
            return {TokenType::REF, "&", line, column - 1};
        }
        default:
            advance();
            return {TokenType::EOF_TOKEN, std::string(1, c), line, column - 1};
    }
    
    return {TokenType::EOF_TOKEN, "", line, column};
}

Token Lexer::peek() const {
    const size_t currentPos = position;
    const size_t currentLine = line;
    const size_t currentColumn = column;
    
    Lexer tempLexer(source);
    tempLexer.position = currentPos;
    tempLexer.line = currentLine;
    tempLexer.column = currentColumn;
    
    return tempLexer.getNextToken();
}

void Lexer::skipWhitespace() {
    while (position < source.length()) {
        char c = currentChar();
        
        // 跳过单行注释
        if (c == '/' && position + 1 < source.length() && source[position + 1] == '/') {
            while (position < source.length() && currentChar() != '\n') {
                advance();
            }
            continue;
        }
        
        if (isspace(c)) {
            if (c == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            advance();
        } else {
            break;
        }
    }
}

Token Lexer::processIdentifier() {
    const size_t start = position;
    const auto startLine = line;
    const auto startColumn = column;

    while (position < source.length() && (isalnum(currentChar()) || currentChar() == '_')) {
        advance();
    }

    std::string value = source.substr(start, position - start);



    if (const auto it = keywords.find(value); it != keywords.end()) {
        return {it->second, value, startLine, startColumn};
    }

    return {TokenType::IDENTIFIER, value, startLine, startColumn};
}

Token Lexer::processNumber() {
    const size_t start = position;
    const size_t startLine = line;
    const size_t startColumn = column;
    bool isFloat = false;
    
    while (position < source.length() && isdigit(currentChar())) {
        advance();
    }
    
    if (position < source.length() && currentChar() == '.') {
        isFloat = true;
        advance();
        while (position < source.length() && isdigit(currentChar())) {
            advance();
        }
    }
    
    std::string value = source.substr(start, position - start);
    
    if (isFloat) {
        return {TokenType::FLOAT, value, startLine, startColumn};
    }
    
    return {TokenType::NUM, value, startLine, startColumn};
}

Token Lexer::processString() {
    advance();
    const size_t start = position;
    const size_t startLine = line;
    const size_t startColumn = column;
    
    std::string value;

    while (position < source.length() && currentChar() != '"') {
        char c = currentChar();
        
        // 处理转义序列
        if (c == '\\') {
            advance();
            char escaped = currentChar();
            switch (escaped) {
                case 'n': value += '\n'; break;
                case 'r': value += '\r'; break;
                case 't': value += '\t'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                default: value += escaped; break;
            }
            advance();
        } else {
            value += c;
            advance();
        }
        
        if (c == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
    }

    if (position < source.length()) {
        advance();
    }

    return {TokenType::STRING, value, startLine, startColumn};
}

char Lexer::currentChar() const {
    if (position < source.length()) {
        return source[position];
    }
    return '\0';
}

void Lexer::advance() {
    column++;
    position++;
}