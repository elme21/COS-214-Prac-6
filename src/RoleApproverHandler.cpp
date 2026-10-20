#include "RoleApproverHandler.h"
#include <iostream>
using namespace std;

void RoleApproverHandler::handleRequest()
{
    if (isAuthorized())
    {
        cout << "RoleApproverHandler: handled request." << endl;
    }
    else
    {
        ApprovalHandler::handleRequest();
    }
}

bool RoleApproverHandler::isAuthorized()
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    bool randomBool = std::rand() % 2;
    return randomBool;
}