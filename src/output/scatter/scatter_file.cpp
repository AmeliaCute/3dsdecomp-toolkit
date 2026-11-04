#include "output/scatter/scatter_file.hpp"

void ScatterFile::write(std::ofstream& output) const {
    for (const auto& loadRegion : _loadRegions)
        loadRegion.write(output);
}

void ScatterFile::addLoadRegion(const LoadRegion& loadRegion) {
    _loadRegions.push_back(loadRegion);
}