#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/pool/reward_pool.hpp>
#include <deip/chain/dbs_reward.hpp>
#include <deip/chain/dbs_budget.hpp>
#include <deip/protocol/config.hpp>

#include "../common/database_fixture.hpp"

#include <limits>

using namespace deip;
using namespace deip::chain;
using namespace deip::protocol;
using fc::string;

class dbs_reward_fixture : public clean_database_fixture
{
public:
    dbs_reward_fixture()
        : reward_service(db.obtain_service<dbs_reward>())
        , budget_service(db.obtain_service<dbs_budget>())
    {
    }

    dbs_reward& reward_service;
    dbs_budget& budget_service;

    const asset NULL_BALANCE = asset(0, DEIP_SYMBOL);
};

BOOST_FIXTURE_TEST_SUITE(reward_service, dbs_reward_fixture)

BOOST_AUTO_TEST_CASE(check_reward_pool_creation)
{
    try
    {
        BOOST_REQUIRE_NO_THROW(reward_service.get_pool());
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_double_creation)
{
    try
    {
        BOOST_REQUIRE_THROW(reward_service.create_pool(asset(10000, DEIP_SYMBOL)), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_initial_supply_distribution)
{
    try
    {
        const reward_pool_object& pool = reward_service.get_pool();

        asset total_reward_supply = pool.balance;
        total_reward_supply += budget_service.get_fund_budget().balance;

        BOOST_REQUIRE_EQUAL(total_reward_supply, TEST_REWARD_INITIAL_SUPPLY);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_initial_balance_is_not_null)
{
    try
    {
        const reward_pool_object& pool = reward_service.get_pool();

        BOOST_REQUIRE_GT(pool.balance, NULL_BALANCE);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_initial_per_block_reward_is_not_null)
{
    try
    {
        const reward_pool_object& pool = reward_service.get_pool();

        BOOST_REQUIRE_GT(pool.current_per_block_reward, NULL_BALANCE);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_initial_balancing)
{
    try
    {
        const reward_pool_object& pool = reward_service.get_pool();

        BOOST_REQUIRE_GE(pool.balance.amount,
                         pool.current_per_block_reward.amount * DEIP_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS
                             * DEIP_BLOCKS_PER_DAY);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_balance_increasing)
{
    try
    {
        const reward_pool_object& pool = reward_service.get_pool();

        asset current_balance = pool.balance;
        asset INCREASE_BALANCE(100, DEIP_SYMBOL);

        BOOST_REQUIRE_EQUAL(reward_service.increase_pool_ballance(INCREASE_BALANCE),
                            current_balance + INCREASE_BALANCE);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_per_block_reward_decreases_balance)
{
    try
    {
        const reward_pool_object& pool = reward_service.get_pool();

        asset initial_balance = pool.balance;
        asset block_reward = reward_service.take_block_reward();

        BOOST_REQUIRE_EQUAL(pool.balance + block_reward, initial_balance);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_automatic_reward_increasing)
{
    try
    {
        const reward_pool_object& pool = reward_service.get_pool();

        asset initial_per_block_reward = pool.current_per_block_reward;
        asset threshold_balance(pool.current_per_block_reward.amount * DEIP_REWARD_INCREASE_THRESHOLD_IN_DAYS
                                * DEIP_BLOCKS_PER_DAY);

        BOOST_REQUIRE_EQUAL(reward_service.increase_pool_ballance(threshold_balance + asset(1) - pool.balance),
                            threshold_balance + asset(1));

        asset current_per_block_reward = reward_service.take_block_reward();

        BOOST_REQUIRE_EQUAL(current_per_block_reward.amount,
                            initial_per_block_reward.amount * (100 + DEIP_ADJUST_REWARD_PERCENT) / 100);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_automatic_reward_decreasing)
{
    try
    {
        const reward_pool_object& pool = reward_service.get_pool();

        asset initial_per_block_reward = pool.current_per_block_reward;
        asset threshold_balance(pool.current_per_block_reward.amount * DEIP_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS
                                * DEIP_BLOCKS_PER_DAY);

        while (pool.balance >= threshold_balance)
        {
            reward_service.take_block_reward();
        }

        asset current_per_block_reward = reward_service.take_block_reward();

        BOOST_REQUIRE_EQUAL(current_per_block_reward.amount.value,
                            initial_per_block_reward.amount.value
                                - (initial_per_block_reward.amount.value * DEIP_ADJUST_REWARD_PERCENT) / 100);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

#endif