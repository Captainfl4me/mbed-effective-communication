#include <stdlib.h>
#include <list>
#include <string>

enum JSONTokenType { 
    StartObject,
    EndObject,
    StartArray,
    EndArray,
    Comma,
    Colon,

    String,
    Boolean,
    Number,
    Null
};

struct JSONToken {
    JSONTokenType type;

    std::string stringValue;
    bool boolValue;
    int intValue;
};

struct LexerResult {
    std::list<JSONToken> tokens;
    bool isLastTokenFinishLexing = false;
    size_t lastTokenStartIndex = 0;
};

LexerResult JsonLexer(char* buffer, int buffer_length);

/*
{
    "a" : [1, 2, 3],
    "b" : {
        "a" : "1"
    }
}

*/