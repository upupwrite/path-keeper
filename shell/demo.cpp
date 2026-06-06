#include <iostream>
#include <string>

int main(){
    std::string name;
    std::cerr<<"name:";
    std::getline(std::cin,name);
    std::cout<<name;
    return 0;
}
