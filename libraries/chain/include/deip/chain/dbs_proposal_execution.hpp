#pragma once

#include <functional>

#include <deip/protocol/deip_operations.hpp>

#include <deip/chain/global_property_object.hpp>

#include <deip/chain/evaluator.hpp>
#include <deip/chain/dbservice.hpp>

#include <deip/chain/dbs_account.hpp>
#include <deip/chain/dbs_proposal.hpp>
#include <deip/chain/dbs_research_group.hpp>
#include <deip/chain/dbs_research_token.hpp>
#include <deip/chain/dbs_research.hpp>
#include <deip/chain/dbs_research_content.hpp>
#include <deip/chain/dbs_research_token_sale.hpp>
#include <deip/chain/dbs_dynamic_global_properties.hpp>
#include <deip/chain/dbs_discipline.hpp>
#include <deip/chain/dbs_research_discipline_relation.hpp>
#include <deip/chain/dbs_research_group_invite.hpp>
#include <deip/chain/dbs_research_group_join_request.hpp>
#include <deip/chain/dbs_vote.hpp>
#include <deip/chain/dbs_proposal_execution.hpp>

#include <deip/chain/dbs_base_impl.hpp>
#include <deip/chain/proposal_object.hpp>
#include <deip/chain/proposal_data_types.hpp>

namespace deip {
namespace chain {

class dbs_proposal_execution : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_proposal_execution() = delete;

public:
    using evaluator_callback = std::function<void(const proposal_object&)>;

    class proposal_evaluators_register
    {
    public:
        void set(proposal_action_type action, evaluator_callback callback)
        {
            _register.insert(std::make_pair(action, callback));
        }

        void execute(const proposal_object& proposal)
        {
            if (_register.count(proposal.action) == 0)
            {
                FC_ASSERT("Invalid proposal action type");
            }
            else
            {
                _register[proposal.action](proposal);
            }
        }

    private:
        fc::flat_map<proposal_action_type, evaluator_callback> _register;
    };

    void execute_proposal(const proposal_object& proposal)
    {
        executions.execute(proposal);
    }

protected:
    explicit dbs_proposal_execution(database &db);

    void invite(const proposal_object& proposal);

private:
    proposal_evaluators_register executions;

    template <typename DataType> DataType get_data(const proposal_object& proposal)
    {
        auto data = fc::json::from_string(
                fc::to_string(proposal.data)
        ).as<DataType>();
        data.validate();
        return data;
    }

};

} // namespace chain
} // namespace deip