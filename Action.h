#ifndef ACTION_H
#define ACTION_H

#include "Community.h"
#include "Board.h"
#include <string.h>
#include <stdbool.h>
#include "Grid.h" // Include Grid.h for access to Cell and related constants

// Function to pick an action for a cell based on its identity vector and a target vector
void pickAction(Cell* grid, int index, unsigned int* targetVector);

// Function to pick a neighbor target for a cell based on team and similarity preferences
void pickNeighbourTarget(Cell* grid, int index, bool sameTeam, bool similar);

#endif // ACTION_H