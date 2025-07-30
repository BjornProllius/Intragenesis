#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

// Declare the pcg32 function
uint32_t pcg32(uint64_t* state);

// Declare the shared state variable
extern uint64_t pcgState;

#endif // BOARD_H