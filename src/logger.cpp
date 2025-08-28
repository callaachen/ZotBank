// Calla Chen
// Source Code File 7/11 for EECS 111 Project #3
#include "logger.h"
#include "log_global.h"
#include <iostream>
#include <ctime>
#include <iomanip>
#include <fstream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

using namespace std;

// Comparator for sorting command usage by count (descending)
struct CommandUsageComparator {
    bool operator()(const std::pair<std::string, int>& a,
                    const std::pair<std::string, int>& b) const {
        return a.second > b.second;
    }
};

// Define the static member variable
ofstream Logger::logFile;

/**
* @brief Initializes the logging system.
*
* Creates the "logs" directory if it does not already exist and opens the specified file for writing log messages. A
* timestamp is also written to the file.
*
* @param filePath The path to the log file to open. Defaults to "logs/events.log".
*/
void Logger::init(const string& filePath) {
    // Create logs directory if it doesn't exist
    mkdir("logs", 0777);

    // Open the specified log file for writing
    logFile.open(filePath.c_str(), ios::out);
    if (logFile.is_open()) {
        // Write a timestamp marking the start of the log
        time_t now = time(NULL);
        char* dt = ctime(&now);
        logFile << "[LOG STARTED] " << dt;
    }
}

/**
* @brief Logs a message with a specified level
*
* Writes the log message to both the terminal and opened log file. Each message is prefixed with a timestamp and the
* severity level.
*
* @param messge the server to be logged.
* @param level The severity level (INFO, WARN, ERROR). Defaults to INFO
*/
void Logger::log(const string& message, Level level) {
    if (!logFile.is_open()) return; // Skip if log file isn't open

    // Get current time
    time_t now = time(NULL);
    tm* timeinfo = localtime(&now);

    char timeStr[9]; // formated in HH:MM:SS
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);

    // Prefix based on log level
    string prefix;
    switch (level) {
        case INFO:  prefix = "[INFO] "; break;
        case WARN:  prefix = "[WARN] "; break;
        case ERROR: prefix = "[ERROR] "; break;
        default:    prefix = "[INFO] "; break;
    }

    // Write to log file
    logFile << "[" << timeStr << "] " << prefix << message << endl;

    // Also print to terminal
    string color;
    switch (level) {
        case INFO:  color = COLOR_GREEN; break;
        case WARN:  color = COLOR_YELLOW; break;
        case ERROR: color = COLOR_RED; break;
        default:    color = COLOR_RESET; break;
    }

    cout << color << prefix << message << COLOR_RESET << endl;
}

/**
* @brief Finalizes the logging session.
*
* Writing a closing timestamp to the log file and closes the stream. Should be called before the program exits to
* properly terminate logging.
*/
void Logger::close() {
    if (logFile.is_open()) {
        // Write a timestamp marking the end of the log
        time_t now = time(NULL);
        char* dt = ctime(&now);
        logFile << "[LOG ENDED] " << dt << endl;
        logFile.close();
    }
}

void Logger::initFullSessionLog() {
    mkdir("logs", 0777);  // ensure logs/ exists
    fullLog.open("logs/full_session.txt", ios::out | ios::trunc);

    if (!fullLog.is_open()) {
        cerr << "Failed to open logs/full_session.txt" << endl;
    } else {
        fullLog << "[FULL LOG STARTED]\n";
        fullLog.flush();
    }
}

void Logger::logSummaryCSV(const SessionStats& stats, const string& path) {
    ofstream out(path.c_str());
    if (!out) {
        log("Failed to open " + path + " for summary CSV logging", Logger::WARN);
        return;
    }

    out << "Timestamp,Total Requests,Total Releases,Total Denied,Safe Requests,Unsafe Requests,";
    out << "Denied Need,Denied Availability,Denied Unsafe,";
    out << "RQ,RL,*,safety,reset,report,explain,undo,history,help,verbose,color,snapshot,load,save,exit,unknown\n";

    time_t now = time(NULL);
    tm* tm_info = localtime(&now);
    char buffer[20];  // enough for "YYYY-MM-DD HH:MM:SS"
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    out << buffer << ",";

    out << stats.totalRequests << "," << stats.totalReleases << "," << stats.totalDenied << ",";
    out << stats.safeRequests << "," << stats.unsafeRequests << ",";
    out << stats.deniedNeed << "," << stats.deniedAvailability << "," << stats.deniedUnsafe << ",";
    out << stats.countRQ << "," << stats.countRL << "," << stats.countStar << "," << stats.countSafety << ","
        << stats.countReset << "," << stats.countReport << "," << stats.countExplain << "," << stats.countUndo << ","
        << stats.countHistory << "," << stats.countHelp << "," << stats.countVerbose << "," << stats.countColor << ","
        << stats.countSnapshot << "," << stats.countLoad << "," << stats.countSave << ","
        << stats.countExit << "," << stats.countUnknown << "\n";

    out.close();
    log("SUMMARY CSV → Written to " + path, Logger::INFO);
}

void Logger::logCustomerCSV(int id, int wait, int retry, int arrival, int turnaround, const string& path) {
    // Check if the file already exists and has content
    bool fileHasHeader = false;
    std::ifstream check(path.c_str());
    fileHasHeader = check && check.peek() != std::ifstream::traits_type::eof();
    check.close();

    std::ofstream out(path.c_str(), std::ios::app);
    if (!fileHasHeader) {
        out << "CustomerID,WaitTime,RetryCount,ArrivalTime,TurnaroundTime\n";
    }

    out << id << "," << wait << "," << retry << "," << arrival << "," << turnaround << "\n";
    out.close();
}

void Logger::logSessionTXT(const SessionStats& stats, const std::string& path) {
    std::ofstream file(path.c_str());
    if (!file.is_open()) return;

    time_t now = time(NULL);
    file << "===== Session Summary =====\n";
    file << "Timestamp: " << std::ctime(&now);
    file << "Total Requests: " << stats.countRQ << "\n";
    file << "Total Releases: " << stats.countRL << "\n";
    file << "Denied Requests: " << stats.totalDenied << "\n";
    file << "Safe Requests: " << stats.safeRequests << "\n";
    file << "Unsafe Requests: " << stats.unsafeRequests << "\n";
    file << "  > Exceeds Need: " << stats.deniedNeed << "\n";
    file << "  > Exceeds Avail: " << stats.deniedAvailability << "\n";
    file << "  > Unsafe State: " << stats.deniedUnsafe << "\n\n";

    // Convert map to vector
    std::vector<std::pair<std::string, int> > sortedUsage;
    std::map<std::string, int>::const_iterator it;
    for (it = stats.commandUsage.begin(); it != stats.commandUsage.end(); ++it) {
        sortedUsage.push_back(*it);
    }

    std::sort(sortedUsage.begin(), sortedUsage.end(), CommandUsageComparator());

    file << "Most Used Commands:\n";
    for (std::vector<std::pair<std::string, int> >::const_iterator it2 = sortedUsage.begin(); it2 != sortedUsage.end(); ++it2) {
        file << "  " << it2->first << ": " << it2->second << "\n";
    }
    file << "===========================\n";
    file.close();
}

void Logger::logRequestHeatmap() {
    mkdir("logs", 0777);
    ofstream heatmap("logs/request_heatmap.csv");
    if (!heatmap.is_open()) return;

    heatmap << "CustomerID,RQ_Count,RL_Count\n";
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
        heatmap << "P" << i << "," << globalStats.requestCount[i]
                << "," << globalStats.releaseCount[i] << "\n";
    }
    heatmap.close();

    Logger::log("HEATMAP → logs/request_heatmap.csv written", Logger::INFO);

    if (verboseMode) {
        cout << COLOR_CYAN << "[HEATMAP] Request/Release counts:\n";
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
            cout << "  P" << i << " → RQ: " << globalStats.requestCount[i]
                 << ", RL: " << globalStats.releaseCount[i] << "\n";
        }
        cout << COLOR_RESET;
    }
}