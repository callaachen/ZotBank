// Calla Chen
// Source Code File 10/11 for EECS 111 Project #3
#include "validator.h"

/**
* @brief Validates the given customer index
*
* Checks whether the customer number falls within the valid range.
*
* @param customerNum Index of the customer to validate
* @return true if customerNum is between 0 and NUMBER_OF_CUSTOMERS - 1; false otherwise
*/
bool Validator::isValidCustomer(int customerNum) {
    return customerNum >= 0 && customerNum < NUMBER_OF_CUSTOMERS;
}

/**
* @brief Validates all requested resources are non-negative.
*
* Ensures the equested resource array contains no negative values.
*
* @param request Array of requested units for each resource.
* @return true if all values are ≥ 0; false if any negative value is found.
*/
bool Validator::isValidRequest(const int request[]) {
    for (int i = 0; i < NUMBER_OF_RESOURCES; ++i) {
        if (request[i] < 0) return false; // Resource request must not be negative
    }
    return true;
}

/**
 * @brief Validates whether a customer can release the specified resources.
 *
 * Checks that the release does not include negative values or exceed the customer's current allocation.
 *
 * @param release Array of units the customer wants to release.
 * @param allocation 2D matrix of current resource allocations per customer.
 * @param customerNum Index of the customer requesting the release.
 * @return true if the release is valid; false otherwise.
 */
bool Validator::isValidRelease(const int release[], const int allocation[][NUMBER_OF_RESOURCES], int customerNum) {
    if (!isValidCustomer(customerNum)) return false; // Request if customer ID is invalid

    for (int i = 0; i < NUMBER_OF_RESOURCES; ++i) {
        // Each release must be within the allocated amount and non-negative
        if (release[i] < 0 || release[i] > allocation[customerNum][i]) return false;
    }
    return true;
}