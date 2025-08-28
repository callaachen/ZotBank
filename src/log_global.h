// Calla Chen
// Source Code File 6/11 for EECS 111 Project #3
#ifndef LOG_GLOBAL_H
#define LOG_GLOBAL_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "banker.h"

// Global log stream used throughout the system (besides Logger class)
extern std::ofstream fullLog;

// Session-wide counters for tracking request/release behavior
struct SessionStats {
    // Request/release tracking
    int totalRequests;
    int totalReleases;
    int totalDenied;
    int safeRequests;
    int unsafeRequests;
    int deniedNeed;
    int deniedAvailability;
    int deniedUnsafe;

    // Per-command usage counters
    int countRQ;
    int countRL;
    int countStar;
    int countSafety;
    int countReset;
    int countReport;
    int countExplain;
    int countUndo;
    int countHelp;
    int countSummary;
    int countVerbose;
    int countColor;
    int countSnapshot;
    int countSavepoint;
    int countRollback;
    int countLoad;
    int countSave;
    int countHistory;
    int countExit;
    int countUnknown;
    int countDeadlock;
    int countHeatmap;
	int countPreview;
	int countSafePreview;
	int countUnsafePreview;
	int countDeniedPreview;
	int countRecap;
	int countCompare;
	int countDiff;

    int requestCount[NUMBER_OF_CUSTOMERS];
    int releaseCount[NUMBER_OF_CUSTOMERS];

    std::map<std::string, int> commandUsage;

    SessionStats(); // Constructor declaration
};

// Global instance used across main.cpp and command_handler.cpp
extern SessionStats globalStats;

// Command history
extern std::vector<std::string> commandHistory;

// Per customer logs - supporting up to 10 customers
extern std::ofstream customerLogs[10];

// Current Time Stamp
std::string currentTimestamp();

// Declare the init function
void initCustomerLogs();

// Help text for commands to put in the terminal
extern const std::string helpText;

// Verbose mode toggle
extern bool verboseMode;

// Enabling & disabling the color-coding in the terminal
extern bool colorEnabled;

// ANSI color codes for terminal output
inline std::string colorWrap(const std::string& code) {
    return colorEnabled ? code : "";
}
#define COLOR_RESET    colorWrap("\033[0m")
#define COLOR_GREEN    colorWrap("\033[1;32m")
#define COLOR_RED      colorWrap("\033[1;31m")
#define COLOR_YELLOW   colorWrap("\033[1;33m")
#define COLOR_CYAN     colorWrap("\033[1;36m")
#define COLOR_MAGENTA  colorWrap("\033[1;95m")
#define COLOR_BLUE     colorWrap("\033[0;34m")

// Per-customer session stats (indexed by customer ID)
extern int customerArrivalTimes[10];
extern int customerRetryCounts[10];
extern int customerWaitTimes[10];
extern int customerTurnaround[10];

// Snapshot of last savepoint (used for undo)
extern std::vector<std::vector<int> > snapshotAllocation;
extern std::vector<std::vector<int> > snapshotNeed;
extern std::vector<std::vector<int> > snapshotAvailable;

#endif // LOG_GLOBAL_H