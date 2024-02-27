#include "logger.hpp"

using namespace Log;
Logger* Logger::instance = NULL;

Logger::Logger(BufferedSerial *pbs): pbs(pbs), log_level(LogFrameType::ERROR) {
    Logger::instance = this;
};
Logger::Logger(BufferedSerial *pbs, LogFrameType log_level): pbs(pbs), log_level(log_level) {
    Logger::instance = this;
};


void Logger::flushLogToSerial() {
    while (!log_queue.empty()) {
        if (this->pbs->writable()) {
            std::string bufToWrite;
            Log::LoggerFrame log = this->log_queue.front();

            if (log.type < LogFrameType::RELEASE)
                bufToWrite.append("[");
            switch (log.type) {
                case LogFrameType::DEBUG:
                    bufToWrite.append("DEBUG");
                    break;
                case LogFrameType::INFO:
                    bufToWrite.append("INFO");
                    break;
                case LogFrameType::WARNING:
                    bufToWrite.append("WARNING");
                    break;
                case LogFrameType::ERROR:
                    bufToWrite.append("ERROR");
                    break;
                default:break;
            }
            if (log.type < LogFrameType::RELEASE)
                bufToWrite.append("] -> ");
            bufToWrite.append(log.msg);
            bufToWrite.append("\r\n");

            this->pbs->write(bufToWrite.c_str(), bufToWrite.size());
            this->log_queue.pop_front();
        } else {
            break;
        }
    }
}

Logger* Logger::getInstance() {
    return Logger::instance;
}