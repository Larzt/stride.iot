#include "lexer.hpp"
#include "liner.hpp"


std::vector<Token> tokenize(const std::string &line) {
    Liner liner(line);
    return liner.process();
}

std::ostream &operator<<(std::ostream &out, TokenType type) {
    switch (type) {
        case TokenType::LED:                out << "LED"; break;
        case TokenType::BUZZER:             out << "BUZZER"; break;
        case TokenType::BUTTON:             out << "BUTTON"; break;
        case TokenType::PIN:                out << "PIN"; break;
        case TokenType::NAME:               out << "NAME"; break;
        case TokenType::WRITE:              out << "WRITE"; break;
        case TokenType::READ:               out << "READ"; break;
        case TokenType::PRINT:              out << "PRINT"; break;
        case TokenType::NUMBER:             out << "NUMBER"; break;
        case TokenType::IDENTIFIER:         out << "IDENTIFIER"; break;
        case TokenType::VALUE:              out << "VALUE"; break;
        case TokenType::STRING:             out << "STRING"; break;

        // FLOW CONTROL
        case TokenType::IF:                 out << "IF"; break;
        case TokenType::ELSE:               out << "ELSE"; break;
        case TokenType::ENDIF:              out << "ENDIF"; break;
        case TokenType::LOOP:               out << "LOOP"; break;
        case TokenType::DLOOP:              out << "DLOOP"; break;
        case TokenType::WAIT:               out << "WAIT"; break;

        // ASSIGN / OPERATORS
        case TokenType::ASSIGN:             out << "ASSIGN"; break;
        case TokenType::ARROW:              out << "ARROW"; break;

        // CONDITIONALS
        case TokenType::IS_EQUAL:           out << "IS_EQUAL"; break;
        case TokenType::NOT_EQUAL:          out << "NOT_EQUAL"; break;
        case TokenType::LESS_EQUAL:         out << "LESS_EQUAL"; break;
        case TokenType::GREATER_EQUAL:      out << "GREATER_EQUAL"; break;
        case TokenType::LESS_THAN:          out << "LESS_THAN"; break;
        case TokenType::GREATER_THAN:       out << "GREATER_THAN"; break;

        // I2C
        case TokenType::I2C:                out << "I2C"; break;
        case TokenType::INIT:               out << "INIT"; break;
        case TokenType::SDA:                out << "SDA"; break;
        case TokenType::SCL:                out << "SCL"; break;
        case TokenType::HEX_NUMBER:         out << "HEX_NUMBER"; break;

        case TokenType::FILE:               out << "FILE"; break;
        default:                            out << "UNKNOWN"; break;
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, const Token& token) {
    out << token.type << ": " << token.value;
    return out;
}
