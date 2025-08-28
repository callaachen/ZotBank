// Calla Chen
// Source Code File 3/11 for EECS 111 Project #3
#include "command_handler.h"
#include <sstream>
#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include <cstdlib>
#include <dirent.h>
#include "logger.h"
#include "log_global.h"
#include "validator.h"

bool exitAfterTest = false; // Flag for terminating test mode after file execution

using namespace std;

// Initializing alias map to handle alternate forms of commands
static map<string, string> initAliasMap() {
    map<string, string> m;
    m["req"] = "RQ"; m["rq"] = "RQ";
    m["rel"] = "RL"; m["rl"] = "RL";
    m["rep"] = "report"; m["sum"] = "summary";
    m["xpl"] = "explain"; m["hist"] = "history";
    m["snap"] = "snapshot"; m["undo!"] = "undo";
    m["v"] = "verbose"; m["c"] = "color";
    m["sp"] = "savepoint"; m["rb"] = "rollback";
    m["q"] = "exit"; m["hm"] = "heatmap";
	m["pre"] = "preview"; m["cmp"] = "compare"; m["df"] = "diff";
    return m;
}

// Global alias map during command processing
static const map<string, string> aliasMap = initAliasMap();

// Resource alias map used during command processing
string resolveAlias(const string& cmd) {
    map<string, string>::const_iterator it = aliasMap.find(cmd);
    return (it != aliasMap.end()) ? it->second : cmd;
}

// Main command interpreter: parses input and invokes matching functionality
CommandHandler::Result CommandHandler::process(const std::string& input, Banker& banker) {
    Result res = { CONTINUE, false, false, false, false, false, false };

    string trimmed = input;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
    trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
    if (trimmed.empty()) return res;

    // Handle history recall (!N)
    if (trimmed[0] == '!' && trimmed.length() > 1) {
    int index = atoi(trimmed.substr(1).c_str());
    if (index > 0 && index <= (int)commandHistory.size()) {
        string recalled = commandHistory[index - 1];

        // Prevent recursive recall
        if (!recalled.empty() && recalled[0] == '!' && isdigit(recalled[1])) {
            cout << COLOR_RED << "[ERROR] Cannot recall a recall command (!N).\n" << COLOR_RESET;
            fullLog << "[ERROR] Attempted recursive recall of: " << recalled << "\n";
            return res;
        }

        cout << COLOR_MAGENTA << "[RECALLING] " << recalled << COLOR_RESET << "\n";
        fullLog << "[RECALLING] " << recalled << "\n";
        if (verboseMode)
            fullLog << "[VERBOSE] Recalled command from history index !" << index << "\n";
        return CommandHandler::process(recalled, banker);
        } else {
            cout << COLOR_RED << "[ERROR] Invalid history index.\n" << COLOR_RESET;
            fullLog << "[ERROR] Invalid history index.\n";
            return res;
        }
    }

    stringstream iss(trimmed);
	vector<string> parts;
	string token;
	while (iss >> token) {
    	parts.push_back(token);
	}

	if (parts.empty()) return res;

	string cmd = resolveAlias(parts[0]);

    if (cmd == "*") {
		// Handle '*' command displays current system matrices (Available, Allocation, Need, Max)
        globalStats.countStar++;			// Track how many times '*' has been called
        banker.printState();
        if (verboseMode)
            fullLog << "[VERBOSE] System matrix printed (command '*')\n"; // log action if verbose is on
        return res;
    }
    else if (cmd == "RQ") {
		// Handle resource request from a customer
        globalStats.countRQ++;				// Track RQ usage count
        globalStats.commandUsage["RQ"]++;	// Record usage in detailed command app
        res.isRequest = true;

        int cust, req[NUMBER_OF_RESOURCES]; // Array to store requested resources
        stringstream ss(trimmed.substr(3)); // Parse after "RQ"
        ss >> cust;
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) ss >> req[j];

		// Track per-customer request count
        if (cust >= 0 && cust < NUMBER_OF_CUSTOMERS) {
            globalStats.requestCount[cust]++;
        }

        // Track first arrival time if this is the customer's first appearance
        if (customerArrivalTimes[cust] == -1)
            customerArrivalTimes[cust] = time(NULL);

		// Validate the request: valid customer and no negative values
        if (!Validator::isValidCustomer(cust) || !Validator::isValidRequest(req)) {
            string msg = "Invalid request: bad customer ID or negative values.\n";
            cout << msg;
            fullLog << msg;
            Logger::log("RQ " + trimmed.substr(3) + " → INVALID", Logger::WARN);

            if (verboseMode)
                fullLog << "[VERBOSE] RQ " << trimmed.substr(3) << "  → INVALID input\n";

            res.wasDenied = true;
            return res;
        }

		// Attempt to grant the request using Banker's Algorithm
        bool granted = (banker.request(cust, req) == Banker::GRANTED);

		// Verbose logging output
        if (verboseMode) {
            fullLog << "[VERBOSE] RQ " << cust << " ";
            for (int i = 0; i < NUMBER_OF_RESOURCES; ++i)
                fullLog << req[i] << " ";
            fullLog << "→ " << (granted ? "GRANTED" : "DENIED") << "\n";
        }

		// Record whether the request was granted or denied
        int result = granted ? Banker::GRANTED : -1;
        res.wasDenied = (result != Banker::GRANTED);
        res.status = (result == Banker::GRANTED) ? CONTINUE : DENIED;

		// If denied, then count retry and classify the reason
        if (res.wasDenied)
            customerRetryCounts[cust]++;

        if (res.wasDenied) {
            string reason = banker.getLastDenialReason();
            if (reason.find("need") != string::npos) res.exceedsNeed = true;
            else if (reason.find("available") != string::npos) res.exceedsAvail = true;
            else if (reason.find("unsafe") != string::npos) res.wasUnsafe = true;
        }

		// Print the final outcome and log to console/log files
        string statusStr = (result == Banker::GRANTED) ? "GRANTED" : "DENIED";
        string outputMsg = "Request " + string(result == Banker::GRANTED ? "granted.\n" : "denied.\n");

        Logger::log("RQ " + trimmed.substr(3) + " → " + statusStr,
                result == Banker::GRANTED ? Logger::INFO : Logger::ERROR);

        cout << (result == Banker::GRANTED ? COLOR_GREEN : COLOR_RED)
             << outputMsg << COLOR_RESET;
        fullLog << outputMsg;

		// Log to per-customer CSV if open
        if (cust >= 0 && cust < 10 && customerLogs[cust].is_open()) {
            customerLogs[cust] << "[" << currentTimestamp() << "] ";
            customerLogs[cust] << trimmed << " → " << statusStr << "\n";
            customerLogs[cust].flush();
        }

		// Record wait and turnaround time if request was granted
        if (result == Banker::GRANTED && cust >= 0 && cust < 10) {
            int now = time(NULL);
            customerWaitTimes[cust] = now - customerArrivalTimes[cust];
            customerTurnaround[cust] = customerWaitTimes[cust];
        }

		// Update overal system stats
        globalStats.totalRequests++;
        if (res.wasDenied) {
            globalStats.unsafeRequests++;
            globalStats.totalDenied++;
            if (res.exceedsNeed) globalStats.deniedNeed++;
            if (res.exceedsAvail) globalStats.deniedAvailability++;
            if (res.wasUnsafe) globalStats.deniedUnsafe++;
        } else {
            globalStats.safeRequests++;
        }

        return res;
    }
    else if (cmd == "RL") {
		// Handles resource request from customer
        globalStats.countRL++; 				// Track global RL usage count
        globalStats.commandUsage["RL"]++;	// Track usage in detailed command map
        res.isRelease = true;

        int cust, rel[NUMBER_OF_RESOURCES]; // rel[] holds release amounts for each resource
        stringstream ss(trimmed.substr(3));	// Prase after "RL "
        ss >> cust;
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) ss >> rel[j];

		// Track now how many times this customer has released resources
        if (cust >= 0 && cust < NUMBER_OF_CUSTOMERS) {
            globalStats.releaseCount[cust]++;
        }

		// Validate customer and release vector using current allocation
        const int (*alloc)[NUMBER_OF_RESOURCES] = banker.getAllocation();
        bool valid = Validator::isValidCustomer(cust) &&
             Validator::isValidRelease(rel, alloc, cust);

        if (verboseMode) {
			// verbose log for release command
            fullLog << "[VERBOSE] RL " << cust << " ";
            for (int i = 0; i < NUMBER_OF_RESOURCES; ++i)
                fullLog << rel[i] << " ";
            fullLog << "→ " << (valid ? "RELEASED" : "INVALID") << "\n";
        }

		// Showing error and log if release is invalid
        if (!Validator::isValidCustomer(cust) || !Validator::isValidRelease(rel, alloc, cust)) {
            string msg = "Invalid release: too much released or bad customer ID.\n";
            cout << msg;
            fullLog << msg;
            Logger::log("RL " + trimmed.substr(3) + " → INVALID", Logger::WARN);
            return res;
        }

		// Perform the release
        banker.release(cust, rel);
        Logger::log("RL " + trimmed.substr(3) + " → RELEASED", Logger::INFO);
		globalStats.totalReleases++;

        // Update turnaround time if arrival is known
        if (cust >= 0 && cust < 10 && customerArrivalTimes[cust] != -1) {
            customerTurnaround[cust] = time(NULL) - customerArrivalTimes[cust];
        }

		// Notify user and log release success
        string msg = "Resources released.\n";
        cout << msg;
        fullLog << msg;

		// Append release to per-customer log if open
        if (cust >= 0 && cust < 10 && customerLogs[cust].is_open()) {
            customerLogs[cust] << "[" << currentTimestamp() << "] ";
            customerLogs[cust] << trimmed << " → RELEASED\n";
            customerLogs[cust].flush();
        }
        return res;
    }
    else if (cmd == "safety") {
		// Toggles the visibility of safe sequence output after resource requests
        globalStats.countSafety++;				// Increment command usage stats
        globalStats.commandUsage["safety"]++;

        banker.toggleSafeSequence();					// Flip internal toggle
        bool enabled = banker.isSafeSequenceEnabled();	// Checks new toggle

		// Generate status message
        string msg = "Safe sequence display " + string(enabled ? "enabled.\n" : "disabled.\n");

		// Print satatus with color and log it
        cout << (enabled ? COLOR_GREEN : COLOR_RED) << msg << COLOR_RESET;
        fullLog << msg;
        Logger::log("SAFETY → " + string(enabled ? "ON" : "OFF"), Logger::INFO);

		// Extra logging if verbose mode is on
        if (verboseMode)
            fullLog << "[INFO] Safety sequence toggled to " << (enabled ? "ON" : "OFF") << "\n";
        return res;
    }
    else if (cmd == "reset") {
		// Reset system state to initial snapshot taken at program start
        globalStats.countReset++;				// Track reset command usage
        globalStats.commandUsage["reset"]++;

        banker.reset(); 							// Restore available, allocation, need from snapshot

		string msg = "System state reset.\n";

        cout << COLOR_MAGENTA << msg << COLOR_RESET;	// Displaying message in magenta in terminal
        fullLog << msg;									// Log to fullLog
        Logger::log("RESET → All matrices cleared", Logger::INFO);	// Log to persistent logger

        banker.printState(); // Show updated system matrices

		// Logging for verbose mode
        if (verboseMode)
            fullLog << "[INFO] System state reset.\n";
        return res;
    }
    else if (cmd == "report") {
		// Outputs a detailed summary of system resources (allocation, need, etc.)
        globalStats.countReport++;				// Incrementing report usage sats
        globalStats.commandUsage["report"]++;

        banker.printReport();					// Print forwarded resource report to terminal

		if (verboseMode)
            fullLog << "[INFO] Report command executed.\n"; // Log to fullLog if verbose is active
        Logger::log("REPORT → System resource summary printed", Logger::INFO); // persistent log entry
        return res;
    }
    else if (cmd == "explain") {
        globalStats.countExplain++;					// Track explain command usage
        globalStats.commandUsage["explain"]++;

        string reason = banker.getLastDenialReason(); // Retrieve denial reason from Banker

        cout << COLOR_YELLOW << reason << COLOR_RESET << endl;	// Show reason in yellow in terminal
        fullLog << reason << endl;								// Log session to log

        Logger::log("EXPLAIN → " + reason, Logger::INFO);		// Persistent log entry for debugging

        if (verboseMode)
            fullLog << "[INFO] Explain command output: " << reason << "\n";
        return res;
    }
	else if (cmd == "preview") {
    	globalStats.countPreview++; // Increment usage count for analytics
    	vector<int> tokens;
		// Parse numerical arguments from input
    	for (int i = 1; i < (int)parts.size(); ++i) {
        	int val;
        	if (stringstream(parts[i]) >> val)
            	tokens.push_back(val);
        	else {
				// Invalid non-integer argument
            	cout << "[PREVIEW] Invalid argument: " << parts[i] << "\n";
            	if (verboseMode)
                	fullLog << "[PREVIEW] Invalid argument: " << parts[i] << "\n";
            	return res;
        	}
    	}

		// Validate argument count: <cust> + NUMBER_OF_RESOURCES
    	if (tokens.size() != NUMBER_OF_RESOURCES + 1) {
        	cout << "[PREVIEW] Usage: preview <cust> r0 r1 r2 r3\n";
        	if (verboseMode)
            	fullLog << "[PREVIEW] Incorrect usage: preview <cust> r0 r1 r2 r3\n";
        	return res;
    	}

    	int cust = tokens[0];
		// Validate customer number
    	if (cust < 0 || cust >= NUMBER_OF_CUSTOMERS) {
        	cout << "[PREVIEW] Invalid customer number: P" << cust << "\n";
        	if (verboseMode)
            	fullLog << "[PREVIEW] Invalid customer number: P" << cust << "\n";
        	return res;
    	}

		// Extract request factor
    	int req[NUMBER_OF_RESOURCES];
    	for (int i = 0; i < NUMBER_OF_RESOURCES; ++i)
        	req[i] = tokens[i + 1];

		// Check if request would be granted based on current state
    	if (!banker.wouldGrantRequest(cust, req)) {
			globalStats.countDeniedPreview++; // Log denial
        	cout << "[PREVIEW] Request would be denied.\n";
        	if (verboseMode)
            	fullLog << "[PREVIEW] Request denied: exceeds available or max for P" << cust << "\n";
			globalStats.totalDenied++;
			globalStats.deniedAvailability++;
    	} else {
			// Simulate to find a safe sequence
        	vector<int> safeSeq = banker.simulateSequence(cust, req);
        	if (safeSeq.empty()) {
				globalStats.countUnsafePreview++;
            	cout << "[PREVIEW] Unsafe request. No valid safe sequence.\n";
            	if (verboseMode)
                	fullLog << "[PREVIEW] Unsafe request. No safe sequence possible for P" << cust << "\n";
				globalStats.totalDenied++;
				globalStats.deniedAvailability++;
				globalStats.deniedUnsafe++;
        	} else {
				globalStats.countSafePreview++;
            	cout << "[PREVIEW] Safe. Possible sequence: ";
            	if (verboseMode)
                	fullLog << "[PREVIEW] Safe sequence for P" << cust << ": ";
            	for (size_t i = 0; i < safeSeq.size(); ++i) {
                	cout << "P" << safeSeq[i];
                	if (verboseMode)
                    	fullLog << "P" << safeSeq[i];
                	if (i < safeSeq.size() - 1) {
                    	cout << " → ";
                    	if (verboseMode)
                        	fullLog << " → ";
                	}
            	}
            	cout << "\n";
            	if (verboseMode)
                	fullLog << "\n";
        	}
    	}

    	return res;
	}
    else if (cmd == "snapshot") {
        globalStats.countSnapshot++;			// Records how many times 'snapshot' has been used
        globalStats.commandUsage["snapshot"]++; // Record frequency for analytics
        banker.saveUndoSnapshot();				// Save current system state for manual undo
        cout << COLOR_CYAN << "[INFO] Manual snapshot saved.\n" << COLOR_RESET;
        fullLog << "[INFO] Manual snapshot saved.\n";
        if (verboseMode)
            fullLog << "[INFO] Manual snapshot saved.\n";
        return res;
    }
    else if (cmd == "undo") {
        globalStats.countUndo++;			// Increment total 'undo' command counter
        globalStats.commandUsage["undo"]++; // track usage frequency for analytics
        banker.restoreUndoSnapshot();		// Restore system state from last manual snapshot

        cout << COLOR_YELLOW << "[INFO] System restored to last snapshot.\n" << COLOR_RESET;
        fullLog << "[INFO] System restored to last snapshot.\n";

        banker.printState();				// Print the restored matrices to terminal
        if (verboseMode)
            fullLog << "[INFO] Undo snapshot restored.\n";
        return res;
    }
    else if (cmd == "test") {
        stringstream ss(trimmed.substr(5));	// Parse filename (in tests directory) from input
        string filename;
        ss >> filename;

        ifstream infile(filename.c_str()); // Open the test file in tests directory
        if (!infile) {
            string msg = "Test file not found: " + filename + "\n";
            cout << COLOR_RED << msg << COLOR_RESET; // Display error to terminal
            fullLog << msg;							 // Log error
            Logger::log("TEST → Failed to open " + filename, Logger::ERROR);
            return res;
        }

        string line;
        int count = 0;
        cout << COLOR_CYAN << "[TEST MODE] Running commands from: " << filename << "\n" << COLOR_RESET;
        Logger::log("TEST → Running commands from " + filename, Logger::INFO);

		//Process each command from file
        while (getline(infile, line)) {
            if (line.empty()) continue;

			// Trim whitespace
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

            cout << "> " << line << endl;	// Echo command to terminal
            fullLog << "> " << line << endl;
            fullLog.flush();

            CommandHandler::Result result = CommandHandler::process(line, banker); // Execute command
            fullLog.flush();

            if (result.status == CommandHandler::EXIT) { // Stop if 'exit' command encountered in the test file
                exitAfterTest = true;
                break;  // End test mode early so summary prints
            }

			// Track request statistics (inputed in any of the test files)
            if (result.isRequest) {
                globalStats.totalRequests++;
                if (result.wasDenied) {
                    globalStats.unsafeRequests++;
                    globalStats.totalDenied++;
                    if (result.exceedsNeed) globalStats.deniedNeed++;
                    if (result.exceedsAvail) globalStats.deniedAvailability++;
                    if (result.wasUnsafe) globalStats.deniedUnsafe++;
                } else {
                    globalStats.safeRequests++;
                }
            }
			// Track release statistics
            if (result.isRelease)
                globalStats.totalReleases++;

            if (result.status == EXIT)
                break;

            ++count; // Count number of commands executed
        }

  		// Test completed
        cout << COLOR_CYAN << "[TEST MODE COMPLETE] " << count << " commands executed from file.\n" << COLOR_RESET;
        stringstream ss_count;
        ss_count << count;
        Logger::log("TEST → " + ss_count.str() + " commands executed from " + filename, Logger::INFO);
        fullLog << "[TEST MODE COMPLETE] " << count << " commands executed from file.\n";
        fullLog.flush();

        if (verboseMode)
            fullLog << "[VERBOSE] Processed " << count << " commands in test mode from " << filename << "\n";
        return res;
    }
    else if (cmd == "history") {
        globalStats.countHistory++; 			// Incremement history command counter
        string header = "\033[35m\nCommand History:\033[0m\n"; // Purple colored header
        cout << header;
        fullLog << "\nCommand History:\n";

		// Print each past command with index
        for (size_t i = 0; i < commandHistory.size(); ++i) {
            stringstream ss;
            ss << "\033[36m  " << (i + 1) << ": " << commandHistory[i] << "\033[0m\n";
            string entry = ss.str();
            cout << entry; // Print to terminal
            fullLog << "  " << i + 1 << ": " << commandHistory[i] << "\n"; // Log to file
        }

        cout << "\n";
        fullLog << "\n";
        fullLog.flush(); // Flush log buffer
        if (verboseMode)
            fullLog << "[INFO] Command history printed\n";

        return res;
    }
	else if (cmd == "recap") {
		globalStats.countRecap++; // Track usage of 'recap' command
		vector<string> filtered; // Store up to 5 most recent meaningful commands

		// Traverse command history in reverse to collect non-trivial commands
		for (int i = (int)commandHistory.size() - 1; i >= 0 && (int)filtered.size() < 5; --i) {
			string entry = commandHistory[i];
			if (entry.empty()) continue; // Skipping empty lines

			string head;
			stringstream ss(entry); ss >> head;
			head = resolveAlias(head); // Resolve command aliases if any

			// Skip commands that are not user-actionable
			if (head == "test" || head == "help" || head == "recap" || head == "history" || head == "*") continue;
			filtered.push_back(entry); // Valid user command found
		}
		if (filtered.empty()) {
			// No valid user commands found in history
			cout << "[RECAP] No valid user commands yet.\n";
			if (verboseMode)
				fullLog << "[RECAP] Nothing to show.\n";
		} else {
			// Print the recap of the last valid commands
			cout << COLOR_BLUE << "[RECAP] Last " << filtered.size() << " valid commands.\n" << COLOR_RESET;
			for (int i = (int)filtered.size() - 1; i >= 0; --i) {
				cout << " , " << filtered[i] << "\n";
				if (verboseMode) fullLog << "[RECAP] " << filtered[i] << "\n";
			}
		}
		return res;
	}
	else if (cmd == "help") {
    globalStats.countHelp++;
    globalStats.commandUsage["help"]++;
    stringstream ss(trimmed);
    string dummy, topic;
    ss >> dummy >> topic;

	topic = resolveAlias(topic); // Resolve alias before checking help

    if (topic.empty()) { // Print help menu and alias table when no specific topic is requested
        cout << helpText;
        cout << "\nCommand Aliases:\n"
             << "  req → RQ\n"
             << "  rel → RL\n"
             << "  rep → report\n"
             << "  sum → summary\n"
             << "  xpl → explain\n"
             << "  hist → history\n"
             << "  snap → snapshot\n"
             << "  undo! → undo\n"
             << "  v → verbose\n"
             << "  c → color\n"
             << "  sp → savepoint\n"
             << "  rb → rollback\n"
             << "  hm → heatmap\n"
			 << "  pre → preview\n"
             << "  cmp → compare\n"
             << "  q → exit\n";
        if (verboseMode) {
            fullLog << helpText;
            fullLog << "\nCommand Aliases:\n"
                    << "  req → RQ\n"
                    << "  rel → RL\n"
                    << "  rep → report\n"
                    << "  sum → summary\n"
                    << "  xpl → explain\n"
                    << "  hist → history\n"
                    << "  snap → snapshot\n"
                    << "  undo! → undo\n"
                    << "  v → verbose\n"
                    << "  c → color\n"
                    << "  sp → savepoint\n"
                    << "  rb → rollback\n"
                    << "  hm → heatmap\n"
					<< "  pre → preview\n"
                    << "  cmp → compare\n"
                    << "  q → exit\n"
                    << "[INFO] General help message printed (no topic specified)\n";
            }
        } else { // Output descriptions per each individual command when help topic is specified
            if (topic == "RQ") {
                cout << "RQ <cust> r0 r1 r2 r3  - Request resources for customer <cust>.\n";
            } else if (topic == "RL") {
                cout << "RL <cust> r0 r1 r2 r3  - Release resources held by <cust>.\n";
            } else if (topic == "*") {
                cout << "*  - Print all resource matrices: Available, Max, Alloc, Need\n";
            } else if (topic == "safety") {
                cout << "safety  - Toggle safety sequence output on/off.\n";
            } else if (topic == "snapshot") {
                cout << "snapshot  - Save a manual undo snapshot.\n";
            } else if (topic == "undo") {
                cout << "undo  - Revert system to last snapshot.\n";
            } else if (topic == "report") {
                cout << "report  - Print current available, allocation, and need totals.\n";
            } else if (topic == "summary") {
                cout << "summary  - Show command usage counts.\n";
            } else if (topic == "explain") {
                cout << "explain  - Show reason for last denied request.\n";
            } else if (topic == "test") {
				cout << "test <file> - run commands from a file in auto-tested.\n";
			} else if (topic == "save") {
				cout << "save - Save current system to logs/save.txt.\n";
			} else if (topic == "load") {
				cout << "load - Load previously saved system state from logs/save.txt.\n";
			} else if (topic == "history") {
				cout << "history - Print previously entered commands.\n";
			} else if (topic == "recap") {
				cout << "recap - Show the last 5 meaningful commands.\n";
			} else if (topic == "!N") {
				cout << "!N - Replay the Nth command from command history.\n";
			} else if (topic == "help") {
				cout << "help [cmd] - Show help for a specific command, or all if ommitted.\n";
			} else if (topic == "exit") {
                cout << "exit  - Exit the session and save logs.\n";
            } else if (topic == "color") {
                cout << "color [on/off]  - Toggle color-coded output.\n";
            } else if (topic == "verbose") {
                cout << "verbose [on/off]  - Enable or disable verbose logging.\n";
            } else if (topic == "savepoint") {
                cout << "savepoint  - Saves a recovery state to rollback to later.\n";
            } else if (topic == "rollback") {
                cout << "rollback - Restores state to last saved savepoint.\n";
            } else if (topic == "heatmap") {
                cout << "heatmap - Logs request heatmap for all customers to CSV.\n";
            } else if (topic == "preview") {
    			cout << "preview <cust> r0 r1 r2 r3  - Show safe sequence if request is made (but do NOT apply).\n";
			} else if (topic == "compare") {
                cout << "compare <name> - Compare current system with a savepoint\n";
            } else if (topic == "all") { // Displaying summary list of ALL available commands for "help all"
			    cout << "\nCOMMAND HELP OVERVIEW:\n"
                     << "  RQ <cust> r0 r1 r2 r3  		- Request resources for customer <cust>\n"
                     << "  RL <cust> r0 r1 r2 r3  		- Release resources held by <cust>\n"
                     << "  *                      		- Print all resource matrices\n"
                     << "  safety                 		- Toggle safe sequence display\n"
                     << "  snapshot               		- Save a manual undo snapshot\n"
                     << "  undo                   		- Revert system to last snapshot\n"
                     << "  report                 		- Show current resource usage\n"
                     << "  explain                		- Show last denial reason\n"
                     << "  summary                		- Show command usage counts\n"
                     << "  test <file>            		- Execute commands from file\n"
                     << "  history                		- Display command history\n"
				     << "  recap 						- Show the last 5 meaningful commands\n"
                     << "  !N                    		- Replay the Nth command from history\n"
                     << "  help [cmd]             		- Show help for a command or 'all'\n"
                     << "  verbose [on/off]       		- Toggle verbose logging\n"
                     << "  color [on/off]         		- Toggle color-coded output\n"
                     << "  savepoint <name>       		- Create named system snapshot\n"
                     << "  rollback <name>        		- Restore system to a savepoint\n"
                     << "  heatmap                		- Log current RQ/RL heatmap\n"
					 << "  preview <cust> r0 r1 r2 r3   - Show save sequence if request is made (but do NOT apply)\n"
                     << "  compare <name>               - Compare current system with a savepoint\n"
					 << "  diff <savepoint name> 		- View differences from savepoint\n"
                     << "  exit                   		- Exit ZotBank\n\n";
            } else { // Handle and report all unknown or mispelled help topics
                cout << COLOR_RED << "[ERROR] Unknown help topic: " << topic << ". Try 'help all'.\n" << COLOR_RESET;
            }
            if (verboseMode)
                fullLog << "[INFO] Help shown for topic: " << topic << "\n";
            }
        return res;
    }
    // Summary command: prints the current running totals
    else if (cmd == "summary") {
        globalStats.countSummary++;
        stringstream ss;
        ss << "Command Usage Breakdown:\n"
           << "  RQ:         " << globalStats.countRQ << "\n"
           << "  RL:         " << globalStats.countRL << "\n"
           << "  safety:     " << globalStats.countSafety << "\n"
           << "  reset:      " << globalStats.countReset << "\n"
           << "  report:     " << globalStats.countReport << "\n"
           << "  explain:    " << globalStats.countExplain << "\n"
           << "  undo:       " << globalStats.countUndo << "\n"
           << "  snapshot:   " << globalStats.countSnapshot << "\n"
           << "  history:    " << globalStats.countHistory << "\n"
		   << "  recap:      " << globalStats.countRecap << "\n"
           << "  help:       " << globalStats.countHelp << "\n"
           << "  verbose:    " << globalStats.countVerbose << "\n"
           << "  color:      " << globalStats.countColor << "\n"
           << "  savepoint:  " << globalStats.countSavepoint << "\n"
           << "  rollback:   " << globalStats.countRollback << "\n"
           << "  preview:    " << globalStats.countPreview << "\n"
           << "  compare:    " << globalStats.countCompare << "\n"
		   << "  diff:       " << globalStats.countDiff << "\n"
           << "  unknown:    " << globalStats.countUnknown << "\n";

        if (globalStats.countPreview > 0) {
            ss << "\nPreview Outcome Summary:\n"
               << " - Denied (invalid/exceeds): " << globalStats.countDeniedPreview << "\n"
               << " - Unsafe (no safe sequence): " << globalStats.countUnsafePreview << "\n"
               << " - safe (not applied):       " << globalStats.countSafePreview << "\n";
        }

        cout << COLOR_CYAN << ss.str() << COLOR_RESET;
        fullLog << ss.str();
        if (verboseMode)
            fullLog << "[INFO] Command usage summary printed\n";
        return Result();
    }
    else if (cmd == "verbose") {
        globalStats.countVerbose++; // Track usage of 'verbose' command
        globalStats.commandUsage["verbose"]++;
        string arg;
        stringstream ss(trimmed);
        ss >> arg >> arg; // Skip the command itself and capture the next token

        if (arg == "on") {
            verboseMode = true; // Enable verbose mode (all recorded in full_session.txt)
            cout << "[VERBOSE] Enabled verbose output.\n";
            fullLog << "[VERBOSE] Verbose mode enabled\n";
        } else if (arg == "off") {
            verboseMode = false; // Disable verbose mode
            cout << "[VERBOSE] Disabled verbose output.\n";
            fullLog << "[VERBOSE] Verbose mode disabled\n";
        } else {
			// Invalid argument passed to verbose command
            cout << "[ERROR] Usage: verbose [on/off]\n";
            fullLog << "[ERROR] Invalid verbose argument\n";
        }
		// Log current verbose state for clarity
        if (verboseMode)
            fullLog << "[VERBOSE] Verbose logging is now ENABLED\n";
        else
            fullLog << "[VERBOSE] Verbose logging is now DISABLED\n";
        return res;
    }
    else if (cmd == "color") {
    	globalStats.countColor++; // Increment 'color' command count
    	globalStats.commandUsage["color"]++;

    	if (parts.size() < 2) {
			// Missing argument (expected 'on' or 'off')
        	cout << "[ERROR] Usage: color [off/on]\n";
        	fullLog << "[ERROR] Invalid color mode\n";
        	return res;
    	}

    	string mode = parts[1];
		// Count input to lowercase for case-sensitive comparison
    	for (size_t i = 0; i < mode.length(); ++i)
    		mode[i] = tolower(mode[i]);
    	if (mode == "off") {
        	colorEnabled = false; // Disable color-coded output
        	cout << "[INFO] Color output disabled.\n";
        	fullLog << "[INFO] Color output disabled\n";
    	} else if (mode == "on") {
        	colorEnabled = true; // Enable color-coded output
        	cout << "[INFO] Color output enabled.\n";
        	fullLog << "[INFO] Color output enabled\n";
    	} else {
			// Invalid argument provided
        	cout << "[ERROR] Usage: color [off/on]\n";
        	fullLog << "[ERROR] Invalid color mode\n";
    	}
    	return res;
	}
    else if (cmd == "heatmap") {
        globalStats.countHeatmap++; // Track usage count for 'heatmap' command
		globalStats.commandUsage["heatmap"]++;
        Logger::logRequestHeatmap(); // Log current request statistics to CSV

		// Notify user and log the heatmap generation
        cout << "[INFO] Request heatmap logged to logs/request_heatmap.csv\n";
        fullLog << "[INFO] Request heatmap logged to logs/request_heatmap.csv\n";

		// Optionally log verbose output
        if (verboseMode)
            fullLog << "[VERBOSE] Mid-session heatmap written to logs/request_heatmap.csv\n";
        return res;
    }
    else if (cmd == "exit") {
        globalStats.countExit++;  			// Increment usage counter for 'exit'
        Logger::log("EXIT → Session ended", Logger::INFO); // Log session termination
        if (verboseMode)
            fullLog << "[INFO] Exit command invoked. Session ending. \n";
        Logger::logRequestHeatmap();		// Log final request heatmap before exit
        res.status = EXIT;					// Set result status to EXIT to trigger shutdown
        return res;
    }
    else if (cmd == "save") {
        globalStats.countSave++; 			// Increment usage counter for 'save'
        banker.saveState("logs/save.txt");	// Save current system state to file
        Logger::log("SAVE → State saved to logs/save.txt", Logger::INFO); // Log save action
        cout << "State saved to logs/save.txt\n"; // Inform user on console
        if (verboseMode)
            fullLog << "[VERBOSE] Snapshot saved to logs/save.txt\n";
        return res;
    }
    else if (cmd == "load") {
        globalStats.countLoad++;							// Increment 'load' command usage
        bool success = banker.loadState("logs/save.txt");   // Attempt to load system state from file

        if (success) {
            Logger::log("LOAD → State loaded from logs/save.txt", Logger::INFO); // Log successful load
            cout << "State loaded from logs/save.txt\n";						 // Inform the user on console
        } else {
            Logger::log("LOAD → Failed to load state", Logger::WARN);			// Log failure to load
            cout << "Failed to load state from logs/save.txt\n";				// Inform user of failure
        }

        if (verboseMode) {
            if (success)
                fullLog << "[VERBOSE] Snapshot loaded from logs/save.txt\n";	// Verbose log for success
            else
                fullLog << "[VERBOSE] Failed to load snapshot\n";				// Verbose log for failure
        }
        return res;
    }
    else if (cmd == "savepoint") {
        globalStats.countSavepoint++;				// Track usage of the savepoint command
        globalStats.commandUsage["savepoint"]++;	// Increment named command usage count

        stringstream ss(trimmed);
        string dummy, label;
        ss >> dummy >> label;						// Parse the label after the command

        if (label.empty()) label = "default";		// Using "default" if no label is provided
        banker.savepoint(label);					// Create a savepoint with the given label

        cout << "[INFO] Savepoint '" << label << "' created.\n"; 	// Notify user of success
        fullLog << "[INFO] Savepoint '" << label << "' created.\n"; // Log in session file
        Logger::log("SAVEPOINT → Named savepoint '" + label + "' created", Logger::INFO); // Log system message

        if (verboseMode)
            fullLog << "[INFO] Savepoint '" << label << "' created via savepoint command.\n";
        res.status = CommandHandler::SUCCESS; // Mark result as successful
        return res;
    }
	else if (cmd == "compare") {
        globalStats.countCompare++;				// Track total uses of 'compare
        globalStats.commandUsage["compare"]++;  // Increment command-specific usage count

        string dummy, name;
        stringstream ss(trimmed);				// Parse command and savepoint name
        ss >> dummy >> name;

        if (name.empty()) {
			// User did not provide a savepoint name
            cout << COLOR_RED << "[ERROR] Usage: compare <savepoint_name>\n" << COLOR_RESET;
            fullLog << "[ERROR] Usage: compare <savepoint_name>\n";
			if (verboseMode)
				fullLog << "[VERBOSE] Compare command called without a savepoint name.\n";
        } else {
            bool success = banker.compareToSavepoint(name); // Try comparing with named savepoint
            if (!success) {
				// Savepoint was not found
                cout << COLOR_RED << "[COMPARE] Savepoint \"" << name << "\" not found.\n" << COLOR_RESET;
                fullLog << "[COMPARE] Savepoint \"" << name << "\" not found.\n";
				if (verboseMode)
					fullLog << "[VERBOSE] Savepoint \"" << name << "\" does not exist.\n";
            } else {
				// Successful comparison
				if (verboseMode)
					fullLog << "[VERBOSE] Compared current system to savepoint '" << name << "'.\n";
			}
        }
        return res;
    }
    else if (cmd == "diff") {
		// Track usage of 'diff command'
        globalStats.countDiff++;
        globalStats.commandUsage["diff"]++;

		// Parse savepoint name for command input
        string dummy, name;
        stringstream ss(trimmed);
        ss >> dummy >> name;

		// Check if savepoint name is provided
        if (name.empty()) {
            cout << COLOR_RED << "[ERROR] Usage: diff <savepoint_name>\n" << COLOR_RESET;
            fullLog << "[ERROR] Usage: diff <savepoint_name>\n";
			if (verboseMode)
				fullLog << "[VEBOSE] Diff command called without a savepoint name\n";
        } else {
			// Actual current state with specified savepoint and display differences
            bool found = banker.diffFromSavepoint(name, true);  // display = true
            if (found) {
                if (verboseMode)
					fullLog << "[VERBOSE] Differences from savepoint '" << name << "' displayed.\n";
				// Actuall diff logging logged in Banker::diffFromSavepoint
            }
            else { // Savepoint is not found
                fullLog << "[INFO] DIFF → Compared current state with savepoint \"" << name << "\"\n";
				if (verboseMode)
					fullLog << "[VERBOSE] Savepoint '" << name << "' not found for diff.\n";
            }
        }
        return res;
    }
    else if (cmd == "rollback") {
		// Increment command usage statistics for rollback
        globalStats.countRollback++;
        globalStats.commandUsage["rollback"]++;

		// Parse optional savepoint label from input (default if empty)
        stringstream ss(trimmed);
        string dummy, label;
        ss >> dummy >> label;
        if (label.empty()) label = "default";

		// Attempt rollback to specific savepoint
        bool success = banker.rollback(label);

        if (success) {
			// Rollback succeeds - print confirmation and log the action
            cout << COLOR_YELLOW << "[INFO] Rolled back to savepoint '" << label << "'.\n" << COLOR_RESET;
            fullLog << "[INFO] Rolled back to savepoint '" << label << "'.\n";
            Logger::log("ROLLBACK → State restored from savepoint '" + label + "'", Logger::INFO);
            if (verboseMode)
                fullLog << "[INFO] Successfully rolled back to savepoint '" << label << "'.\n";
        } else {
			// Savepoint not found - print and log error
            cout << COLOR_RED << "[ERROR] Savepoint '" << label << "' not found.\n" << COLOR_RESET;
            fullLog << "[ERROR] Savepoint '" << label << "' not found.\n";
            Logger::log("ROLLBACK → Failed: savepoint '" + label + "' does not exist", Logger::WARN);
            if (verboseMode)
                fullLog << "[WARN] Rollback failed: no savepoint named '" << label << "' exists.\n";
        }
        return res;
    }
    else {
        globalStats.countUnknown++; // Incrementing list of valid command options

		// Constructing error message with a list of valid command options
		string msg = "Unknown command. Try:\n"
             "  RQ, RL, *, safety, snapshot, undo, report, explain,\n"
             "  summary, test, save, load, history, !N, verbose, color,\n"
			 "  savepoint, rollback, heatmap, help, compare, diff, exit\n";
		// Print error to console & log it
        cout << COLOR_RED<< msg << COLOR_RESET;
        fullLog << msg;
        if (verboseMode)
            fullLog << "[INFO] Unknown command received " << cmd << "\n";
        return res;
    }
}
