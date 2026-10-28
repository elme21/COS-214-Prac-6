#ifndef WORKITERATOR_H
#define WORKITERATOR_H

class WorkComponent;

/**
 * @brief Abstract iterator over a tree of WorkComponents.
 *
 * Iterator role in the Iterator pattern. It has no base implementation and
 * only interacts with the Composite pattern through WorkComponent. Callers
 * delete the iterator when they are done with it.
 */
class WorkIterator
{
    public:
        /** @brief Virtual destructor so concrete iterators are deleted correctly. */
        virtual ~WorkIterator() {}

        /** @brief Checks whether another item remains. 
         * @return True if next() will return an item. 
         */
        virtual bool hasNext() = 0;

        /**
         * @brief Returns the next item and advances the iterator.
         * @return The next item, or nullptr if there are no more items.
         */
        virtual WorkComponent* next() = 0;      

        /** @brief Returns the iterator to the start of the traversal. */
        virtual void reset() = 0;

        /** @brief Gets the item most recently returned by next(). 
         * @return The current item, or nullptr if next() has not returned one yet. 
         */
        virtual WorkComponent* currentItem() = 0;
};

#endif