#pragma once

#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

namespace in {

class HeatmapExporter {
public:
    static void writeCongestionPPM(const std::string& filename, 
                                   const std::vector<GlobalRoutingCell>& grid, 
                                   int cols, 
                                   int rows) 
    {
        std::ofstream ofs(filename, std::ios::binary);
        if (!ofs || grid.empty()) {
            return;
        }

        ofs << "P6\n" << cols << " " << rows << "\n255\n";

        for (int y = rows - 1; y >= 0; --y) 
        {
            for (int x = 0; x < cols; ++x) 
            {
                int idx = y * cols + x;
                int cong = grid[idx].overallCongestion;
                unsigned char r = static_cast<unsigned char>(std::min(255, cong));
                unsigned char g = 0;
                unsigned char b = 0;
                ofs.put(static_cast<char>(r));
                ofs.put(static_cast<char>(g));
                ofs.put(static_cast<char>(b));
            }
        }
        ofs.close();
    }
};

}