// Director - determines the construction sequence for the clinic appointment workflow.
#ifndef WEB_HEALTHCAREDIRECTOR_H
#define WEB_HEALTHCAREDIRECTOR_H

class DefinitionBuilder;
class ApprovalHandler;

/**
 * @brief Second Director for the engine's DefinitionBuilder (Builder pattern).
 *
 * Builds "patient appointment booking at a clinic" with the same builder
 * interface EnrolmentDirector uses, so both industries share one engine.
 */
class HealthcareDirector {
public:
    /** @param referralChain first handler of the Nurse -> GP -> Specialist chain (not owned). */
    explicit HealthcareDirector(ApprovalHandler* referralChain);

    void construct(DefinitionBuilder& builder);

private:
    ApprovalHandler* referralChain;
};

#endif
