#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "Grid.h"
#include "config.h"
#include <math.h>
#include "Board.h"
#include <time.h>
#include "Community.h"
#include <pthread.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SDL_Color teamColors[16] = {
    {255, 255, 255, 255}, // Team 0: White
    {0, 0, 0, 255},       // Team 1: Black
    {0, 0, 255, 255},     // Team 2: Blue
    {255, 0, 0, 255},     // Team 3: Red
    {0, 255, 0, 255},     // Team 4: Green
    {255, 255, 0, 255},   // Team 5: Yellow
    {255, 0, 255, 255},   // Team 6: Magenta
    {0, 255, 255, 255},   // Team 7: Cyan
    {128, 128, 128, 255}, // Team 8: Gray
    {128, 0, 0, 255},     // Team 9: Dark Red
    {0, 128, 0, 255},     // Team 10: Dark Green
    {0, 0, 128, 255},     // Team 11: Dark Blue
    {128, 128, 0, 255},   // Team 12: Olive
    {128, 0, 128, 255},   // Team 13: Purple
    {0, 128, 128, 255},   // Team 14: Teal
    {64, 64, 64, 255}     // Team 15: Dark Gray
};

//there might be an error in how cells pick idex their parent/children. Maybe the issue is with the render


__thread uint64_t threadLocalPcgState = 123456789; // Thread-local state for each thread


uint32_t pcg32() {
    uint64_t oldState = threadLocalPcgState;
    threadLocalPcgState = oldState * 6364136223846793005ULL + 1;
    uint32_t xorshifted = ((oldState >> 18u) ^ oldState) >> 27u;
    uint32_t rot = oldState >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}



typedef struct {
    Cell* grid;
    int totalLevels;
    int rowsPerLevel[LEVELS];
    int colsPerLevel[LEVELS];
    int threadId;
    int numThreads;
    void (*callback)(Cell*, int);
} ThreadArgs;

void* threadFunction(void* args) {
    ThreadArgs* threadArgs = (ThreadArgs*)args;

    // Initialize thread-local random state with a unique seed
    threadLocalPcgState = (uint64_t)pthread_self(); // Use thread ID as seed

    Cell* grid = threadArgs->grid;
    int threadId = threadArgs->threadId;
    int numThreads = threadArgs->numThreads;

    int startIndex = 0;

    for (int level = 0; level < threadArgs->totalLevels; level++) {
        int rows = threadArgs->rowsPerLevel[level];
        int cols = threadArgs->colsPerLevel[level];

        // Divide rows among threads
        int rowsPerThread = rows / numThreads;
        int startRow = threadId * rowsPerThread;
        int endRow = (threadId == numThreads - 1) ? rows : (threadId + 1) * rowsPerThread;

        // Process the assigned rows for this level
        for (int row = startRow; row < endRow; row++) {
            for (int col = 0; col < cols; col++) {
                int index = startIndex + row * cols + col;
                threadArgs->callback(grid, index);
            }
        }

        // Move to the next level
        startIndex += rows * cols;
    }

    return NULL;
}




int renderByTeam = 0; // 0: Render by internal values, 1: Render by team



unsigned int targetVectors[16][NUM_VALUES];

void generateRandomTargetForThread(unsigned int threadTargetVectors[16][NUM_VALUES]) {
    for (int team = 0; team < 16; team++) {
        for (int i = 0; i < NUM_VALUES; i++) {
            threadTargetVectors[team][i] = pcg32() % 16; // Use thread-safe pcg32
        }
    }
}




void iterateCellsMultithreaded(Cell* grid, int totalCells, void (*callback)(Cell*, int)) {
    const int numThreads = 4; // Number of threads
    pthread_t threads[numThreads];
    ThreadArgs threadArgs[numThreads];

    // Calculate rows and columns for each level
    int rows = BASE_ROWS;
    int cols = BASE_COLS;

    for (int i = 0; i < numThreads; i++) {
        threadArgs[i].grid = grid;
        threadArgs[i].totalLevels = LEVELS;
        threadArgs[i].numThreads = numThreads;
        threadArgs[i].callback = callback;
        threadArgs[i].threadId = i;

        for (int level = 0; level < LEVELS; level++) {
            threadArgs[i].rowsPerLevel[level] = rows;
            threadArgs[i].colsPerLevel[level] = cols;
            rows /= 2; // Each level has half the rows
            cols /= 2; // Each level has half the columns
        }
    }

    // Create threads
    for (int i = 0; i < numThreads; i++) {
        pthread_create(&threads[i], NULL, threadFunction, &threadArgs[i]);
    }

    // Wait for threads to finish
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }
}

void pickActionCallback(Cell* grid, int index) {
    // Select a random target vector from the pre-generated array
    int randomTeam = pcg32() % 16; // Random team index (0-15)
    unsigned int* randomTargetVector = targetVectors[randomTeam];

    // Pass the random target vector to the pickAction function
    pickAction(grid, index, randomTargetVector);
}

// Callback for collecting community data
void collectCommunityDataCallback(Cell* grid, int index) {
    collectCommunityData(grid, index);
}

// Callback for updating cell identities
void updateCellIdentityCallback(Cell* grid, int index) {
    updateCellIdentity(grid, index);
}



void drawGridByTeam(SDL_Renderer* renderer, Cell* grid, int rows, int cols, int level) {
    int startIndex = 0;
    for (int i = 0; i < level; i++) {
        startIndex += rows * cols;
        rows /= 2;
        cols /= 2;
    }

    int scaleFactor = 1 << level;
    int scaledCellSize = CELL_SIZE * scaleFactor;

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int index = startIndex + row * cols + col;
            Cell* cell = &grid[index];

           
            
            uint8_t team = (cell->team >> 4) & 0xF; // Extract the first 4 bits
            SDL_Color color = teamColors[team];
                   if (index == 500) {

       // printf("Cell at index 500= %d\n", team);
    }

            // Dim the color by scaling down the RGB values
            SDL_SetRenderDrawColor(renderer, 
                (uint8_t)(color.r * 0.5), // Reduce red by 50%
                (uint8_t)(color.g * 0.5), // Reduce green by 50%
                (uint8_t)(color.b * 0.5), // Reduce blue by 50%
                color.a);

            SDL_Rect rect = { col * scaledCellSize, row * scaledCellSize, scaledCellSize, scaledCellSize };
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}

void drawGridByInternalValues(SDL_Renderer* renderer, Cell* grid, int rows, int cols, int level) {
    int startIndex = 0;
    for (int i = 0; i < level; i++) {
        startIndex += rows * cols;
        rows /= 2;
        cols /= 2;
    }

    int scaleFactor = 1 << level;
    int scaledCellSize = CELL_SIZE * scaleFactor;

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int index = startIndex + row * cols + col;
            Cell* cell = &grid[index];

            // Use the SimHash value to determine the color
            uint32_t simHash = cell->identityHash; // Assuming identityHash is a 32-bit value
            uint8_t red = (simHash >> 16) & 0xFF;  // Extract bits 16-23 for red
            uint8_t green = (simHash >> 8) & 0xFF; // Extract bits 8-15 for green
            uint8_t blue = simHash & 0xFF;         // Extract bits 0-7 for blue

            // Set the color based on the SimHash value
            SDL_SetRenderDrawColor(renderer, red, green, blue, 255);

            SDL_Rect rect = { col * scaledCellSize, row * scaledCellSize, scaledCellSize, scaledCellSize };
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}

int main() {
    srand((unsigned int)time(NULL)); // Seed the random number generator with the current time


    // // Initialize SDL
    // if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    //     if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    //         fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    //         return 1;
    //     }
    // }

    // // Create SDL window
    // SDL_Window* window = SDL_CreateWindow("Grid Visualization", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, BASE_COLS * CELL_SIZE, BASE_ROWS * CELL_SIZE, SDL_WINDOW_SHOWN);
    // if (!window) {
    //     fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
    //     SDL_Quit();
    //     return EXIT_FAILURE;
    // }

    // // Create an SDL renderer
    // SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    // if (!renderer) {
    //     fprintf(stderr, "Failed to create SDL renderer: %s\n", SDL_GetError());
    //     SDL_DestroyWindow(window);
    //     SDL_Quit();
    //     return EXIT_FAILURE;
    // }

    // Create the grid
    Cell* grid = createGrid();
    if (!grid) {
        fprintf(stderr, "Failed to create grid\n");
        // SDL_DestroyRenderer(renderer);
        // SDL_DestroyWindow(window);
        // SDL_Quit();
        return EXIT_FAILURE;
    }

    // Calculate total number of cells across all levels
    int totalCells = 0;
    int rows = BASE_ROWS;
    int cols = BASE_COLS;

    for (int level = 0; level < LEVELS; level++) {
        totalCells += rows * cols;
        rows /= 2; // Each level has half the rows
        cols /= 2; // Each level has half the columns
    }

    rows = BASE_ROWS; // Reset rows for iteration
    cols = BASE_COLS; // Reset cols for iteration

    // Main loop
    int running = 1;
    int iterationCount = 0; // Counter for iterations
    int renderByTeam = 1; // 1: Render by team, 0: Render by internal values
    int currentLevel = 0; // Start rendering at level 0

    // Start clock for performance measurement
    clock_t startTime = clock();

    while (running) {
        // Generate random target vectors for this iteration
        generateRandomTargetForThread(targetVectors);      
        
        // Handle SDL events
        // SDL_Event event;
        // while (SDL_PollEvent(&event)) {
        //     if (event.type == SDL_QUIT) {
        //         running = 0; // Quit if the window is closed
        //     } else if (event.type == SDL_KEYDOWN) {
        //         if (event.key.keysym.sym == SDLK_q) {
        //             running = 0; // Quit if the 'q' key is pressed
        //         } else if (event.key.keysym.sym == SDLK_t) {
        //             renderByTeam = 1; // Switch to rendering by team
        //         } else if (event.key.keysym.sym == SDLK_i) {
        //             renderByTeam = 0; // Switch to rendering by internal values
        //         } else if (event.key.keysym.sym == SDLK_UP) {
        //             // Increase the level, but ensure it doesn't exceed the maximum level
        //             if (currentLevel < LEVELS - 1) {
        //                 currentLevel++;
        //             }
        //         } else if (event.key.keysym.sym == SDLK_DOWN) {
        //             // Decrease the level, but ensure it doesn't go below 0
        //             if (currentLevel > 0) {
        //                 currentLevel--;
        //             }
        //         }
        //     }
        // }



        // // Pick actions for all cells
        //  iterateCells(grid, totalCells, pickActionCallback);

        // // Collect community data for all cells
        //  iterateCells(grid, totalCells, collectCommunityDataCallback);

        // // Update cell identities for all cells
        //  iterateCells(grid, totalCells, updateCellIdentityCallback);


                // Pick actions for all cells
        iterateCellsMultithreaded(grid, totalCells, pickActionCallback);
        
        // Collect community data for all cells
        iterateCellsMultithreaded(grid, totalCells, collectCommunityDataCallback);
        
        // Update cell identities for all cells
        iterateCellsMultithreaded(grid, totalCells, updateCellIdentityCallback);

        // // Render the grid
        // SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Set background color to black
        // SDL_RenderClear(renderer); // Clear the screen

        // if (renderByTeam) {
        //     //drawGridByTeam(renderer, grid, rows, cols, currentLevel); // Render by team at the current level
        // } else {
        //     //drawGridByInternalValues(renderer, grid, rows, cols, currentLevel); // Render by internal values at the current level
        // }

        // SDL_RenderPresent(renderer); // Present the rendered frame

        iterationCount++; // Increment the iteration count
    }

    // End clock for performance measurement
    clock_t endTime = clock();

    // Print average time per iteration
    double elapsedTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    double averageTimePerIteration = elapsedTime / iterationCount;
    printf("Average time per iteration: %.6f seconds\n", averageTimePerIteration);

    // Free the allocated memory for the grid
    free(grid);
    grid = NULL; // Set the pointer to NULL to avoid accidental access

    // Clean up SDL
    // SDL_DestroyRenderer(renderer);
    // SDL_DestroyWindow(window);
    // SDL_Quit();

    return 0;
}