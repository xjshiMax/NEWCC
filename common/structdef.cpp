#include "structdef.h"
#include <iostream>
#include <fstream>
#include <sstream>

std::string base_script_t::bill_info() {
    ostringstream ostm;


    ostm << '"' << voice_version_id << '"' << ',';
    ostm << '"' << type << '"' << ',';
    ostm << '"' << nodeId << '"' << ',';
    ostm << '"' << desc << '"' << ',';
    ostm << '"' << userWord << '"' << ',';
    ostm << '"' << vox_base << '"' << ',';
    ostm << '"' << taskId << '"' << ',';

    return ostm.str();

}
