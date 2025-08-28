// Calla Chen
// Source Code File 11/11 for EECS 111 Project #3
#ifndef VALIDATOR_H
#define VALIDATOR_H

// Constants defining problem size
#define NUMBER_OF_CUSTOMERS 5
#define NUMBER_OF_RESOURCES 4

namespace Validator {
    // Returns true if customerNum is betwen 0 and NUMBER_OF_CUSTOMERS - 1
    bool isValidCustomer(int customerNum);

    // Returns true if all values in request[][] are non-negative
    bool isValidRequest(const int request[]);

    // Returns true if release[] values are non-negative and ≤ current allocation
    bool isValidRelease(const int release[], const int allocation[][NUMBER_OF_RESOURCES], int customerNum);
};

#endif //VALIDATOR_H