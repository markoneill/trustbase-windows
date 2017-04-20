#include "stdafx.h"
#include "THLogger.h"

// define statics
std::ofstream thlog::log_file;
bool thlog::also_cout = false;