/* JSON parser utils library
 * This library implemented dynamic JSON support for C++11.
 * Aiming for serialization, deserialization and representation.
 *
 * Author: Nicolas THIERRY
 */
#include "json_parser.hpp"
#include <algorithm>
#include <vector>

JSONLexer::LexerResult JSONLexer::LexBuffer(char* buffer, int buffer_length) {
    std::list<JSONLexer::JSONToken> tokens;
    bool isLastTokenFinishLexing = true;
    size_t lastTokenStartIndex = 0;

    JSONToken current_token;
    for (int i = 0; i < buffer_length; i++) {
begin_of_loop:
        // If we are lexing a new token
        if (isLastTokenFinishLexing) {
            switch(buffer[i]) {
                case '{':{
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::StartObject;
                    tokens.push_back(current_token);
                    // Instant finish tokenize as it is a single char token.
                    isLastTokenFinishLexing = true;
                };break;
                case '}':{
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::EndObject;
                    tokens.push_back(current_token);
                    // Instant finish tokenize as it is a single char token.
                    isLastTokenFinishLexing = true;
                };break;
                case '[':{
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::StartArray;
                    tokens.push_back(current_token);
                    // Instant finish tokenize as it is a single char token.
                    isLastTokenFinishLexing = true;
                };break;
                case ']':{
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::EndArray;
                    tokens.push_back(current_token);
                    // Instant finish tokenize as it is a single char token.
                    isLastTokenFinishLexing = true;
                };break;
                case ',':{
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::Comma;
                    tokens.push_back(current_token);
                    // Instant finish tokenize as it is a single char token.
                    isLastTokenFinishLexing = true;
                };break;
                case ':':{
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::Colon;
                    tokens.push_back(current_token);
                    // Instant finish tokenize as it is a single char token.
                    isLastTokenFinishLexing = true;
                };break;
                case '\"':{
                    // We are expecting to read a string until find another ", so no instant return.
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::String;
                    isLastTokenFinishLexing = false;
                };break;
                case '0' ... '9':{
                    // We are expecting to read a number until we keep finding digit chars, so no instant return.
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::Integer;
                    isLastTokenFinishLexing = false;
                    // Converting ASCII to int (as 48 is the code for zero).
                    current_token.intValue = buffer[i] - 48;
                };break;
                case 'n':{
                    // We are expecting to find the keyword null, so no instant return.
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::Null;
                    isLastTokenFinishLexing = false;
                    current_token.stringValue.push_back(buffer[i]);
                };break;
                case 't':
                case 'f':{
                    // We are expected to find the keyword true or false, so no instant return.
                    current_token = JSONToken();
                    current_token.type = JSONTokenType::Boolean;
                    isLastTokenFinishLexing = false;
                    current_token.stringValue.push_back(buffer[i]);
                };break;
            }
            lastTokenStartIndex = i;
        } else {
            // If we are currently parsing a token
            switch (current_token.type) {
                case String:{
                    // The " should mean that we reached the end of the string to parse, else we keep parsing.
                    if (buffer[i] == '\"') {
                        tokens.push_back(current_token);
                        isLastTokenFinishLexing = true;                        
                    } else {
                        current_token.stringValue.push_back(buffer[i]);
                    }
                };break;
                case Integer:{
                    // If current char is still between '0' and '9' we append the digit at the end.
                    if (buffer[i] >= 48 && buffer[i] <= 57) {
                        current_token.intValue *= 10;
                        current_token.intValue += buffer[i] - 48;
                    } else if (buffer[i] == '.') {
                        // If we find a decimal separator it means that it is actually a float value
                        current_token.type = JSONTokenType::Float;
                        current_token.floatValue = current_token.intValue;
                        // We then use the int value to store the next decimal precision
                        current_token.intValue = 1;
                    } else {
                        // If we read anything other than digits or decimal separators then it is the end of the number.
                        tokens.push_back(current_token);
                        isLastTokenFinishLexing = true;
                        // Instant return to the beginning of the loop within the same iteration to correctly parse the non-number token.
                        goto begin_of_loop;
                    }
                };break;
                case Float:{
                    if (buffer[i] >= 48 && buffer[i] <= 57) {
                        // We add the readed digit at the end of our float
                        current_token.intValue *= 10;
                        current_token.floatValue += ((float)(buffer[i] - 48)) / current_token.intValue;
                    } else {
                        // If we read anything other than digits save token and instant return to beginning of the loop to correctly parse the non-number token.
                        tokens.push_back(current_token);
                        isLastTokenFinishLexing = true;
                        goto begin_of_loop;
                    }                    
                };break;
                case Boolean:{
                    // Add new char to end of string
                    current_token.stringValue.push_back(buffer[i]);

                    // If keyword is equal to false
                    if (current_token.stringValue.compare("false") == 0) {
                        current_token.boolValue = false;
                        current_token.stringValue.clear();
                        tokens.push_back(current_token);
                        isLastTokenFinishLexing = true;
                    } else if (current_token.stringValue.compare("true") == 0) { // If keyword is equal to true
                        current_token.boolValue = true;
                        current_token.stringValue.clear();
                        tokens.push_back(current_token);
                        isLastTokenFinishLexing = true;
                    } else if (current_token.stringValue.length() >= 5) { 
                    // If keyword is longer than 5 and not equal to both keyword it means that keyword is unknow.
                        Log::Logger::getInstance()->addLogToQueue(Log::LogFrameType::ERROR, "Invalid true/false keyword got: %s!", current_token.stringValue.c_str());
                        // Make a immediate return because JSON is invalid.
                        goto end_of_loop;
                    }
                };break;
                case Null:{
                    // Add new char to end of string
                    current_token.stringValue.push_back(buffer[i]);

                    // If keyword is null
                    if (current_token.stringValue.compare("null") == 0) {
                        current_token.stringValue.clear();
                        tokens.push_back(current_token);
                        isLastTokenFinishLexing = true;
                    } else if (current_token.stringValue.length() >= 4) { // If keyword is not null then keyword is unknown
                        Log::Logger::getInstance()->addLogToQueue(Log::LogFrameType::ERROR, "Invalid null keyword got: %s!", current_token.stringValue.c_str());
                        // Make a immediate return because JSON is invalid
                        goto end_of_loop;
                    }
                };break;
                default:{
                    Log::Logger::getInstance()->addLogToQueue(Log::LogFrameType::ERROR, "Type unknow, should be String, Number, Boolean or null!");
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

JSONParser::JSONValue::JSONValue(std::string* str) {
    this->type = JSONParser::JSONValueType::String;
    this->value.stringValue = str;
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
    if (!this->isBoolean()) {
        Log::Logger::getInstance()->addLogToQueue(Log::LogFrameType::WARNING, "Value is not a Boolean !");
        return false;
    }
    
    return this->value.boolValue;
}

float JSONParser::JSONValue::getFloat() {
    if (!this->isFloat()) {
        Log::Logger::getInstance()->addLogToQueue(Log::LogFrameType::WARNING, "Value is not a Float !");
        return 0.0f;
    }
    
    return this->value.floatValue;
}

int JSONParser::JSONValue::getInt() {
    if (!this->isInt()) {
        Log::Logger::getInstance()->addLogToQueue(Log::LogFrameType::WARNING, "Value is not a Int !");
        return 0;
    }
    
    return this->value.intValue;
}

int JSONParser::JSONValue::getNull() {
    if (!this->isNull())
        Log::Logger::getInstance()->addLogToQueue(Log::LogFrameType::WARNING, "Value is not NULL !");
    
    return NULL;
}

std::string JSONParser::JSONValue::getString() {
    if (!this->isString()) {
        Log::Logger::getInstance()->addLogToQueue(Log::LogFrameType::ERROR, "Value is not a String !");
        return "";
    }
    
    return *this->value.stringValue;
}

std::map<std::string, JSONParser::JSONValue>* JSONParser::JSONValue::getMap() {
    if (!this->isMap()) {
        Log::Logger::getInstance()->addLogToQueue(Log::LogFrameType::ERROR, "Value is not a Map !");
        return new std::map<std::string, JSONParser::JSONValue>();
    }
    
    return this->value.mapValue;
}

std::vector<JSONParser::JSONValue>* JSONParser::JSONValue::getArray() {
    if (!this->isArray()) {
        Log::Logger::getInstance()->addLogToQueue(Log::LogFrameType::ERROR, "Value is not an Array !");
        return new std::vector<JSONParser::JSONValue>();
    }
    
    return this->value.arrayValue;
}

std::string JSONParser::JSONValue::Serialize() const {
    switch(this->type) {
        case JSONParser::JSONValueType::Null:{
            return std::string("null");
        };break;
        case JSONParser::JSONValueType::String:{
            // Format string with "" string encapsulator.
            std::ostringstream ss;
            ss << "\"" << *this->value.stringValue << "\"";
            // Return converted ouput stream to str.
            return ss.str();
        };break;
        case JSONParser::JSONValueType::Integer:{
            // Convert raw int value to string representation.
            return std::to_string(this->value.intValue);
        };break;
        case JSONParser::JSONValueType::Float:{
            // Convert raw float value to string representation.
            // Needed to specify "target.printf_lib": "std" in mbed_app.json to work.
            return std::to_string(this->value.floatValue);
        };break;
        case JSONParser::JSONValueType::Boolean:{
            if (this->value.boolValue)
                return "true";
            else
                return "false";
        };break;
        case JSONParser::JSONValueType::Array:{
            std::ostringstream ss;
            ss << '[';
            for (int i = 0; i < this->value.arrayValue->size(); i++) {
                // Recursively get value string representation.
                ss << this->value.arrayValue->at(i).Serialize();
                // If there is more value after we need to add ',' separator.
                if (i < this->value.arrayValue->size()-1) ss << ',';
            }
            ss << ']';
            return ss.str();
        };break;
        case JSONParser::JSONValueType::Object:{
            std::ostringstream ss;
            ss << '{';
            bool isFirstPair = true;
            for (const std::pair<const std::string, JSONParser::JSONValue>& kv: *this->value.mapValue) {
                // If not the first key/value pair then add a comma as separator.
                if (isFirstPair) isFirstPair = false;
                else ss << ',';

                // Create a key/value formatting and recursively get the value string representation.
                ss << '"' << kv.first << "\":" << kv.second.Serialize();
            }
            ss << '}';
            return ss.str();
        };break;
    }
    return "";
}

JSONParser::JSONValue JSONParser::JSONValue::Deserialize(std::list<JSONLexer::JSONToken> *tokens, bool isRoot) {
    JSONParser::JSONValue value = JSONParser::JSONValue();
    // If there is no tokens then return an empty JSONValue
    if (tokens->empty()) return value;

    // If we are parsing an object
    switch (tokens->front().type) {
        case JSONLexer::JSONTokenType::StartObject:{
            std::map<std::string, JSONParser::JSONValue> map;
            tokens->pop_front();

            std::string key;
            while(tokens->front().type != JSONLexer::JSONTokenType::EndObject) {
                // Check if key is a string (should be)
                if (tokens->front().type != JSONLexer::JSONTokenType::String) {
                    Log::Logger::getInstance()->addLogToQueue(Log::LogFrameType::ERROR, "Expected key token should be string !");
                    return JSONParser::JSONValue();
                }
                // Save key value for later
                key = tokens->front().stringValue;
                tokens->pop_front();

                // Expect a Colon separator between key and value (JSON format)
                if (tokens->front().type != JSONLexer::JSONTokenType::Colon) {
                    Log::Logger::getInstance()->addLogToQueue(Log::LogFrameType::ERROR, "Expected Colon between key and value!");
                    return JSONParser::JSONValue();
                }
                tokens->pop_front();

                // Recursively get value (can be another map, array or standard type)
                map[key] = JSONParser::JSONValue::Deserialize(tokens);

                if (tokens->front().type != JSONLexer::JSONTokenType::EndObject) {
                    // After, we expect either an EndObject token or a comma (meaning that there is more entries)
                    if (tokens->front().type == JSONLexer::JSONTokenType::Comma){
                        tokens->pop_front();
                    } else {
                        Log::Logger::getInstance()->addLogToQueue(Log::LogFrameType::ERROR, "Expected Comma between entries!");
                        return JSONParser::JSONValue();
                    }
                }
            }

            value = JSONParser::JSONValue(&map);
        };break;
        case JSONLexer::JSONTokenType::StartArray: {
            std::vector<JSONParser::JSONValue> vec;
            tokens->pop_front();

            while(tokens->front().type != JSONLexer::JSONTokenType::EndArray) {
                // Recursively get value (can be another array, map or standard type)
                vec.push_back(JSONParser::JSONValue::Deserialize(tokens));

                if (tokens->front().type != JSONLexer::JSONTokenType::EndArray) {
                    // After, we expect either an EndArray token or a comma (meaning that there is more entries)
                    if (tokens->front().type == JSONLexer::JSONTokenType::Comma) {
                        tokens->pop_front();
                    } else {
                        Log::Logger::getInstance()->addLogToQueue(Log::LogFrameType::ERROR, "Expected Comma between entries!");
                        return JSONParser::JSONValue();
                    }
                }
            }

            value = JSONParser::JSONValue(&vec);
        };break;
        case JSONLexer::JSONTokenType::String: {
            value.type = JSONValueType::String;
            value.value.stringValue = new std::string(tokens->front().stringValue);
        };break;
        case JSONLexer::JSONTokenType::Boolean: {
            value.type = JSONValueType::Boolean;
            value.value.boolValue = tokens->front().boolValue;
        };break;
        case JSONLexer::JSONTokenType::Null: {
            value.type = JSONValueType::Null;
            value.value = { 0 };
        };break;
        case JSONLexer::JSONTokenType::Integer: {
            value.type = JSONValueType::Integer;
            value.value.intValue = tokens->front().intValue;
        };break;
        case JSONLexer::JSONTokenType::Float: {
            value.type = JSONValueType::Float;
            value.value.floatValue = tokens->front().floatValue;
        };break;
        default:{
            Log::Logger::getInstance()->addLogToQueue(Log::LogFrameType::ERROR, "Couldn't create JSONValue from this token !");
        }
    }
    tokens->pop_front();
    return value;
}
