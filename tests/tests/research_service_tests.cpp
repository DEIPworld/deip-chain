#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/research_object.hpp>
#include <deip/chain/dbs_research.hpp>

#include "database_fixture.hpp"

#define RESEARCH_NAME "RESEARCH"
#define DISCIPLINE_MATH 10
#define DISCIPLINE_PHYSICS 20
#define RESEARCH_GROUP_ID 1
#define ABSTRACT "ABSTRACT"

namespace deip {
namespace chain {

class research_service_fixture : public clean_database_fixture
{
public:
    research_service_fixture()
            : data_service(db.obtain_service<dbs_research>())
    {
    }

    void create_researches() {
        db.create<research_object>([&](research_object& r) {
            r.id = 1;
            r.name = RESEARCH_NAME;
            r.permlink = RESEARCH_NAME;
            r.research_group_id = RESEARCH_GROUP_ID;
            r.percent_for_review = 10 * DEIP_1_PERCENT;
            r.is_finished = false;
            r.created = db.head_block_time();
            r.abstract = ABSTRACT;
            r.owned_tokens = DEIP_100_PERCENT;
        });
    }

    dbs_research& data_service;
};

BOOST_FIXTURE_TEST_SUITE(research_service, research_service_fixture)

BOOST_AUTO_TEST_CASE(create_research)
{
    try
    {
        auto& research = data_service.create(RESEARCH_NAME, ABSTRACT, RESEARCH_NAME, RESEARCH_GROUP_ID, 10 * DEIP_1_PERCENT);

        BOOST_CHECK(research.name == RESEARCH_NAME);
        BOOST_CHECK(research.permlink == RESEARCH_NAME);
        BOOST_CHECK(research.research_group_id == RESEARCH_GROUP_ID);
        BOOST_CHECK(research.percent_for_review == 10 * DEIP_1_PERCENT);
        BOOST_CHECK(research.is_finished == false);
        BOOST_CHECK(research.created <= db.head_block_time());
        BOOST_CHECK(research.abstract == ABSTRACT);
        BOOST_CHECK(research.owned_tokens == DEIP_100_PERCENT);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_researches)
{
    try
    {
        create_researches();

        const auto& researches = data_service.get_researches();

        BOOST_CHECK(researches.size() == 1);

        for (const research_object& research: researches) {

            BOOST_CHECK(research.name == RESEARCH_NAME);
            BOOST_CHECK(research.permlink == RESEARCH_NAME);
            BOOST_CHECK(research.research_group_id == RESEARCH_GROUP_ID);
            BOOST_CHECK(research.percent_for_review == 10 * DEIP_1_PERCENT);
            BOOST_CHECK(research.is_finished == false);
            BOOST_CHECK(research.created <= db.head_block_time());
            BOOST_CHECK(research.abstract == ABSTRACT);
            BOOST_CHECK(research.owned_tokens == DEIP_100_PERCENT);
        }
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research)
{
    try
    {
        create_researches();

        auto& research = data_service.get_research(1);

        BOOST_CHECK(research.id == 1);
        BOOST_CHECK(research.name == RESEARCH_NAME);
        BOOST_CHECK(research.permlink == RESEARCH_NAME);
        BOOST_CHECK(research.research_group_id == RESEARCH_GROUP_ID);
        BOOST_CHECK(research.percent_for_review == 10 * DEIP_1_PERCENT);
        BOOST_CHECK(research.is_finished == false);
        BOOST_CHECK(research.created <= db.head_block_time());
        BOOST_CHECK(research.abstract == ABSTRACT);
        BOOST_CHECK(research.owned_tokens == DEIP_100_PERCENT);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_by_permlink)
{
    try
    {
        create_researches();

        auto& research = data_service.get_research_by_permlink(RESEARCH_NAME);

        BOOST_CHECK(research.name == RESEARCH_NAME);
        BOOST_CHECK(research.permlink == RESEARCH_NAME);
        BOOST_CHECK(research.research_group_id == RESEARCH_GROUP_ID);
        BOOST_CHECK(research.percent_for_review == 10 * DEIP_1_PERCENT);
        BOOST_CHECK(research.is_finished == false);
        BOOST_CHECK(research.created <= db.head_block_time());
        BOOST_CHECK(research.abstract == ABSTRACT);
        BOOST_CHECK(research.owned_tokens == DEIP_100_PERCENT);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
