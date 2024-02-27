#include "mbed.h"
#include "json_parser.hpp"
#include "logger.hpp"

#include <chrono>
#include <cstddef>
#include <string>
#include <list>

#include "chrono_utils.hpp"

#define READ_BUFFER_LENGTH 64

int length;
char previous_read_buffer[READ_BUFFER_LENGTH] = {0};
char read_buffer[READ_BUFFER_LENGTH] = {0};

PwmOut led(LED1);
float blinkSeconds = 1.0f;
BufferedSerial pc(USBTX, USBRX, 115200);

#ifdef MBED_DEBUG
Log::Logger logger(&pc, Log::LogFrameType::DEBUG);
#else 
Log::Logger logger(&pc, Log::LogFrameType::RELEASE);
#endif

Thread thread;
void watchdog_thread(){
    while (true) {
        logger.flushLogToSerial();
        ThisThread::sleep_for(10ms);
    }
}
Thread *blink_thread;
void blink_loop(){
    auto delay = round<std::chrono::milliseconds>(std::chrono::duration<double>{blinkSeconds});
    while (true) {
        led.write(1.0f);
        ThisThread::sleep_for(delay);
        led.write(0.0f);
        ThisThread::sleep_for(delay);
    }
}

// main() runs in its own thread in the OS
int main()
{
    thread.start(callback(watchdog_thread));

    pc.set_blocking(true);
    logger.addLogToQueue(Log::LogFrameType::INFO, "Program started!");

    size_t read_length = -EAGAIN;
    size_t previous_buffer_length = 0;
    std::list<JSONLexer::JSONToken> lexer_tokens;
    while (true) {
        // if we read something
        if ((read_length = pc.read(read_buffer, READ_BUFFER_LENGTH)) != -EAGAIN){
            do {
                // Sanitize buffer by removing last \r and \n
                while(read_length >= 0 && (read_buffer[read_length - 1] == '\r' || read_buffer[read_length - 1] == '\n')) {
                    read_length -= 1;
                }
                if (read_length <= 0) continue; 

                // DEBUG: write readed buffer
                logger.addLogToQueue(Log::LogFrameType::DEBUG, "buff: %.*s (len: %d)", read_length, read_buffer, read_length);

                int left_in_read_buffer = read_length;
                while(left_in_read_buffer > 0) {
                    // Starting point of read buffer after slicing it
                    size_t read_buffer_offset = read_length - left_in_read_buffer;
                    // Get size of char left when we merge new buffer with previous one
                    left_in_read_buffer = previous_buffer_length + read_length - READ_BUFFER_LENGTH;
                    if (left_in_read_buffer < 0) left_in_read_buffer = 0;

                    // Merging previous buffer and new one
                    std::memcpy(previous_read_buffer + previous_buffer_length, read_buffer + read_buffer_offset, read_length - read_buffer_offset - left_in_read_buffer);
                    previous_buffer_length = previous_buffer_length + read_length - read_buffer_offset - left_in_read_buffer;

                    // DEBUG: Show what is the input buffer of the lexer
                    logger.addLogToQueue(Log::LogFrameType::DEBUG, "lex: %.*s (len: %d)", previous_buffer_length, previous_read_buffer, previous_buffer_length);

                    JSONLexer::LexerResult lex_result = JSONLexer::LexBuffer(previous_read_buffer, previous_buffer_length);
                    
                    if (lex_result.isLastTokenFinishLexing) {
                        previous_buffer_length = 0;
                    } else {
                        previous_buffer_length = read_length - lex_result.lastTokenStartIndex;

                        std::memmove(previous_read_buffer, previous_read_buffer + lex_result.lastTokenStartIndex, previous_buffer_length);
                    }
                    
                    // // DEBUG: Show what is the ouput of the Lexer
                    logger.addLogToQueue(Log::LogFrameType::DEBUG, "Tokens_len = %d | isFinishLexing = %d | lastTokenIndex = %d",  lex_result.tokens.size(), lex_result.isLastTokenFinishLexing, lex_result.lastTokenStartIndex);

                    while (!lex_result.tokens.empty()) {
                        lexer_tokens.push_back(lex_result.tokens.front());
                        lex_result.tokens.pop_front();
                    }
                }

                ThisThread::sleep_for(50ms);
                pc.set_blocking(false);
            }while((read_length = pc.read(read_buffer, READ_BUFFER_LENGTH)) != -EAGAIN);

            logger.addLogToQueue(Log::LogFrameType::DEBUG, "End Lexing: tokens = %d !", lexer_tokens.size());

            JSONParser::JSONValue value = JSONParser::JSONValue::Deserialize(&lexer_tokens);

            // Handling JSON request
            JSONParser::JSONValue response(new std::map<std::string, JSONParser::JSONValue>());
            if (value.isMap()) {
                auto rootMap = value.getMap();
                if (rootMap->count("mode") && rootMap->at("mode").isInt()) {
                    int mode = rootMap->at("mode").getInt();

                    if (blink_thread != NULL) {
                        blink_thread->terminate();
                        delete blink_thread;
                        blink_thread = NULL;
                        led.write(0.0f);
                        
                        logger.addLogToQueue(Log::LogFrameType::INFO, "Terminate blink thread!");
                    }
                    switch (mode) {
                        case 0:{
                            if(rootMap->count("on") && rootMap->at("on").isBoolean()) {
                                led.write(rootMap->at("on").getBoolean() ? 1 : 0);
                            } else {
                                logger.addLogToQueue(Log::LogFrameType::ERROR, "Mode 0 expect boolean \\\"on\\\" to be defined!");
                                response.getMap()->insert(std::pair<std::string, JSONParser::JSONValue>("err", JSONParser::JSONValue(new std::string("Mode 0 expect boolean \\\"on\\\" to be defined."))));
                            }
                        };break;
                        case 1:{
                            if(rootMap->count("v") && (rootMap->at("v").isFloat())) {
                                float val = rootMap->at("v").getFloat();
                                if (val >= 0.0f && val <= 1.0f) {
                                    led.write(val);
                                } else {
                                    logger.addLogToQueue(Log::LogFrameType::ERROR, "Mode 1 expect float \\\"v\\\" to be between 0 and 1!");
                                    response.getMap()->insert(std::pair<std::string,JSONParser::JSONValue>("err", JSONParser::JSONValue(new std::string("Mode 1 expect float \\\"v\\\" to be between 0 and 1."))));
                                }
                            } else {
                                logger.addLogToQueue(Log::LogFrameType::ERROR, "Mode 1 expect float \\\"v\\\" to be defined!");
                                response.getMap()->insert(std::pair<std::string,JSONParser::JSONValue>("err", JSONParser::JSONValue(new std::string("Mode 1 expect float \\\"v\\\" to be defined."))));
                            }
                        };break;
                        case 2:{
                            if(rootMap->count("d") && (rootMap->at("d").isFloat())) {
                                blinkSeconds = rootMap->at("d").getFloat();
                                blink_thread = new Thread();
                                blink_thread->start(callback(blink_loop));
                            } else {
                                logger.addLogToQueue(Log::LogFrameType::ERROR, "Mode 2 expect float \\\"d\\\" to be defined!");
                                response.getMap()->insert(std::pair<std::string,JSONParser::JSONValue>("err", JSONParser::JSONValue(new std::string("Mode 2 expect float \\\"d\\\" to be defined."))));
                            }
                        };break;
                        default:{
                            response.getMap()->insert(std::pair<std::string, JSONParser::JSONValue>("err", JSONParser::JSONValue(new std::string("Unknown mode."))));
                        };break;
                    }
                }
            }
            logger.addLogToQueue(Log::LogFrameType::RELEASE, response.Serialize());

            logger.addLogToQueue(Log::LogFrameType::INFO, "End Parsing obj: %s !", value.Serialize().c_str());

            lexer_tokens.clear();
            pc.set_blocking(true);
        }
    }
}
