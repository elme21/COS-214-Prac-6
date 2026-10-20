#include "FinalApproverHandler.h"
#include <iostream>
using namespace std;

void FinalApproverHandler::handleRequest()
{
    if (isAuthorized())
    {
        cout << "FinalApproverHandler: handled request." << endl;
    }
    else
    {
        ApprovalHandler::handleRequest();
    }
}

bool FinalApproverHandler::isAuthorized()
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    bool randomBool = std::rand() % 2;
    return randomBool;
}