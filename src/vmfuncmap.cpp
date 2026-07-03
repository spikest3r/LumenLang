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
    }}
};