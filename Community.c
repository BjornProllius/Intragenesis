#include "Grid.h"
#include "Community.h"
#include "Board.h"
#include <string.h>
#include "Helpers.h"


void resolveInput(Cell* grid, Cell* cell, int index);


//currently the whole cell is being copied each time. In the future, only copy parts of the cell

// Define relative offsets for the 8 neighbours as a global constant
const int directions[8][2] = {
    {-1, -1}, {-1, 0}, {-1, 1}, // Top-left, Top, Top-right
    {0, -1},          {0, 1},  // Left, Right
    {1, -1}, {1, 0}, {1, 1}    // Bottom-left, Bottom, Bottom-right
};

//shuffle these as well
const int childDirections[4][2] = {
    {0, 0}, // Top-left child
    {0, 1}, // Top-right child
    {1, 0}, // Bottom-left child
    {1, 1}  // Bottom-right child
};



//make local copies of the cell data then do math on that, then copy changes back
void collectParentData(Cell* grid, Cell* cell, int index) {
  

    // Check if the cell has a parent
    if (cell->level == LEVELS - 1) {
        return; // Exit the function if the parent does not exist
    }
    int cols = BASE_COLS >> cell->level; //find total cols of current layer
    //int rows = BASE_ROWS >> cell->level; // find total rows of current layer

    int parentRow = cell->row / 2;
    int parentCol = cell->col / 2;
    int parentCols = cols / 2; // Number of columns in the parent layer
    int parentOffset = parentRow * parentCols + parentCol; // Calculate and return parent offset


    int parentStartIndex = findStartIndexOfLevel(cell->level + 1); // Get the start index of the parent level
    int parentIndex = parentStartIndex + parentOffset;

    


    Cell* parent = &grid[parentIndex]; // Access the parent cell
    Cell tempParent;

    // Lock the neighbour and make a copy
    pthread_mutex_lock(&parent->lock);
    //maybe just copy the important data instead of the whole parent
    memcpy(&tempParent, parent, sizeof(Cell));
    pthread_mutex_unlock(&parent->lock);


    
    populateForeignCellData(&cell->parentInfo, &tempParent, parentIndex);  
     


}

//consider making local copies the the cells rather than modifying them in place
void collectNeighbourData(Cell* grid, Cell* cell, int index) {

    int rows = BASE_ROWS >> cell->level;
    int cols = BASE_COLS >> cell->level;
    int row = cell->row;
    int col = cell->col;

    int levelStartIndex = findStartIndexOfLevel(cell->level); // Get the start index of the current level

    int localDirections[8][2]; // Local copy of directions to shuffle
    memcpy(localDirections, directions, sizeof(directions)); // Copy global directions to local array
    shuffleDirections(localDirections, 8); // Shuffle the local directions array

    for (int i = 0; i < MAX_NEIGHBOURS; i++) {
        int neighbourRow = row + localDirections[i][0];
        int neighbourCol = col + localDirections[i][1];

        // Precompute bounds checks
        if (neighbourRow < 0 || neighbourRow >= rows || neighbourCol < 0 || neighbourCol >= cols) {
            cell->neighbourInfo[i].exists = 0;
            continue; // Skip out-of-bounds neighbors
        }

        // Calculate the 1D index of the neighbour
        int neighbourIndex = levelStartIndex + neighbourRow * cols + neighbourCol;

        // Access the neighbour cell
        Cell* neighbour = &grid[neighbourIndex];
        Cell tempNeighbour;

        
        // Lock the neighbour and make a copy
        pthread_mutex_lock(&neighbour->lock);
        memcpy(&tempNeighbour, neighbour, sizeof(Cell));
        pthread_mutex_unlock(&neighbour->lock);

        // Populate foreign cell data
        populateForeignCellData(&cell->neighbourInfo[i], &tempNeighbour, neighbourIndex);


    }
}


void collectChildData(Cell* grid, Cell* cell, int index) {


     // Check if the cell has children
     if (cell->level == 0) {
        return; // Exit the function if the cell does not have children
    }

    int rows = BASE_ROWS >> cell->level;
    int cols = BASE_COLS >> cell->level;
    int row = cell->row;
    int col = cell->col;


    int childStartIndex = findStartIndexOfLevel(cell->level - 1); // Get the start index of the child level


    int topLeftChildRow = row * 2;
    int topLeftChildCol = col * 2;
    int childCols = cols * 2; // Number of columns in the child layer

    //shuffle the child directions here
    int localChildDirections[4][2]; // Local copy of directions to shuffle
    memcpy(localChildDirections, childDirections, sizeof(childDirections)); // Copy global directions to local array
    shuffleDirections(localChildDirections, 4); // Shuffle the local directions array
   

    // Iterate through the 4 possible children
    for (int i = 0; i < MAX_CHILDREN; i++) {
        int childRow = topLeftChildRow + localChildDirections[i][0];
        int childCol = topLeftChildCol + localChildDirections[i][1];

        

        // Check if the child is within bounds
        if (childRow >= 0 && childRow < rows * 2 && childCol >= 0 && childCol < cols * 2) {
            // Calculate the 1D index of the child
            int childIndex = childStartIndex + childRow * childCols + childCol;

            Cell* child = &grid[childIndex]; // Access the child cell
            Cell tempChild;

            // Lock the neighbour and make a copy
            pthread_mutex_lock(&child->lock);
            memcpy(&tempChild, child, sizeof(Cell));
            pthread_mutex_unlock(&child->lock);


            populateForeignCellData(&cell->childInfo[i], &tempChild, childIndex);

 
        }
            
    }
}

void collectCommunityData(Cell* grid, int index) {
    // Access the current cell
    Cell* cell = &grid[index];
    Cell tempCell;

    // Lock the cell and make a copy
    pthread_mutex_lock(&cell->lock);
    memcpy(&tempCell, cell, sizeof(Cell));
    pthread_mutex_unlock(&cell->lock);

    // Collect data from parent, neighbours, and children using the temporary copy
    collectParentData(grid, &tempCell, index);
    collectNeighbourData(grid, &tempCell, index);
    collectChildData(grid, &tempCell, index);

    // Resolve the input for the cell
    resolveInput(grid, &tempCell, index);

    // Lock the cell again and write back the updated data
    pthread_mutex_lock(&cell->lock);
    memcpy(cell, &tempCell, sizeof(Cell));
    pthread_mutex_unlock(&cell->lock);
}




void initializeInputs(int* friendlyAttackerIndexes, int* enemyAttackerIndexes, int* talkerIndexes) {

    memset(friendlyAttackerIndexes, -1, sizeof(int) * (MAX_CHILDREN + MAX_NEIGHBOURS + 1));
    memset(enemyAttackerIndexes, -1, sizeof(int) * (MAX_CHILDREN + MAX_NEIGHBOURS + 1));
    memset(talkerIndexes, -1, sizeof(int) * (MAX_CHILDREN + MAX_NEIGHBOURS + 1));
}




void resolveParentInput(Cell* cell,int cellTeam, bool* parentSameTeam, int* highestAttackLevel, int* talkerIndexes, int* numTalkers) {
    //printf("ParentExists: %d, Team: %d, Action: %d\n", parentInput->exists, parentInput->team, parentInput->action);
    if (cell->parentInfo.exists) {
        if (cell->parentInfo.team == cellTeam) {
            *parentSameTeam = true;
        }
        if (cell->parentInfo.action == 0) { // Attack
     
            *highestAttackLevel = 2;
        } else if (cell->parentInfo.action == 1) { // Talk
            talkerIndexes[0] = cell->parentInfo.index;
            (*numTalkers)++;
        }
    }
}

void resolveNeighborInput(Cell* cell, int cellTeam, int* highestAttackLevel,
                          int* friendlyAttackerIndexes, int* numFriendlyAttackers,
                          int* enemyAttackerIndexes, int* numEnemyAttackers,
                          int* talkerIndexes, int* numTalkers) {
    for (int i = 0; i < MAX_NEIGHBOURS; i++) {
        if (cell->neighbourInfo[i].exists) {
            if (cell->neighbourInfo[i].action == 0) { // Attack
                if (*highestAttackLevel < 2) {
                    *highestAttackLevel = 1;
                }
                if (cell->neighbourInfo[i].team == cellTeam) {
                    friendlyAttackerIndexes[(*numFriendlyAttackers)++] = cell->neighbourInfo[i].index;
                } else {
                    enemyAttackerIndexes[(*numEnemyAttackers)++] = cell->neighbourInfo[i].index;
                }
            } else if (cell->neighbourInfo[i].action == 1) { // Talk
                talkerIndexes[(*numTalkers)++] = cell->neighbourInfo[i].index;
            }
        }
    }
}

void resolveChildInput(Cell* cell, int cellTeam, int* highestAttackLevel,
                       int* friendlyAttackerIndexes, int* numFriendlyAttackers,
                       int* enemyAttackerIndexes, int* numEnemyAttackers,
                       int* talkerIndexes, int* numTalkers) {
    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (cell->childInfo[i].exists) {
            if (cell->childInfo[i].action == 0) { // Attack
                if (*highestAttackLevel < 1) {
                    *highestAttackLevel = 1;
                }
                if (cell->childInfo[i].team == cellTeam) {
                    friendlyAttackerIndexes[(*numFriendlyAttackers)++] = cell->childInfo[i].index;
                } else {
                    enemyAttackerIndexes[(*numEnemyAttackers)++] = cell->childInfo[i].index;
                }
            } else if (cell->childInfo[i].action == 1) { // Talk
                talkerIndexes[(*numTalkers)++] = cell->childInfo[i].index;
            }
        }
    }
}

int determineWinningTalker(int numTalkers, int* talkerIndexes) {
    if (numTalkers > 0) {
        int random = pcg32() % numTalkers;
        return talkerIndexes[random];
    }
    return -1;
}


int determineWinningAttacker(int highestAttackLevel, int numEnemyAttackers, int* enemyAttackerIndexes,
                             int numFriendlyAttackers, int* friendlyAttackerIndexes, bool parentSameTeam) {


    
    if (highestAttackLevel == 2) {
        
        return parentSameTeam ? -1 : enemyAttackerIndexes[0]; // Parent attack
    } else if (numEnemyAttackers > numFriendlyAttackers) {
        int random = pcg32() % numEnemyAttackers;
        return enemyAttackerIndexes[random];
    } else if (numFriendlyAttackers > 0) {
        int random = pcg32() % numFriendlyAttackers;
        return friendlyAttackerIndexes[random];
    }
    return -1;
}



void applyWinningTalker(Cell* cell, Cell* winningTalker) {
    // pthread_mutex_lock(&cell->lock); // Lock the cell
    // pthread_mutex_lock(&winningTalker->lock); // Lock the winning talker cell

    if (winningTalker->influence == 0) { // Talker decides to clone
        for (int i = 0; i < NUM_VALUES; i++) {
            uint8_t talkerFirst4Bits = (winningTalker->identity[i] >> 4) & 0x0F; // Extract the first 4 bits
            cell->identity[i] = (cell->identity[i] & 0xF0) | talkerFirst4Bits;   // Update the last 4 bits
        }
    } else if (winningTalker->influence == 1) { // Talker decides to inject payload
        for (int i = 0; i < NUM_VALUES; i++) {
            uint8_t talkerPayloadFirst4Bits = (winningTalker->payload[i] >> 4) & 0x0F; // Extract the first 4 bits of the payload
            cell->identity[i] = (cell->identity[i] & 0xF0) | talkerPayloadFirst4Bits;   // Update the last 4 bits
        }
    }

    // pthread_mutex_unlock(&cell->lock); // Unlock the cell
    // pthread_mutex_unlock(&winningTalker->lock); // Unlock the winning talker cell
}

void applyWinningAttacker(Cell* cell, Cell* winningAttacker, bool powerAttack) {
    // pthread_mutex_lock(&winningAttacker->lock); // Lock the cell
    // pthread_mutex_lock(&cell->lock); // Lock the cell

    if (powerAttack) {
        if (winningAttacker->influence == 0) { // Attacker decides to clone
            for (int i = 0; i < NUM_VALUES; i++) {
                uint8_t attackerFirst4Bits = (winningAttacker->identity[i] >> 4) & 0x0F; // Extract the first 4 bits
                cell->identity[i] = (cell->identity[i] & 0xF0) | attackerFirst4Bits;   // Update the last 4 bits
            }
            uint8_t attackerTeamLast4Bits = winningAttacker->team & 0x0F; // Extract the last 4 bits of the attacker's team
            cell->team = (cell->team & 0xF0) | attackerTeamLast4Bits;     // Update only the last 4 bits of the cell's team
        } else if (winningAttacker->influence == 1) { // Attacker decides to inject payload
            for (int i = 0; i < NUM_VALUES; i++) {
                uint8_t attackerPayloadFirst4Bits = (winningAttacker->payload[i] >> 4) & 0x0F; // Extract the first 4 bits of the payload
                cell->identity[i] = (cell->identity[i] & 0xF0) | attackerPayloadFirst4Bits;   // Update the last 4 bits
            }
            uint8_t attackerTeamLast4Bits = winningAttacker->team & 0x0F; // Extract the last 4 bits of the attacker's team
            cell->team = (cell->team & 0xF0) | attackerTeamLast4Bits;     // Update only the last 4 bits of the cell's team
        }
    } else {
        uint8_t attackerTeamLast4Bits = winningAttacker->team & 0x0F; // Extract the last 4 bits of the attacker's team
        cell->team = (cell->team & 0xF0) | attackerTeamLast4Bits;     // Update only the last 4 bits of the cell's team
    }

    // pthread_mutex_unlock(&cell->lock); // Unlock the cell
    // pthread_mutex_unlock(&winningAttacker->lock); // Unlock the winning attacker cell
}

void resolveInput(Cell* grid, Cell* cell, int index) {

    int cellTeam = cell->team;


    int friendlyAttackerIndexes[MAX_CHILDREN + MAX_NEIGHBOURS + 1];
    int enemyAttackerIndexes[MAX_CHILDREN + MAX_NEIGHBOURS + 1];
    int talkerIndexes[MAX_CHILDREN + MAX_NEIGHBOURS + 1];

    int numFriendlyAttackers = 0, numEnemyAttackers = 0, numTalkers = 0;
    int highestAttackLevel = -1;
    bool parentSameTeam = false;

    initializeInputs(friendlyAttackerIndexes, enemyAttackerIndexes, talkerIndexes);
    
    //pass copy of cell rather than the cell itself to avoid thread conflict
    resolveParentInput(cell, cellTeam, &parentSameTeam, &highestAttackLevel, talkerIndexes, &numTalkers);
    resolveNeighborInput(cell, cellTeam, &highestAttackLevel, friendlyAttackerIndexes, &numFriendlyAttackers,
                         enemyAttackerIndexes, &numEnemyAttackers, talkerIndexes, &numTalkers);
    resolveChildInput(cell, cellTeam, &highestAttackLevel, friendlyAttackerIndexes, &numFriendlyAttackers,
                      enemyAttackerIndexes, &numEnemyAttackers, talkerIndexes, &numTalkers);

    
    int winningTalkerIndex = determineWinningTalker(numTalkers, talkerIndexes);
    int winningAttackerIndex = determineWinningAttacker(highestAttackLevel, numEnemyAttackers, enemyAttackerIndexes,
                                                        numFriendlyAttackers, friendlyAttackerIndexes, parentSameTeam);
    //printf("winningAttackerindex = %d \n", winningTalkerIndex);
                                                        
    // Determine if it's a power attack
    bool mixedAttacks = (numEnemyAttackers > 0 && numFriendlyAttackers > 0);
    bool powerAttack = !mixedAttacks;
    if (highestAttackLevel == 2 && !parentSameTeam) powerAttack = false;
    if (numTalkers > 0 && highestAttackLevel < 2) powerAttack = false;

    // Apply the winning talker logic
    if (winningTalkerIndex >= 0) {
        Cell tempWinningTalker;
    
        // Lock the winning talker and make a copy
        pthread_mutex_lock(&grid[winningTalkerIndex].lock);
        memcpy(&tempWinningTalker, &grid[winningTalkerIndex], sizeof(Cell));
        pthread_mutex_unlock(&grid[winningTalkerIndex].lock);
    
        // Pass the copy to the function
        applyWinningTalker(cell, &tempWinningTalker);
    }
    
    // Apply the winning attacker logic
    if (winningAttackerIndex >= 0) {
        Cell tempWinningAttacker;
    
        // Lock the winning attacker and make a copy
        pthread_mutex_lock(&grid[winningAttackerIndex].lock);
        memcpy(&tempWinningAttacker, &grid[winningAttackerIndex], sizeof(Cell));
        pthread_mutex_unlock(&grid[winningAttackerIndex].lock);
    
        // Pass the copy to the function
        applyWinningAttacker(cell, &tempWinningAttacker, powerAttack);
    }

    //making copies should be ok because the only part that changes are the parts other threads dont
    // check in this step?


    //in this step cells only look at the neighbours first 4 bits while only modifying their own last 4 bits



}