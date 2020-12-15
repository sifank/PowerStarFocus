#include <iostream>
#include <string.h>
#include "marks.h"

#pragma once

using namespace std;

class Student
{
	public:
	Student();
	//object to Marks class
	Marks objM;
	char name[30];
	void readStudent(void);
	void printStudent(void);
};
