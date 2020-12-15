#include <iostream>
#include <string.h>

#pragma once

using namespace std;

class Marks
{
	private:
		int   rno;
		float perc;
		
    public:
        Marks();
        void readMarks(void);
        void printMarks(void);
        
};
