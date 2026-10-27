#ifndef SERVER_APPLICATION_H
#define SERVER_APPLICATION_H

#include "map.h"

class ServerApplication {
private:
    game::Map make_initial_map();

public:
    int run(int argc, char* argv[]);
};

#endif
