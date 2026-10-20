#include "ApprovalHandler.h"
#include <iostream>
using namespace std;

void ApprovalHandler::handleRequest()
{
    if (successor)
    {
        successor->handleRequest();
    }
    else
    {
        cout << "No successor to handle the request." << endl;
    }
}