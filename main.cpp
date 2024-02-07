#include "mbed.h"
#include "json_parser.hpp"

#include <string>
#include <list>

#define READ_BUFFER_LENGTH 64
#define LOG_BUFFER_LENGTH 256

int length;
char previous_read_buffer[READ_BUFFER_LENGTH] = {0};
char read_buffer[READ_BUFFER_LENGTH] = {0};
char log_buffer[LOG_BUFFER_LENGTH] = {0};

PwmOut led(LED1);

// main() runs in its own thread in the OS
int main()
{
    // Initialize serial bus
    BufferedSerial pc(USBTX, USBRX, 115200);
    pc.set_blocking(true);
    
    length = snprintf(log_buffer, LOG_BUFFER_LENGTH, "Hello !\r\n");
    pc.write(log_buffer, length);

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
                length = snprintf(log_buffer, LOG_BUFFER_LENGTH, "[DEBUG]: buff: %.*s (len: %d)\r\n", read_length, read_buffer, read_length);
                pc.write(log_buffer, length);

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
                    length = snprintf(log_buffer, LOG_BUFFER_LENGTH, "[DEBUG]: lex: %.*s (len: %d)\r\n", previous_buffer_length, previous_read_buffer, previous_buffer_length);
                    pc.write(log_buffer, length);

                    JSONLexer::LexerResult lex_result = JSONLexer::LexBuffer(previous_read_buffer, previous_buffer_length);
                    
                    if (lex_result.isLastTokenFinishLexing) {
                        previous_buffer_length = 0;
                    } else {
                        previous_buffer_length = read_length - lex_result.lastTokenStartIndex;

                        std::memmove(previous_read_buffer, previous_read_buffer + lex_result.lastTokenStartIndex, previous_buffer_length);
                    }
                    
                    // // DEBUG: Show what is the ouput of the Lexer
                    length = snprintf(log_buffer, LOG_BUFFER_LENGTH, "[DEBUG]: Tokens_len = %d | isFinishLexing = %d | lastTokenIndex = %d\r\n",  lex_result.tokens.size(), lex_result.isLastTokenFinishLexing, lex_result.lastTokenStartIndex);
                    pc.write(log_buffer, length);

                    while (!lex_result.tokens.empty()) {
                        lexer_tokens.push_back(lex_result.tokens.front());
                        lex_result.tokens.pop_front();
                    }
                }

                ThisThread::sleep_for(50ms);
                pc.set_blocking(false);
            }while((read_length = pc.read(read_buffer, READ_BUFFER_LENGTH)) != -EAGAIN);

            length = snprintf(log_buffer, LOG_BUFFER_LENGTH, "[DEBUG]: End Lexing: tokens = %d !\r\n", lexer_tokens.size());
            pc.write(log_buffer, length);

            JSONParser::JSONValue map = JSONParser::ParseTokens(lexer_tokens);
            if (map.getMap()->count("led")) {
                int val = map.getMap()->at("led").getInt();
                length = snprintf(log_buffer, LOG_BUFFER_LENGTH, "[DEBUG]: Change LED value %d !\r\n", val);
                pc.write(log_buffer, length);
                led.write((float)val / 255);
            }

            length = snprintf(log_buffer, LOG_BUFFER_LENGTH, "[DEBUG]: End Parsing !\r\n");
            pc.write(log_buffer, length);

            lexer_tokens.clear();
            pc.set_blocking(true);
        }
    }
}

