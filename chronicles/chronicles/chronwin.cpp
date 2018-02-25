#include "chronwin.h"
#include <nowide/convert.hpp>
#include <Windows.h>

std::string openFile(OpenFileConfig const& config) {
   auto blah = nowide::widen(std::string("hello!"));
   return 0;
}