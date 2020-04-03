#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <deip/protocol/asset.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

struct reject_award_operation : public base_operation
{
    string award_number;
    account_name_type rejector;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(rejector);
    }
};

}
}

FC_REFLECT(deip::protocol::reject_award_operation, 
  (award_number)
  (rejector)
)