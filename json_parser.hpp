#pragma once

#include <stdlib.h>
#include <list>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include "logger.hpp"

namespace JSONLexer {
    enum JSONTokenType { 
        StartObject,
        EndObject,
        StartArray,
        EndArray,
        Comma,
        Colon,

        String,
        Boolean,
        Integer,
        Float,
        Null
    };

    struct JSONToken {
        JSONTokenType type;

        std::string stringValue;
        bool boolValue;
        int intValue;
        float floatValue;
    };

    struct LexerResult {
        std::list<JSONToken> tokens;
        bool isLastTokenFinishLexing = false;
        size_t lastTokenStartIndex = 0;
    };

    LexerResult LexBuffer(char* buffer, int buffer_length);
}

namespace JSONParser {
    class JSONValue;
    union JSONValueMemory {
        bool boolValue;
        int intValue;
        float floatValue;

        std::string* stringValue;
        std::vector<JSONValue>* arrayValue;
        std::map<std::string, JSONValue>* mapValue;
    };
    enum JSONValueType {
        Object,
        Array,
        String,
        Boolean,
        Integer,
        Float,
        Null
    };

    class JSONValue {
        JSONValueType type;
        JSONValueMemory value;

    public:
        JSONValue();
        JSONValue(JSONLexer::JSONToken *token);
        JSONValue(std::map<std::string, JSONParser::JSONValue> *map);
        JSONValue(std::vector<JSONParser::JSONValue> *vec);

        bool isBoolean();
        bool isInt();
        bool isFloat();
        bool isString();
        bool isNull();
        bool isMap();
        bool isArray();

        bool getBoolean();
        int getInt();
        float getFloat();
        std::string getString();
        int getNull();

        std::map<std::string, JSONParser::JSONValue>* getMap();
        std::vector<JSONParser::JSONValue>* getArray();

        std::string Serialize() const;
        static JSONValue Deserialize(std::list<JSONLexer::JSONToken> *tokens);
    };
}
