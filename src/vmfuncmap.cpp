#include "vm.h"

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
    }}
};