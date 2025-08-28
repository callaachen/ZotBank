// Calla Chen
// Source Code File 5/11 for EECS 111 Project #3
#include "log_global.h"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <cerrno>

using namespace std;

ofstream fullLog("logs/full_session.txt");

SessionStats globalStats;     // Track stats for final summary

vector<string> commandHistory;

ofstream customerLogs[10];
int customerArrivalTimes[10] = { -1 };
int customerRetryCounts[10] = { 0 };
int customerWaitTimes[10] = { -1 };
int customerTurnaround[10] = { -1 };

// Constructor to initialize all counts to 0
SessionStats::SessionStats()
    : totalRequests(0), totalReleases(0), totalDenied(0),
      safeRequests(0), unsafeRequests(0),
      deniedNeed(0), deniedAvailability(0), deniedUnsafe(0),
      countRQ(0), countRL(0), countStar(0), countSafety(0),
      countReset(0), countReport(0), countExplain(0),
      countUndo(0), countHelp(0), countSummary(0), countVerbose(0),
      countColor(0), countSnapshot(0), countSavepoint(0),
      countRollback(0), countLoad(0), countSave(0),
      countHistory(0), countExit(0), countUnknown(0), countDeadlock(0),
      countHeatmap(0), countPreview(0), countSafePreview(0), countUnsafePreview(0),
	  countDeniedPreview(0), countRecap(0), countCompare(0), countDiff(0)
{
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
        requestCount[i] = 0;
        releaseCount[i] = 0;
    }
}

string currentTimestamp() {
    time_t now = time(0);
    struct tm* t = localtime(&now);
    ostringstream oss;
    oss << setfill('0') << "[" << setw(2) << t->tm_hour << ":"
        << setw(2) << t->tm_min << ":" << setw(2) << t->tm_sec << "]";
    return oss.str();
}

void initCustomerLogs() {
    // Ensure logs/ directory exists
    if (access("logs", F_OK) == -1) {
        int status = mkdir("logs", 0755);
        if (status != 0) {
            cerr << "[FATAL] Failed to create logs/ directory: "
                      << strerror(errno) << endl;
            exit(1);
        }
    }

    // Open customer log files (only for defined NUMBER_OF_CUSTOMERS)
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
        ostringstream filename;
        filename << "logs/customer_P" << i << ".txt";
        customerLogs[i].open(filename.str().c_str());
    }
}

const string helpText =
    "\nCommands:\n"
    "  RQ <cust> r0 r1 r2 r3   		- Request resources\n"
    "  RL <cust> r0 r1 r2 r3   		- Release resources\n"
    "  *                       		- Print all matrices\n"
    "  safety                  		- Toggle safe sequence display\n"
    "  snapshot                		- Save a manual undo snapshot\n"
    "  undo                    		- Revert to last snapshot\n"
    "  report                  		- Show current usage\n"
    "  explain                 		- Show last denial reason\n"
    "  summary                 		- Command usage breakdown\n"
    "  test <file>             		- Run commands from a file\n"
    "  save                    		- Save current system state\n"
    "  load                    		- Load previously saved state\n"
    "  history                 		- View past commands\n"
	"  recap 						- Show the last 5 meaningful commands\n"
    "  !N                      		- Replay a previous command\n"
    "  verbose [on/off]        		- Toggle verbose log output\n"
    "  color [on/off]          		- Toggle ANSI color output\n"
    "  savepoint <name>        		- Create named system snapshot\n"
    "  rollback <name>         		- Restore system to a savepoint\n"
    "  heatmap                 		- Log current RQ/RL heatmap\n"
    "  help [cmd]              		- Show help for a command or all\n"
	"  preview <cust> r0 r1 r2 r3   - Show save sequence if request is made (but do NOT apply)\n"
    "  compare <name>               - Compare current system with a savepoint\n"
	"  diff <savepoint name> 		- View differences from savepoint\n"
    "  exit                    		- Quit the session\n";

bool verboseMode = true;  // Default for verbose mode is ON

bool colorEnabled = true; // Default setting for the bonus features in the terminal is color coded

// Savepoint state
vector<vector<int> > snapshotAllocation;
vector<vector<int> > snapshotNeed;
vector<vector<int> > snapshotAvailable;

bool hasSnapshot = false;
