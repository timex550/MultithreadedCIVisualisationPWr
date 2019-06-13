#include "jobs.hpp"
#include "resources.hpp"
#include "sync.hpp"
#include <ncurses.h>
#include <iostream>

void foo(int,int);

int main()
{
	
	// ncurses initialization
	initscr();
	curs_set(0);

	startVisualisation();

	clear();
	mvprintw(0, 0, "CI platform has finished working!\nPress any key to quit...");
	refresh();
	std::cin.get();
	endwin();

	return 0;
}
