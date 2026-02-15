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
                                independentSpaces){}

    GlobalSolutions SpacePartitionedGPUSA::optimize()
    {
        std::cout << "Calculating manhattan lengths for all nets on GPU..." << std::endl;
        compute::device device = compute::system::default_device();
        compute::context context(device);
        compute::command_queue queue(context, device);
        compute::vector<int> manhattanLengths(m_initialSolutions.size(), context);
        std::vector<int> hostManhattanLengths(m_initialSolutions.size());
        compute::vector<compute::int2_> deviceNetStarts(m_initialSolutions.size(), context);
        compute::vector<compute::int2_> deviceNetEnds(m_initialSolutions.size(), context);
        std::vector<int> netStarts(m_initialSolutions.size() * 2);
        std::vector<int> netEnds(m_initialSolutions.size() * 2);

        std::generate(netStarts.begin(), netStarts.end(), [this, n = 0]() mutable
            {
                if (n % 2)
                    return m_initialSolutions[n++ / 2].endpoints.first.x();
                else
                    return m_initialSolutions[n++ / 2].endpoints.first.y();
            });

        std::cout << "--- GPU Manhattan lengths calculation ---" << std::endl;

        std::generate(netEnds.begin(), netEnds.end(), [this, n = 0]() mutable
            {
                if (n % 2)
                    return m_initialSolutions[n++ / 2].endpoints.second.x();
                else
                    return m_initialSolutions[n++ / 2].endpoints.second.y();
            });


        compute::copy(reinterpret_cast<compute::int2_*>(netStarts.data()), reinterpret_cast<compute::int2_*>(netStarts.data()) + m_initialSolutions.size(), deviceNetStarts.begin(), queue);
        compute::copy(reinterpret_cast<compute::int2_*>(netEnds.data()), reinterpret_cast<compute::int2_*>(netEnds.data()) + m_initialSolutions.size(), deviceNetEnds.begin(), queue);

        BOOST_COMPUTE_FUNCTION(int, manhattanLength, (compute::int2_ start, compute::int2_ end),
            {
                return abs(start.x - end.x) + abs(start.y - end.y);
            });

        compute::transform(
            deviceNetStarts.begin(), deviceNetStarts.end(),
            deviceNetEnds.begin(),
            manhattanLengths.begin(),
            manhattanLength,
            queue
        );

        compute::sort_by_key(manhattanLengths.begin(), manhattanLengths.end(), deviceNetStarts.begin(), queue);

        compute::copy(manhattanLengths.begin(), manhattanLengths.end(), hostManhattanLengths.begin(), queue);

        queue.finish();
        const char source[] = BOOST_COMPUTE_STRINGIZE_SOURCE(
            float getCellPenalty(int* grid, int index)
            {
                return grid[index] * grid[index];
            }
            float getGridPenalty(int* grid, int size)
            {
                float penalty = 0.0f;
                for (int i = 0; i < size; ++i)
                {
                    penalty += getCellPenalty(grid, i);
                }
                return penalty;
            }
            int getHorizantalIndex(int x, int y, int cols)
            {
                return y * cols + x;
            }
            int getVerticalIndex(int x, int y, int rows)
            {
                return x * rows + y;
            }
            // Returns difference between penalty after adding segment to the grid and penalty before adding it
            float addSegment(int* grid, int start, int end)
            {
                float penalty = 0.0f;
                for (int i = start; i <= end; ++i)
                {
                    float oldPenalty = getCellPenalty(grid, i);
                    grid[i] += 1;
                    penalty += (getCellPenalty(grid, i) - oldPenalty);
                }
                return penalty;
            }

            // Returns difference between penalty after substracting segment from the grid and penalty before substracting it
            float substractSegment(int* grid, int start, int end)
            {
                float penalty = 0.0f;
                for (int i = start; i <= end; ++i)
                {
                    float oldPenalty = getCellPenalty(grid, i);
                    grid[i] -= 1;
                    penalty += (getCellPenalty(grid, i) - oldPenalty);
                }
                return penalty;
            }
            void getVerticalSegment(int2 netStart, int2 netEnd, int type, int2* start, int2* end)
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
            void getHorizontalSegment(int2 netStart, int2 netEnd, int type, int2* start, int2* end)
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
            float addSolution(int* horizontalGrid, int* verticalGrid, int cols, int rows, int2 netStart, int2 netEnd, int type)
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
            float substractSolution(int* horizontalGrid, int* verticalGrid, int cols, int rows, int2 netStart, int2 netEnd, int type)
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
            float ripUpAndReroute(int* horizontalGrid, int* verticalGrid, int cols, int rows, int2 netStart, int2 netEnd, int oldType, int newType)
            {
                float penaltyDiffAfterSubstraction = substractSolution(horizontalGrid, verticalGrid, cols, rows, netStart, netEnd, oldType);
                float penaltyDiffAfterAddition = addSolution(horizontalGrid, verticalGrid, cols, rows, netStart, netEnd, newType);
                return penaltyDiffAfterSubstraction + penaltyDiffAfterAddition;
            }


            uint splitmix32(uint x) {
                x += 0x9E3779B9u;
                x = (x ^ (x >> 16)) * 0x85EBCA6Bu;
                x = (x ^ (x >> 13)) * 0xC2B2AE35u;
                return x ^ (x >> 16);
            }


            int rand(int* seed) // 1 <= *seed < m
            {
                int const a = 16807; //ie 7**5
                int const m = 2147483647; //ie 2**31-1
                long x = (long)(*seed);
                x = ((long)a * x) % (long)m;

                *seed = x;
                return(*seed);
            }

            // form 0 to 1
            float randf(int* seed) // 1 <= *seed < m
            {
                int r = rand(seed);

                return ((float)r * (1.0f / 2147483647.0f) + 1.0f) / 2.0f;
            }

            int getUniform(int* seed, int min, int max)
            {
                return min + rand(seed) % (max - min + 1);
            }

            __kernel void uniformOnGpu(
                __global int* uniformFromGpu,
                int timeSeed,
                int size)
            {
                int id = get_global_id(0);
                if (id > size)
                {
                    return;
                }
                int seed = splitmix32((uint)timeSeed ^ (uint)id) | 1u;
                uniformFromGpu[id] = getUniform(&seed, 0, 255);
            }

            __kernel void uniformFOnGpu(
                __global float* uniformFromGpu,
                int timeSeed,
                int size)
            {
                int id = get_global_id(0);
                if (id > size)
                {
                    return;
                }
                int seed = splitmix32((uint)timeSeed ^ (uint)id) | 1u;
                uniformFromGpu[id] = randf(&seed);
            }
                        
            __kernel void simulatedAnnealing(
                __global const int2* netStarts,
                __global const int2* netEnds,
                __global int* doglegTypes,
                int netCount,
                __global const int* horizontalGrid,
                __global const int* verticalGrid,
                int cols,
                int rows,
                float initialTemperature,
                float coolingRate,
                float eps,
                int maxIterations,
                __global int* uniformFromGpu)
            {
                int id = get_global_id(0);
                uniformFromGpu[id] = getUniform(&id, 0, 255);
            }
        );
        const int uniformSize = 1024;
        compute::vector<float> uniformOnGpu(uniformSize, context);
        std::array<float, uniformSize> uniformOnHost{};
        compute::program program = compute::program::create_with_source(source, context);
        try
        {
            const auto timeSeed = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
            program.build();
            compute::kernel kernel(program, "uniformFOnGpu");
            kernel.set_arg(0, uniformOnGpu);
            kernel.set_arg(1, timeSeed);
            kernel.set_arg(2, uniformSize);
            queue.enqueue_1d_range_kernel(kernel, 0, uniformSize, 0);
            compute::copy(uniformOnGpu.begin(), uniformOnGpu.end(), uniformOnHost.begin(), queue);
            for (auto i : uniformOnHost)
            {
                std::cout << i << " ";;
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