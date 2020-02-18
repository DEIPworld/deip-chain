#pragma once

#include <deip/chain/schema/deip_object_types.hpp>
#include <fc/shared_buffer.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/operations.hpp>
#include <deip/protocol/deip_operations.hpp>

#include <boost/multi_index/composite_key.hpp>

#ifndef ACCOUNT_ECI_HISTORY_SPACE_ID
#define ACCOUNT_ECI_HISTORY_SPACE_ID 19
#endif


namespace deip {
namespace account_eci_history {

using deip::protocol::transaction_id_type;
using deip::protocol::operation;
using namespace deip::chain;

enum account_eci_history_object_type
{
    account_eci_operations_history = (ACCOUNT_ECI_HISTORY_SPACE_ID << 8),
    all_account_eci_operations_history
};

class account_eci_operation_object : public object<account_eci_operations_history, account_eci_operation_object>
{
    account_eci_operation_object() = delete;

 public:
    template <typename Constructor, typename Allocator>
    account_eci_operation_object(Constructor&& c, allocator<Allocator> a)
        : serialized_op(a)
    {
        c(*this);
    }

    typedef typename object<account_eci_operations_history, account_eci_operation_object>::id_type id_type;

    id_type id;

    transaction_id_type trx_id;
    uint32_t block = 0;

    time_point_sec timestamp;
    fc::shared_buffer serialized_op;
};

struct by_transaction_id;
typedef chainbase::shared_multi_index_container<account_eci_operation_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<account_eci_operation_object,
                                                                       account_eci_operation_object::id_type,
                                                                       &account_eci_operation_object::id>>
#ifndef SKIP_BY_TX_ID
                                                                      ,
                                                ordered_unique<tag<by_transaction_id>,
                                                               composite_key<account_eci_operation_object,
                                                                             member<account_eci_operation_object,
                                                                                    transaction_id_type,
                                                                                    &account_eci_operation_object::trx_id>,
                                                                             member<account_eci_operation_object,
                                                                                     account_eci_operation_object::id_type,
                                                                                     &account_eci_operation_object::id>>>
#endif
                                                >>
        account_eci_operation_index;

}
}

FC_REFLECT(deip::account_eci_history::account_eci_operation_object,
           (id)(trx_id)(block)(timestamp)(serialized_op))
CHAINBASE_SET_INDEX_TYPE(deip::account_eci_history::account_eci_operation_object,
        deip::account_eci_history::account_eci_operation_index)