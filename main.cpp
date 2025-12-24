
#include "logger.hpp"
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>

namespace fs = std::filesystem;

static bool parse_mcfunction(const fs::path& file, int& center_x, int& center_y, int& center_z){
	std::ifstream fin(file);
	if (!fin) return false;

	std::vector<int> xs, ys, zs;
	std::string line;

	while (std::getline(fin, line)){
		std::stringstream ss(line);
		std::string cmd;
		ss >> cmd;
		if (cmd == "setblock"){
			int x,y,z;
			if (ss >> x >> y >> z){
				xs.push_back(x);
				ys.push_back(y);
				zs.push_back(z);
			}
		}
	}

	if (xs.empty()) return false;

	int min_x = *std::min_element(xs.begin(), xs.end());
	int max_x = *std::max_element(xs.begin(), xs.end());
	int min_z = *std::min_element(zs.begin(), zs.end());
	int max_z = *std::max_element(zs.begin(), zs.end());
	int max_y = *std::max_element(ys.begin(), ys.end());

	center_x = (min_x + max_x) / 2;
	center_z = (min_z + max_z) / 2;
	center_y = max_y + 32;
	return true;
}

static bool collect_entries(const std::string& mcfunction_folder, std::vector<std::string>& out_lines, std::string& error){
	if (!fs::exists(mcfunction_folder) || !fs::is_directory(mcfunction_folder)){
		error = "Folder does not exist.";
		return false;
	}

	std::map<std::string, std::vector<std::pair<int, std::string>>> groups;

	for (const auto& entry : fs::directory_iterator(mcfunction_folder)){
		if (!entry.is_regular_file()) continue;
		auto p = entry.path();
		if (p.extension() != ".mcfunction") continue;

		std::string name = p.stem().string();
		if (name.size() < 9) continue;

		std::string prefix = name.substr(0,8);
		std::string num_str = name.substr(8);
		int num = 0;
		try { num = std::stoi(num_str); } catch (...) { num = 0; }

		groups[prefix].push_back({ num, name });
	}

	std::vector<std::string> sorted_names;
	for (auto& g : groups){
		std::sort(g.second.begin(), g.second.end(), [](auto& a, auto& b){ return a.first < b.first; });
		for (auto& p : g.second) sorted_names.push_back(p.second);
	}

	for (const auto& name : sorted_names){
		fs::path file = fs::path(mcfunction_folder) / (name + ".mcfunction");
		int cx, cy, cz;
		if (!parse_mcfunction(file, cx, cy, cz)) continue;
		out_lines.push_back(std::string("tp @s ") + std::to_string(cx) + " " + std::to_string(cy) + " " + std::to_string(cz));
		out_lines.push_back(std::string("function ") + name);
	}

	return true;
}

static bool generate_to_file(const std::string& mcfunction_folder, const std::string& output_file, std::string& error){
	std::vector<std::string> lines;
	if (!collect_entries(mcfunction_folder, lines, error)) return false;
	std::ofstream fout(output_file);
	if (!fout){ error = "Failed to open output file."; return false; }
	for (const auto& l : lines) fout << l << "\n";
	return true;
}

static bool execute_to_stdout(const std::string& mcfunction_folder, int& out_count, std::string& error){
	std::vector<std::string> lines;
	if (!collect_entries(mcfunction_folder, lines, error)) return false;
	out_count = 0;
	for (const auto& l : lines){
		std::cout << l << std::endl;
		++out_count;
	}
	return true;
}

int main(){
	logf(INF, "Interactive mode. Type 'help'");
	std::string line;
	while (true) {
		std::cout << "mcfunction2list>";
		std::cout.flush();
		if (!std::getline(std::cin, line)) break;
		// trim whitespace (including CR) from both ends
		auto l = line.find_first_not_of(" \t\r\n");
		if (l == std::string::npos) continue;
		auto r = line.find_last_not_of(" \t\r\n");
		line = line.substr(l, r - l + 1);
		if (line.empty()) continue;
		if (line == "help") {
			logf(INF, "Commands: help, generate <mcfunction_folder> <output_file>, execute <mcfunction_folder>, exit");
			continue;
		}
		if (line.rfind("generate ", 0) == 0){
			std::istringstream iss(line);
			std::string cmd, folder, out;
			iss >> cmd >> folder >> out;
			if (folder.empty() || out.empty()){
				logf(WARN, "Usage: generate <mcfunction_folder> <output_file>");
				continue;
			}
			std::string err;
			logf(INF, "Generating file: %s -> %s", folder.c_str(), out.c_str());
			if (!generate_to_file(folder, out, err)){
				logf(ERR, "%s", err.c_str());
			} else {
				logf(INF, "Generate completed.");
			}
			continue;
		}
		if (line.rfind("execute ", 0) == 0){
			std::istringstream iss(line);
			std::string cmd, folder;
			iss >> cmd >> folder;
			if (folder.empty()){
				logf(WARN, "Usage: execute <mcfunction_folder>");
				continue;
			}
			std::string err;
			logf(INF, "Executing folder: %s", folder.c_str());
			int count = 0;
			if (!execute_to_stdout(folder, count, err)){
				logf(ERR, "%s", err.c_str());
			} else {
				logf(INF, "Executed %d lines.", count);
			}
			continue;
		}
		if (line == "exit" || line == "quit") {
			logf(INF, "Exiting.");
			break;
		}
		logf(WARN, "Unknown command: %s", line.c_str());
	}
	return 0;
}

