#include "pk.h"


void PathKeeper::shellCommand(const std::string& command, const std::string& cwd)
{
    Json::Value config = file.loadConfig();
    std::string myshell = config["shell"].asString();

//    std::cout<<myshell<<" -c cd "<<cwd<<" "<<command;
    std::cout<<"cd "<<cwd<<" "<<command;

}
