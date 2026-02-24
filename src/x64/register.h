//
// Created by root on 2026/2/22.
//

#ifndef POLO_COMPILER_PRE_REGISTER_H
#define POLO_COMPILER_PRE_REGISTER_H
#include <bitset>
#include <ranges>
#include <unordered_map>
inline const char* func_call_regs[] =
#if defined(__X86_64__) || defined(_M_X64) || defined(__amd64)

#if defined(_WIN32) || defined(_WIN64)
	{"rcx", "rdx", "r8", "r9"};
inline const char* free_regs[] = {"rbx", "rsi", "rdi", "r10", "r11", "r12", "r13", "r14", "r15"};

#else
	{"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
inline const char* free_regs[] = {"rbx", "r10", "r11", "r12", "r13", "r14", "r15"};
#endif
inline auto result_reg = "rax";

#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__ARM64_ARCH_8)
		{}
#endif

class RegAlloc {

	std::unordered_map<std::string, std::pair<bool, size_t>> vars;		// name<is_reg, offest>
public:
	static std::bitset<std::size(free_regs)> used;
	static void free(const size_t i) { used.reset(i); };
	static size_t size() { return used.size(); };
	std::pair<bool, size_t> alloc(const std::string& name) {
		std::pair<bool, size_t> result = {false, -1};
		for (size_t r = 0; r < std::size(free_regs); r++) {
			if (!used[r]) {
				used.set(r);
				result = std::pair{true, r};
			}
		}
		if (result.second == -1) {
			for (auto &[is_reg, offest]: vars | std::views::values) {
				if (!is_reg) {
					offest += 8;
				}
			}
		}
		vars[name] = result;
		return result;
	}
	static std::string get_string(const std::pair<bool, size_t>& robj) {
		if (robj.first)
			return free_regs[robj.second];
		if (robj.second == 0) return "[rsp]";
		return "[rsp+" + std::to_string(robj.second) + "]";
	}

};

#endif //POLO_COMPILER_PRE_REGISTER_H