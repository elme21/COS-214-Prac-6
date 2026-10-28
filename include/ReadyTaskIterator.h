#ifndef READYTASKITERATOR_H
#define READYTASKITERATOR_H

#include <stack>
#include "WorkIterator.h"

/**
 * @brief Concrete iterator that finds work ready to be picked up.
 *
 * Walks the tree depth-first and returns only leaf items in the available
 * state (AvailableState). Stages are descended into but never returned.
 * The next ready item is found ahead of time so hasNext() is always accurate.
 */
class ReadyTaskIterator : public WorkIterator 
{
    private:
        WorkComponent* root;                    ///< Start of the traversal, not owned
        std::stack<WorkComponent*> pending;     ///< Nodes still to visit
        WorkComponent* forward;                 ///< Next ready item, or nullptr if none
        WorkComponent* current;                 ///< Item most recently returned by next()

        /** @brief Searches ahead for the next ready item and stores it in forward, or nullptr if none remains. */
        void advance();

        /**
         * @brief Defines what it means for an item to be ready.
         * @param node The item to check.
         * @return True if the item is a leaf in the available state, false otherwise.
         */
        static bool isReadyItem(WorkComponent* node);

    public:
        /**
         * @brief Creates an iterator positioned at the first ready item.
         * @param root The component to start from. Not owned, and must outlive the iterator.
         */
        explicit ReadyTaskIterator(WorkComponent* root);

        /** @brief Checks whether another ready item exists. 
         * @return True if next() will return an item. 
         */
        bool hasNext() override;

        /**
         * @brief Returns the next ready item and searches ahead for the one after it.
         * @return The next ready item, or nullptr if there are none left.
         */
        WorkComponent* next() override;

        /** @brief Restarts the search from the root. */
        void reset() override;

        /** @brief Gets the item most recently returned by next(). 
         * @return The current item, or nullptr if next() has not returned one yet. 
         */
        WorkComponent* currentItem() override;
};

#endif