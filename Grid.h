#ifndef GRID_H
#define GRID_H
#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include <pthread.h>

// this can be used to store data about another cell in the grid
typedef struct foreignCellData {
    int index; // Index of the cell in the grid
    int targetIndex;
    uint32_t hashValue; // Hash value of the cell's identity vector
    uint8_t team;
    int8_t action; // -1 if not targeting current cell, other numbers for other things
    unsigned int influence : 1; // clone = 0, payload = 1
    unsigned int exists : 1; // 0 if invalid, 1 if valid
  

} foreignCellData;



//first thing a cell should look at is whether its targeting its own team or a different team. 
// it then uses the rest of the targeting vector to filter the candidates



typedef struct Cell {

    // option to reorder interaction order?


    //Identity Vector:
    //  [0]:     attack vs talk
    //  [1]:     target same level or different level 
    //  [2]:     target most similar or most different values
    //  [3]:     target parent or child
    //  [4]:     target same team or different team
    //  [5]:     clone or payload





    foreignCellData neighbourInfo[MAX_NEIGHBOURS]; // Array of foreign cell data for neighbors (up to 8 neighbors)
    foreignCellData childInfo[MAX_CHILDREN]; // Array of foreign cell data for children (up to 4 children)  
    foreignCellData parentInfo; // Foreign cell data for the parent (1 parent)

    int targetIndex; // Index of the target cell for interaction (-1 if invalid, -10 if child, -20 if parent)

    uint8_t level; // Level of the cell in the grid (0 for base level, increases with depth)

    uint8_t row;
    uint8_t col;

    uint8_t team;

    uint8_t identity[NUM_VALUES];  // first 4 bits stores the current identity, last 4 bits stores the future identity
                                    // 0-3 are the targeting vector
                                    // 4+ are state information (e.g., team)   


    uint8_t payload[NUM_VALUES];


    uint32_t identityHash; // Hash of the identity vector for quick comparisons


    
    unsigned int action : 1; // Action of the cell (0 = attack, 1 = talk) (1 bit)
    unsigned int influence : 1; // clone = 0, payload = 1
    
    pthread_mutex_t lock; // Mutex for synchronizing access to this data

} Cell;





// Function declarations

// Create the grid and initialize cells
Cell* createGrid();

// Initialize a single cell with default values
void initializeCell(Cell* cell, uint8_t level, int index);

// Update the future identity of a cell based on input identity
void updateFutureIdentity(uint8_t* identity, uint8_t* inputIdentity);


// Find the starting index of a given level in the grid
int findStartIndexOfLevel(int level);



// Update the identity of a cell for the next simulation step
void updateCellIdentity(Cell* grid, int index);


// Decide the action for a cell (attack or talk) and set its target
void pickAction(Cell* grid, int index, unsigned int* targetVector);

// Pick a neighbor target based on similarity or team
void pickNeighbourTarget(Cell* grid, int index, bool sameTeam, bool similar);


// Reset a cell's identity and state for the next simulation step
void resetCell(Cell* cell);

void shuffleDirections(int directions[][2], int size);



#endif // GRID_H