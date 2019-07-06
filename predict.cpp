#include "predict.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include "beacon.h"
#include "gps.h"
#include "debug.h"

// VARS
PredictStruct predict;

bool is_visible(string sat, int min_ele) {		// use PREDICT to figure out if this sat is around to try
	if (predict.path.compare("") == 0) return false;	// short circuit if predict not defined
	ofstream qthfile;
	gps::PosStruct gps = gps::getPos();
	qthfile.open("/tmp/picrumbs.qth", ios::trunc);		// PREDICT doesn't seem to be able to take a location via command line, so we need to build a "qth file" based on the current position.
	qthfile.precision(6);
	qthfile << fixed;
	qthfile << beacon::mycall << '\n';
	qthfile << ' ' << gps.lat << '\n';
	qthfile << ' ' << -gps.lon << '\n';	// predict expects negative values for east
	qthfile << ' ' << gps.alt << '\n';
	qthfile.close();

	stringstream predict_cmd;
	predict_cmd << predict.path;
	predict_cmd << " -f " << '"' << sat << "\" -q /tmp/picrumbs.qth -t " << predict.tlefile;	// build the command line

	try {
		FILE* predict = popen(predict_cmd.str().c_str(), "r");
		if (!predict) {
			fprintf(stderr, "Could not execute %s\n", predict_cmd.str().c_str());
			return false;
		}

		char buff[256];
		fgets(buff, 256, predict);
		pclose(predict);
		string buff_s = buff;

		int ele = atoi(buff_s.substr(32, 4).c_str());
		if (debug.fh) printf("FH_DEBUG: %s elevation is %d.\n", sat.c_str(), ele);
		return (ele > min_ele);
	} catch (exception& e) {
		fprintf(stderr, "Error while executing PREDICT (%s): %s\n", predict_cmd.str().c_str(), e.what());
		return false;
	}
	return false;
}	// END OF 'is_visible'