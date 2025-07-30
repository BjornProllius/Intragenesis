#include "Grid.h"
#include "Community.h"
#include "Board.h"
#include <string.h>

uint32_t calculateSimHash(uint8_t* identity) {
    uint32_t hash = 0;
    for (int i = 0; i < NUM_VALUES; i++) {
        uint8_t first4Bits = (identity[i] >> 4) & 0x0F; // Extract the first 4 bits
        hash ^= (first4Bits << (i * 4)); // Shift each value by its index multiplied by 4
    }
    return hash;
}

int calculateHammingDistance(uint32_t hash1, uint32_t hash2) {
    uint32_t x = hash1 ^ hash2; // XOR the two hashes
    int distance = 0;

    // Count the number of set bits (Hamming distance)
    while (x) {
        distance += x & 1; // Increment for each set bit
        x >>= 1; // Shift right to check the next bit
    }

    return distance;
}


void shuffleDirections(int directions[][2], int size) {
    for (int i = size - 1; i > 0; i--) {
        int j = pcg32(&pcgState) % (i + 1); // Pick a random index
        // Swap directions[i] and directions[j]
        int temp0 = directions[i][0];
        int temp1 = directions[i][1];
        directions[i][0] = directions[j][0];
        directions[i][1] = directions[j][1];
        directions[j][0] = temp0;
        directions[j][1] = temp1;
    }
}


int findStartIndexOfLevel(int level) {
    int rows = BASE_ROWS;
    int cols = BASE_COLS;
    int startIndex = 0;

    // Calculate the start index for the given level
    for (int i = 0; i < level; i++) {
        startIndex += rows * cols; // Add the number of cells in the current level
        rows /= 2; // Halve the number of rows for the next level
        cols /= 2; // Halve the number of columns for the next level
    }

    return startIndex; // Return the calculated start index
}


void updateCellIdentity(Cell* grid, int index) {
    Cell* cell = &grid[index]; // Access the current cell


    // Iterate through every value in the identity vector
    
    for (int i = 0; i < NUM_VALUES; i++) {
        uint8_t last4Bits = cell->identity[i] & 0x0F; // Extract last 4 bits
        cell->identity[i] = (last4Bits << 4) | last4Bits; // Copy last 4 bits into the first 4 bits
    }
    //move last 4 bits of team into first 4 bits of team
    cell->team = (cell->team & 0x0F) << 4 | (cell->team & 0x0F); // Copy last 4 bits into the first 4 bits

    cell->identityHash = calculateSimHash(cell->identity);

}

void populateForeignCellData(foreignCellData* foreignData, Cell* cell, int index) {
    // Populate the foreignCellData struct with information from the Cell
    foreignData->index = index; // Use the cell's target index
    foreignData->hashValue = cell->identityHash; // Use the cell's identity hash
    foreignData->team = cell->team; // Use the cell's team
    foreignData->action = cell->action; // Use the cell's action
    foreignData->influence = cell->influence; // Use the cell's influence
    foreignData->exists = 1; //the cell exists
}

void initializeForeignCellData(foreignCellData* data) {
    data->index = -1;          // Default index (-1 indicates invalid)
    data->hashValue = 0;       // Default hash value (0 indicates no data)
    data->team = 0;            // Default team (0 indicates no team assigned)
    data->action = -1;         // Default action (-1 indicates no action)
    data->influence = 0;       // Default influence (clone = 0)
    data->exists = 0;          // Mark as invalid (exists = 0)
    data->targetIndex =-1;
}