// Calla Chen
// Source Code File 8/11 for EECS 111 Project #3
#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include "log_global.h"

extern std::ofstream fullLog;

class Logger {
public:
    enum Level { INFO, WARN, ERROR };

    // Initializes logger and opens log file for writing
    static void init(const std::string& filePath = "logs/events.log");

    // Logs a message to the file with a specified level
    static void log(const std::string& message, Level level = INFO);

    // Creating initializer for full_session.txt
    static void initFullSessionLog();

    // Finalizes the logging session (writing end timestamp and closes file)
    static void close();

    // CSV logging methods
    static void logSummaryCSV(const SessionStats& stats, const std::string& path = "logs/log_summary.csv");
    static void logCustomerCSV(int customerID, int waitTime, int retryCount, int arrivalTime, int turnaroundTime, const std::string& path = "logs/per_customer_log.csv");
    static void logRequestHeatmap();

    // Session summary txt file
    static void logSessionTXT(const SessionStats& stats, const std::string& path = "logs/session_summary.txt");

private:
    static std::ofstream logFile; // File stream for logging messages
};

#endif // LOGGER_H
