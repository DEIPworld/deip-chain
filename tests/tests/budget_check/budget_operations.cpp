#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/deip_objects.hpp>

#include <deip/chain/dbs_account.hpp>
#include <deip/chain/dbs_budget.hpp>

#include "database_fixture.hpp"

#include <limits>

using namespace deip;
using namespace deip::chain;
using namespace deip::protocol;
using fc::string;

//
// usage for all budget tests 'chain_test  -t budget_*'
//

class budget_operation_check_fixture : public timed_blocks_database_fixture
{
public:
    budget_operation_check_fixture()
    {
        create_budget_op.owner = "alice";        
        create_budget_op.balance = asset(BUDGET_BALANCE_DEFAULT, DEIP_SYMBOL);
        create_budget_op.start_block = 0;
        create_budget_op.end_block = 1;
        create_budget_op.target_discipline = "1";
    }

    create_budget_operation create_budget_op;

    const int BUDGET_BALANCE_DEFAULT = 200;
    const char* BUDGET_CONTENT_PERMLINK = "https://gitlab.com/DEIP/deip-blockchain/blob/master/README.md";
};

BOOST_FIXTURE_TEST_SUITE(budget_operation_check, budget_operation_check_fixture)

DEIP_TEST_CASE(create_budget_operation_check)
{
    BOOST_REQUIRE_NO_THROW(create_budget_op.validate());
}

DEIP_TEST_CASE(create_budget_operation_check_invalid_balance_amount)
{
    create_budget_op.balance = asset(0, DEIP_SYMBOL);

    BOOST_REQUIRE_THROW(create_budget_op.validate(), fc::assert_exception);

    create_budget_op.balance = asset(-BUDGET_BALANCE_DEFAULT, DEIP_SYMBOL);

    BOOST_REQUIRE_THROW(create_budget_op.validate(), fc::assert_exception);
}

DEIP_TEST_CASE(create_budget_operation_check_invalid_balance_currency)
{
    create_budget_op.balance = asset(BUDGET_BALANCE_DEFAULT, VESTS_SYMBOL);

    BOOST_REQUIRE_THROW(create_budget_op.validate(), fc::assert_exception);
}

DEIP_TEST_CASE(create_budget_operation_check_invalid_owner_name)
{
    create_budget_op.owner = "";

    BOOST_REQUIRE_THROW(create_budget_op.validate(), fc::assert_exception);

    create_budget_op.owner = "wrong;\n'j'";

    BOOST_REQUIRE_THROW(create_budget_op.validate(), fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()

class budget_transaction_check_fixture : public budget_operation_check_fixture
{
public:
    budget_transaction_check_fixture()
        : budget_service(db.obtain_service<dbs_budget>())
        , account_service(db.obtain_service<dbs_account>())
    {
    }

    dbs_budget& budget_service;
    dbs_account& account_service;

    private_key_type alice_create_budget(const asset& balance);
};

private_key_type budget_transaction_check_fixture::alice_create_budget(const asset& balance)
{
    BOOST_REQUIRE(BLOCK_LIMIT_DEFAULT > 0);

    ACTORS((alice))

    fund("alice", BUDGET_BALANCE_DEFAULT);

    create_budget_operation op;
    op.owner = "alice";
    op.balance = balance;
    op.start_block = 0;
    op.end_block = 1;
    op.target_discipline = "1";    

    BOOST_REQUIRE_NO_THROW(op.validate());

    signed_transaction tx;

    tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
    tx.operations.push_back(op);

    BOOST_REQUIRE_NO_THROW(tx.sign(alice_private_key, db.get_chain_id()));
    BOOST_REQUIRE_NO_THROW(tx.validate());

    BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

    BOOST_REQUIRE(budget_service.get_budgets("alice").size() == 1);

    return alice_private_key;
}

BOOST_FIXTURE_TEST_SUITE(budget_transaction_check, budget_transaction_check_fixture)

// DEIP_TEST_CASE(create_budget_check)
// {
//     asset balance(BUDGET_BALANCE_DEFAULT, DEIP_SYMBOL);

//     BOOST_REQUIRE_NO_THROW(alice_create_budget(balance));

//     const budget_object& budget = (*budget_service.get_budgets("alice").cbegin());

//     BOOST_REQUIRE(budget.owner == "alice");
//     // BOOST_REQUIRE(!budget.content_permlink.compare(BUDGET_CONTENT_PERMLINK));
//     BOOST_REQUIRE(budget.balance == balance);
//     // BOOST_REQUIRE(budget.deadline == default_deadline);

//     BOOST_REQUIRE_NO_THROW(validate_database());
// }

BOOST_AUTO_TEST_SUITE_END()

#endif
