// Calla Chen
// Source Code File 4/11 for EECS 111 Project #3
#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <string>
#include "banker.h"

// Global flag to signal exit from test mode
extern bool exitAfterTest;

class CommandHandler{
public:
    // Status codes returned by the process function
    enum Status {
        CONTINUE, // Continue processing commands
        EXIT,     // Exiting the simulation
        DENIED,   // Request denied (invalid or unsafe)
        SUCCESS   // Savepoint success
    };

    std::string message;

    struct Result {
        Status status;
        bool isRequest;
        bool isRelease;
        bool wasDenied;
        bool wasUnsafe;
        bool exceedsNeed;
        bool exceedsAvail;
    };

    /**
    * @brief Parses and processes a user command string
    * Supports: RQ (request), RL (release), *, reset, exit
    *
    * @param input The raw command string entered by the user
    * @param banker Reference to the Banker instance for resource control
    * @return Status code indicating action taken
    */
    static Result process(const std::string& input, Banker& banker);
};

#endif //COMMAND_HANDLER_H