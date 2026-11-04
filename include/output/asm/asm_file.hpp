#ifndef ASM_FILE_HPP_
#define ASM_FILE_HPP_

#include "asm_block.hpp"
#include <vector>

class ASMFile : public Writable {
    public:
        ASMFile(const std::string &section);
        ~ASMFile() = default;
        void write(std::ofstream& output) const override;
        void addBlock(const ASMBlock &block);
        void reset();
    protected:
    private:
        std::vector<ASMBlock> _blocks;
        const std::string _section;
};

#endif /* !ASM_FILE_HPP_ */

