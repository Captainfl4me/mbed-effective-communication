/* JSON parser utils library
 * This library implemented dynamic JSON support for C++11.
 * Aiming for serialization, deserialization and representation.
 *
 * Author: Nicolas THIERRY
 */
#pragma once

#include <stdlib.h>
#include <list>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include "logger.hpp"

namespace JSONLexer {
    //Enum type of tokens possible for the Lexer.
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

    // Lexer token. If type is either: String, Boolean, Integer or Float, then read value is stored in proper attribute matching type.
    struct JSONToken {
        JSONTokenType type;

        std::string stringValue;
        bool boolValue;
        int intValue;
        float floatValue;
    };

    //Result of a LexBuffer operation. Give information about state of lexer at return. If isLastTokenFinishLexing = false, it means that either the provided buffer does not contains the entire buffer or the JSON message is not valid. In first case, you may want to retry the lexing with a what is left in the previous buffer (from lastTokenStartIndex to end) and a new buffer.
    struct LexerResult {
        std::list<JSONToken> tokens;
        bool isLastTokenFinishLexing = false;
        size_t lastTokenStartIndex = 0;
    };

    /** Try to create an intermediate representation (tokenize) from the given buffer.
    *
    *  @param buffer Input buffer.
    *  @param buffer_length Size of input buffer.
    */
    LexerResult LexBuffer(char* buffer, int buffer_length);
}

namespace JSONParser {
    class JSONValue;

    // Union of possibles values for a JSON entry.
    union JSONValueMemory {
        bool boolValue;
        int intValue;
        float floatValue;

        std::string* stringValue;
        std::vector<JSONValue>* arrayValue;
        std::map<std::string, JSONValue>* mapValue;
    };

    // Enum for possible JSON value union
    enum JSONValueType {
        Object,
        Array,
        String,
        Boolean,
        Integer,
        Float,
        Null
    };

    // JSONValue is a class representing a JSON entry. It store the actual value to memory through a union to prevent taking too much memory and is type safe to prevent reading non-sense data.
    class JSONValue {
        JSONValueType type;
        JSONValueMemory value;

    public:
        /** Default constructor for JSONValue.
        *
        * @return JSONValue of type Null.
        */
        JSONValue();

        /** Construct JSONValue from std::string.
        *
        * @param str reference to std::string.
        * @return JSONValue of type String.
        */
        JSONValue(std::string* str);

        /** Construct JSONValue from int.
        *
        * @param i int value.
        * @return JSONValue of type Integer.
        */
        JSONValue(int i);

        /** Construct JSONValue from float.
        *
        * @param f float value.
        * @return JSONValue of type Float.
        */
        JSONValue(float f);

        /** Construct JSONValue from bool.
        *
        * @param b bool value.
        * @return JSONValue of type Boolean.
        */
        JSONValue(bool b);

        /** Construct JSONValue from std::map.
        *
        * @param map reference to std::map.
        * @return JSONValue of type Object.
        */
        JSONValue(std::map<std::string, JSONParser::JSONValue> *map);

        /** Construct JSONValue from std::vector.
        *
        * @param vec reference to std::vector.
        * @return JSONValue of type Array.
        */
        JSONValue(std::vector<JSONParser::JSONValue> *vec);

        bool isBoolean();
        bool isInt();
        bool isFloat();
        bool isString();
        bool isNull();
        bool isMap();
        bool isArray();

        /** Return boolean representation of value.
        *
        * @return boolean value if type is boolean else return false and add WARNING to log.
        */
        bool getBoolean();

        /** Return int representation of value.
        *
        * @return int value if type is int else return 0 and add WARNING to log.
        */
        int getInt();

        /** Return float representation of value.
        *
        * @return float value if type is float else return 0.0f and add WARNING to log.
        */
        float getFloat();

        /** Return std::string representation of value.
        *
        * @return string value if type is std::string else return empty string "" and add WARNING to log.
        */
        std::string getString();

        /** Return null representation of value.
        *
        * @return null value if type is null else return NULL and add WARNING to log.
        */
        int getNull();

        /** Return std::map representation of value.
        *
        * @return map pointer if type is Object else return empty map and add WARNING to log.
        */
        std::map<std::string, JSONParser::JSONValue>* getMap();

        /** Return std::vector representation of value.
        *
        * @return vector pointer if type is Array else return empty vector and add WARNING to log.
        */
        std::vector<JSONParser::JSONValue>* getArray();

        /** Serialize the current JSONValue into a string representation.
        *
        * @return std::string representation of object without any formating, line return character or carriage return.
        */
        std::string Serialize() const;
        /** Deserialize JSON message from list of tokens from the lexer.
        *
        * @param tokens reference to list of tokens. List will be consummed by the function.
        * @param isRoot (optional) toggle the check if message is an array or an object and so can be used as a root. This requirements is imposed by JSON format.
        * @return JSONValue with nested values from the list of tokens.
        */
        static JSONValue Deserialize(std::list<JSONLexer::JSONToken> *tokens, bool isRoot = true);
    };
}
