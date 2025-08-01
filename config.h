#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// Grid dimensions and hierarchy levSels
#define BASE_ROWS 256
#define BASE_COLS 256
#define LEVELS 5 // Number of levels in the hierarchy

// Interaction constants
#define MAX_NEIGHBOURS 8
#define MAX_CHILDREN 4

// Identity vector size q
#define NUM_VALUES 6 // Number of values in the identity vector

// Identity vector indices
#define ATTACK_VS_TALK 0 // Index for attack vs talk in the identity vector
#define TARGET_SAME_LEVEL 1 // Index for targeting same level or different level in the identity vector
#define TARGET_SIMILARITY 2 // Index for targeting most similar or most different values in the identity vector
#define TARGET_PARENT_OR_CHILD 3 // Index for targeting parent or child in the identity vector
#define TARGET_TEAM 4 // Index for targeting team in the identity vector
#define INFLUENCE 5 // Index for the team in the identity vector


// Simulation constants
#define NUM_TEAMS 15 // Number of teams in the simulation


#define CELL_SIZE 2 // Size of each cell in pixels

#endif // CONFIG_H