#include <iostream>
#include <vector>  
#include <string>  
#include <stdio.h>  
#include <stdlib.h> 

#include <cgicc/CgiDefs.h> 
#include <cgicc/Cgicc.h> 
#include <cgicc/HTTPHTMLHeader.h> 
#include <cgicc/HTMLClasses.h>  

using namespace std;
using namespace cgicc;

struct Position {
		int id = 4045;
		double x = -0.6110712607613635;
		double y = 5.975599709099018;
		double z = 0.8;
		bool stay = true;
		string sample_time = "2022-10-24,11:43:04";
		int sample_batch = 27;
};

int main () {
	Cgicc formData;
	struct Position position;
	cout << "Content-type:application/json\r\n\r\n";
	form_iterator fi = formData.getElement("id");
    cout << "{\"id\":" << **fi << ",";
	cout << "\"x\":" << position.x << ",";
	cout << "\"y\":" << position.y << ",";
	cout << "\"z\":" << position.z << ",";
	cout << "\"stay\":" << position.stay << ",";
	cout << "\"sample_time\":" << "\"" << position.sample_time<< "\",";
	cout << "\"sample_batch\":" << position.sample_batch<< "}" << endl;
	return 0;
}

