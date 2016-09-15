#include <iostream>
#include "../algo.hpp"
#include <stdlib.h>

using namespace std;

int main(int argc, char **argv){
	if (argc < 4){
		cout << "No input" << endl;
		return 1;
	}
	
	for (auto s : regex_split(argv[3], regex(argv[1]), atol(argv[2]))){
		cout << "'" << s << "'" << endl;
	}
	
	return 0;
}

