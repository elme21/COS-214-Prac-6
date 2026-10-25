#ifndef APPROVALREQUEST_H
#define APPROVALREQUEST_H

class WorkItem;

class ApprovalRequest
{
public:
    ApprovalRequest(WorkItem* item, double value = 0.0)
        : item(item), value(value) {}

    WorkItem* getItem() const { return item; }
    double getValue() const { return value; }

private:
    WorkItem* item;
    double value;
};

#endif