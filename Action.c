#include "Action.h"
#include "limits.h"
#include "Helpers.h"



void pickAction(Cell* grid, int index, unsigned int* targetVector) {
    Cell* cell = &grid[index]; // Access the current cell

    //int rows = BASE_ROWS >> cell->level; // Find total rows of current layer
    //int cols = BASE_COLS >> cell->level; // Find total cols of current layer

    // Extract the first 4 bits of the identity vector values
    unsigned int cellAttackVsTalk = (cell->identity[ATTACK_VS_TALK] >> 4) & 0x0F; // Get the first 4 bits of attack vs talk
    unsigned int cellTargetSameLevel = (cell->identity[TARGET_SAME_LEVEL] >> 4) & 0x0F; // Get the first 4 bits of target same level
    unsigned int cellTargetMostSimilar = (cell->identity[TARGET_SIMILARITY] >> 4) & 0x0F; // Get the first 4 bits of target most similar
    unsigned int cellTargetParentOrChild = (cell->identity[TARGET_PARENT_OR_CHILD] >> 4) & 0x0F; // Get the first 4 bits of target parent or child
    unsigned int cellTargetSameTeam = (cell->identity[TARGET_TEAM] >> 4) & 0x0F; // Get the first 4 bits of target team
    unsigned int cellInfluence = (cell->identity[INFLUENCE] >> 4) & 0x0F; // Get the first 4 bits of influence

    bool targetSimilar = false;
    bool targetSameTeam = false; 
    //printf("targetSameTeam value: %d\n", targetSameTeam);

    int sameLevel = 0; // Initialize same level flag

    // Check if cell chooses to attack or talk
    if (cellAttackVsTalk >= targetVector[ATTACK_VS_TALK]) {
        // Cell chooses to attack
        cell->action = 0; // Set action to attack
        //printf("Cell %d chooses to attack\n", index); // Debugging output
        
    } else {
        // Cell chooses to talk
        
        cell->action = 1; // Set action to talk
    }

    // Check if cell chooses to target same level or different level
     if (cellTargetSameLevel >= targetVector[TARGET_SAME_LEVEL]) {
         // Cell chooses to target same level
         sameLevel = 1; // Set same level flag
         
     } else {
         // Cell chooses to target different level
         sameLevel = 0; // Reset same level flag
         
    }

     //If cell chose to attack different level, target parent or child
     if (!sameLevel && cellTargetParentOrChild >= targetVector[TARGET_PARENT_OR_CHILD]) {
         // Cell chooses to target parent
         cell->targetIndex = -20; // Set target index to -20 for parent (indicating parent target)
     } else if (!sameLevel) {
         // Cell chooses to target child
         cell->targetIndex = -10; // Set target index to -10 for child (indicating child target)
     }


    // pick influence value
    //printf("cellInfluence value: %d\n", cellInfluence);
    //targetVector[INFLUENCE] = 5;
    //cellInfluence = 4; // Default to clone
    if (cellInfluence >= targetVector[INFLUENCE]) {
        // Cell chooses to clone
        cell->influence = 0; // Set influence to clone
    } else {
        // Cell chooses to payload
        cell->influence = 1; // Set influence to payload
    }

    // If cell chose to target same level, pick a neighbour or target
    if (sameLevel) {

        if(cellTargetMostSimilar >= targetVector[TARGET_SIMILARITY]) {
            // Cell chooses to target most similar identity
            targetSimilar = true; // Set target similar flag
        } else {
            // Cell chooses to target different identity
            targetSimilar = false; // Reset target similar flag
        }

        if (cellTargetSameTeam >= targetVector[TARGET_TEAM]) {
            // Cell chooses to target same team
            targetSameTeam = true; // Set target same team flag
        } else {
            // Cell chooses to target different team
            targetSameTeam = false; // Reset target same team flag

        }
        pickNeighbourTarget(grid, index, targetSameTeam, targetSimilar); // Call function to pick neighbour target based on identity vector
    }
 

    //resetCell(cell); // Reset the cell's identity vector for the next loop
}


void pickNeighbourTarget(Cell* grid, int index, bool sameTeam, bool similar) {
    Cell* cell = &grid[index]; // Access the current cell

    // Make local copies of the neighbour data
    foreignCellData neighbourInfo[MAX_NEIGHBOURS];
    memcpy(neighbourInfo, cell->neighbourInfo, sizeof(foreignCellData) * MAX_NEIGHBOURS);

    // Filter neighbors based on team
    for (int i = 0; i < MAX_NEIGHBOURS; i++) {
        if (neighbourInfo[i].exists) { // Check if the neighbour cell exists
            if (sameTeam && !(neighbourInfo[i].team == cell->team)) {
                neighbourInfo[i].exists = 0; // Mark as invalid if not the same team
            } else if (!sameTeam && (neighbourInfo[i].team == cell->team)) {
                neighbourInfo[i].exists = 0; // Mark as invalid if the same team
            }
        }
    }

    uint32_t cellIdentityHash = cell->identityHash; // Get the current cell's identity hash

    // Variables to track the best neighbor
    int bestNeighborIndex = -1;
    int bestHammingDistance = similar ? INT_MAX : INT_MIN; // Initialize based on similarity

    // Find the most similar or most different neighbor
    for (int i = 0; i < MAX_NEIGHBOURS; i++) {
        if (neighbourInfo[i].exists) { // Check if the neighbor cell exists
            int hammingDistance = calculateHammingDistance(cellIdentityHash, neighbourInfo[i].hashValue);

            if (similar) {
                // Find the smallest Hamming distance
                if (hammingDistance < bestHammingDistance) {
                    //printf("neighbourIndex = %d \n", neighbourInfo[i].index[i]);
                    bestHammingDistance = hammingDistance;
                    bestNeighborIndex = neighbourInfo[i].index;
                }
            } else {
                // Find the largest Hamming distance
                if (hammingDistance > bestHammingDistance) {
                    bestHammingDistance = hammingDistance;
                    bestNeighborIndex = neighbourInfo[i].index;
                }
            }
        }
    }
    //printf("bestNeighbourINdex = %d \n", bestNeighborIndex);

    // Set the target index to the best neighbor's index
    //printf("bestniehgbourindex = %d \n ", bestNeighborIndex);
    if (bestNeighborIndex != -1) {
        cell->targetIndex = bestNeighborIndex;
    } else {
        cell->targetIndex = -1; // No valid neighbor found
    }
}


void resetCell(Cell* cell) {
    // Reset the identity vector
    for (int i = 0; i < NUM_VALUES; i++) {
        // Copy the first 4 bits into the last 4 bits for each element
        cell->identity[i] = (cell->identity[i] & 0xF0) | ((cell->identity[i] >> 4) & 0x0F);
    }

    // Copy the first 4 bits of the team into the last 4 bits
    //cell->team = (cell->team & 0xF0) | ((cell->team >> 4) & 0x0F);

    // Recalculate the identity hash
    cell->identityHash = calculateSimHash(cell->identity);
}