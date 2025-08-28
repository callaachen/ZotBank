// Calla Chen
// Source Code File 9/11 for EECS 111 Project #3
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <fstream>

#include "banker.h"
#include "command_handler.h"
#include "logger.h"
#include "log_global.h"

using namespace std;

#define VERSION "ZotBank v1.0"

typedef CommandHandler::Result CHResult;

void loadHistory() {
    ifstream infile("logs/history.txt");
    string line;
    while (getline(infile, line)) {
        if (!line.empty() && line[0] != '!')
            commandHistory.push_back(line);
    }
}
void saveHistory() {
    ofstream outfile("logs/history.txt");
    for (size_t i = 0; i < commandHistory.size(); ++i) {
        outfile << commandHistory[i] << "\n";
    }
}

int main(int argc, char* argv[]) {
    cout << "=================================\n";
    cout << VERSION << " - EECS 111 Project #3\n";
    cout << "=================================\n";
    cout << "Type 'help' for command syntax.\n\n";

    fullLog << "=================================\n";
    fullLog << " " << VERSION << " - EECS 111 Project #3\n";
    fullLog << "=================================\n";
    fullLog << "Type 'help' for command syntax.\n\n";

    if (
     argc != NUMBER_OF_RESOURCES + 2 &&
     !(argc == NUMBER_OF_RESOURCES + 3 && string(argv[NUMBER_OF_RESOURCES + 2]) == "test") &&
     !(argc == NUMBER_OF_RESOURCES + 4 && string(argv[NUMBER_OF_RESOURCES + 2]) == "test")
     ) {
        cout << "Usage: " << argv[0] << " <inputfile> r0 r1 r2 r3 [test <testfile>]\n";
        return 1;
    }

    Logger::init();
    initCustomerLogs();
    Banker banker;
    loadHistory();

    if (!banker.loadMaximumFromFile(argv[1])) {
        cout << "Failed to load input file.\n";
        fullLog << "Failed to load input file.\n";
        return 1;
    }

    int availableResources[NUMBER_OF_RESOURCES];
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j)
        availableResources[j] = atoi(argv[j + 2]);
    banker.setAvailable(availableResources);

    if (argc == NUMBER_OF_RESOURCES + 4 && string(argv[NUMBER_OF_RESOURCES + 2]) == "test") {
        string testfile = argv[NUMBER_OF_RESOURCES + 3];
        ifstream infile(testfile.c_str());
        if (!infile.is_open()) {
            cout << "[ERROR] Cannot open test file: " << testfile << "\n";
            return 1;
        }

        string line;
        extern bool exitAfterTest;
        exitAfterTest = true;

        while (getline(infile, line)) {
            if (!line.empty()) {
                commandHistory.push_back(line);
                fullLog << "> " << line << endl;
                CommandHandler::Result result = CommandHandler::process(line, banker);
                if (result.status == CommandHandler::EXIT)
                    break;
            }
        }

        cout << "[INFO] TEST → " << commandHistory.size() << " commands executed from " << testfile << "\n";
        return 0;
    }

    banker.snapshot();

    string line;
    extern bool exitAfterTest;
    while (!exitAfterTest) {
        cout << "> ";
        cout.flush();
        getline(cin, line);
        if (!line.empty())
            commandHistory.push_back(line);

        fullLog << "> " << line << endl;

        CHResult result = CommandHandler::process(line, banker);

        if (result.status == CommandHandler::EXIT)
            break;
    }

    cout << COLOR_CYAN << "\n===== Session Summary =====\n" << COLOR_RESET;
    cout << "Total Requests:  " << globalStats.totalRequests << "\n";
    cout << "Total Releases:  " << globalStats.totalReleases << "\n";
    cout << "Denied Requests: " << COLOR_RED << globalStats.totalDenied << COLOR_RESET << "  (RQ only)\n";
    cout << "Safe Requests:   " << COLOR_GREEN << globalStats.safeRequests << COLOR_RESET << "  (RQ only)\n";
    cout << "Unsafe Requests: " << COLOR_YELLOW << globalStats.unsafeRequests << COLOR_RESET << "  (RQ only)\n";
    cout << " > Exceeds Need:   " << COLOR_YELLOW << globalStats.deniedNeed << COLOR_RESET << "\n";
    cout << " > Exceeds Avail:  " << COLOR_YELLOW << globalStats.deniedAvailability << COLOR_RESET << "\n";
    cout << " > Unsafe State:   " << COLOR_YELLOW << globalStats.deniedUnsafe << COLOR_RESET << "\n";
    cout << "Deadlocks detected: " << COLOR_RED << globalStats.countDeadlock << COLOR_RESET << "\n";
    cout << COLOR_CYAN << "===========================\n" << COLOR_RESET;

    if (globalStats.countPreview > 0) {
        cout << COLOR_CYAN << "\n===== Preview Outcome Summary =====\n" << COLOR_RESET;
        cout << "Total Previewed: " << globalStats.countPreview << "\n";
        cout << " - Denied (invalid/exceeds): " << globalStats.countDeniedPreview << "\n";
        cout << " - Unsafe (no safe sequence): " << globalStats.countUnsafePreview << "\n";
        cout << " - Safe (not applied):        " << globalStats.countSafePreview << "\n";
        cout << COLOR_CYAN << "===========================\n" << COLOR_RESET;
    }

    // For fullLog
    stringstream summary;
    summary << "\n===== Session Summary =====\n"
            << "Total Requests:  " << globalStats.totalRequests << "\n"
            << "Total Releases:  " << globalStats.totalReleases << "\n"
            << "Denied Requests: " << globalStats.totalDenied << "  (RQ only)\n"
            << "Safe Requests:   " << globalStats.safeRequests << "  (RQ only)\n"
            << "Unsafe Requests: " << globalStats.unsafeRequests << "  (RQ only)\n"
            << " > Exceeds Need:   " << globalStats.deniedNeed << "\n"
            << " > Exceeds Avail:  " << globalStats.deniedAvailability << "\n"
            << " > Unsafe State:   " << globalStats.deniedUnsafe << "\n"
            << "Deadlocks detected: " << globalStats.countDeadlock << "\n"
            << "===========================\n";

    if (globalStats.countPreview > 0) {
        summary << "\n===== Preview Outcome Summary =====\n"
                << "Total Previewed: " << globalStats.countPreview << "\n"
                << " - Denied (invalid/exceeds): " << globalStats.countDeniedPreview << "\n"
                << " - Unsafe (no safe sequence): " << globalStats.countUnsafePreview << "\n"
                << " - Safe (not applied):        " << globalStats.countSafePreview << "\n"
                << "===========================\n";
    }

    fullLog << summary.str();

    fullLog << "[SESSION ENDED]" << endl;
    fullLog.flush();

    char buffer[100];
    sprintf(buffer, "Session ended with %d RQs and %d RLs.",
            globalStats.totalRequests, globalStats.totalReleases);
    Logger::log(buffer);

    // Write summary CSV for analysis.py
    Logger::logSummaryCSV(globalStats);  // Creates logs/log_summary.csv

    // Writes session summary
    Logger::logSessionTXT(globalStats);

    // Write per-customer CSV
    for (int i = 0; i < 10; ++i) {
        if (customerWaitTimes[i] >= 0) {  // Replace with your real condition
            Logger::logCustomerCSV(
                i,
                customerWaitTimes[i],       // Fill with actual wait time
                customerRetryCounts[i],     // Retry attempts
                customerArrivalTimes[i],    // Arrival time
                customerTurnaround[i]       // Turnaround time
            );
        }
    }
    Logger::close();

    // Close per-customer logs
    for (int i = 0; i < 10; ++i) {
        if (customerLogs[i].is_open())
            customerLogs[i].close();
    }
    saveHistory();

    return 0;
}
