/* Main program entry for Effective Communication project
 * This code is implemented using Mbed OS 6.17.0
 *
 * Author: Nicolas THIERRY
 */
#include "mbed.h"
#include "json_parser.hpp"
#include "logger.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
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

struct State{
    int mode = 0;
    float led_value = 0.0f;
};
State current_state;

void write_builtin_led(float value) {
    led.write(value);
    current_state.led_value = led.read();
}

Thread thread;
void watchdog_thread(){
    while (true) {
        // Write all log in queue to the output stream (serial communication).
        logger.flushLogToSerial();
        ThisThread::sleep_for(10ms);
    }
}
Thread *blink_thread;
void blink_loop(){
    // Convert float representing seconds to chrono object with milliseconds precision.
    auto delay = round<std::chrono::milliseconds>(std::chrono::duration<double>{blinkSeconds});
    while (true) {
        write_builtin_led(1.0f);
        ThisThread::sleep_for(delay);
        write_builtin_led(0.0f);
        ThisThread::sleep_for(delay);
    }
}

// main() runs in its own thread in the OS
int main()
{
    // Start watchdog thread, will flush the log queue.
    thread.start(callback(watchdog_thread));

    // Set blocking to serial connection so that its wait for input to be received.
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
                    // Starting point of the read buffer after slicing it
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
                        // Handling non finish lexing
                        previous_buffer_length = read_length - lex_result.lastTokenStartIndex;

                        // shift non-lex char to beginning of the buffer.
                        std::memmove(previous_read_buffer, previous_read_buffer + lex_result.lastTokenStartIndex, previous_buffer_length);
                    }
                    
                    // DEBUG: Show what is the ouput of the Lexer
                    logger.addLogToQueue(Log::LogFrameType::DEBUG, "Tokens_len = %d | isFinishLexing = %d | lastTokenIndex = %d",  lex_result.tokens.size(), lex_result.isLastTokenFinishLexing, lex_result.lastTokenStartIndex);

                    // Save result list by merging it into tokens list 
                    lexer_tokens.splice(lexer_tokens.end(), lex_result.tokens);
                }

                ThisThread::sleep_for(50ms);
                // Set non-blocking so that we can check if there is no more data to read.
                pc.set_blocking(false);
            }while((read_length = pc.read(read_buffer, READ_BUFFER_LENGTH)) != -EAGAIN);

            logger.addLogToQueue(Log::LogFrameType::DEBUG, "End Lexing: tokens = %d !", lexer_tokens.size());

            JSONParser::JSONValue value = JSONParser::JSONValue::Deserialize(&lexer_tokens);

            // Create JSON response object from empty map
            JSONParser::JSONValue response(new std::map<std::string, JSONParser::JSONValue>());

            // Handling JSON request
            if (value.isMap()) {
                auto rootMap = value.getMap();
                if (rootMap->count("mode") && rootMap->at("mode").isInt()) {
                    // Get requested mode
                    int mode = rootMap->at("mode").getInt();

                    // If blink thread was previously running we stop it
                    if (blink_thread != NULL) {
                        blink_thread->terminate();
                        delete blink_thread;
                        // Unassigned now dangling pointer.
                        blink_thread = NULL;
                        current_state.led_value = 0.0f;
                        write_builtin_led(0.0f);
                        
                        logger.addLogToQueue(Log::LogFrameType::INFO, "Terminate blink thread!");
                    }
                    switch (mode) {
                        case 0:{
                            if(rootMap->count("on") && rootMap->at("on").isBoolean()) {
                                // Set led state to 1 (on) if on is true, else set led state to 0 (off) 
                                write_builtin_led(rootMap->at("on").getBoolean() ? 1 : 0);
                                current_state.mode = 0;
                            } else {
                                logger.addLogToQueue(Log::LogFrameType::ERROR, "Mode 0 expect boolean \\\"on\\\" to be defined!");
                                // Insert err message in response object
                                response.getMap()->insert(std::pair<std::string, JSONParser::JSONValue>("err", JSONParser::JSONValue(new std::string("Mode 0 expect boolean \\\"on\\\" to be defined."))));
                            }
                        };break;
                        case 1:{
                            if(rootMap->count("v") && (rootMap->at("v").isFloat())) {
                                float val = rootMap->at("v").getFloat();
                                if (val >= 0.0f && val <= 1.0f) {
                                    write_builtin_led(val);
                                    current_state.mode = 1;
                                } else {
                                    logger.addLogToQueue(Log::LogFrameType::ERROR, "Mode 1 expect float \\\"v\\\" to be between 0 and 1!");
                                    // Insert err message in response object
                                    response.getMap()->insert(std::pair<std::string,JSONParser::JSONValue>("err", JSONParser::JSONValue(new std::string("Mode 1 expect float \\\"v\\\" to be between 0 and 1."))));
                                }
                            } else {
                                logger.addLogToQueue(Log::LogFrameType::ERROR, "Mode 1 expect float \\\"v\\\" to be defined!");
                                // Insert err message in response object
                                response.getMap()->insert(std::pair<std::string,JSONParser::JSONValue>("err", JSONParser::JSONValue(new std::string("Mode 1 expect float \\\"v\\\" to be defined."))));
                            }
                        };break;
                        case 2:{
                            if(rootMap->count("d") && (rootMap->at("d").isFloat())) {
                                blinkSeconds = rootMap->at("d").getFloat();
                                // Create new work thread to asynchronously run our blink command.
                                blink_thread = new Thread();
                                blink_thread->start(callback(blink_loop));
                                current_state.mode = 2;
                            } else {
                                logger.addLogToQueue(Log::LogFrameType::ERROR, "Mode 2 expect float \\\"d\\\" to be defined!");
                                // Insert err message in response object
                                response.getMap()->insert(std::pair<std::string,JSONParser::JSONValue>("err", JSONParser::JSONValue(new std::string("Mode 2 expect float \\\"d\\\" to be defined."))));
                            }
                        };break;
                        default:{
                            // Insert err message in response object
                            response.getMap()->insert(std::pair<std::string, JSONParser::JSONValue>("err", JSONParser::JSONValue(new std::string("Unknown mode."))));
                        };break;
                    }
                }
                if (rootMap->count("req") && rootMap->at("req").isInt()) {
                    int request = rootMap->at("req").getInt();
                    switch (request) {
                        case 0:{
                            JSONParser::JSONValue status(new std::map<std::string, JSONParser::JSONValue>);

                            status.getMap()->insert(std::pair<std::string, JSONParser::JSONValue>("mode", JSONParser::JSONValue(current_state.mode)));
                            status.getMap()->insert(std::pair<std::string, JSONParser::JSONValue>("led", JSONParser::JSONValue(current_state.led_value)));
                            
                            // Insert status message in response object
                            response.getMap()->insert(std::pair<std::string, JSONParser::JSONValue>("status", status));
                        };break;                     
                        default:{
                            // Insert err message in response object
                            response.getMap()->insert(std::pair<std::string, JSONParser::JSONValue>("err", JSONParser::JSONValue(new std::string("Unknown request."))));
                        };break;
                    }
                }
            }
            // Output string formatted JSON message.
            logger.addLogToQueue(Log::LogFrameType::RELEASE, response.Serialize());

            logger.addLogToQueue(Log::LogFrameType::INFO, "End Parsing obj: %s !", value.Serialize().c_str());

            // Clear tokens list for the next input.
            lexer_tokens.clear();
            // Set to blocking to wait for new message.
            pc.set_blocking(true);
        }
    }
}
