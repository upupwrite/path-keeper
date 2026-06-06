#pragma once
#include "pk.h"

class Interaction
{
public:
    Interaction();
    void main(int argc, char *argv[]);

private:
    PathKeeper pk;
    void dir();
};
