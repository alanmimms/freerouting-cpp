#ifndef FREEROUTING_IO_SEXPRLEXER_H
#define FREEROUTING_IO_SEXPRLEXER_H

#include "core/Types.h"
#include <string>
#include <string_view>
#include <cctype>

namespace freerouting {

// S-expression token types
enum class SExprTokenType {
  LeftParen,    // (
  RightParen,   // )
  Symbol,       // unquoted identifier
  String,       // "quoted string"
  Number,       // integer or floating point
  EndOfFile
};

// Single token from S-expression
struct SExprToken {
  SExprTokenType type;
  std::string value;
  int line;
  int column;

  SExprToken(SExprTokenType t, std::string v, int l, int c)
    : type(t), value(std::move(v)), line(l), column(c) {}

  SExprToken()
    : type(SExprTokenType::EndOfFile), value(""), line(0), column(0) {}
};

// Lexer for S-expression tokenization
// Converts input text into sequence of tokens
class SExprLexer {
public:
  explicit SExprLexer(std::string_view input)
    : input_(input), pos_(0), line_(1), column_(1) {}

  // Get next token from input
  SExprToken nextToken() {
    skipWhitespaceAndComments();

    if (pos_ >= input_.size()) {
      return SExprToken(SExprTokenType::EndOfFile, "", line_, column_);
    }

    int tokenLine = line_;
    int tokenColumn = column_;
    char ch = input_[pos_];

    // Left parenthesis
    if (ch == '(') {
      advance();
      return SExprToken(SExprTokenType::LeftParen, "(", tokenLine, tokenColumn);
    }

    // Right parenthesis
    if (ch == ')') {
      advance();
      return SExprToken(SExprTokenType::RightParen, ")", tokenLine, tokenColumn);
    }

    // Quoted string
    if (ch == '"') {
      return readString(tokenLine, tokenColumn);
    }

    // Number or symbol
    if (std::isdigit(ch) || ch == '-' || ch == '+' || ch == '.') {
      // Try to read as number first
      std::string token = readAtom();

      // Check if it's a valid number
      if (isNumber(token)) {
        return SExprToken(SExprTokenType::Number, token, tokenLine, tokenColumn);
      }

      // Otherwise it's a symbol
      return SExprToken(SExprTokenType::Symbol, token, tokenLine, tokenColumn);
    }

    // Symbol (identifier)
    std::string symbol = readAtom();
    return SExprToken(SExprTokenType::Symbol, symbol, tokenLine, tokenColumn);
  }

  // Peek at current character without consuming
  char peek() const {
    if (pos_ >= input_.size()) return '\0';
    return input_[pos_];
  }

  // Get current line number
  int getLine() const { return line_; }

  // Get current column number
  int getColumn() const { return column_; }

private:
  std::string_view input_;
  size_t pos_;
  int line_;
  int column_;

  // Advance position by one character
  void advance() {
    if (pos_ < input_.size()) {
      if (input_[pos_] == '\n') {
        line_++;
        column_ = 1;
      } else {
        column_++;
      }
      pos_++;
    }
  }

  // Skip whitespace and comments (# to end of line)
  void skipWhitespaceAndComments() {
    while (pos_ < input_.size()) {
      char ch = input_[pos_];

      // Skip whitespace
      if (std::isspace(ch)) {
        advance();
        continue;
      }

      // Skip line comments (# to end of line)
      if (ch == '#') {
        while (pos_ < input_.size() && input_[pos_] != '\n') {
          advance();
        }
        continue;
      }

      break;
    }
  }

  // Read quoted string
  SExprToken readString(int tokenLine, int tokenColumn) {
    FR_ASSERT(input_[pos_] == '"');
    advance();  // Skip opening quote

    std::string result;
    while (pos_ < input_.size() && input_[pos_] != '"') {
      char ch = input_[pos_];

      // Handle escape sequences
      if (ch == '\\' && pos_ + 1 < input_.size()) {
        advance();
        char next = input_[pos_];
        switch (next) {
          case 'n': result += '\n'; break;
          case 't': result += '\t'; break;
          case 'r': result += '\r'; break;
          case '"': result += '"'; break;
          case '\\': result += '\\'; break;
          default: result += next; break;
        }
        advance();
      } else {
        result += ch;
        advance();
      }
    }

    if (pos_ < input_.size() && input_[pos_] == '"') {
      advance();  // Skip closing quote
    }

    return SExprToken(SExprTokenType::String, result, tokenLine, tokenColumn);
  }

  // Read unquoted atom (symbol or number)
  std::string readAtom() {
    std::string result;

    while (pos_ < input_.size()) {
      char ch = input_[pos_];

      // Stop at delimiters
      if (std::isspace(ch) || ch == '(' || ch == ')' || ch == '"' || ch == '#') {
        break;
      }

      result += ch;
      advance();
    }

    return result;
  }

  // Check if string is a valid number
  bool isNumber(const std::string& str) const {
    if (str.empty()) return false;

    size_t i = 0;

    // Optional sign
    if (str[i] == '+' || str[i] == '-') {
      i++;
    }

    if (i >= str.size()) return false;

    // Check for digits and optional decimal point
    bool hasDigit = false;
    bool hasDecimal = false;

    while (i < str.size()) {
      if (std::isdigit(str[i])) {
        hasDigit = true;
        i++;
      } else if (str[i] == '.' && !hasDecimal) {
        hasDecimal = true;
        i++;
      } else if (str[i] == 'e' || str[i] == 'E') {
        // Scientific notation
        i++;
        if (i < str.size() && (str[i] == '+' || str[i] == '-')) {
          i++;
        }
        while (i < str.size() && std::isdigit(str[i])) {
          i++;
        }
        break;
      } else {
        return false;
      }
    }

    return hasDigit && i == str.size();
  }
};

} // namespace freerouting

#endif // FREEROUTING_IO_SEXPRLEXER_H
