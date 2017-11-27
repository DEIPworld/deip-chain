#pragma once

#include <fc/fixed_string.hpp>

#include <scorum/protocol/authority.hpp>
#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/scorum_object_types.hpp>
#include <scorum/chain/witness_objects.hpp>
#include <scorum/chain/shared_authority.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace scorum {
namespace chain {

using scorum::protocol::asset;

class budget_object : public object<budget_object_type, budget_object>
{
    budget_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    budget_object(Constructor&& c, allocator<Allocator> a): content_permlink(a)
    {
        c(*this);
    }

    id_type id;

    account_name_type owner;
    shared_string content_permlink;

    time_point_sec created = time_point_sec::min();

    asset balance = asset(0, SCORUM_SYMBOL);
    share_type per_block = 0;

    uint32_t last_allocated_block = 0;

    time_point_sec deadline = time_point_sec::maximum();
};

struct by_owner_name;

typedef multi_index_container<budget_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<budget_object,
                                                               budget_id_type,
                                                               &budget_object::id>>,
                                         ordered_non_unique<tag<by_owner_name>,
                                                        member<budget_object,
                                                               account_name_type,
                                                               &budget_object::owner>>>,
                              allocator<budget_object>>
    budget_index;

}
}

FC_REFLECT( scorum::chain::budget_object,
             (id)(owner)(content_permlink)(created)(balance)(per_block)(last_allocated_block)(deadline)
)

CHAINBASE_SET_INDEX_TYPE( scorum::chain::budget_object, scorum::chain::budget_index )

