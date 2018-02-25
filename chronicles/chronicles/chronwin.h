#pragma once

//insert all windows shit here

#include <string>

struct OpenFileConfig {
   std::string defaultExt;
   std::string fileName;
   std::string filter;
   int filterIndex = 0;
   std::string initialDir;
   std::string title;
};

std::string openFile(OpenFileConfig const& config);