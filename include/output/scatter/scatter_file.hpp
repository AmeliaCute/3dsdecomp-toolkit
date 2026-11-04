#ifndef SCATTER_FILE_HPP_
#define SCATTER_FILE_HPP_

#include "output/scatter/load_region.hpp"

class ScatterFile : public Writable {
    public:
        ScatterFile() = default;
        ~ScatterFile() = default;
        void write(std::ofstream& output) const override;
        void addLoadRegion(const LoadRegion& loadRegion);
    protected:
    private:
        std::vector<LoadRegion> _loadRegions;
};

#endif /* !SCATTER_FILE_HPP_ */
