#include <iostream>
#include <cstring> // for strcmp()

using namespace std;

int main(int argc, char* argv[]) {
	
	// default values
	bool rocchio = false;
	string query_file = "";
	string model_dir = "";
	string output_file = "";
	string NTCIR_dir = "";

	// process arguments
	for(int i=0; i<argc; i++) {
		if(strcmp(argv[i], "-r") == 0) // enable rocchio
			rocchio = true;
		if(strcmp(argv[i], "-i") == 0) // get query_file
			query_file = string(argv[i+1]);
		if(strcmp(argv[i], "-o") == 0) // get output_file
			output_file = string(argv[i+1]);
		if(strcmp(argv[i], "-m") == 0) // get model_dir
			model_dir = string(argv[i+1]);
		if(strcmp(argv[i], "-d") == 0) // get NTCIR_dir
			NTCIR_dir = string(argv[i+1]);
	}

	// output the makefile command
	cout << "make run"; // main command
	cout << " MODEL_DIR=" << model_dir;
	cout << " QUERY_PATH=" << query_file;
	cout << " OUTPUT_PATH=" << output_file;
	if(rocchio)
		cout << " ROCCHIO_FLAG=true";
	else
		cout << " ROCCHIO_FLAG=false";
	cout << endl;
	return 0;
}
