#include "marks.h"

using namespace std;

//********************************************
Marks::Marks()  { rno = 0; perc = 0.0; }
		
//input roll numbers and percentage
void Marks::readMarks(void)
{
	cout<<"Enter roll number: ";
	cin>>rno;
	cout<<"Enter percentage: ";
	cin>>perc;
}
		
//print roll number and percentage
void Marks::printMarks(void)
{
	cout<<"Roll No.: "<<rno<<endl;
	cout<<"Percentage: "<<perc<<"%"<<endl;
}


