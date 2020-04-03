#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void pay_award_withdrawal_request_operation::validate() const
{
    validate_account_name(payer);
    validate_award_number(award_number);
    validate_payment_number(payment_number);
    if (subaward_number.valid())
    {
        validate_award_number(*subaward_number);
    }
}


} /* deip::protocol */
} /* protocol */