#pragma once
#include "pk.h"
#include "loadfile.h"
#include "aliasmanager.h"
#include "hashverifier.h"
#include "searcher.h"

class Interaction {
public:
    Interaction();
    void main(int argc, char *argv[]);
private:
    PathKeeper pk;
    void dir();
    void handleConfig(int argc, char **argv);
    void handleAlias(int argc, char **argv);
};
