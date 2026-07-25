#include "vm.h"

#include <set>
#include <random>

std::set<std::string> capabilitySet = {
    "FS", "random"
};

int fileHandleId = 0;
std::unordered_map<int, std::fstream*> fileHandles;

static std::mt19937 rngEngine(std::random_device{}());

std::unordered_map<int, NativeFn> funcMap = {
    {0x01, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        auto arg0 = stack.back(); stack.pop_back();
        std::visit([](const auto& val) { std::cout << val; }, arg0.data);
        std::cout << std::endl;
    }},
    {0x02, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        auto arg0 = stack.back(); stack.pop_back();
        std::visit([](const auto& val) { std::cout << val; }, arg0.data);
    }},
    {0x03, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        auto varIndex = getInt(stack.back()); stack.pop_back();
        std::string input;
        std::cin >> input;
        int64_t result = 0;
        try {
            result = std::stoll(input);
        } catch(...) {
            std::cout << "Invalid value!" << std::endl;
        }
        variables[varIndex].type = TAG_INT;
        variables[varIndex].data = result;
    }},
    {0x04, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        auto varIndex = getInt(stack.back()); stack.pop_back();
        std::string input;
        std::cin >> input;
        variables[varIndex].type = TAG_STRING;
        variables[varIndex].data = input;
    }},
    {0x05, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        // str2int
        auto varIndex = getInt(stack.back()); stack.pop_back();
        auto value = stack.back(); stack.pop_back();

        int num = 0;
        std::string str = "0";
        if(value.type == TAG_STRING) str = std::get<std::string>(value.data);

        try {
            num = std::stoi(str);
        } catch (const std::invalid_argument& e) {
            num = 0;
        } catch (const std::out_of_range& e) {
            num = 0;
        }

        variables[varIndex].type = TAG_INT;
        variables[varIndex].data = num;
    }},
    {0x06, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        // int2str
        auto varIndex = getInt(stack.back()); stack.pop_back();
        auto value = stack.back(); stack.pop_back();

        int num = 0;
        if(value.type == TAG_INT) num = getInt(value);

        std::string str = std::to_string(num);
        variables[varIndex].type = TAG_STRING;
        variables[varIndex].data = str;
    }},
    {0x07, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        // str2float
        auto varIndex = getInt(stack.back()); stack.pop_back();
        auto value = stack.back(); stack.pop_back();

        double num = 0.0;
        std::string str = "0";
        if(value.type == TAG_STRING) str = std::get<std::string>(value.data);

        try {
            num = std::stod(str);
        } catch (const std::invalid_argument& e) {
            num = 0.0;
        } catch (const std::out_of_range& e) {
            num = 0.0;
        }

        variables[varIndex].type = TAG_FLOAT;
        variables[varIndex].data = num;
    }},
    {0x08, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        // float2str
        auto varIndex = getInt(stack.back()); stack.pop_back();
        auto value = stack.back(); stack.pop_back();

        double num = 0.0;
        if(value.type == TAG_FLOAT) num = std::get<double>(value.data);
        else if(value.type == TAG_INT) num = static_cast<double>(getInt(value)); // accept int too, same leniency as int2str only handling its own type

        std::string str = std::to_string(num);
        variables[varIndex].type = TAG_STRING;
        variables[varIndex].data = str;
    }},
    // stdlib impl
    {0xA0, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        // assertCapability
        auto value = stack.back(); stack.pop_back();
        if(value.type != TAG_STRING) {
            throw std::runtime_error("assertCapability failed: invalid value type");
        }
        auto str = std::get<std::string>(value.data);
        auto it = capabilitySet.find(str);
        if(it == capabilitySet.end()) {
            std::stringstream ss;
            ss << "assertCapability failed: capability " << str << " is not present";
            throw std::runtime_error(ss.str());
        }

        // capability present, proceed with execution
    }},
    {0xA1, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        // openFile
        auto handleVarIndex = getInt(stack.back()); stack.pop_back();

        auto value = stack.back(); stack.pop_back();
        if(value.type != TAG_STRING) {
            throw std::runtime_error("openFile failed: invalid value type");
        }

        auto filename = std::get<std::string>(value.data);

        auto stream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::trunc);
        if(!stream->is_open()) {
            throw std::runtime_error("openFile failed: unable to open file " + filename);
        }

        fileHandles[fileHandleId] = stream;

        variables[handleVarIndex].type = TAG_INT;
        variables[handleVarIndex].data = fileHandleId++;
    }},
    {0xA2, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        // writeFile
        auto handle = getInt(stack.back()); stack.pop_back();

        auto value = stack.back(); stack.pop_back();
        if(value.type != TAG_STRING) {
            throw std::runtime_error("writeFile failed: invalid value type");
        }

        auto valueToWrite = std::get<std::string>(value.data);

        auto it = fileHandles.find(handle);
        if(it != fileHandles.end()) {
            auto f = it->second;
            *f << valueToWrite;
        } else {
            throw std::runtime_error("writeFile failed: invalid file handle");
        }
    }},
    {0xA3, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        // readFile
        auto handle = getInt(stack.back()); stack.pop_back();
        auto varIndex = getInt(stack.back()); stack.pop_back();

        auto it = fileHandles.find(handle);
        if(it != fileHandles.end()) {
            auto f = it->second;
            std::string contents((std::istreambuf_iterator<char>(*f)), std::istreambuf_iterator<char>());

            variables[varIndex].type = TAG_STRING;
            variables[varIndex].data = contents;
        } else {
            throw std::runtime_error("readFile failed: invalid file handle");
        }
    }},
    {0xA4, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        // closeFile
        auto handle = getInt(stack.back()); stack.pop_back();

        auto it = fileHandles.find(handle);
        if(it != fileHandles.end()) {
            auto f = it->second;
            f->close();
            fileHandles.erase(it);
        } else {
            throw std::runtime_error("writeFile failed: invalid file handle");
        }
    }},
    {0xA5, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        // randomSeed
        auto seed = getInt(stack.back()); stack.pop_back();

        rngEngine.seed(seed);
    }},
    {0xA6, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        // random
        auto varIndex = getInt(stack.back()); stack.pop_back();

        static std::uniform_real_distribution<double> dist(0.0, 1.0);
        float val = dist(rngEngine);

        variables[varIndex].type = TAG_FLOAT;
        variables[varIndex].data = val;
    }},
    {0xA7, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        // randomRange
        auto varIndex = getInt(stack.back()); stack.pop_back();
        auto max = getInt(stack.back()); stack.pop_back();
        auto min = getInt(stack.back()); stack.pop_back();

        std::uniform_int_distribution<int64_t> dist(min, max); // inclusive on both ends
        int64_t val = dist(rngEngine);

        variables[varIndex].type = TAG_INT;
        variables[varIndex].data = val;
    }},
};