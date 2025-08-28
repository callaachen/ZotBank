// Calla Chen
// Source Code File 2/11 for EECS 111 Project #3
#ifndef BANKER_H
#define BANKER_H

#include <string>
#include <map>
#include <vector>

#define NUMBER_OF_CUSTOMERS 5
#define NUMBER_OF_RESOURCES 4

class Banker {
public:
    Banker();                                          // Constructor: initializes all source metrics and arrays to be 0
    void setAvailable(int res[]);                      // Loads minimum demand matrix from input file
    bool loadMaximumFromFile(const std::string& filename);    // Loads max demand matrix from input file
    void calculateNeed();                                     // Computes the need matrix from input file
    int request(int customerNum, int request[]);              // Attempts to allocate requested resources if safe
    void release(int customerNum, int release[]);             // Releases held resources back to the system
    void snapshot();                                          // Saves a backup of current status state
    void restore();                                           // Restores system state from last snapshot
    void reset();
    void printState() const;                                  // Displays current state: available, max, alloc, need
    void printReport() const;                                 // Prints system resource report
    void toggleSafeSequence();                                // Toggles the display of safe sequence
    bool isSafeSequenceEnabled() const;                       // Returns whether safe sequence is enabled

	// Utility to print formatted matrix
    void printMatrix(const std::string& title, const int matrix[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES]) const;

    void saveUndoSnapshot();                                  // Called on `snapshot` command
    void restoreUndoSnapshot();                               // Called on `undo` command
    void savepoint(const std::string& name);                  // Savepoint command
    bool rollback(const std::string& name);                  // Rollback to savepoint
    bool compareToSavepoint (const std::string& name);		 // Compares current state to savepoint (prints diffs)
    bool diffFromSavepoint(const std::string& name, bool display = true); // Diffs & optionally displays results

    const int (*getAllocation() const) [NUMBER_OF_RESOURCES]; // Getter for allocation matrix (used externally)

    enum RequestResult {
        GRANTED = 0,
        DENIED_NEED = -1,		// Rejected due to exceeding declared max need
        DENIED_AVAIL = -2,		// Rejected due to insufficient available resources
        DENIED_UNSAFE = -3		// Rejected due to unsafe system state
    };

    std::string getLastDenialReason() const; // Explanation of last rejected request

    void saveState(const std::string& filename) const; // Dumps system state to file
    bool loadState(const std::string& filename);	   // Loads system from file

	bool wouldGrantRequest(int customerNum, const int request[]) const; // Pre-checks if request is valid
	std::vector<int> simulateSequence(int customerNum, const int request[]); // Simulates safe sequence if granted req

private:
	// Core matrices
    int available[NUMBER_OF_RESOURCES];                       // Currently available units per source
    int maximum[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];    // Max demand per customer
    int allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES]; // Currently allocated units
    int need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];       // Remaining need per customer

	// Backup snapshot used by 'snapshot()' / 'restore()'
    int backupAvailable[NUMBER_OF_RESOURCES];                        // Snapshot: available
    int backupAllocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];  // Snapshot: allocation
    int backupNeed[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];        // Snapshot: need

    bool isSafe();                  // Internal safety check using Banker's Algorithm
    bool showSafeSequence;          // Controls whether safe sequence is printed

	int lastActiveCustomer;        // Set to -1 initially (nobody yet)
    bool hasUndoSnapshot;		   // True if a manual undo snapshot is available
    bool hasSavepoint;			   // Unused in logic but is researved for potential extensions

    // Snapshot of the initial state for the reset feature
    int allocationSnapshot[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
    int needSnapshot[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
    int availableSnapshot[NUMBER_OF_RESOURCES];

    // Manual undo snapshot
    int undoAvailable[NUMBER_OF_RESOURCES];
    int undoAllocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
    int undoNeed[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

    // Savepoint system
    std::map<std::string, std::vector<int> > namedAvailable;					// Savepoint: Available
    std::map<std::string, std::vector<std::vector<int> > > namedAllocation;		// Savepoint: Allocation
    std::map<std::string, std::vector<std::vector<int> > > namedNeed;			// Savepoint: Need

    std::string lastDenialReason; // Reason for last denied request
};
#endif //BANKER_H