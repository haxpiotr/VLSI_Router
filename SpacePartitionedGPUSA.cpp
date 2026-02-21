#include "SpacePartitionedGPUSA.hpp"

#include <boost/compute.hpp>

namespace in
{
    namespace compute = boost::compute;
    SpacePartitionedGPUSA::SpacePartitionedGPUSA(const GlobalRoutingGrid& globalGrid,
                            const GlobalRoutingCells& startingGrid,
                            const GlobalSolutions& initialSolutions,
                            float initialTemperature,
                            float coolingRate,
                            float eps,
                            size_t maxIterations,
                            size_t independentSpaces) : SpacePartitionedSA(globalGrid,
                                startingGrid,
                                initialSolutions,
                                initialTemperature,
                                coolingRate,
                                eps,
                                maxIterations,
                                independentSpaces)
    {
    }

    GlobalSolutions SpacePartitionedGPUSA::optimize()
    {
        compute::device device = compute::system::default_device();
        compute::context context(device);
        compute::command_queue queue(context, device);

        const auto solutionsSize = m_initialSolutions.size();

        compute::vector<compute::int2_> deviceNetStarts(solutionsSize, context);
        compute::vector<compute::int2_> deviceNetEnds(solutionsSize, context);
        std::vector<compute::int2_> netStarts(solutionsSize);
        std::vector<compute::int2_> netEnds(solutionsSize);

        const auto& sortedSolution = m_spaces[0];
        for(int i = 0; i < solutionsSize; ++i)
        {
            netStarts[i] = { sortedSolution[i].endpoints.first.x(), sortedSolution[i].endpoints.first.y()};
            netEnds[i] = { sortedSolution[i].endpoints.second.x(), sortedSolution[i].endpoints.second.y() };
        }

        std::vector<GlobalRoutingCells> grids(m_independentSpacesSize, m_startingGrid);

        std::vector<float> penalties(m_independentSpacesSize);
        compute::vector<float> devicePenalties(m_independentSpacesSize, context);
        for(int i = 0; i < m_independentSpacesSize; ++i)
        {
            addSolutions(grids[i], m_spaces[i]);
            penalties[i] = grids[i].penalty;
        }
        compute::copy(penalties.begin(), penalties.end(), devicePenalties.begin(), queue);
        compute::copy(netStarts.begin(), netStarts.end(), deviceNetStarts.begin(), queue);
        compute::copy(netEnds.begin(), netEnds.end(), deviceNetEnds.begin(), queue);

        const auto horizontalGridSize = m_startingGrid.horizontalCells.size();
        const auto verticalGridSize = m_startingGrid.verticalCells.size();
        const auto horizontalGridsSize = horizontalGridSize * m_independentSpacesSize;
        const auto verticalGridsSize = verticalGridSize * m_independentSpacesSize;
        const auto doglegTypesSize = solutionsSize * m_independentSpacesSize;

        std::vector<int> horizontalGrids(horizontalGridsSize);
        for(int i = 0; i < horizontalGridsSize; ++i)
        {
            horizontalGrids[i] = grids[i / horizontalGridSize].horizontalCells[i % horizontalGridSize].congestion;
        }
        compute::vector<int> deviceHorizontalGrids(horizontalGridsSize, context);
        compute::copy(horizontalGrids.begin(), horizontalGrids.end(), deviceHorizontalGrids.begin(), queue);

        std::vector<int> verticalGrids(verticalGridsSize);
        for (int i = 0; i < verticalGridsSize; ++i)
        {
            verticalGrids[i] = grids[i / verticalGridSize].verticalCells[i % verticalGridSize].congestion;
        }
        compute::vector<int> deviceVerticalGrids(verticalGridsSize, context);
        compute::copy(verticalGrids.begin(), verticalGrids.end(), deviceVerticalGrids.begin(), queue);

        std::vector<char> doglegTypes(doglegTypesSize);
        for (int i = 0; i < doglegTypesSize; ++i)
        {
            doglegTypes[i] = static_cast<char>(m_spaces[i / solutionsSize][i % solutionsSize].type);
        }
        compute::vector<char> deviceDoglegTypes(doglegTypesSize, context);
        compute::copy(doglegTypes.begin(), doglegTypes.end(), deviceDoglegTypes.begin(), queue);

        const char source[] = BOOST_COMPUTE_STRINGIZE_SOURCE(
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
            void getVerticalSegment(int2 netStart, int2 netEnd, char type, int2* start, int2* end)
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
                    start->x = startVX;
                    start->y = startVY;
                    end->x = startVX;
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
            void getHorizontalSegment(int2 netStart, int2 netEnd, char type, int2* start, int2* end)
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
            }

            uint splitmix32(uint x) 
            {
                x += 0x9E3779B9u;
                x = (x ^ (x >> 16)) * 0x85EBCA6Bu;
                x = (x ^ (x >> 13)) * 0xC2B2AE35u;
                return x ^ (x >> 16);
            }

            int randi(int* seed) // 1 <= *seed < m
            {
                int const a = 16807; //ie 7**5
                int const m = 2147483647; //ie 2**31-1
                long x = (long)(*seed);
                x = ((long)a * x) % (long)m;

                *seed = x;
                return(*seed);
            }

            uint randu(uint* seed) // 1 <= *seed < m
            {
                const uint a = 16807; //ie 7**5
                const uint m = 4294967295; //ie 2**32-1
                long long x = (long long)(*seed);
                x = (a * x) % m;

                *seed = (uint)x;
                return(*seed);
            }

            // form 0 to 1
            float randf(int* seed) // 1 <= *seed < m
            {
                int r = randi(seed);
                return ((float)r * (1.0f / 2147483647.0f) + 1.0f) / 2.0f;
            }

            int getUniI(int* seed, int min, int max)
            {
                return min + randi(seed) % (max - min + 1);
            }

            uint getUniU(uint* seed, uint min, uint max)
            {
                return min + randu(seed) % (max - min + 1);
            }

            __kernel void simulatedAnnealing(
                __global const int2* netStarts,
                __global const int2* netEnds,
                __global char* doglegTypes,
                __global int* horizontalGrid,
                __global int* verticalGrid,
                __global float* penalties,
                const float initialTemperature,
                const float coolingRate,
                const float eps,
                const uint maxIterations,
                const uint timeSeed,
                const uint netCount,
                const uint cols,
                const uint rows,
                const uint spaces)
            {
                uint id = get_global_id(0);
                uint threadCount = get_global_size(0);

                uint seed = splitmix32(timeSeed ^ id) | 1u;
                int seedf = splitmix32(timeSeed ^ id) | 1u;
                
                const uint gridSize = cols * rows;
                const uint gridStartIndex = gridSize * id;
                __global int* localHorizontalGrid = horizontalGrid + gridStartIndex;
                __global int* localVerticalGrid = verticalGrid + gridStartIndex;
                                
                const uint solStartIndex = id * netCount;
                __global char* localDoglegTypes = doglegTypes + solStartIndex;

                float currentPenalty = penalties[id];
                float temperature = initialTemperature;

                const uint spaceIndex = sqrt((float)spaces);
                
                while (temperature > eps)
                {
                    for (uint i = 0; i < maxIterations/ threadCount; ++i)
                    {
                        uint flipIndex = getUniU(&seed, spaceIndex, netCount - 1);
                        
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

                        if (penaltyDiff < 0 || exp(-penaltyDiff / temperature) > randf(&seedf))
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
                    
                    temperature*= coolingRate;
                }
                penalties[id] = currentPenalty;
            }
        );

        compute::program program = compute::program::create_with_source(source, context);
        try
        {
            const auto timeSeed = static_cast<unsigned int>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
            
            program.build();

            compute::kernel kernel(program, "simulatedAnnealing");
            kernel.set_arg(0, deviceNetStarts);
            kernel.set_arg(1, deviceNetEnds);
            kernel.set_arg(2, deviceDoglegTypes);
            kernel.set_arg(3, deviceHorizontalGrids);
            kernel.set_arg(4, deviceVerticalGrids);
            kernel.set_arg(5, devicePenalties);
            kernel.set_arg(6, m_initialTemperature);
            kernel.set_arg(7, m_coolingRate);
            kernel.set_arg(8, m_eps);
            kernel.set_arg(9, static_cast<unsigned int>(m_maxIterations));
            kernel.set_arg(10,timeSeed);
            kernel.set_arg(11, static_cast<unsigned int>(solutionsSize));
            kernel.set_arg(12, static_cast<unsigned int>(m_globalGrid.getCols()));
            kernel.set_arg(13, static_cast<unsigned int>(m_globalGrid.getRows()));
            kernel.set_arg(14, static_cast<unsigned int>(m_spaces.size()));
            queue.enqueue_1d_range_kernel(kernel, 0, m_independentSpacesSize, 0);
            std::vector<float> hostPenalties(m_independentSpacesSize);
            compute::copy(devicePenalties.begin(), devicePenalties.end(), hostPenalties.begin(), queue);
            compute::copy(deviceHorizontalGrids.begin(), deviceHorizontalGrids.end(), horizontalGrids.begin(), queue);
            compute::copy(deviceVerticalGrids.begin(), deviceVerticalGrids.end(), verticalGrids.begin(), queue);

            for(int i = 0; i < hostPenalties.size(); ++i)
            {
                std::cout << "Space " << i << " penalty: " << hostPenalties[i] << std::endl;
                float recalculatedPenalty = 0.0f;
                for (int j = 0; j < horizontalGridSize; ++j)
                {
                  recalculatedPenalty += horizontalGrids[i * horizontalGridSize + j]
                                         * horizontalGrids[i * horizontalGridSize + j];
                }
                for (int j = 0; j < verticalGridSize; ++j)
                {
                    recalculatedPenalty += verticalGrids[i * verticalGridSize + j]
                        * verticalGrids[i * verticalGridSize + j];
                }
                std::cout << "Space " << i << " recalculated penalty: " << recalculatedPenalty << std::endl;
            }

        }
        catch (const compute::opencl_error& e)
        {
            std::cerr << "OpenCL error: " << e.what() << std::endl;
            std::cout << program.build_log() << std::endl;
        }

        return {};
    }
}