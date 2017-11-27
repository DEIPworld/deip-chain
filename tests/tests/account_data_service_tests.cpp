#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/account_object.hpp>
#include <scorum/chain/dbs_account.hpp>

#include "../common/database_fixture.hpp"

namespace scorum {
namespace chain {

private_key_type generate_private_key(const std::string& str)
{
    return private_key_type::regenerate(fc::sha256::hash(std::string(str)));
}

class create_account_with_data_service_fixture : public clean_database_fixture
{
public:
    create_account_with_data_service_fixture()
        : public_key(generate_private_key("user private key").get_public_key())
        , data_service(db.obtain_service<dbs_account>())
    {
    }

    void create_account()
    {
        data_service.create_account_by_faucets("user", "initdelegate", public_key, "", authority(), authority(),
                                               authority(), asset(0, SCORUM_SYMBOL));
    }

    share_type calc_fee()
    {
        return std::max(db.get_witness_schedule_object().median_props.account_creation_fee.amount
                            * SCORUM_CREATE_ACCOUNT_WITH_SCORUM_MODIFIER,
                        share_type(100));
    }

    const public_key_type public_key;
    dbs_account& data_service;
};

BOOST_FIXTURE_TEST_SUITE(create_account_with_data_service, create_account_with_data_service_fixture)

BOOST_AUTO_TEST_CASE(check_account_name)
{
    try
    {
        create_account();

        const account_object& acount = db.get_account("user");

        BOOST_CHECK(acount.name == "user");
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_recovery_account_name)
{
    try
    {
        create_account();

        const account_object& acount = db.get_account("user");

        BOOST_CHECK(acount.recovery_account == "initdelegate");
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(fail_on_second_creation)
{
    try
    {
        create_account();

        BOOST_CHECK_THROW(
            data_service.create_account_by_faucets("user", "initdelegate", public_key, "", authority(), authority(),
                                                   authority(), asset(0, SCORUM_SYMBOL)),
            boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<std::logic_error>>);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(create_without_fee)
{
    try
    {
        const asset balance_before_creation = db.get_account("initdelegate").balance;

        create_account();

        BOOST_CHECK(db.get_account("initdelegate").balance == balance_before_creation);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_fee_after_creation)
{
    try
    {
        const asset balance_before_creation = db.get_account("initdelegate").balance;

        const share_type fee = calc_fee();

        data_service.create_account_by_faucets("user", "initdelegate", public_key, "", authority(), authority(),
                                               authority(), asset(fee, SCORUM_SYMBOL));

        BOOST_CHECK(db.get_account("initdelegate").balance == asset(balance_before_creation.amount - fee));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace scorum

#endif
