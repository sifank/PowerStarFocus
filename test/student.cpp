#include "student.h"
#include "marks.h"

using namespace std;

//********************************************
Student::Student()  { Marks objM; }
	
//input student details
void Student::readStudent(void)
{
	//Input name
	cout<<"Enter name: ";
	cin.getline(name, 30);
	//input Marks
	objM.readMarks();			
}
		
//print student details
void Student::printStudent(void)
{
	//print name
	cout<<"Name: "<<name<<endl;
	//print marks
	objM.printMarks();
}

