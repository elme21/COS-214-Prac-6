#ifndef WORKSTAGE_H
#define WORKSTAGE_H

#include <vector>
#include <string>
#include "WorkComponent.h"

/**
 * @brief Composite node that groups WorkComponents into one stage of a workflow.
 *
 * Composite role in the Composite pattern, and the aggregate that creates
 * iterators in the Iterator pattern. The stage owns its children and deletes
 * them in its destructor.
 */
class WorkStage : public WorkComponent 
{
    private:
        std::vector<WorkComponent*> children;   ///< Owned children, in execution order
        std::string name;

    public:
        /** @brief Creates an empty stage. */
        WorkStage() {}

        /** @brief Deletes every child the stage owns. */
        virtual ~WorkStage();

        // Composite / Template Method / Prototype / Iterator

        /** @brief Executes each child in order. */
        void execute() override;

        /**
         * @brief Adds a child to the end of this stage.
         * @param component The child to add. The stage takes ownership. A null pointer is ignored.
         */
        void add(WorkComponent* component) override;

        /**
         * @brief Removes a child from this stage without deleting it.
         *
         * The caller keeps ownership of the removed child. Nothing happens if
         * the component is not a child of this stage.
         *
         * @param component The child to remove.
         */
        void remove(WorkComponent* component) override;

        /**
         * @brief Gets the child at a position.
         * @param index Zero-based position of the child.
         * @return The child, or nullptr if index is out of range.
         */
        WorkComponent* getChild(int index) override;

        /**
         * @brief Creates an iterator over this stage and everything beneath it.
         * @return A new DepthFirstIterator. The caller must delete it.
         */
        WorkIterator* createIterator() override;

        /**
         * @brief Makes a deep copy of this stage.
         * @return A new stage holding a clone of every child. The caller must delete it.
         */
        WorkComponent* clone() const override;

        // Lifecycle: a stage has no state of its own, so these are unsupported

        /** @brief Unsupported on a stage. 
         * @return Always false. 
         */
        bool makeAvailable() override;

        /**
         * @brief Unsupported on a stage.
         * @param participant Ignored.
         * @return Always false.
         */
        bool assignTo(Participant* participant) override;

        /** @brief Unsupported on a stage. 
         * @return Always false. 
         */
        bool start() override;

        /** @brief Unsupported on a stage. 
         * @return Always false. 
         */
        bool approve() override;

        /** @brief Unsupported on a stage. 
         * @return Always false. 
         */
        bool complete() override;

        /** @brief Unsupported on a stage. 
         * @return Always false. 
         */
        bool reject() override;

        /** @brief Unsupported on a stage. 
         * @return Always false. 
         */
        bool cancel() override;

        /** @brief Unsupported on a stage. 
         * @return Always false. 
         */
        bool escalate() override;

        // Undo support

        /** @brief A stage has no state of its own. 
         * @return Always nullptr. 
         */
        const WorkItemState* getState() const override;

        /** @brief A stage has no assignee. 
         * @return Always nullptr. 
         */
        Participant* getAssignee() const override;

        /**
         * @brief Does nothing, because a stage has no state to restore.
         * @param state Ignored.
         * @param assignee Ignored.
         */
        void restoreState(const WorkItemState* state, Participant* assignee) override;
};

#endif