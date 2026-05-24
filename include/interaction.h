#pragma once
#include "aliasmanager.h"
#include "hashverifier.h"
#include "loadfile.h"
#include "pk.h"
#include "searcher.h"

class Interaction
{
public:
    Interaction();
    void main(int argc, char *argv[]);

private:
    PathKeeper pk;
    void dir();
    void handleConfig(int argc, char **argv);
    void handleAlias(int argc, char **argv);
};
