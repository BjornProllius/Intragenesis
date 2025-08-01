#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "Grid.h"
#include "config.h"
#include "Board.h"
#include "Community.h"
#include "Helpers.h"


// last n cells of the array are odd where n is the current level
// watch for team removal issues

//only works with square grids for now


Cell* createGrid() {
    // Calculate total number of cells across all levels
    int totalCells = 0;
    int rows = BASE_ROWS;
    int cols = BASE_COLS;
    int levels = LEVELS;

    for (int i = 0; i < levels; i++) {
        totalCells += rows * cols;
        rows /= 2; // Each level has half the rows
        cols /= 2; // Each level has half the columns
    }

    // Allocate memory for the flattened grid
    Cell* grid = malloc(totalCells * sizeof(Cell));
    if (!grid) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Initialize each cell in the grid
    uint8_t level = 0;
    int rowTracker = BASE_ROWS;
    int colTracker = BASE_COLS;
    int lastIndexOfLevel = rowTracker * colTracker - 1; // Last index of the current level

    for (int i = 0; i < totalCells; i++) {
        if (i > lastIndexOfLevel) {
            // Move to the next level
            level++;
            rowTracker /= 2; // Halve the number of rows for the next level
            colTracker /= 2; // Halve the number of columns for the next level
            lastIndexOfLevel += rowTracker * colTracker; // Update last index for the new level
        }

        initializeCell(&grid[i], level, i);

        // Debugging: Print the initialized cell
        //printf("Cell %d initialized at level %d\n", i, level);
    }

    return grid;
}








void initializeCell(Cell* cell, uint8_t level, int index) {


    //     if (pthread_mutex_init(&cell->lock, NULL) != 0) {
    //     fprintf(stderr, "Mutex initialization failed for cell at index %d\n", index);
    //     exit(EXIT_FAILURE);
    // }

        // Initialize interaction-related fields
    cell->action = rand() % 2; // Random action: 0 = attack, 1 = talk
    cell->influence = rand() % 2; // Random influence: 0 = clone, 1 = payload
    cell->targetIndex = -1;    // Default target index: no target (-1)

    unsigned int randomTeam = rand() % NUM_TEAMS; // Random value between 0 and NUM_TEAMS - 1
    cell->team = ((randomTeam & 0x0F) << 4) | (randomTeam & 0x0F);


    
    // Set the cell's level
    cell->level = level;
    //int rows = BASE_ROWS >> level;
    int cols = BASE_COLS >> level;

    //find row and col
    int startIndex = findStartIndexOfLevel(level);
    int relativeIndex = index - startIndex;
    cell->row = relativeIndex / cols;
    cell->col = relativeIndex % cols;


    // Initialize the identity vector
    for (int i = 0; i < NUM_VALUES; i++) {
        unsigned int randomValue = rand() % 16; // Random value between 0 and 15
        cell->identity[i] = (randomValue << 4) | randomValue; // Set the first 4 bits and copy them into the last 4 bits
    }

    //initialize the payload vector
    for (int i = 0; i < NUM_VALUES; i++) {
        unsigned int randomValue = rand() % 16; // Random value between 0 and 15
        cell->payload[i] = (randomValue << 4) | randomValue; // Set the first 4 bits and copy them into the last 4 bits
    }


    cell->identityHash = calculateSimHash(cell->identity); // Calculate the identity hash


        // Initialize neighbor, child, and parent info
    for (int i = 0; i < MAX_NEIGHBOURS; i++) {
        initializeForeignCellData(&cell->neighbourInfo[i]);
    }
    for (int i = 0; i < MAX_CHILDREN; i++) {
        initializeForeignCellData(&cell->childInfo[i]);
    }
    initializeForeignCellData(&cell->parentInfo);



}



void updateFutureIdentity(uint8_t* identity, uint8_t* inputIdentity) {
    for (int i = 0; i < NUM_VALUES; i++) { 
        // Get the last 4 bits of the current identity and the first 4 bits of the input identity
        unsigned int identityLast4Bits = identity[i] & 0x0F; // Mask to get last 4 bits
        unsigned int inputFirst4Bits = (inputIdentity[i] >> 4) & 0x0F; // Mask to get first 4 bits

        // Use branchless operations to update the last 4 bits
        int diff = inputFirst4Bits - identityLast4Bits;
        identity[i] = (identity[i] & 0xF0) | ((identityLast4Bits + (diff > 0) - (diff < 0)) & 0x0F);
    }
}








