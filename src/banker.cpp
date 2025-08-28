// Calla Chen
// Source Code File 1/11 for EECS 111 Project #3
#include "banker.h"
#include "logger.h"
#include "log_global.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

using namespace std;
/**
* @brief Constructor for the Banker class.
*
* Initializing all resource matrices (allocation, maximum, need) and available resources arrays to zero. This sets up a
* clean starting state for tbe Banker's Algorithm simulation.
*/
Banker::Banker() {
    // Initializing allocation, maximum, and need matrices to 0
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i)
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            allocation[i][j] = 0;
            maximum[i][j] = 0;
            need[i][j] = 0;
        }
    // Initialize available resources vector to 0
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j)
        available[j] = 0;

    hasUndoSnapshot = false;
    hasSavepoint = false;
}

/**
* @brief Sets up the number of available resources in the system.'
*
* Copies the provided resource array into the 'available' array, which represents the count of unallocated resources
* for each type.
*
* @param res Integer of size NUMBER_OF_RESOURCES containing the available units for each resource type.
*/
void Banker::setAvailable(int res[]) {
    // Copy resource availability into internal array
    for (int i = 0; i < NUMBER_OF_RESOURCES; ++i) {
        available[i] = res[i];
        availableSnapshot[i] = res[i]; // Save snapshot for reset
    }
}

/**
* @brief Loads the maximum resource claims for each customer from a file.
*
* Reads a comma-separated file where each line corresponds to one customer's maximum demand for each resource type. It
* populates the internal 'maximum' matrix and then computes the 'need' matrix on current allocations (assumed zero
* initialize)
*
* @param filename Name of the file to read from.
* @return true if the file is successfully parsed and all data is valid;
* 		  false if file I/O fails or the format is incorrect.
*/
bool Banker::loadMaximumFromFile(const string& filename) {
    ifstream infile(filename.c_str());
    if (!infile) return false; // File open failed

    string line;
    for (int i = 0; i < NUMBER_OF_CUSTOMERS && getline(infile, line); ++i) {
        stringstream ss(line);
        string token;
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            if (!getline(ss, token, ',')) return false; // Malformed line
            maximum[i][j] = atoi(token.c_str()); // Parse max demand
        }
    }

    calculateNeed(); // Recalculate need after loading maximum

    // Save initial state after max and need are calculated
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i)
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            allocationSnapshot[i][j] = allocation[i][j]; // should be 0 at startup
            needSnapshot[i][j] = need[i][j];
        }
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j)
        availableSnapshot[j] = available[j];

    return true;
}

/**
* @brief Calculates the remaining need matrix.
*
* For each customer and resource type, this computes:
*		need [i][j] = maximum[i][j] - allocation[i][j]
* The result reflects on how many more units each customer may still request.
*/
void Banker::calculateNeed() {
    // Calculate need = maximum - allocation for each customer and resource
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i)
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j)
            need[i][j] = maximum[i][j] - allocation[i][j];
}

/**
 * @brief Saves a backup of the current system state.
 *
 * Stores the current values of `available`, `allocation`, and `need` arrays so that they can be restored later in case
 * of unsafe allocation. This is used during tentative resource allocation.
 */
void Banker::snapshot() {
    // [CRITICAL SECTION START] Saving current system state (snapshot)
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j)
        backupAvailable[j] = available[j];

    // Save current allocation and needed matrices
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i)
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            backupAllocation[i][j] = allocation[i][j];
            backupNeed[i][j] = need[i][j];
        }
    // [CRITICAL SECTION END] Snapshot saved
}

/**
* @brief Saves a backup of the current system state.
*
* Stores the current values of 'available', 'allocation', and 'need' arrays so that they can be restored later in case
* of unsafe allocation. This is used during tentative resource allocation.
*/
void Banker::restore() {
    // [CRITICAL SECTION START] Restoring system state from snapshot
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j)
        available[j] = backupAvailable[j];

    // Restore allocation and need matrices from backup
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i)
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            allocation[i][j] = backupAllocation[i][j];
            need[i][j] = backupNeed[i][j];
        }
    // [CRITICAL SECTION END] Restore complete
}

/**
 * @brief Determines if the system is in a safe state.
 *
 * Implements the Banker's safety algorithm by simulating a sequence of process completions. The system is safe if for
 * all unfinished processes Pi, Need[i] <= Work, and their resources can eventually be released to allow others to
 * finish.
 *
 * @return true if a safe sequence exists; false otherwise.
 */
bool Banker::isSafe() {
    int work[NUMBER_OF_RESOURCES];
    bool finish[NUMBER_OF_CUSTOMERS] = { false };
    int safeSequence[NUMBER_OF_CUSTOMERS];
    int idx = 0;

    // Step 1: Initialize Work = Available
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j)
        work[j] = available[j];

    bool progress = true;
    while (progress) {
        progress = false;

        // Step 2. Find an unfinished customer i such that Need[i] <= Work
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
            if (!finish[i]) {
                bool canFinish = true;
                for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
                    if (need[i][j] > work[j]) {
                        canFinish = false;
                        break;
                    }
                }

                // Step 3. If found, simulate completion: Work += Allocation[i]
                if (canFinish) {
                    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j)
                        work[j] += allocation[i][j];
                    finish[i] = true;
                    safeSequence[idx++] = i;
                    progress = true;
                }
            }
        }
    }

    // Step 4: Check if all customers are finished
    bool deadlockDetected = false;
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
        if (!finish[i]) {
            deadlockDetected = true;
            break;
        }
    }

    if (deadlockDetected) {
        globalStats.countDeadlock++;

        cout << COLOR_RED << "[DEADLOCK] No process can proceed — potential deadlock state.\n";
        cout << "Blocked customers: ";
        fullLog << "[DEADLOCK] No process can proceed — potential deadlock state.\n";
        fullLog << "Blocked customers: ";
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
            if (!finish[i]) {
                cout << "P" << i << " ";
                fullLog << "P" << i << " ";
            }
        }
        cout << "\n";
        fullLog << "\n";

        // Detailed Explanation: Why each process is blocked
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
            if (!finish[i]) {
                cout << COLOR_RED << "  - P" << i << " is blocked because it needs: ";
                fullLog << "  - P" << i << " is blocked because it needs: ";
                for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
                    if (need[i][j] > available[j]) {
                        cout << COLOR_YELLOW << "R" << j << "(" << need[i][j] << ") " << COLOR_RED;
                        fullLog << "R" << j << "(" << need[i][j] << ") ";
                    }
                }
                cout << "\n";
                fullLog << "\n";
            }
        }

        // Shows who is holding each critical resource
        cout << "\nResource holders:\n";
        fullLog << "\nResource holders:\n";
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            if (available[j] == 0) {
                cout << "  - R" << j << " held by: ";
                fullLog << "  - R" << j << " held by: ";
                for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
                    if (allocation[i][j] > 0) {
                        cout << "P" << i << "(" << allocation[i][j] << ") ";
                        fullLog << "P" << i << "(" << allocation[i][j] << ") ";
                    }
                }
                cout << "\n";
                fullLog << "\n";
            }
        }

        cout << COLOR_RESET;

        // Append to deadlock_log.csv
        ofstream dlog("logs/deadlock_log.csv", ios::app);
        if (dlog.is_open()) {
            time_t now = time(NULL);
            dlog << "[" << ctime(&now);
            dlog.seekp(-1, ios::cur); // Remove trailing newline from ctime
            dlog << "] Blocked Customers,";

            // List blocked customers
            for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
                if (!finish[i]) dlog << "P" << i << " ";
            }
            dlog << "\n";

            // Log missing resource causes
            for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
                if (!finish[i]) {
                    dlog << "P" << i << " needs:";
                    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
                        if (need[i][j] > available[j])
                            dlog << " R" << j << "(" << need[i][j] << ")";
                    }
                    dlog << "\n";
                }
            }

            dlog.close();
        }
        // Auto-savepoint before returning due to deadlock
        string autoLabel = "auto_P3_deadlock";
        savepoint(autoLabel);
        Logger::log("SAVEPOINT → Automatically saved as \"" + autoLabel + "\" before deadlock exit", Logger::INFO);
        return false;
    }

    // Optionally print safe sequence if enabled
    if (showSafeSequence) {
        cout << "[SAFE] Safe sequence: ";
        fullLog << "[SAFE] Safe sequence: ";
        for (int i = 0; i < idx; ++i) {
            cout << "P" << safeSequence[i] << (i < idx - 1 ? " → " : "");
            fullLog << "P" << safeSequence[i] << (i < idx - 1 ? " → " : "");
        }
        cout << endl;
        fullLog << endl;
    }

    return true;
}

/**
 * @brief Handles a resource request from a customer.
 *
 * Implements the Banker's resource-request algorithm:
 *     1. Ensure that the request does not exceed the customer's declared need
 *     2. Ensure that the request does not exceed available resources
 *     3. Tentatively allocate the resources
 *     4. Call 'ifSafe()' to check whether the system remains in a safe state
 *     5. If not safe, roll back to the previous state
 *
 * @param customerNum Index of the requesting customer (0-based).
 * @param request Array of requested units for each resource type.
 * @return 0 if the request is granted and the system remains safe;
 *         -1 if the request exceeds limits or results in an unsafe state.
 *         -2 if the request exceeds available resources
 *         -3 if granting the request would make the system unsafe
 */
int Banker::request(int customerNum, int request[]) {
    // Step 1: Check if request exceeds customer's declared need
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
        if (request[j] > need[customerNum][j]) {
            lastDenialReason = "Request denied: exceeds declared need.";
            Logger::log(lastDenialReason, Logger::WARN);
            return DENIED_NEED; // Equivalent to error conditions in ZyBook Section 8.6 Step 1
        }
    }

    // Step 2: Check if request exceeds currently available resources
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
        if (request[j] > available[j]) {
            lastDenialReason = "Request denied: exceeds available resources.";
            Logger::log(lastDenialReason, Logger::WARN);
            return DENIED_AVAIL; // Equivalent to must wait in Zybook Section 8.6 Step 2
        }
    }

    // Step 3: Tentatively allocate resources
    snapshot(); // Save current state in case we need to roll back

    // [CRITICAL SECTION START] Tentative allocation for safety check
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
        available[j] -= request[j];                      // Available -= Request
        allocation[customerNum][j] += request[j];        // Allocation += Request
        need[customerNum][j] -= request[j];             // Need -= Request
    }
    // [CRITICAL SECTION END] Tentative allocation

    // Step 4: Check if the system remains in a safe state
    if (isSafe()) {
        lastActiveCustomer = customerNum; // Mark who made the request
        lastDenialReason.clear();         // Clear previous denial
        return GRANTED;
    } else {
        // Step 5: Roll back if unsafe
        lastDenialReason = "Request denied: would lead to unsafe state.";
        Logger::log(lastDenialReason, Logger::WARN);
        restore();  // Restore system to state before tentative allocation
        return DENIED_UNSAFE;
    }
}

/**
 * @brief Releases resources back to the system from a customer.
 *
 * Decrements the allocation and increments the available and need matrices accordingly. Assumes the customer has the
 * resources being released.
 *
 * @param customerNum Index of the releasing customer.
 * @param release Array of units being released for each resource type.
 */
void Banker::release(int customerNum, int release[]) {
    // [CRITICAL SECTION START] Releasing resources back to system
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
        allocation[customerNum][j] -= release[j];
        available[j] += release[j];
        need[customerNum][j] += release[j];
    }
    // [CRITICAL SECTION END] Release complete
}

/**
 * @brief Prints the current state of the system.
 *
 * Outputs the `available`, `maximum`, `allocation`, and `need` matrices to both standard output and the `fullLog`
 * stream. Helpful for debugging and tracking resource distribution.
 */
void Banker::printMatrix(const string& title, const int matrix[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES]) const {
    cout << COLOR_MAGENTA << "\n" << title << ":\n" << COLOR_RESET;
    fullLog << "\n" << title << ":\n";
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
        cout << "P" << i << (i == lastActiveCustomer ? "*: " : ": ");
        fullLog << "P" << i << (i == lastActiveCustomer ? "*: " : ": ");
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            cout << matrix[i][j] << " ";
            fullLog << matrix[i][j] << " ";
        }
        cout << endl;
        fullLog << endl;
    }
}
/**
* @brief Handles a resource request from customer
*
* This method processes a resource request by a customer (e.g., "RQ 1 1 0 2 0"). It first checks whether the request is
* valid (i.e., does not exceed the customer's declared maximum need or the currently available resources). If the
* request is valid, the method simulates allocation and uses the Banker's Safety Algorithm to verify if the system
* remains in a safe state after granting the request.
*
* If the state is safe, the resources are officially allocated. If the request would result in an unsafe state, it is
* denied and rolled back.
*
* @param cust Index of the requesting customer (0-based)
* @param req Array of resource quantities requested for each resource type
* @ return true if the request is granted; false if denied due to safety or availability
*/
void Banker::printState() const {
    cout << COLOR_CYAN << "\n=== SYSTEM STATE ===\n" << COLOR_RESET;
    fullLog << "\n=== SYSTEM STATE ===\n";

    // Print Available Resources
    cout << COLOR_MAGENTA << "\nAvailable:\n" << COLOR_RESET << "    ";
    fullLog << "\nAvailable:\n    ";
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
        cout << available[j] << " ";
        fullLog << available[j] << " ";
    }
    cout << "\n    R0 R1 R2 R3\n";
    fullLog << "\n    R0 R1 R2 R3\n";

    // Print the other matrices
    printMatrix("Maximum", maximum);
    printMatrix("Allocation", allocation);
    printMatrix("Need", need);

    cout << endl;
    fullLog << endl;
}

/**
 * @brief Returns the current allocation matrix.
 *
 * Provides external access to the internal 2D allocation array, used for logging or testing purposes.
 *
 * @return A pointer to the allocation matrix of size [NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES].
 */
const int (*Banker::getAllocation() const)[NUMBER_OF_RESOURCES] {
    return allocation; // Return pointer to internal allocation matrix
}

/**
 * @brief Prints a summary report of system-wide resource usage.
 *
 * This includes:
 * - Current available units for each resource
 * - Total allocated units across all customers
 * - Total remaining need across all customers
 *
 * Output is written to both standard output and fullLog for traceability.
 */
void Banker::printReport() const {
    int totalAllocated[NUMBER_OF_RESOURCES] = {0};  // Tracks total allocated per resource
    int totalNeeded[NUMBER_OF_RESOURCES] = {0};     // Tracks total remaining need per resource

    // Compute column-wise totals from allocation and need matrices
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            totalAllocated[j] += allocation[i][j];  // Sum allocated resources
            totalNeeded[j] += need[i][j];           // Sum unmet needs
        }
    }

    // Begin report
    cout << "\n===== SYSTEM RESOURCE REPORT =====\n";
    fullLog << "\n===== SYSTEM RESOURCE REPORT =====\n";

    // Print available resources
    cout << "Available: ";
    fullLog << "Available: ";
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
        cout << available[j] << " ";
        fullLog << available[j] << " ";
    }
    cout << endl;
    fullLog << endl;

    // Print total allocated resources
    cout << "Allocated: ";
    fullLog << "Allocated: ";
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
        cout << totalAllocated[j] << " ";
        fullLog << totalAllocated[j] << " ";
    }
    cout << endl;
    fullLog << endl;

    // Print total remaining need
    cout << "Remaining Need: ";
    fullLog << "Remaining Need: ";
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
        cout << totalNeeded[j] << " ";
        fullLog << totalNeeded[j] << " ";
    }
    cout << endl;
    fullLog << endl;

    cout << "==================================\n";
    fullLog << "==================================\n";

    // Output same data to CSV file
    ofstream csv("logs/report.csv");
    if (csv.is_open()) {
        csv << "Type,R0,R1,R2,R3\n";

        // Available row
        csv << "Available,";
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            csv << available[j];
            if (j < NUMBER_OF_RESOURCES - 1) csv << ",";
        }
        csv << "\n";

        // Allocated row
        csv << "Allocated,";
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            csv << totalAllocated[j];
            if (j < NUMBER_OF_RESOURCES - 1) csv << ",";
        }
        csv << "\n";

        // Remaining need row
        csv << "RemainingNeed,";
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            csv << totalNeeded[j];
            if (j < NUMBER_OF_RESOURCES - 1) csv << ",";
        }
        csv << "\n";

        csv.close();
    }
}

/**
* @brief Resets the system to its initial state
*
* This method restores the system's resource state (allocation, need, available). to the snapshot captured after the
* maximum matrix was loaded. It is typically used in repsonse to the 'reset' command to undo all changes made since
* initialization. This allows repeatable testing or recovery without terminating the program.
*
* Also resets any runtime state variables, such as last active customer and the last denial reason, to their default
* states.
*/
void Banker::reset() {
    // [CRITICAL SECTION START] Resetting system to initial snapshot state
	// Restoring allocation and need matrices from snapshot
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            allocation[i][j] = allocationSnapshot[i][j]; // restoring allocations
            need[i][j] = needSnapshot[i][j];		     // restore needs
        }
    }

	// Restore available resource pool
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
        available[j] = availableSnapshot[j];
    }

	// Reset auxiliary state for tracking simulation behavior
    lastActiveCustomer = -1; // No customer is considered active anymore
    lastDenialReason.clear(); // Clear last denial resason
// [CRITICAL SECTION NEND] Reset complete
}

/**
 * @brief Saves the current system state for manual undo.
 *
 * This method captures the current available, allocation, and need matrices in temporary buffers. These can later be
 * restored via `restoreUndoSnapshot()` to manually revert the system state.
 */
void Banker::saveUndoSnapshot() {
	// Save available resource
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j)
        undoAvailable[j] = available[j];

	// Save allocation and need matrices
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i)
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            undoAllocation[i][j] = allocation[i][j];
            undoNeed[i][j] = need[i][j];
        }

    hasUndoSnapshot = true; // Mark snapshot as available
    Logger::log("SNAPSHOT → Manual snapshot saved", Logger::INFO);
}

/**
* @brief Restores the system state from the most manual undo snapshot.
*
* This method is used to rollback the system to a previously save state captured by a manual snapshot (e.g., using the
* 'snapshot' or 'savepoint' command). If no manual snapshot has been created, the operation fails with an error message.
*
* On successful restoration, the allocation, need, and available matrices are reset to the original snapshot. It also
* clears any previously recorded denial reasons or customer availability to a neutral state.
*/
void Banker::restoreUndoSnapshot() {
	// If no manual snapshot exists, log the failure and exit
    if (!hasUndoSnapshot) {
        Logger::log("UNDO → Failed: No snapshot to restore", Logger::WARN);
        cout << COLOR_RED << "[ERROR] No manual snapshot to restore.\n" << COLOR_RESET;
        fullLog << "[ERROR] No manual snapshot to restore.\n";
        return;
    }

	// Restpre available resources from undo snapshot
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j)
        available[j] = undoAvailable[j];

	// Restore allocation and need matrices for all customers
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i)
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            allocation[i][j] = undoAllocation[i][j];
            need[i][j] = undoNeed[i][j];
        }

	// Log the successful restoration
    Logger::log("UNDO → Manual snapshot restored", Logger::INFO);

	// Reset tracking states for diagnostics
    lastDenialReason.clear();	// Clear last denial explanation
    lastActiveCustomer = -1;	// Reset customer activity tracking
}
/**
* @brief Creates a named savepoint of the current system state.
*
* This method stores a snapshot of the current allocation, need, and available resource matrices using the given name
* as a key. These savepoints can be later be compared to or restored using commands like 'compare', 'diff', or 'load'.
*
* Savepoints enable users to track and analyze how the system state changes over time and are essential for debugging,
* rollback, and auditing purposes
*
* @param name The unique identifyer used to store the snapshot
*/
void Banker::savepoint(const string& name) {
	// Store the current available vector under a given name
    namedAvailable[name] = vector<int>(available, available + NUMBER_OF_RESOURCES);

	// Create deep copies of current allocation and need matrices
    vector<vector<int> > alloc(NUMBER_OF_CUSTOMERS, vector<int>(NUMBER_OF_RESOURCES));
    vector<vector<int> > needM(NUMBER_OF_CUSTOMERS, vector<int>(NUMBER_OF_RESOURCES));

	// Populate the temporary matrices with current system state
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i)
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            alloc[i][j] = allocation[i][j];	// Copy allocation matrix
            needM[i][j] = need[i][j];		// Copy need matrix
        }

	// Save named copies into global savepoint maps
    namedAllocation[name] = alloc;
    namedNeed[name] = needM;

	// Log the savepoint creation event
    Logger::log("SAVEPOINT → Named savepoint \"" + name + "\" created", Logger::INFO);
}

/**
* @brief Restores the system state to a previously named savepoint
*
* This method reverts the allocation, need, and available resource matrices to the state stored under the provided
* savepoint name. This enables users to undo changes and return the system to a known safe state.
*
* If the named savepoint does not exist, the rollback fails with an error.
*
* @param name The name of the savepoint to revert to
* @ return true if rollback succeeded, false if the savepoint does not exist
*/
bool Banker::rollback(const string& name) {
	// Check if the savepoint exists
    if (namedAvailable.find(name) == namedAvailable.end()) {
        Logger::log("ROLLBACK → Failed: No savepoint \"" + name + "\"", Logger::WARN);
        cout << COLOR_RED << "[ERROR] No savepoint named \"" << name << "\" exists.\n" << COLOR_RESET;
        fullLog << "[ERROR] No savepoint named \"" << name << "\" exists.\n";
        return false;
    }

	// Reference the saved matrices for efficiency
    vector<int>& savedAvail = namedAvailable[name];
    vector<vector<int> >& savedAlloc = namedAllocation[name];
    vector<vector<int> >& savedNeed = namedNeed[name];

	// Restore available matrices
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j)
        available[j] = savedAvail[j];

	// Restore allocation and need matrices
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i)
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            allocation[i][j] = savedAlloc[i][j];
            need[i][j] = savedNeed[i][j];
        }

    Logger::log("ROLLBACK → Reverted to savepoint \"" + name + "\"", Logger::INFO);

	// Restore status flags
    lastDenialReason.clear();
    lastActiveCustomer = -1;

    return true;
}
/**
* @brief Toggles whether to display the safe sequence after each request.
*
* When enabled, the system will print the safe sequence to standard output every time the Banker's Algorithm is run and
* determines a safe state.
*/
void Banker::toggleSafeSequence() {
    showSafeSequence = !showSafeSequence;
}

/**
* @brief Checks if safe sequence display is currently enabled.
*
* @return true if safe sequence output is enabled; false otherwise
*/
bool Banker::isSafeSequenceEnabled() const {
    return showSafeSequence;
}

/**
* @brief Retrieves the reason why the last resource was denied.
*
* if no request has been denied yet, a default message is returned. This supports better error feedback to users during
* to users during interactive mode.
*
* @return A string describing the last denial reason or a default message.
*/
string Banker::getLastDenialReason() const {
    return lastDenialReason.empty() ? "No request has been denied yet." : lastDenialReason;
}

/**
* @brief Saves the current system state to a file.
*
* This method writes the available resources, maximum demand matrix, and current allocation matrix to a specified file
* in a human-readble format. The output can be used for debugging, auditing, or resuming simulations.
*
* Each section (Available, Maximum, Allocation) is labeled and formatted with process indices (e.g., P0, P1)... for
* clarity.
*
* @param filename The name of the file to which the state will be saved.
*/
void Banker::saveState(const string& filename) const {
    ofstream out(filename.c_str());
    if (!out) return; // Abort if the file can't be opened

    // Save the available resource vector
    out << "Available:";
    for (int i = 0; i < NUMBER_OF_RESOURCES; ++i)
        out << " " << available[i];
    out << "\n";

    // Save the maximum resource demands per process
    out << "Maximum:\n";
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
        out << "P" << i << ":"; // Label each process row
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j)
            out << " " << maximum[i][j];
        out << "\n";
    }

    // Save the currently allocated resources per process
    out << "Allocation:\n";
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
        out << "P" << i << ":";
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j)
            out << " " << allocation[i][j];
        out << "\n";
    }
    out.close(); // Finalize file write
}

/**
* @brief Loads system state from a file.
*
* This method reads a system snapshot file previously generated by 'saveState()'.
* It expects the file to contain three structured sections:
* 	- Available resource vector
*	- Maximum matrix per process
* 	- Current allocation matrix per process
*
* After loading, it recalculates the need matrix as (maximum - allocation). This allows the system to resume from a
* saved state without reinitializing.
*
* @param filename The name of the file continuing from saved sate.
* @return true if the loading is successful, false if the file canot be opened or parsed
*/
bool Banker::loadState(const string& filename) {
    ifstream in(filename.c_str());
    if (!in) return false; // File couldn't be opened

    string line;

	// Read available line
    getline(in, line);  // Line format: Available: x, y, z....
    stringstream ss(line);
    string label;
    ss >> label;  // skip "Available:"
    for (int i = 0; i < NUMBER_OF_RESOURCES; ++i)
        ss >> available[i]; // Load available resources

	// Read maximum matrix
    getline(in, line);  // Skip line "Maximum:"
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
        getline(in, line); // Line format: "P0: x, y, z..."
        stringstream cs(line);
        cs >> label; // Skip "P#"
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j)
            cs >> maximum[i][j]; // Load maximum resource demand
    }

	// Read allocation matrix
    getline(in, line);  // Skip line "Allocation:"
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
        getline(in, line); // Line format: "P0: x, y, z..."
        stringstream cs(line);
        cs >> label; // Skip "P#"
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j)
            cs >> allocation[i][j]; // Load allocated resources
    }

    // Compute need matrix
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i)
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j)
            need[i][j] = maximum[i][j] - allocation[i][j];

    return true; // Successfully loaded all data
}

/**
 * @brief Checks if a resource request can be granted.
 *
 * Validates that the request does not exceed the customer's remaining need
 * or the currently available resources. This does not check for system safety.
 *
 * @param customerNum Index of the requesting customer.
 * @param request Resource amounts requested per type.
 * @return true if request is valid and grantable; false otherwise.
 */
bool Banker::wouldGrantRequest(int customerNum, const int request[]) const {
    for (int i = 0; i < NUMBER_OF_RESOURCES; ++i) {
		// Check 1: Does request exceed remaining need?
        if (request[i] > need[customerNum][i])
            return false; // request exceeds declared need

		// Check 2: Does request exceed what is available?
        if (request[i] > available[i])
            return false; // Not enough resources currently available
    }
	// Request is valid and can be considered for safety simulation
    return true;
}
/**
* @brief Simulates Banker's Algorithm to find a safe sequence after a request.
*
* This method simulates granting a resource request and runs the Banker's safety check to dtermine if the system would
* still be in a safe state.
*
* @param customerNum Index of the customer making the request
* @param request Resource amounts requested.
* @return A vector representing a safe state sequence of customer execution. Returns an empty vector if the system
* 		  would be unsafe
*/
vector<int> Banker::simulateSequence(int customerNum, const int request[]) {
    int work[NUMBER_OF_RESOURCES]; // Temporary resource tracker
    bool finish[NUMBER_OF_CUSTOMERS]; // Tracks which process can finish
    int allocCopy[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES]; // Simulated allocations
    int needCopy[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES]; // Simulated needs

    // Clone current system state
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            allocCopy[i][j] = allocation[i][j];
            needCopy[i][j] = need[i][j];
        }
        finish[i] = false;
    }

	// Simulate the request
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
        allocCopy[customerNum][j] += request[j];
        needCopy[customerNum][j] -= request[j];
    }

    vector<int> safeSeq;

	// Try to build a safe sequence
    bool progress = true;
    while (progress) {
        progress = false;
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
            if (!finish[i]) {
                bool canFinish = true;
                for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
                    if (needCopy[i][j] > work[j]) {
                        canFinish = false;
                        break;
                    }
                }
                if (canFinish) {
                    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j)
                        work[j] += allocCopy[i][j]; // Release resources
                    finish[i] = true;
                    safeSeq.push_back(i); // Add to safe sequence
                    progress = true;
                }
            }
        }
    }

	// If any process is not finished, system is not safe
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i)
        if (!finish[i])
            return vector<int>(); // return empty vector if not safe

    return safeSeq; // safe sequence found
}

/**
 * @brief Compares the current state with a named savepoint.
 *
 * Checks for any differences between the current allocation, need, and available matrices and the corresponding values
 * stored in the given savepoint. Outputs all mismatches to standard output.
 *
 * @param name The name of the savepoint to compare against.
 * @return true if the comparison was performed (savepoint exists),
 *         false if the savepoint is missing.
 */
bool Banker::compareToSavepoint(const string& name) {
	// Check if savepoint exists in all 3 stored maps
    if (namedAvailable.find(name) == namedAvailable.end() ||
        namedAllocation.find(name) == namedAllocation.end() ||
        namedNeed.find(name) == namedNeed.end()) {
        cout << "[COMPARE] Savepoint \"" << name << "\" not found.\n";
        return false;
        }

    bool changes = false; // Tracks if any mismatch is found

    // Comparing allocation matrix
    const vector<vector<int> >& savedAlloc = namedAllocation[name];
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            if (allocation[i][j] != savedAlloc[i][j]) {
                cout << "Allocation mismatch P" << i << " R" << j
                     << ": now " << allocation[i][j]
                     << ", was " << savedAlloc[i][j] << "\n";
                changes = true;
            }
        }
    }
    // Comparing need matrix
    const vector<vector<int> >& savedNeed = namedNeed[name];
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            if (need[i][j] != savedNeed[i][j]) {
                cout << "Need mismatch P" << i << " R" << j
                     << ": now " << need[i][j]
                     << ", was " << savedNeed[i][j] << "\n";
                changes = true;
            }
        }
    }
    // Comparing available matrix
    const vector<int>& savedAvail = namedAvailable[name];
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
        if (available[j] != savedAvail[j]) {
            cout << "Available mismatch R" << j
                 << ": now " << available[j]
                 << ", was " << savedAvail[j] << "\n";
            changes = true;
        }
    }
	// If no mimatches found, then notify the user
    if (!changes) {
        cout << "[COMPARE] No differences from savepoint \"" << name << "\"\n";
    }

    return true;
}

/**
 * @brief Compares current state with a savepoint and optionally prints the differences.
 *
 * Checks for any changes in allocation, need, or available matrices compared to a named savepoint.
 * If `display` is true, differences are printed to stdout.
 *
 * @param name The name of the savepoint to compare against.
 * @param display If true, output the detected differences to the console.
 * @return true if comparison was performed (savepoint exists), false if savepoint not found.
 */
bool Banker::diffFromSavepoint(const string& name, bool display) {
	// Validating savepoint existence in all three tracked maps
    if (namedAvailable.find(name) == namedAvailable.end() ||
        namedAllocation.find(name) == namedAllocation.end() ||
        namedNeed.find(name) == namedNeed.end()) {
        if (display) {
            cout << COLOR_RED << "[DIFF] Savepoint \"" << name << "\" not found.\n" << COLOR_RESET;
            fullLog << "[DIFF] Savepoint \"" << name << "\" not found.\n";
        }
        return false;
    }

    bool changes = false;

    if (display) cout << COLOR_CYAN << "[DIFF] Comparing to savepoint \"" << name << "\"\n" << COLOR_RESET;

	// Diff allocation
    const vector<vector<int> >& savedAlloc = namedAllocation[name];
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            if (allocation[i][j] != savedAlloc[i][j]) {
                if (display)
                    cout << "  Allocation P" << i << " R" << j
                         << " → now " << allocation[i][j]
                         << ", was " << savedAlloc[i][j] << "\n";
                changes = true;
            }
        }
    }
	// Diff need
    const vector<vector<int> >& savedNeed = namedNeed[name];
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
            if (need[i][j] != savedNeed[i][j]) {
                if (display)
                    cout << "  Need P" << i << " R" << j
                         << " → now " << need[i][j]
                         << ", was " << savedNeed[i][j] << "\n";
                changes = true;
            }
        }
    }
	// Diff available
    const vector<int>& savedAvail = namedAvailable[name];
    for (int j = 0; j < NUMBER_OF_RESOURCES; ++j) {
        if (available[j] != savedAvail[j]) {
            if (display)
                cout << "  Available R" << j
                     << " → now " << available[j]
                     << ", was " << savedAvail[j] << "\n";
            changes = true;
        }
    }
		// Notify if the system state has not changed
    if (!changes && display) {
        cout << COLOR_GREEN << "[DIFF] No changes from savepoint \"" << name << "\"\n" << COLOR_RESET;
    }

    return true;
}
