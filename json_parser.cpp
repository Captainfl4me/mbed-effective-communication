#include "json_parser.hpp"
#include <vector>

JSONLexer::LexerResult JSONLexer::LexBuffer(char* buffer, int buffer_length) {
    std::list<JSONLexer::JSONToken> tokens;
    bool isLastTokenFinishLexing = true;
    size_t lastTokenStartIndex = 0;

    JSONToken current_token;
    for (int i = 0; i < buffer_length; i++) {
begin_of_loop:
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
                    current_token.type = JSONTokenType::Comma;
                    tokens.push_back(current_token);
                    isLastTokenFinishLexing = true;
                };break;
                case ':':{
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::Colon;
                    tokens.push_back(current_token);
                    isLastTokenFinishLexing = true;
                };break;
                case '\"':{
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::String;
                    isLastTokenFinishLexing = false;
                };break;
                case '0' ... '9':{
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::Integer;
                    isLastTokenFinishLexing = false;
                    current_token.intValue = buffer[i] - 48;
                };break;
                case 'n':{
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::Null;
                    isLastTokenFinishLexing = false;
                    current_token.stringValue.push_back(buffer[i]);
                };break;
                case 't':
                case 'f':{
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::Boolean;
                    isLastTokenFinishLexing = false;
                    current_token.stringValue.push_back(buffer[i]);
                };break;
            }
            lastTokenStartIndex = i;
        } else {
            switch (current_token.type) {
                case String:{
                    if (buffer[i] == '\"') {
                        tokens.push_back(current_token);
                        isLastTokenFinishLexing = true;                        
                    } else {
                        current_token.stringValue.push_back(buffer[i]);
                    }
                };break;
                case Integer:{
                    if (buffer[i] >= 48 && buffer[i] <= 57) {
                        current_token.intValue *= 10;
                        current_token.intValue += buffer[i] - 48;
                    } else if (buffer[i] == '.') {
                        current_token.type = JSONTokenType::Float;
                        current_token.floatValue = current_token.intValue;
                        current_token.intValue = 1;
                    } else {
                        tokens.push_back(current_token);
                        isLastTokenFinishLexing = true;
                        goto begin_of_loop;
                    }
                };break;
                case Float:{
                    if (buffer[i] >= 48 && buffer[i] <= 57) {
                        current_token.intValue *= 10;
                        current_token.floatValue += ((float)(buffer[i] - 48)) / current_token.intValue;
                    } else {
                        tokens.push_back(current_token);
                        isLastTokenFinishLexing = true;
                        goto begin_of_loop;
                    }                    
                };break;
                case Boolean:{
                    // Add new char to end of string
                    current_token.stringValue.push_back(buffer[i]);

                    if (current_token.stringValue.compare("false") == 0) {
                        current_token.boolValue = false;
                        current_token.stringValue.clear();
                        tokens.push_back(current_token);
                        isLastTokenFinishLexing = true;
                    } else if (current_token.stringValue.compare("true") == 0) {
                        current_token.boolValue = true;
                        current_token.stringValue.clear();
                        tokens.push_back(current_token);
                        isLastTokenFinishLexing = true;
                    } else if (current_token.stringValue.length() >= 5) {
                        fprintf(stderr, "\r\n[ERR]: Invalid true/false keyword got: %s!\r\n", current_token.stringValue.c_str());
                        // Make a immediate return
                        goto end_of_loop;
                    }
                };break;
                case Null:{
                    // Add new char to end of string
                    current_token.stringValue.push_back(buffer[i]);

                    if (current_token.stringValue.compare("null") == 0) {
                        current_token.stringValue.clear();
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

    return JSONLexer::LexerResult {
        tokens,
        isLastTokenFinishLexing,
        lastTokenStartIndex
    };
}

JSONParser::JSONValue::JSONValue() {
    this->type = JSONParser::JSONValueType::Null;
    this->value = { 0 };
}

JSONParser::JSONValue::JSONValue(JSONLexer::JSONToken *token) {
    switch (token->type) {
        case JSONLexer::JSONTokenType::String: {
            this->type = JSONValueType::String;
            this->value.stringValue = new std::string(token->stringValue);
        };break;
        case JSONLexer::JSONTokenType::Boolean: {
            this->type = JSONValueType::Boolean;
            this->value.boolValue = token->boolValue;
        };break;
        case JSONLexer::JSONTokenType::Null: {
            this->type = JSONValueType::Null;
            this->value = { 0 };
        };break;
        case JSONLexer::JSONTokenType::Integer: {
            this->type = JSONValueType::Integer;
            this->value.intValue = token->intValue;
        };break;
        case JSONLexer::JSONTokenType::Float: {
            this->type = JSONValueType::Float;
            this->value.floatValue = token->floatValue;
        };break;
        default:{
            fprintf(stderr, "\r\n[ERR]: Couldn't create JSONValue from this token !\r\n");
        }
    }
}

JSONParser::JSONValue::JSONValue(std::map<std::string, JSONParser::JSONValue> *map) {
    this->type = JSONParser::JSONValueType::Object;
    this->value.mapValue = new std::map<std::string, JSONParser::JSONValue>(*map);
}

JSONParser::JSONValue::JSONValue(std::vector<JSONParser::JSONValue> *vec) {
    this->type = JSONParser::JSONValueType::Array;
    this->value.arrayValue = new std::vector<JSONParser::JSONValue>(*vec);
}

bool JSONParser::JSONValue::isBoolean() {
    return this->type == JSONParser::JSONValueType::Boolean;
}
bool JSONParser::JSONValue::isInt() {
    return this->type == JSONParser::JSONValueType::Integer;
}
bool JSONParser::JSONValue::isFloat() {
    return this->type == JSONParser::JSONValueType::Float;
}
bool JSONParser::JSONValue::isString() {
    return this->type == JSONParser::JSONValueType::String;
}
bool JSONParser::JSONValue::isNull() {
    return this->type == JSONParser::JSONValueType::Null;
}
bool JSONParser::JSONValue::isMap() {
    return this->type == JSONParser::JSONValueType::Object;
}
bool JSONParser::JSONValue::isArray() {
    return this->type == JSONParser::JSONValueType::Array;
}


bool JSONParser::JSONValue::getBoolean() {
    if (!this->isBoolean())
        fprintf(stderr, "\r\n[ERR]: Value is not a Boolean !\r\n");
    
    return this->value.boolValue;
}
float JSONParser::JSONValue::getFloat() {
    if (!this->isFloat())
        fprintf(stderr, "\r\n[ERR]: Value is not a Float !\r\n");
    
    return this->value.floatValue;
}
int JSONParser::JSONValue::getInt() {
    if (!this->isInt())
        fprintf(stderr, "\r\n[ERR]: Value is not a Int !\r\n");
    
    return this->value.intValue;
}
int JSONParser::JSONValue::getNull() {
    if (!this->isNull())
        fprintf(stderr, "\r\n[ERR]: Value is not NULL !\r\n");
    
    return NULL;
}
std::string JSONParser::JSONValue::getString() {
    if (!this->isString())
        fprintf(stderr, "\r\n[ERR]: Value is not a String !\r\n");
    
    return *this->value.stringValue;
}
std::map<std::string, JSONParser::JSONValue>* JSONParser::JSONValue::getMap() {
    if (!this->isMap())
        fprintf(stderr, "\r\n[ERR]: Value is not a Map !\r\n");
    
    return this->value.mapValue;
}
std::vector<JSONParser::JSONValue>* JSONParser::JSONValue::getArray() {
    if (!this->isArray())
        fprintf(stderr, "\r\n[ERR]: Value is not an Array !\r\n");
    
    return this->value.arrayValue;
}

JSONParser::JSONValue JSONParser::ParseTokens(std::list<JSONLexer::JSONToken> *tokens) {
    if (tokens->front().type == JSONLexer::JSONTokenType::StartObject) {
        std::map<std::string, JSONParser::JSONValue> map;
        tokens->pop_front();

        std::string key;
        while(tokens->front().type != JSONLexer::JSONTokenType::EndObject) {
            if (tokens->front().type != JSONLexer::JSONTokenType::String) {
                fprintf(stderr, "\r\n[ERR]: Expected key token should be string !\r\n");
                return JSONParser::JSONValue();
            }
            key = tokens->front().stringValue;
            tokens->pop_front();

            if (tokens->front().type != JSONLexer::JSONTokenType::Colon) {
                fprintf(stderr, "\r\n[ERR]: Expected Colon between key and value!\r\n");
                return JSONParser::JSONValue();
            }
            tokens->pop_front();

            if (tokens->front().type == JSONLexer::JSONTokenType::StartArray 
            || tokens->front().type == JSONLexer::JSONTokenType::StartObject) {
                map[key] = JSONParser::ParseTokens(tokens);
            } else {
                map[key] = JSONParser::JSONValue(&tokens->front());
                tokens->pop_front();
            }

            if (tokens->front().type != JSONLexer::JSONTokenType::EndObject) {
                if (tokens->front().type == JSONLexer::JSONTokenType::Comma) {
                    tokens->pop_front();
                } else {
                    fprintf(stderr, "\r\n[ERR]: Expected Comma between entries!\r\n");
                    return JSONParser::JSONValue();
                }
            }
        }
        tokens->pop_front();

        return JSONParser::JSONValue(&map);
    } else if (tokens->front().type == JSONLexer::JSONTokenType::StartArray) {
        std::vector<JSONParser::JSONValue> vec;
        tokens->pop_front();

        while(tokens->front().type != JSONLexer::JSONTokenType::EndArray) {
            
            if (tokens->front().type == JSONLexer::JSONTokenType::StartArray 
            || tokens->front().type == JSONLexer::JSONTokenType::StartObject) {
                vec.push_back(JSONParser::ParseTokens(tokens));
            } else {
                vec.push_back(JSONParser::JSONValue(&tokens->front()));
                tokens->pop_front();
            }

            if (tokens->front().type != JSONLexer::JSONTokenType::EndArray) {
                if (tokens->front().type == JSONLexer::JSONTokenType::Comma) {
                    tokens->pop_front();
                } else {
                    fprintf(stderr, "\r\n[ERR]: Expected Comma between entries!\r\n");
                    return JSONParser::JSONValue();
                }
            }
        }
        tokens->pop_front();

        return JSONParser::JSONValue(&vec);
    }else {
        fprintf(stderr, "\r\n[ERR]: First token should be '{' or '[' !\r\n");
        return JSONParser::JSONValue();
    }
}
