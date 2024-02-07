#include "json_parser.hpp"

LexerResult JsonLexer(char* buffer, int buffer_length) {
    std::list<JSONToken> tokens;
    bool isLastTokenFinishLexing = true;
    size_t lastTokenStartIndex = 0;

    JSONToken current_token;
    for (int i = 0; i < buffer_length; i++) {
        if (isLastTokenFinishLexing) {
            switch(buffer[i]) {
                case '{':{
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::StartObject;
                    tokens.push_back(current_token);
                    isLastTokenFinishLexing = true;
                };break;
                case '}':{
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::EndObject;
                    tokens.push_back(current_token);
                    isLastTokenFinishLexing = true;
                };break;
                case '[':{
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::StartArray;
                    tokens.push_back(current_token);
                    isLastTokenFinishLexing = true;
                };break;
                case ']':{
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::EndArray;
                    tokens.push_back(current_token);
                    isLastTokenFinishLexing = true;
                };break;
                case ',':{
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::Colon;
                    tokens.push_back(current_token);
                    isLastTokenFinishLexing = true;
                };break;
                case ':':{
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::Comma;
                    tokens.push_back(current_token);
                    isLastTokenFinishLexing = true;
                };break;
                case '\"':{
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::String;
                    isLastTokenFinishLexing = false;
                };break;
                case '1' ... '9':{

                };break;
                case 'n':{
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::Null;
                    isLastTokenFinishLexing = false;
                    current_token.stringValue.push_back(buffer[i]);
                };break;
            }
            lastTokenStartIndex = i;
        } else {
            switch (current_token.type) {
                case String:{

                };break;
                case Number:{

                };break;
                case Boolean:{

                };break;
                case Null:{
                    // Add new char to end of string
                    current_token.stringValue.push_back(buffer[i]);

                    if (current_token.stringValue.compare("null") == 0) {
                        tokens.push_back(current_token);
                        isLastTokenFinishLexing = true;
                    } else if (current_token.stringValue.length() >= 4) {
                        fprintf(stderr, "\r\n[ERR]: Invalid null keyword got: %s!\r\n", current_token.stringValue.c_str());
                        // Make a immediate return
                        goto end_of_loop;
                    }
                };break;
                default:{
                    fprintf(stderr, "\r\n[ERR]: Type unknow, should be String, Number, Boolean or null!\r\n");
                    current_token = JSONToken();
                    isLastTokenFinishLexing = true;
                };break;
            }
        }
    }
end_of_loop:

    return LexerResult {
        tokens,
        isLastTokenFinishLexing,
        lastTokenStartIndex
    };
}