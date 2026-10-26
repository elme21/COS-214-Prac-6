//Director - Determines the construction sequence
#ifndef ENROLMENTDIRECTOR_H
#define ENROLMENTDIRECTOR_H

class DefinitionBuilder;

/**
 * @brief Director in the Builder design pattern.
 *
 * Defines the sequence of construction steps needed
 * to create a university enrolment workflow.
 *
 * Delegates the actual construction to DefinitionBuilder.
 */
class EnrolmentDirector {
public:
    /**
     * @brief Constructs the enrolment workflow.
     * @param builder Builder used to assemble the workflow.
     */
    void construct(DefinitionBuilder& builder);
};

#endif
