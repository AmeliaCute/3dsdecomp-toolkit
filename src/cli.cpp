
#include "output/ninja/build_rule.hpp"
#include "output/ninja/build_script.hpp"
#include "output/scatter/input_section.hpp"
#include "parser/cxi.hpp"
#include "parser/function_heuristic.hpp"
#include <filesystem>
#include "output/asm/asm_file.hpp"
#include "output/scatter/scatter_file.hpp"

void writeBinaryFile(const std::string& filename, uint8_t *data, size_t size,  cs_mode mode = CS_MODE_ARM, bool isData = false, const std::string &section = "text")
{
    std::ofstream output(filename + ".s");
    ASMFile asmFile(section);
    asmFile.addBlock(ASMBlock(data, size, isData ? "" : filename, mode == CS_MODE_THUMB));
    asmFile.write(output);
}

void splitFunctions(std::vector<Function> &functions, ExecutionRegion &textER, BuildRule &buildRule, BuildScript &buildScript, std::vector<uint8_t> &text, uint32_t baseAddress) {
    ASMFile asmfile("text");
    std::ofstream output;
    std::string firstName;
    uintptr_t lastAddress = 0;
    size_t functionCounter = 0;
    for (size_t i = 0; i < functions.size(); ++i) {
        const auto& function = functions[i];
        std::string name = std::format("{:x}", function.start);
        if (function.start > lastAddress && lastAddress != 0)
            asmfile.addBlock(ASMBlock(text.data() + (lastAddress - baseAddress), function.start - lastAddress, "", function.mode == CS_MODE_THUMB));
        if (!output.is_open() || (functionCounter >= 30 && function.start % 4 == 0)) {
            if (output.is_open()) {
                asmfile.write(output);
                textER.addInputSection(InputSection(firstName + ".o", {"text"}));
                buildRule.addInput("build/" + firstName + ".o");
                buildScript.addBuildRule(BuildRule("output/" + firstName + ".s", "build/" + firstName + ".o", "compile"));
                asmfile.reset();
                output.close();
            }
            firstName = name;
            output = std::ofstream("output/" + firstName + ".s");
            functionCounter = 0;
        }
        asmfile.addBlock(ASMBlock(text.data() + (function.start - baseAddress), function.end - function.start, name, function.mode == CS_MODE_THUMB, function.start == baseAddress));
        lastAddress = function.end;
        ++functionCounter;
    }
    uintptr_t endAddress = baseAddress + text.size();
    if (lastAddress != 0 && lastAddress < endAddress) {
        std::string name = std::format("{:x}", lastAddress);
        asmfile.addBlock(ASMBlock(text.data() + (lastAddress - baseAddress), endAddress - lastAddress, name, false));
    }
    if (output.is_open()) {
        asmfile.write(output);
        textER.addInputSection(InputSection(firstName + ".o", {"text"}));
        buildRule.addInput("build/" + firstName + ".o");
        buildScript.addBuildRule(BuildRule("output/" + firstName + ".s", "build/" + firstName + ".o", "compile"));
        asmfile.reset();
        output.close();
    }
}

int main(int argc, char* argv[])
{
    CXI cxi("../../test/ac.3ds");
    auto functions = function_heuristic::findFunctions(cxi.getTextSection(), cxi.getHeader().extendedHeader.systemControlInfo.text.address);
    std::filesystem::create_directory("output");
    uintptr_t lastAddress = 0;
    std::vector<uint8_t> &text = cxi.getTextSection();
    std::vector<uint8_t> &rodata = cxi.getRodataSection();
    std::vector<uint8_t> &data = cxi.getDataSection();
    writeBinaryFile("output/rodata", rodata.data(), rodata.size(), CS_MODE_ARM, true, "rodata");
    writeBinaryFile("output/data", data.data(), data.size(), CS_MODE_ARM, true, "data");
    std::ofstream ldfile("scatter.sct");
    std::ofstream buildfile("build.ninja");
    ScatterFile scatterFile;
    LoadRegion textLR("text", true, cxi.getHeader().extendedHeader.systemControlInfo.text.address);
    LoadRegion rodataLR("rodata", true, cxi.getHeader().extendedHeader.systemControlInfo.rodata.address);
    LoadRegion dataLR("data", true, cxi.getHeader().extendedHeader.systemControlInfo.data.address);
    ExecutionRegion textER("text", true, cxi.getHeader().extendedHeader.systemControlInfo.text.address);
    ExecutionRegion rodataER("rodata", true, cxi.getHeader().extendedHeader.systemControlInfo.rodata.address);
    ExecutionRegion dataER("data", true, cxi.getHeader().extendedHeader.systemControlInfo.data.address);
    ExecutionRegion bssER("bss", true, cxi.getHeader().extendedHeader.systemControlInfo.data.address + cxi.getHeader().extendedHeader.systemControlInfo.data.size, cxi.getHeader().extendedHeader.systemControlInfo.bssSize);
    rodataER.addInputSection(InputSection("rodata.o", {"rodata"}));
    dataER.addInputSection(InputSection("data.o", {"data"}));
    bssER.addAttribute(ExecutionRegion::Attribute::EMPTY);
    BuildScript buildScript;
    buildScript.addRule(Rule("compile", "armasm --cpu=MPCore -o $out $in", "Compiling $out"));
    Rule linkRule("link", "armlink --sort=lexical --no_remove --scatter=scatter.sct --map --list=$out.map --output=$out --via=$rspfile", "Linking $out");
    linkRule.setRspFile("$out.rsp");
    linkRule.setRspFileContent("$in");
    buildScript.addRule(linkRule);
    buildScript.addBuildRule(BuildRule("output/data.s", "build/data.o", "compile"));
    buildScript.addBuildRule(BuildRule("output/rodata.s", "build/rodata.o", "compile"));
    BuildRule buildRule("program.elf", "link");
    buildRule.addInput("build/data.o");
    buildRule.addInput("build/rodata.o");
    uint32_t baseAddress = cxi.getHeader().extendedHeader.systemControlInfo.text.address;
    splitFunctions(functions, textER, buildRule, buildScript, text, baseAddress);
    buildScript.addBuildRule(buildRule);
    buildScript.write(buildfile);
    textLR.addExecutionRegion(textER);
    rodataLR.addExecutionRegion(rodataER);
    dataLR.addExecutionRegion(dataER);
    dataLR.addExecutionRegion(bssER);
    scatterFile.addLoadRegion(textLR);
    scatterFile.addLoadRegion(rodataLR);
    scatterFile.addLoadRegion(dataLR);
    scatterFile.write(ldfile);
    return 0;
}

