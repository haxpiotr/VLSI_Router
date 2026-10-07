#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <iomanip>

namespace in
{
    struct RGB
    {
        uint8_t r{ 0 };
        uint8_t g{ 0 };
        uint8_t b{ 0 };
    };

    struct CongestionStats
    {
        uint64_t sumSqHorizontal{ 0 };
        uint64_t sumSqVertical{ 0 };
        uint64_t sumSqTotal{ 0 };          // H^2 + V^2
        uint64_t sumSqCombinedCell{ 0 };   // (H + V)^2
    };

    
    inline CongestionStats calculateCongestionStats(const GlobalRoutingCells& cells, int cols, int rows)
    {
        CongestionStats stats;
        const int totalCells = cols * rows;

        for (int i = 0; i < totalCells; ++i)
        {
            uint64_t h = static_cast<uint64_t>(cells.horizontalCells[i].congestion);
            uint64_t v = static_cast<uint64_t>(cells.verticalCells[i].congestion);

            stats.sumSqHorizontal += h * h;
            stats.sumSqVertical += v * v;
            stats.sumSqCombinedCell += (h + v) * (h + v);
        }
        stats.sumSqTotal = stats.sumSqHorizontal + stats.sumSqVertical;

        return stats;
    }

    RGB congestionToRGB(float value)
    {

        if (value <= 0.0f)
        {
            return { 80, 80, 80 };
        }

        value = std::clamp(value, 0.0f, 1.0f);

        float v = std::sqrt(value);

        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;

        if (v < 0.5f)
        {
            float t = v / 0.5f;
            r = t;
            g = 1.0f;
            b = 0.0f;
        }
        else
        {
            float t = (v - 0.5f) / 0.5f;
            r = 1.0f;
            g = 1.0f - t;
            b = 0.0f;
        }

        return
        {
            static_cast<uint8_t>(r * 255.0f),
            static_cast<uint8_t>(g * 255.0f),
            static_cast<uint8_t>(b * 255.0f)
        };
    }
    void exportDiffToPPM(const std::string& filename,
        const GlobalRoutingCells& before,
        const GlobalRoutingCells& after,
        int cols, int rows, int cellSize = 10)
    {
        if (cols <= 0 || rows <= 0 || cellSize <= 0) return;

        const int imgWidth = cols * cellSize;
        const int imgHeight = rows * cellSize;

        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "Cannot open file:" << filename << "\n";
            return;
        }

        file << "P6\n" << imgWidth << " " << imgHeight << "\n255\n";

        for (int row = rows - 1; row >= 0; --row)
        {
            for (int py = 0; py < cellSize; ++py)
            {
                for (int col = 0; col < cols; ++col)
                {
                    int idx = row * cols + col;
                    int valBefore = before.horizontalCells[idx].congestion + before.verticalCells[idx].congestion;
                    int valAfter = after.horizontalCells[idx].congestion + after.verticalCells[idx].congestion;

                    int diff = valAfter - valBefore;
                    RGB color;

                    if (diff == 0)
                    {
                        color = { 25, 25, 30 };
                    }
                    else if (diff < 0)
                    {
                        color = { 0, 230, 100 };
                    }
                    else
                    {
                        if (valAfter <= 2)
                        {
                            color = { 60, 130, 240 };
                        }
                        else
                        {
                            color = { 255, 40, 0 };
                        }
                    }

                    for (int px = 0; px < cellSize; ++px)
                    {
                        file.put(static_cast<char>(color.r));
                        file.put(static_cast<char>(color.g));
                        file.put(static_cast<char>(color.b));
                    }
                }
            }
        }

        file.close();
    }

    void exportGridToPPM(const std::string& filename, const GlobalRoutingCells& cells, int maxCongestion, int cols = 100, int rows = 100, int cellSize = 10)
    {
        if (cols <= 0 || rows <= 0 || cellSize <= 0 || maxCongestion <= 0)
        {
            std::cerr << "Wrong parameters for grid export\n";
            return;
        }

        const int imgWidth = cols * cellSize;
        const int imgHeight = rows * cellSize;

        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "Cannot open file:" << filename << "\n";
            return;
        }

        file << "P6\n" << imgWidth << " " << imgHeight << "\n255\n";

        for (int row = rows - 1; row >= 0; --row)
        {
            for (int py = 0; py < cellSize; ++py)
            {
                for (int col = 0; col < cols; ++col)
                {
                    int cellIndex = row * cols + col;

                    int combinedCongestion = cells.horizontalCells[cellIndex].congestion + cells.verticalCells[cellIndex].congestion;

                    float normalizedVal = static_cast<float>(combinedCongestion) / maxCongestion;
                    RGB color = congestionToRGB(normalizedVal);

                    for (int px = 0; px < cellSize; ++px)
                    {
                        file.put(static_cast<char>(color.r));
                        file.put(static_cast<char>(color.g));
                        file.put(static_cast<char>(color.b));
                    }
                }
            }
        }

        file.close();
    }
}