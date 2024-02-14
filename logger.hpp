#pragma once
#include "mbed.h"
#include <string>
#include <list>
#include <vector>

#define LOG_BUFFER_LENGTH 256

namespace Log {
    enum LogFrameType {
        DEBUG = 0,
        INFO = 1,
        WARNING = 2,
        ERROR = 3
    };

    struct LoggerFrame {
        std::chrono::milliseconds timestamp;
        LogFrameType type;
        std::string msg;
    };

    class Logger {
        std::list<LoggerFrame> log_queue;
        LogFrameType log_level;
        BufferedSerial *pbs;
        char log_buffer[LOG_BUFFER_LENGTH] = {0};
        static Logger* instance;
    public:
        Logger(BufferedSerial *pbs);
        Logger(BufferedSerial *pbs, LogFrameType log_level);

        template<typename ... Args>
        void addLogToQueue(LogFrameType type, const std::string& format, Args ... args);
        void flushLogToSerial();
        static Logger* getInstance();
    };
    
    template<typename ... Args>
    void Logger::addLogToQueue(LogFrameType type, const std::string& format, Args ... args) {
        if (type >= this->log_level) {
            int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...);
            if (size_s > 0) {
                std::vector<char> buf(size_s + 1); // note +1 for null terminator
                std::sprintf(buf.data(), format.c_str(), args ...); // certain to fit

                LoggerFrame frame;
                frame.timestamp = Kernel::Clock::now().time_since_epoch();
                frame.type = type;
                frame.msg = std::string(buf.data(), buf.size());

                this->log_queue.push_back(frame);
            }
        }
    }
}