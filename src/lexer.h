#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class TokenType {
    LET,
    FN,
    RETURN,
    TRUE,
    FALSE,
    IDENTIFIER,
    NUM,
    FLOAT,
    STRING,
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MOD,
    EQ,
    NE,
    LT,
    GT,
    LE,
    GE,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    COMMA,
    SEMICOLON,
    ASSIGN,
    COLON,
    ARROW,
    AND,
    OR,
    IF,
    ELSE,
    FOR,
    BREAK,
    CONTINUE,
    EOF_TOKEN, COL_COLON, NOT, CONST, GRID, LBRACKET, RBRACKET, AS, REF,
    STRUCT,
    ENUM,
    TRAIT,
    IMPL,
    PUB,
    STATIC,
    CONSTRUCTOR,
    DOT,
};

struct Token {
    TokenType type;
    std::string value;
    size_t line, column;
};

class Lexer {
public:

    explicit Lexer(std::string source);

    Token getNextToken();
    
    [[nodiscard]] Token peek() const;
    
private:
    std::string source;
    size_t position{0}, line{1}, column{1};
    
    void skipWhitespace();
    Token processIdentifier();
    Token processNumber();
    Token processString();
    [[nodiscard]] char currentChar() const;
    void advance();
};

#endif // LEXER_H
