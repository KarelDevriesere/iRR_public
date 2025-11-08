// Include std
#include <iostream>

// Include other
#include "ClassInterface.h"
#include "../Algorithms/ClassBreakMax.h"

int main(int argc, char *argv[])
{
	// Display welcome text
	std::string welcomeText = "Starting benders decomposition";
	std::cout << welcomeText << std::endl;

	// Arguments not read yet
	std::string xmlFileName = "randomTest.xml";

	// Default arguments
	int truncationLevel = 5;

	// Provided on the command line
	int nrGamesRow = 3;
	int timeLimit=120;

	if(argc == 4){
		std::cout << "BREAK MAXIMIZATION" << std::endl;	
		xmlFileName = argv[1];
		std::cout << "Instance file provided: " << xmlFileName << std::endl;

		timeLimit = std::atoi(argv[2]);

		// Read the xml instance file
		Interface::get()->readInstanceXml(xmlFileName);
		std::string solName = xmlFileName.substr(0, xmlFileName.size()-4); // Cut off .xml
		solName += "_SolBenders.xml";

		const int method = std::atoi(argv[3]);
		if (method <= 3) {
			BreakMax master(Instance::get(), timeLimit, solName, method);
			master.solve();
			std::cout << "Two-phase approach done" << std::endl;
		} else {
			BreakMax master(Instance::get(), timeLimit, solName, method);
			master.solvePattern();
			std::cout << "Two-phase pattern approach done" << std::endl;
		}
	} else {
		std::cout << "Not enough or too much arguments specified" << std::endl;
		std::cout << "Run $./decomposition filename.xml timeLimit nrGamesRow" << std::endl;
		return EXIT_SUCCESS;
	}

	return 0;
}
