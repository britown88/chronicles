#pragma once

//insert all windows shit here

#include <string>

struct OpenFileConfig {
   std::string filterNames;
   std::string filterExtensions;
   int filterIndex = 0;
   std::string initialDir;
   std::string title;
};

std::string openFile(OpenFileConfig const& config);

std::string cwd();