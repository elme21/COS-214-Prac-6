#ifndef DEPTHFIRSTITERATOR_H
#define DEPTHFIRSTITERATOR_H

#include <stack>
#include "WorkIterator.h"

/**
 * @brief Concrete iterator that walks a tree depth-first.
 *
 * Visits a parent before its children (pre-order), and children from first to
 * last. Uses an explicit stack rather than recursion.
 */
class DepthFirstIterator : public WorkIterator 
{
    private:
        WorkComponent* root;                  ///< Start of the traversal, not owned
        std::stack<WorkComponent*> pending;   ///< Nodes still to visit
        WorkComponent* current;               ///< Item most recently returned by next()

    public:
        /**
         * @brief Creates an iterator positioned at the start of the tree.
         * @param root The component to start from, not owned.
         */
        explicit DepthFirstIterator(WorkComponent* root);

        /** @brief Checks whether any nodes remain to visit. 
         * @return True if next() will return an item. 
         */
        bool hasNext() override;

        /**
         * @brief Returns the next node in depth-first order.
         * @return The next node, or nullptr if the traversal is finished.
         */
        WorkComponent* next() override;

        /** @brief Restarts the traversal from the root. */
        void reset() override;

        /** @brief Gets the node most recently returned by next(). 
         * @return The current node, or nullptr if next() has not returned one yet. 
         */
        WorkComponent* currentItem() override;
};

#endif