#include <drogon/drogon.h>

#include "AppRoutes.h"

int main()
{
    // Load server and database configuration
    drogon::app().loadConfigFile("../config/config.json");

    // Register application routes
    registerAppRoutes();

    // Start server
    drogon::app().run();

    return 0;
}
