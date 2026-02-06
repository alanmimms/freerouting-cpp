#ifndef FREEROUTING_IO_SEXPRPARSER_H
#define FREEROUTING_IO_SEXPRPARSER_H

#include "SExprLexer.h"
#include "core/Types.h"
#include <memory>
#include <vector>
#include <variant>

namespace freerouting {

// Forward declaration
class SExprNode;

// S-expression atom value types
using SExprAtom = std::variant<std::string, double, i64>;

// S-expression node - can be either a list or an atom
class SExprNode {
public:
  enum class Type {
    List,    // (...)
    Atom     // symbol, string, or number
  };

  // Create list node
  static std::unique_ptr<SExprNode> createList(std::vector<std::unique_ptr<SExprNode>> children = {}) {
    auto node = std::make_unique<SExprNode>();
    node->type_ = Type::List;
    node->children_ = std::move(children);
    return node;
  }

  // Create atom node
  static std::unique_ptr<SExprNode> createAtom(SExprAtom value) {
    auto node = std::make_unique<SExprNode>();
    node->type_ = Type::Atom;
    node->value_ = std::move(value);
    return node;
  }

  // Get node type
  Type getType() const { return type_; }

  // Check if node is a list
  bool isList() const { return type_ == Type::List; }

  // Check if node is an atom
  bool isAtom() const { return type_ == Type::Atom; }

  // Get children (for list nodes)
  const std::vector<std::unique_ptr<SExprNode>>& getChildren() const {
    FR_ASSERT(isList());
    return children_;
  }

  // Get child count
  size_t childCount() const {
    return isList() ? children_.size() : 0;
  }

  // Get child at index
  const SExprNode* getChild(size_t index) const {
    FR_ASSERT(isList() && index < children_.size());
    return children_[index].get();
  }

  // Get atom value
  const SExprAtom& getValue() const {
    FR_ASSERT(isAtom());
    return value_;
  }

  // Helper to get atom as string
  const std::string& asString() const {
    FR_ASSERT(isAtom());
    return std::get<std::string>(value_);
  }

  // Helper to get atom as double
  double asDouble() const {
    FR_ASSERT(isAtom());
    if (std::holds_alternative<double>(value_)) {
      return std::get<double>(value_);
    } else if (std::holds_alternative<i64>(value_)) {
      return static_cast<double>(std::get<i64>(value_));
    }
    return 0.0;
  }

  // Helper to get atom as integer
  i64 asInt() const {
    FR_ASSERT(isAtom());
    if (std::holds_alternative<i64>(value_)) {
      return std::get<i64>(value_);
    } else if (std::holds_alternative<double>(value_)) {
      return static_cast<i64>(std::get<double>(value_));
    }
    return 0;
  }

  // Check if list starts with a specific keyword
  bool isListWithKeyword(const char* keyword) const {
    if (!isList() || children_.empty()) {
      return false;
    }
    const auto* first = children_[0].get();
    if (!first->isAtom()) {
      return false;
    }
    const auto& value = first->getValue();
    if (!std::holds_alternative<std::string>(value)) {
      return false;
    }
    return std::get<std::string>(value) == keyword;
  }

  // Add child to list node
  void addChild(std::unique_ptr<SExprNode> child) {
    FR_ASSERT(isList());
    children_.push_back(std::move(child));
  }

  // Default constructor (public for make_unique but use factory methods)
  SExprNode() = default;

private:
  Type type_;
  std::vector<std::unique_ptr<SExprNode>> children_;  // For list nodes
  SExprAtom value_;                                    // For atom nodes
};

// Parser for S-expressions
// Converts tokens into an AST (Abstract Syntax Tree)
class SExprParser {
public:
  explicit SExprParser(SExprLexer& lexer)
    : lexer_(lexer), currentToken_() {
    advance();
  }

  // Parse next S-expression
  // Returns nullptr on EOF
  std::unique_ptr<SExprNode> parse() {
    if (currentToken_.type == SExprTokenType::EndOfFile) {
      return nullptr;
    }

    if (currentToken_.type == SExprTokenType::LeftParen) {
      return parseList();
    }

    return parseAtom();
  }

  // Parse all S-expressions in input
  std::vector<std::unique_ptr<SExprNode>> parseAll() {
    std::vector<std::unique_ptr<SExprNode>> result;

    while (currentToken_.type != SExprTokenType::EndOfFile) {
      auto node = parse();
      if (node) {
        result.push_back(std::move(node));
      }
    }

    return result;
  }

private:
  SExprLexer& lexer_;
  SExprToken currentToken_;

  // Advance to next token
  void advance() {
    currentToken_ = lexer_.nextToken();
  }

  // Parse list: ( ... )
  std::unique_ptr<SExprNode> parseList() {
    FR_ASSERT(currentToken_.type == SExprTokenType::LeftParen);
    advance();  // Skip '('

    auto list = SExprNode::createList();

    while (currentToken_.type != SExprTokenType::RightParen &&
           currentToken_.type != SExprTokenType::EndOfFile) {
      auto child = parse();
      if (child) {
        list->addChild(std::move(child));
      }
    }

    if (currentToken_.type == SExprTokenType::RightParen) {
      advance();  // Skip ')'
    }

    return list;
  }

  // Parse atom: symbol, string, or number
  std::unique_ptr<SExprNode> parseAtom() {
    SExprAtom value;

    if (currentToken_.type == SExprTokenType::Number) {
      // Try to parse as integer first, then as double
      const std::string& str = currentToken_.value;
      if (str.find('.') == std::string::npos &&
          str.find('e') == std::string::npos &&
          str.find('E') == std::string::npos) {
        // Integer
        try {
          value = static_cast<i64>(std::stoll(str));
        } catch (...) {
          value = std::stod(str);
        }
      } else {
        // Floating point
        value = std::stod(currentToken_.value);
      }
    } else {
      // Symbol or string - store as string
      value = currentToken_.value;
    }

    advance();
    return SExprNode::createAtom(std::move(value));
  }
};

} // namespace freerouting

#endif // FREEROUTING_IO_SEXPRPARSER_H
