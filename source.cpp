#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <vector>

enum TokenType {
    UNDEFINED,            
    SKIP,                 
    EXPLICIT_JOIN,        
    IMPLICIT_JOIN,        
    NEWLINE,              
    COMMENT,              
    INDENT,               
    DEDENT,               
    KEYWORD,              
    IDENTIFIER,           
    STRING_LITERAL,       
    INT_NUMBER_LITERAL,   
    FLOAT_NUMBER_LITERAL, 
    OPERATOR,             
    DELIMITER,            
    JOIN_ERROR,           
    INDENT_ERROR,         
    TAB_ERROR,            
    ERROR,                
};

std::string getTokenName(TokenType token) {
    switch (token) {
    case SKIP: return "SKIP";
    case EXPLICIT_JOIN: return "EXPLICIT_JOIN";
    case IMPLICIT_JOIN: return "IMPLICIT_JOIN";
    case NEWLINE: return "NEWLINE";
    case COMMENT: return "COMMENT";
    case INDENT: return "INDENT";
    case DEDENT: return "DEDENT";
    case KEYWORD: return "KEYWORD";
    case IDENTIFIER: return "IDENTIFIER";
    case STRING_LITERAL: return "STRING_LITERAL";
    case INT_NUMBER_LITERAL: return "INT_NUMBER_LITERAL";
    case FLOAT_NUMBER_LITERAL: return "FLOAT_NUMBER_LITERAL";
    case OPERATOR: return "OPERATOR";
    case DELIMITER: return "DELIMITER";
    case JOIN_ERROR: return "JOIN_ERROR";
    case INDENT_ERROR: return "INDENT_ERROR";
    case TAB_ERROR: return "TAB_ERROR";
    case ERROR: return "ERROR";
    default: return "UNDEFINED";
    }
}

class TokenIdent {
public:
    TokenType tokenType;
    std::regex tokenRegEx;

    TokenIdent(TokenType tokenType = TokenType::UNDEFINED, 
                std::regex tokenRegEx = std::regex(".")) 
    {
        this->tokenType = tokenType;
        this->tokenRegEx = tokenRegEx;
    }
};

class Token {
public:
    TokenType tokenType;
    int start_pos;
    int end_pos;

    Token(TokenType tokenType, int start_pos, int end_pos) {
        this->tokenType = tokenType;
        this->start_pos = start_pos;
        this->end_pos = end_pos;
    }
};

class LexicalAnalyser {
public:
    LexicalAnalyser() {
        skipToken = TokenIdent(TokenType::SKIP, std::regex(R"([ \t])"));
        keywordToken = TokenIdent(TokenType::KEYWORD, std::regex(R"(\b(False|await|else|import|pass|None|break|except|in|raise|True|class|finally|is|return|and|continue|lambda|try|as|def|from|nonlocal|while|assert|del|global|for|async|elif|if|or|not|with|yield)\b)"));
        commentToken = TokenIdent(TokenType::COMMENT, std::regex(R"((#([^\n])*))"));
        explicitJoinToken = TokenIdent(TokenType::EXPLICIT_JOIN, std::regex(R"(\\\n)"));
        explicitJoinErrorToken = TokenIdent(TokenType::JOIN_ERROR, std::regex(R"((\\[^\n]*))"));
        newLineToken = TokenIdent(TokenType::NEWLINE, std::regex(R"(\n)"));
        identifierToken = TokenIdent(TokenType::IDENTIFIER, std::regex(R"((?!")\b([a-zA-Z_][a-zA-Z0-9_]*)\b(?!"))"));
        operatorToken = TokenIdent(TokenType::OPERATOR, std::regex(R"(-|/|//|%|@|<<|>>|&|~|:=|<|>|<=|>=|==|!=|\+|\*|\*\*|\||\^)"));
        delimiterToken = TokenIdent(TokenType::DELIMITER, std::regex(R"(\(|\)|\[|\]|\{|\}|,|:|\.|;|@|=|->|\+=|-=|\*=|/=|//=|%=|@=|&=|\|=|\^=|>>=|<<=|\*\*=)"));
        stringLiteralToken = TokenIdent(TokenType::STRING_LITERAL, std::regex(R"((([rufRUF]|fr|Fr|fR|FR|rf|rF|Rf|RF)?(('{3}([^'{3}]|'{1,2}(?!'{3})|\\([\\'"abfn(\n)rtv0{3}(xhh)]{1}))*'{3})|"{3}([^"{3}]|"{1,2}(?!"{3})|\\([\\'"abfn(\n)rtv0{3}(xhh)]{1}))*"{3}|'(\\([\\'"abfn(\n)rtv0{3}(xhh)]{1})|[^'\n])*'|"(\\([\\'"abfn(\n)rtv0{3}(xhh)]{1})|[^"\n])*")))"));
        intLiteralToken = TokenIdent(TokenType::INT_NUMBER_LITERAL, std::regex(R"(\b(0[bB]([01](_?[01])*)|0[oO]([0-7](_?[0-7])*)|0[xX]([0-9a-fA-F](_?[0-9a-fA-F])*)|([1-9](_?[0-9])*)|(0(_?[0])*))\b)"));
        floatLiteralToken = TokenIdent(TokenType::FLOAT_NUMBER_LITERAL, std::regex(R"([+-]?(\d(_?\d)*\.(\d?(_?\d)*)?|\.\d(_?\d)*)([eE][+-]?\d(_?\d)*)?)"));
    }

    bool loadSrc(std::string &file_name) {
        std::ifstream file(file_name);
        if (!file) {
            std::cerr << "Error opening file" << std::endl;
            return false;
        }
        source.clear();
        char ch;
        while (file.get(ch)) {
            source.push_back(ch);
            tokenized_source.push_back(TokenType::UNDEFINED);
        }
        file.close();
        return true;
    }

    void printSrc() {
        for (char ch : source) {
            std::cout << ch;
        }
        std::cout << std::endl;
    }

    void printTokenizedSrc() {
        int level = 0; bool newline = true;
        for (auto token : tokens) {
            if (token.tokenType == TokenType::INDENT) {
                for (int i = 0; i < level; i++) std::cout << "    ";
                level++;
                newline = true;
                std::cout << getTokenName(token.tokenType) << std::endl;
            }
            else if (token.tokenType == TokenType::DEDENT) {
                level--;
                for (int i = 0; i < level; i++) std::cout << "    ";
                newline = true;
                std::cout << getTokenName(token.tokenType)<<std::endl;
            }
            else if (token.tokenType != TokenType::SKIP) {
                if (newline) {
                    newline = false;
                    for (int i = 0; i < level; i++) std::cout << "    ";
                }
                std::cout << getTokenName(token.tokenType) << " ";
            }

            if (token.tokenType == TokenType::NEWLINE) {
                std::cout << std::endl;
                newline = true;
            }
        }
        std::cout << std::endl;
    }

    void printTokens() {
        for (Token token : tokens) {
            if (token.tokenType != TokenType::INDENT &&
                token.tokenType != TokenType::DEDENT &&
                token.tokenType != TokenType::NEWLINE &&
                token.tokenType != TokenType::SKIP &&
                token.tokenType != TokenType::EXPLICIT_JOIN &&
                token.tokenType != TokenType::IMPLICIT_JOIN) {
                std::cout << "<"; 
                for (int i = token.start_pos; i <= token.end_pos; ++i) {
                    std::cout << source[i];
                }
                std::cout << " , " << getTokenName(token.tokenType) << "> " ;
            }
        }
        std::cout << std::endl;
    }

    void printErrors() {
        for (Token token : tokens) {
            if (token.tokenType == TokenType::JOIN_ERROR ||
                token.tokenType == TokenType::TAB_ERROR ||
                token.tokenType == TokenType::INDENT_ERROR ||
                token.tokenType == TokenType::ERROR) {
                std::cout << "< ";
                for (int i = token.start_pos; i <= token.end_pos; ++i) {
                    std::cout << source[i];
                }
                std::cout << " , " << getTokenName(token.tokenType) << " >" << std::endl;
            }
        }
        std::cout << std::endl;
    }

    void tokenize_by_token(TokenIdent tokenIdent) {
        std::smatch res;
        std::string input(source.begin(), source.end());
        std::string::const_iterator start(input.cbegin());

        int checked_pos = 0;
        while (std::regex_search(start, input.cend(), res, tokenIdent.tokenRegEx))
        {
            bool undefined = true;
            for (int i = checked_pos + res.position(); i < checked_pos + res.position() + res.length(); ++i) {
                if (tokenized_source[i] != TokenType::UNDEFINED) {
                    undefined = false;
                    break;
                }
            }
            if (undefined) {
                int i;
                for (i = checked_pos + res.position(); i < checked_pos + res.position() + res.length(); ++i) {
                    tokenized_source[i] = tokenIdent.tokenType;
                }
                tokenized_source.insert(tokenized_source.begin() + i, TokenType::SKIP);
                source.insert(source.begin() + i, '\0');
            }
            checked_pos += res.position() + res.length();
            input = std::string(source.begin() + checked_pos, source.end());
            start = std::string::const_iterator(input.cbegin());
        }  
    }

    void tokenize_implicit_join() {
        int r = 0; int s = 0; int c = 0;
        for (int i = 0; i < source.size(); ++i) {
            char ch = source[i];
            if (ch == '(') r++;
            if (ch == ')') r--;
            if (ch == '[') s++;
            if (ch == ']') s--;
            if (ch == '{') c++;
            if (ch == '}') c--;
            if (ch == '\n' && (s > 0 || r > 0 || c > 0)) {
                tokenized_source[i] = TokenType::IMPLICIT_JOIN;
            }
        }
    }

    void tokenize_identation() {
        bool ident = false;
        int tabs = 0; int spaces = 0; int spaces_raw = 0;
        int prev_tab_change = 0;
        bool is_prev_tab_change = false;
        std::vector<int> stack = { 0 };

        for (int i = 0; i < source.size(); ++i) {
            if (tokenized_source[i] == TokenType::NEWLINE) {
                ident = true; tabs = 0; spaces = 0; spaces_raw = 0;
            }     
            else if (ident) {
                if (source[i] == ' ') { 
                    spaces++; 
                    spaces_raw++; 
                }
                else if (source[i] == '\t') { 
                    if (8 - spaces_raw != prev_tab_change && is_prev_tab_change) {
                        tokenized_source[i] = TokenType::TAB_ERROR;
                    }
                    spaces += 8 - spaces_raw;
                    is_prev_tab_change = true;
                    prev_tab_change = 8 - spaces_raw;
                    spaces_raw = 0; 
                }
                else if (source[i] != '\0'){
                    ident = false;
                    if (spaces > stack.back()) {
                        stack.push_back(spaces); 
                        tokenized_source.insert(tokenized_source.begin() + i, TokenType::INDENT);
                        source.insert(source.begin() + i, '\0');
                    }
                    else {
                        while (spaces < stack.back()) {
                            stack.pop_back();
                            tokenized_source.insert(tokenized_source.begin() + i, TokenType::DEDENT);
                            source.insert(source.begin() + i, '\0');
                            tokenized_source.insert(tokenized_source.begin() + i, TokenType::SKIP);
                            source.insert(source.begin() + i, '\0');
                        }
                        if (spaces != stack.back()) {
                            tokenized_source.insert(tokenized_source.begin() + i, TokenType::INDENT_ERROR);
                            source.insert(source.begin() + i, '\0');
                        }
                    }
                }
            }
        }
        while (stack.back() > 0) {
            stack.pop_back();
            source.push_back('\0');
            tokenized_source.push_back(TokenType::DEDENT);
            source.push_back('\0');
            tokenized_source.push_back(TokenType::SKIP);
        }
    }

    void find_errors() {
        for (int i = 0; i < source.size(); ++i) {
            if (tokenized_source[i] == TokenType::UNDEFINED) {
                tokenized_source[i] = TokenType::ERROR;
            }
        }
    }

    void generate_tokens() {
        TokenType tokenTypePrev = tokenized_source[0];
        int start = 0; 
        for (int i = 0; i < source.size(); ++i) {
            if (tokenTypePrev != tokenized_source[i]) {
                tokens.push_back(Token(tokenTypePrev, start, i - 1));
                start = i;
                tokenTypePrev = tokenized_source[i];
            }
        }
    }

    void tokenize() {
        tokenize_by_token(commentToken);
        tokenize_by_token(stringLiteralToken);
        tokenize_by_token(explicitJoinToken);
        tokenize_by_token(explicitJoinErrorToken);
        tokenize_implicit_join();
        tokenize_by_token(newLineToken);
        tokenize_identation();
        tokenize_by_token(keywordToken);
        tokenize_by_token(identifierToken);
        tokenize_by_token(floatLiteralToken);
        tokenize_by_token(intLiteralToken);
        tokenize_by_token(operatorToken);
        tokenize_by_token(delimiterToken);
        tokenize_by_token(skipToken);
        find_errors();
        generate_tokens();
    }


private:
    TokenIdent keywordToken;
    TokenIdent identifierToken;
    TokenIdent commentToken;
    TokenIdent explicitJoinToken;
    TokenIdent explicitJoinErrorToken;
    TokenIdent operatorToken;
    TokenIdent delimiterToken;
    TokenIdent stringLiteralToken;
    TokenIdent intLiteralToken;
    TokenIdent floatLiteralToken;
    TokenIdent newLineToken;
    TokenIdent skipToken;
    std::vector<char> source;
    std::vector<TokenType> tokenized_source;
    std::vector<Token> tokens;
};

int main()
{
    LexicalAnalyser lexer;

    while (true) {
        std::string file_name;
        std::cout << "Input file name: ";
        std::cin >> file_name;

        if (lexer.loadSrc(file_name)) {
            lexer.tokenize();
            lexer.printSrc();
            lexer.printTokenizedSrc();
            lexer.printTokens();
            lexer.printErrors();
        }
    }
    
    return 0;
}

