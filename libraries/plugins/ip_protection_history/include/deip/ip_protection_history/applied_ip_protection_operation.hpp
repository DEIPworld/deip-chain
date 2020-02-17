#pragma once

#include <deip/protocol/operations.hpp>
#include "ip_protection_operation_object.hpp"

namespace deip {
namespace ip_protection_history {

using deip::protocol::transaction_id_type;
using deip::protocol::operation;

struct applied_ip_protection_operation
{
    applied_ip_protection_operation();
    applied_ip_protection_operation(const ip_protection_operation_object& op_obj);

    transaction_id_type trx_id;
    uint32_t block = 0;

    fc::time_point_sec timestamp;
    operation op;
};

}
}

FC_REFLECT(deip::ip_protection_history::applied_ip_protection_operation, (trx_id)(block)(timestamp)(op))