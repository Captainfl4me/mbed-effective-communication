#pragma once
#include "mbed.h"
#include <string>
#include <list>
#include <vector>

#define LOG_BUFFER_LENGTH 256

namespace Log {
    // Log frame possible types. Level of logs affect displayed informations and filters. Note that RELEASE is the only flag that provides no formatting at all and leave the ouput unchanged. 
    enum LogFrameType {
        DEBUG = 0,
        INFO = 1,
        WARNING = 2,
        ERROR = 3,
        RELEASE = 4,
    };

    // Struct that represent a log frame. A frame is generated when using the addLogToQueue function. This struct is not meant to be use externally.
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
        /** Constructor of Logger. The current instance will be use to populate singleton reference.
        *
        * @param pbs reference to a BufferedSerial communication where frames will be flush.
        */
        Logger(BufferedSerial *pbs);

        /** Constructor of Logger. The current instance will be use to populate singleton reference.
        *
        * @param pbs reference to a BufferedSerial communication where frames will be flush.
        * @param log_level minimal level below which log entries will be ignored. Log with level equal to log_level will be kept.
        */
        Logger(BufferedSerial *pbs, LogFrameType log_level);

    
        /** Create log frame from parameters and push it to the queue. This function act like printf.
        *
        * @param type is the log level. Any log below the defined filter will be ignored.
        * @param format is the format string.
        * @param args are the arguments that need to be provided to format the string. 
        */
        template<typename ... Args>
        void addLogToQueue(LogFrameType type, const std::string& format, Args ... args);

        // Empty the log queue by outputting all waiting frames to the define output stream.
        void flushLogToSerial();

        // Singleton to the current Logger instance. Be carefull when using it, pointer may be undefined or dangling.
        static Logger* getInstance();
    };
    template<typename ... Args>
    void Logger::addLogToQueue(LogFrameType type, const std::string& format, Args ... args) {
        if (type >= this->log_level) {
            // snprintf without output buffer to precomputed necessary space.
            int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...);
            if (size_s > 0) {
                // Creating temp buffer with right size. Note +1 for null terminator
                std::vector<char> buf(size_s + 1); 
                // Insert formatted string inside buffer
                std::sprintf(buf.data(), format.c_str(), args ...);

                LoggerFrame frame;
                frame.timestamp = Kernel::Clock::now().time_since_epoch();
                frame.type = type;
                frame.msg = std::string(buf.data(), buf.size());

                // Push frame to queue waiting to be flush.
                this->log_queue.push_back(frame);
            }
        }
    }
}