#include "mbed.h"
#include "json_parser.hpp"
#include "logger.hpp"

#include <string>
#include <list>

#define READ_BUFFER_LENGTH 64

int length;
char previous_read_buffer[READ_BUFFER_LENGTH] = {0};
char read_buffer[READ_BUFFER_LENGTH] = {0};

PwmOut led(LED1);
BufferedSerial pc(USBTX, USBRX, 115200);

#ifdef MBED_DEBUG
Log::Logger logger(&pc, Log::LogFrameType::DEBUG);
#else 
Log::Logger logger(&pc, Log::LogFrameType::ERROR);
#endif

Thread thread;
void watchdog_thread(){
    while (true) {
        logger.flushLogToSerial();
        ThisThread::sleep_for(10ms);
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
            if (value.isMap()) {
                if (value.getMap()->count("led")) {
                    int val = value.getMap()->at("led").getInt();
                    logger.addLogToQueue(Log::LogFrameType::INFO, "Change LED value %d !", val);
                    led.write((float)val / 255);
                }
                if (value.getMap()->count("a")) {
                    JSONParser::JSONValue a = value.getMap()->at("a");         
                    for(int i = 0; i < a.getArray()->size(); i++) {
                        int val = a.getArray()->at(i).getInt();
                        logger.addLogToQueue(Log::LogFrameType::DEBUG, "Found int %d at %d !", val, i);
                        ThisThread::sleep_for(100ms);
                    }
                }
            } else if (value.isArray()) {
                for(int i = 0; i < value.getArray()->size(); i++) {
                    int val = value.getArray()->at(i).getInt();
                    logger.addLogToQueue(Log::LogFrameType::DEBUG, "Found int %d at %d !\r\n", val, i);
                    ThisThread::sleep_for(100ms);
                }
            }

            logger.addLogToQueue(Log::LogFrameType::DEBUG, "End Parsing !");

            lexer_tokens.clear();
            pc.set_blocking(true);
        }
    }
}

