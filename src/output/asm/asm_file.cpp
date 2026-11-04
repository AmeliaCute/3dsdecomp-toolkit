#include "output/asm/asm_file.hpp"

ASMFile::ASMFile(const std::string &section) : _section(section) {}

void ASMFile::write(std::ofstream& output) const {
    output << "\tAREA " << _section << ", ";
    if (_section == "text")
        output << "CODE, READONLY\n";
    else if (_section == "rodata")
        output << "DATA, READONLY\n";
    else 
        output << "DATA, READWRITE\n";
    output << "\n";
    for (const auto& block : _blocks) {
        block.write(output);
        output << "\n";
    }
    output << "\tEND";
}

void ASMFile::addBlock(const ASMBlock &block) {
    _blocks.push_back(block);
}

void ASMFile::reset() {
    _blocks.clear();
}