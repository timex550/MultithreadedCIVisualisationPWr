PHONY:=clean run

SO2_etap3: main.cpp resources.cpp resources.hpp jobs.cpp jobs.hpp sync.cpp sync.hpp
	@g++ -std=c++17 -g -pthread -I ./ -o SO2_etap3 main.cpp resources.cpp jobs.cpp sync.cpp -lncurses

clean:
	@if [ -e ./SO2_etap3 ]; then \
		rm SO2_etap3;\
	fi

run: SO2_etap3
	@./SO2_etap3
