#include "OptimizationKernels.hpp"

#include <boost/compute.hpp>

namespace krnl
{
    std::string getGlobalRoutingFunctions()
    {
        return BOOST_COMPUTE_STRINGIZE_SOURCE(
            float getCellPenalty(int* grid, uint index)
        {
            return grid[index] * grid[index];
        }
        float getGridPenalty(int* grid, uint size)
        {
            float penalty = 0.0f;
            for (uint i = 0; i < size; ++i)
            {
                penalty += getCellPenalty(grid, i);
            }
            return penalty;
        }
        uint getHorizantalIndex(int x, int y, uint cols)
        {
            return (uint)y * (uint)cols + x;
        }
        uint getVerticalIndex(int x, int y, uint rows)
        {
            return (uint)x * rows + (uint)y;
        }
        // Returns difference between penalty after adding segment to the grid and penalty before adding it
        float addSegment(int* grid, uint start, uint end)
        {
            float penalty = 0.0f;
            for (uint i = start; i <= end; ++i)
            {
                float oldPenalty = getCellPenalty(grid, i);
                grid[i] += 1;
                penalty += (getCellPenalty(grid, i) - oldPenalty);
            }
            return penalty;
        }

        // Returns difference between penalty after substracting segment from the grid and penalty before substracting it
        float substractSegment(int* grid, uint start, uint end)
        {
            float penalty = 0.0f;
            for (uint i = start; i <= end; ++i)
            {
                float oldPenalty = getCellPenalty(grid, i);
                grid[i] -= 1;
                penalty += (getCellPenalty(grid, i) - oldPenalty);
            }
            return penalty;
        }
        void getVerticalSegment(int2 netStart, int2 netEnd, char type, int2 * start, int2 * end)
        {
            int startHX = min(netStart.x, netEnd.x);
            int endHX = max(netStart.x, netEnd.x);
            int horizontalDistance = endHX - startHX;
            int startVY = min(netStart.y, netEnd.y);
            int endVY = max(netStart.y, netEnd.y);
            int coeff = (endHX - netEnd.x)
                / (endHX - startHX);// 0 if end is on the right, 1 if end is on the left
            int startVX = startHX + ((1 - coeff) * horizontalDistance);
            // Lower dogleg type result.verticalSegment = { {endV.x(),startV.y()}, endV };
            if (type == 0)
            {
                start->x = endHX;
                start->y = startVY;
                end->x = endHX;
                end->y = endVY;
            }
            else //upper dogleg type result.verticalSegment = { startV, {startV.x(), endV.y()} };
            {
                start->x = startVX;
                start->y = startVY;
                end->x = startVX;
                end->y = endVY;
            }
        }
        void getHorizontalSegment(int2 netStart, int2 netEnd, char type, int2 * start, int2 * end)
        {
            int startHX = min(netStart.x, netEnd.x);
            int endHX = max(netStart.x, netEnd.x);
            int startVY = min(netStart.y, netEnd.y);
            int endVY = max(netStart.y, netEnd.y);
            // Lower dogleg type result.horizontalSegment = { {startH.x(),startV.y()}, { endH.x(), startV.y() } };
            if (type == 0)
            {
                start->x = startHX;
                start->y = startVY;
                end->x = endHX;
                end->y = startVY;
            }
            else //upper dogleg type result.horizontalSegment = { {startH.x(),endV.y()}, {endH.x(), endV.y()} };
            {
                start->x = startHX;
                start->y = endVY;
                end->x = endHX;
                end->y = endVY;
            }
        }
        // Returns solution penalty after adding it to the grids
        float addSolution(int* horizontalGrid, int* verticalGrid, uint cols, uint rows, int2 netStart, int2 netEnd, char type)
        {
            int2 horizontalStart, horizontalEnd, verticalStart, verticalEnd;
            getHorizontalSegment(netStart, netEnd, type, &horizontalStart, &horizontalEnd);
            getVerticalSegment(netStart, netEnd, type, &verticalStart, &verticalEnd);
            float penalty = 0.0f;
            penalty += addSegment(horizontalGrid, getHorizantalIndex(horizontalStart.x, horizontalStart.y, cols), getHorizantalIndex(horizontalEnd.x, horizontalEnd.y, cols));
            penalty += addSegment(verticalGrid, getVerticalIndex(verticalStart.x, verticalStart.y, rows), getVerticalIndex(verticalEnd.x, verticalEnd.y, rows));
            return penalty;
        }
        // Returns difference between penalty after substracting solution from the grids and penalty before substracting it
        float substractSolution(int* horizontalGrid, int* verticalGrid, uint cols, uint rows, int2 netStart, int2 netEnd, char type)
        {
            int2 horizontalStart, horizontalEnd, verticalStart, verticalEnd;
            getHorizontalSegment(netStart, netEnd, type, &horizontalStart, &horizontalEnd);
            getVerticalSegment(netStart, netEnd, type, &verticalStart, &verticalEnd);
            float penalty = 0.0f;
            penalty += substractSegment(horizontalGrid, getHorizantalIndex(horizontalStart.x, horizontalStart.y, cols), getHorizantalIndex(horizontalEnd.x, horizontalEnd.y, cols));
            penalty += substractSegment(verticalGrid, getVerticalIndex(verticalStart.x, verticalStart.y, rows), getVerticalIndex(verticalEnd.x, verticalEnd.y, rows));
            return penalty;
        }

        // Retruns difference between old and new solution penalties
        // if 0 no difference
        // if negative new solution is better
        // if positive old solution is better
        float ripUpAndReroute(int* horizontalGrid, int* verticalGrid, uint cols, uint rows, int2 netStart, int2 netEnd, char oldType, char newType)
        {

            float penaltyDiffAfterSubstraction = substractSolution(horizontalGrid, verticalGrid, cols, rows, netStart, netEnd, oldType);
            float penaltyDiffAfterAddition = addSolution(horizontalGrid, verticalGrid, cols, rows, netStart, netEnd, newType);
            return penaltyDiffAfterSubstraction + penaltyDiffAfterAddition;
        });
        
    }

    std::string getSABulkRandomsWithTemparatureSteps()
    {
        return BOOST_COMPUTE_STRINGIZE_SOURCE(
        __kernel void sa_bulk_generated_randoms(
            __global const int2 * netStarts,
            __global const int2 * netEnds,
            __global char* doglegTypes,
            __global int* horizontalGrid,
            __global int* verticalGrid,
            __global float* penalties,
            __global const uint * randomIndex,
            __global const float* randomValue,
            __global const float* temperatures,
            const uint netCount,
            const uint cols,
            const uint rows,
            const uint spaces,
            const uint iterationSize,
            const uint temperatureSteps)
        {
            uint id = get_global_id(0);
            uint threadCount = get_global_size(0);

            const uint gridSize = cols * rows;
            const uint gridStartIndex = gridSize * id;
            __global int* localHorizontalGrid = horizontalGrid + gridStartIndex;
            __global int* localVerticalGrid = verticalGrid + gridStartIndex;

            const uint solStartIndex = id * netCount;
            __global char* localDoglegTypes = doglegTypes + solStartIndex;
            float currentPenalty = penalties[id];

            for (uint k = 0; k < temperatureSteps; ++k)
            {
                for (uint i = 0; i < iterationSize; ++i)
                {
                    const uint iterIndex = id * iterationSize * k + i;
                    uint flipIndex = randomIndex[iterIndex];

                    char oldType = localDoglegTypes[flipIndex];
                    char newType = 1 - oldType;
                    float penaltyDiff = ripUpAndReroute(localHorizontalGrid,
                        localVerticalGrid,
                        cols,
                        rows,
                        netStarts[flipIndex],
                        netEnds[flipIndex],
                        oldType,
                        newType);

                    if (penaltyDiff < 0 || exp(-penaltyDiff / temperatures[k]) > randomValue[iterIndex])
                    {
                        localDoglegTypes[flipIndex] = newType;
                        currentPenalty += penaltyDiff;
                    }
                    else
                    {
                        ripUpAndReroute(localHorizontalGrid,
                            localVerticalGrid,
                            cols,
                            rows,
                            netStarts[flipIndex],
                            netEnds[flipIndex],
                            newType,
                            oldType);
                    }
                }
            }

            penalties[id] = currentPenalty;
        });
    }

    std::string getSARandomsPerTemperatureStep()
    {
        return BOOST_COMPUTE_STRINGIZE_SOURCE(
            __kernel void sa_per_temperature_step_generated_randoms(
                __global const int2 * netStarts,
                __global const int2 * netEnds,
                __global char* doglegTypes,
                __global int* horizontalGrid,
                __global int* verticalGrid,
                __global float* penalties,
                __global const uint * randomIndex,
                __global const float* randomValue,
                const float temperature,
                const uint netCount,
                const uint cols,
                const uint rows,
                const uint spaces,
                const uint iterationSize)
        {
            uint id = get_global_id(0);
            uint threadCount = get_global_size(0);

            const uint gridSize = cols * rows;
            const uint gridStartIndex = gridSize * id;
            __global int* localHorizontalGrid = horizontalGrid + gridStartIndex;
            __global int* localVerticalGrid = verticalGrid + gridStartIndex;

            const uint solStartIndex = id * netCount;
            __global char* localDoglegTypes = doglegTypes + solStartIndex;
            float currentPenalty = penalties[id];

            for (uint i = 0; i < iterationSize; ++i)
            {
                const uint iterIndex = id * iterationSize + i;
                uint flipIndex = randomIndex[iterIndex];

                char oldType = localDoglegTypes[flipIndex];
                char newType = 1 - oldType;
                float penaltyDiff = ripUpAndReroute(localHorizontalGrid,
                    localVerticalGrid,
                    cols,
                    rows,
                    netStarts[flipIndex],
                    netEnds[flipIndex],
                    oldType,
                    newType);

                if (penaltyDiff < 0 || exp(-penaltyDiff / temperature) > randomValue[iterIndex])
                {
                    localDoglegTypes[flipIndex] = newType;
                    currentPenalty += penaltyDiff;
                }
                else
                {
                    ripUpAndReroute(localHorizontalGrid,
                        localVerticalGrid,
                        cols,
                        rows,
                        netStarts[flipIndex],
                        netEnds[flipIndex],
                        newType,
                        oldType);
                }
            }

            penalties[id] = currentPenalty;
        });
    }

    std::string getCreateRandomSolution()
    {
        return BOOST_COMPUTE_STRINGIZE_SOURCE(
            __kernel void create_random_solution(
                __global char* doglegTypes,
                __global float* randomValues,
                const uint netCount)
            {
                uint id = get_global_id(0);
                const uint solStartIndex = id * netCount;
                __global char* localDoglegTypes = doglegTypes + solStartIndex;
                __global float* localRandomValues = randomValues + solStartIndex;
                for (uint i = 0; i < netCount; ++i)
                {
                    localDoglegTypes[i] = (uint)step(0.5f, localRandomValues[i]);
                }
            });
    }

    std::string getPlaceAndCalculatePenalty()
    {
        return BOOST_COMPUTE_STRINGIZE_SOURCE(
            __kernel void place_and_calculate_penalty(
                __global const int2 * netStarts,
                __global const int2 * netEnds,
                __global char* doglegTypes,
                __global int* horizontalGrid,
                __global int* verticalGrid,
                __global float* penalties,
                const float temperature,
                const uint netCount,
                const uint cols,
                const uint rows)
        {
            uint id = get_global_id(0);
            uint threadCount = get_global_size(0);

            const uint gridSize = cols * rows;
            const uint gridStartIndex = gridSize * id;
            __global int* localHorizontalGrid = horizontalGrid + gridStartIndex;
            __global int* localVerticalGrid = verticalGrid + gridStartIndex;

            const uint solStartIndex = id * netCount;
            __global char* localDoglegTypes = doglegTypes + solStartIndex;
            
            float penalty = 0;

            for (unit i = 0; i < netCount; ++i)
            {
                penalty += addSolution(localHorizontalGrid, localVerticalGrid, cols, rows, netStarts[i], netEnds[i], localDoglegTypes[i]);
            }

            penalties[id] = penalty;
        });
    }

}