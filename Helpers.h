#ifndef HELPERS_H
#define HELPERS_H

#include <stdint.h>
#include "Grid.h" // Include Grid.h for access to Cell and related constants

// Function to calculate the SimHash of an identity vector
uint32_t calculateSimHash(uint8_t* identity);

// Function to calculate the Hamming distance between two hashes
int calculateHammingDistance(uint32_t hash1, uint32_t hash2);

// Function to shuffle directions in a 2D array
void shuffleDirections(int directions[][2], int size);

// Function to find the start index of a given level in the grid
int findStartIndexOfLevel(int level);

// Function to update the identity of a cell for the next simulation step
void updateCellIdentity(Cell* grid, int index);

void populateForeignCellData(foreignCellData* foreignData, Cell* cell, int index);

void initializeForeignCellData(foreignCellData* data);

#endif // HELPERS_H