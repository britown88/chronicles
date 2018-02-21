
#include "app.h"

int main(int, char**)
{
   auto app = appCreate();

   appCreateWindow(app, WindowConfig{ 1280, 720, "CRN4.EXE" });

   while (appRunning(app)) {      
      appStep(app);
   }

   appDestroy(app);
   return 0;
}
