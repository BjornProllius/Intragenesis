#ifndef COMMUNITY_H
#define COMMUNITY_H

#include "Grid.h" // Include Grid.h for access to Cell and related constants

// Function declarations
void collectParentData(Cell* grid, Cell* cell, int index);
void collectNeighbourData(Cell* grid, Cell* cell, int index);
void collectChildData(Cell* grid, Cell* cell,int index);
void collectCommunityData(Cell* grid, int index);

#endif // COMMUNITY_H