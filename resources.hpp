#pragma once
#include <mutex>
#include <array>
#include <vector>
#include <string>
#include <chrono>

struct Core
{
	bool isFree{ true };
	int ownerId{ -1 };
};

template <unsigned N>
struct CPU
{
	std::array<Core, N> cores;
	int numOfFreeCores{ N };
};

struct File
{
	std::string name;
	int numOfChanges{ 0 };
	int ownerId{ -1 };
	bool isFree{ true };
	std::chrono::system_clock::time_point lastChanged{ std::chrono::system_clock::now() };

	File() = delete;
	File(std::string name): name(name) {};
};
