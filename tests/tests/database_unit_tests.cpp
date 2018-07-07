#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/database.hpp>
#include <deip/chain/research_token_object.hpp>
#include <deip/chain/util/reward.hpp>
#include <deip/chain/grant_objects.hpp>
#include <deip/chain/review_object.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

class database_unit_service_fixture : public clean_database_fixture
{
public:
    database_unit_service_fixture()
            : account_service(db.obtain_service<dbs_account>()),
              vote_service(db.obtain_service<dbs_vote>()),
              research_content_service(db.obtain_service<dbs_research_content>())
    {
    }

    void create_researches()
    {
        db.create<research_object>([&](research_object& d) {
            d.id = 1;
            d.research_group_id = 31;
            d.title = "name1";
            d.abstract = "abstract1";
            d.permlink = "permlink1";
            d.owned_tokens = 100 * DEIP_1_PERCENT;
            d.review_share_in_percent = 15 * DEIP_1_PERCENT;
        });

        db.create<research_object>([&](research_object& d) {
            d.id = 2;
            d.research_group_id = 32;
            d.title = "name2";
            d.abstract = "abstract2";
            d.permlink = "permlink2";
            d.owned_tokens = 50 * DEIP_1_PERCENT;
            d.review_share_in_percent = 15 * DEIP_1_PERCENT;
        });
    }

    void create_research_tokens()
    {
        db.create<research_token_object>([&](research_token_object& d) {
            d.id = 1;
            d.account_name = "alice";
            d.research_id = 2;
            d.amount = 20 * DEIP_1_PERCENT;
        });

        db.create<research_token_object>([&](research_token_object& d) {
            d.id = 2;
            d.account_name = "bob";
            d.research_id = 2;
            d.amount = 30 * DEIP_1_PERCENT;
        });
    }

    void create_research_groups()
    {
        db.create<research_group_object>([&](research_group_object& d) {
            d.id = 31;
            d.permlink = "permlink31";
        });

        db.create<research_group_object>([&](research_group_object& d) {
            d.id = 32;
            d.permlink = "permlink32";
        });
    }

    void create_research_group_tokens()
    {
        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 31;
            d.research_group_id = 31;
            d.owner = "alice";
            d.amount = 2000;
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 32;
            d.research_group_id = 31;
            d.owner = "alex";
            d.amount = 8000;
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 33;
            d.research_group_id = 32;
            d.owner = "alex";
            d.amount = 2000;
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 34;
            d.research_group_id = 32;
            d.owner = "jack";
            d.amount = 8000;
        });
    }

    void create_research_contents()
    {
        db.create<research_content_object>([&](research_content_object& d) {
            d.id = 1;
            d.research_id = 1;
            d.permlink = "milestone_research_1";
            d.type = milestone;
            d.authors = {"alice"};
            d.references.insert(2);
            d.activity_state = research_content_activity_state::active;
        });

        db.create<research_content_object>([&](research_content_object& d) {
            d.id = 2;
            d.research_id = 2;
            d.permlink = "milestone_research_2";
            d.type = milestone;
            d.authors = {"alex"};
            d.references.insert(1);
            d.activity_state = research_content_activity_state::active;
        });
    }

    void create_reviews()
    {
        db.create<review_object>([&](review_object& d) {
            bip::map<discipline_id_type, share_type> reward_weights_per_discipline;
            reward_weights_per_discipline[10] = 100;
            d.id = 1;
            d.research_content_id = 1;
            d.is_positive = true;
            d.author = "alice";
            d.weights_per_discipline.insert(reward_weights_per_discipline.begin(), reward_weights_per_discipline.end());
            d.expertise_amounts_used[10] = 50;
            d.weight_modifiers[10] = 1;
        });

        db.create<review_object>([&](review_object& d) {
            bip::map<discipline_id_type, share_type> reward_weights_per_discipline;
            reward_weights_per_discipline[10] = 100;
            d.id = 2;
            d.research_content_id = 1;
            d.is_positive = true;
            d.author = "bob";
            d.weights_per_discipline.insert(reward_weights_per_discipline.begin(), reward_weights_per_discipline.end());
            d.expertise_amounts_used[10] = 50;
            d.weight_modifiers[10] = 1;
        });
    }

    void create_votes()
    {
        db.create<vote_object>([&](vote_object& d) {
            d.id = 1;
            d.discipline_id = 10;
            d.voter = "bob";
            d.research_id = 1;
            d.research_content_id = 1;
            d.weight = 10;
        });

        db.create<vote_object>([&](vote_object& d) {
            d.id = 2;
            d.discipline_id = 10;
            d.voter = "john";
            d.research_id = 1;
            d.research_content_id = 1;
            d.weight = 30;
        });

        db.create<vote_object>([&](vote_object& d) {
            d.id = 3;
            d.discipline_id = 10;
            d.voter = "alice";
            d.research_id = 2;
            d.research_content_id = 2;
            d.weight = 60;
        });
    }

    void create_total_votes()
    {
        db.create<total_votes_object>([&](total_votes_object& d) {
            d.id = 1;
            d.discipline_id = 10;
            d.research_id = 1;
            d.research_content_id = 1;
            d.total_weight = 100;
        });

        db.create<total_votes_object>([&](total_votes_object& d) {
            d.id = 2;
            d.discipline_id = 10;
            d.research_id = 2;
            d.research_content_id = 2;
            d.total_weight = 100;
        });
    }

    void create_review_votes()
    {
        db.create<review_vote_object>([&](review_vote_object& d) {
            d.id = 1;
            d.discipline_id = 10;
            d.voter = "jack";
            d.review_id = 1;
            d.weight = 40;
        });

        db.create<review_vote_object>([&](review_vote_object& d) {
            d.id = 2;
            d.discipline_id = 10;
            d.voter = "john";
            d.review_id = 1;
            d.weight = 60;
        });

        db.create<review_vote_object>([&](review_vote_object& d) {
            d.id = 3;
            d.discipline_id = 10;
            d.voter = "jack";
            d.review_id = 2;
            d.weight = 40;
        });

        db.create<review_vote_object>([&](review_vote_object& d) {
            d.id = 4;
            d.discipline_id = 10;
            d.voter = "john";
            d.review_id = 2;
            d.weight = 60;
        });
    }

    void create_discipline_with_weight()
    {
        dbs_discipline& discipline_service = db.obtain_service<dbs_discipline>();

        db.create<discipline_object>([&](discipline_object& d) {
            d.id = discipline_service.get_disciplines().size();
            d.parent_id = 1;
            d.name = "Test Discipline With Weight";
            d.total_active_weight = 200;
        });
    }

    void create_grant()
    {
        db.create<grant_object>([&](grant_object& d) {
            d.id = 1;
            d.owner = "bob";
            d.target_discipline = db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").id;
            d.balance = asset(100, DEIP_SYMBOL);
            d.per_block = 100;
            d.start_block = int(db.head_block_num());
            d.end_block = int(db.head_block_num());
        });
    }

    void create_grant_test_case()
    {
        dbs_discipline& discipline_service = db.obtain_service<dbs_discipline>();

        db.create<discipline_object>([&](discipline_object& d) {
            d.id = discipline_service.get_disciplines().size();
            d.parent_id = 1;
            d.name = "Test Discipline For Grant With Weight";
            d.total_active_weight = 150;
        });

        db.create<total_votes_object>([&](total_votes_object& d) {
            d.id = 3;
            d.discipline_id = db.get<discipline_object, by_discipline_name>("Test Discipline For Grant With Weight").id;
            d.research_id = 3;
            d.research_content_id = 3;
            d.total_weight = 50;
        });

        db.create<research_content_object>([&](research_content_object& d) {
            d.id = 3;
            d.research_id = 2;
            d.type = final_result;
            d.authors = {"jack"};
            d.references.insert(1);
        });
    }

    dbs_account& account_service;
    dbs_vote& vote_service;
    dbs_research_content& research_content_service;
};

BOOST_FIXTURE_TEST_SUITE(database_unit_service, database_unit_service_fixture)

//BOOST_AUTO_TEST_CASE(reward_voters)
//{
//    try
//    {
//        ACTORS((bob)(john));
//        create_discipline_with_weight();
//        fund("bob", 100);
//        fund("john", 100);
//
//        share_type reward = 40;
//
//        create_votes();
//        create_total_votes();
//
//        BOOST_CHECK_NO_THROW(db.reward_voters(1, db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").id, reward));
//
//        BOOST_CHECK(db.get_account("bob").balance.amount == 110);
//        BOOST_CHECK(db.get_account("john").balance.amount == 130);
//
//    }
//    FC_LOG_AND_RETHROW()
//}

BOOST_AUTO_TEST_CASE(reward_review_voters)
{
    try
    {
        ACTORS((alice)(bob));
        create_discipline_with_weight();
        create_review_votes();
        create_reviews();
        fund("alice", 100);
        fund("bob", 100);

        share_type reward = 100;
        auto& review = db.get<review_object, by_id>(1);

        BOOST_CHECK_NO_THROW(db.reward_review_voters(review, 10, reward));

        BOOST_CHECK(db.get_account("alice").balance.amount == 140);
        BOOST_CHECK(db.get_account("bob").balance.amount == 160);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reward_with_expertise)
{
    try
    {
        ACTOR(alice);
        create_discipline_with_weight();

        share_type reward = 30;

        BOOST_CHECK_NO_THROW(db.reward_account_with_expertise("alice", 10, reward));

        auto alice_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alice", 10));
        BOOST_CHECK(alice_expert_token.amount == reward);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reward_reviews)
{
    try
    {
        ACTORS((alice)(alex)(jack)(bob)(john));

        create_discipline_with_weight();
        create_research_contents();
        create_researches();
        create_total_votes();
        create_research_tokens();
        create_research_groups();
        create_research_group_tokens();
        create_reviews();
        create_review_votes();

        share_type reward = 1000;
        share_type expertise_reward = 1000;

        BOOST_CHECK_NO_THROW(db.reward_reviews(1, 10, reward, expertise_reward));

        BOOST_CHECK(db.get_account("alice").balance.amount == 495);
        BOOST_CHECK(db.get_account("bob").balance.amount == 505);

        BOOST_CHECK(db.obtain_service<dbs_expert_token>().get_expert_token_by_account_and_discipline("alice", 10).amount == 500);
        BOOST_CHECK(db.obtain_service<dbs_expert_token>().get_expert_token_by_account_and_discipline("bob", 10).amount == 500);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reward_references)
{
    try
    {
        ACTORS((alice)(alex)(jack)(bob)(john));

        create_discipline_with_weight();
        create_research_contents();
        create_researches();
        create_total_votes();
        create_research_tokens();
        create_research_groups();
        create_research_group_tokens();

        share_type reward = 1000;

        BOOST_CHECK_NO_THROW(db.reward_references(1, 10, reward, reward));

        BOOST_CHECK(db.get<research_group_object>(32).balance.amount == 500);

        auto alex_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alex", 10));

        BOOST_CHECK(alex_expert_token.amount == 1000);

        BOOST_CHECK(db.get_account("alice").balance.amount == 200);
        BOOST_CHECK(db.get_account("bob").balance.amount == 300);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reward_research_token_holders)
{
    try
    {
        ACTORS((alice)(alex)(jack)(bob)(john));

        create_discipline_with_weight();
        create_researches();
        create_research_groups();
        create_research_group_tokens();
        create_research_tokens();

        share_type reward = 1000;

        BOOST_CHECK_NO_THROW(db.reward_research_token_holders(db.get<research_object>(2), 1, reward));

        BOOST_CHECK(db.get<research_group_object>(32).balance.amount == 500);

        BOOST_CHECK(db.get_account("alice").balance.amount == 200);
        BOOST_CHECK(db.get_account("bob").balance.amount == 300);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reward_research_content)
{
    try
    {
        ACTORS((alice)(alex)(jack)(bob)(john));

        create_discipline_with_weight();
        create_research_contents();
        create_researches();
        create_total_votes();
        create_research_tokens();
        create_research_groups();
        create_research_group_tokens();

        share_type reward = 1000;
        BOOST_CHECK_NO_THROW(db.reward_research_content(1, 10, reward));

        BOOST_CHECK(db.get<research_group_object>(31).balance.amount == 750);
        BOOST_CHECK(db.get<research_group_object>(32).balance.amount == 50);

        auto alice_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alice", 10));

        BOOST_CHECK(alice_expert_token.amount == 800);


        BOOST_CHECK(alice.balance.amount == 20);
        BOOST_CHECK(bob.balance.amount == 30);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(fund_review_pool)
{
    try
    {
        ACTORS((alice)(alex)(jack)(bob)(john));

        create_discipline_with_weight();
        create_research_contents();
        create_researches();
        create_total_votes();
        create_research_tokens();
        create_research_groups();
        create_research_group_tokens();
        create_reviews();
        create_review_votes();

        share_type reward = 1000;

        auto& discipline = db.get<discipline_object>(10);
        BOOST_CHECK_NO_THROW(db.fund_review_pool(discipline, reward));

        auto& group_1 = db.get<research_group_object>(31);
        auto& group_2 = db.get<research_group_object>(32);

        BOOST_CHECK(alice.balance.amount == 475);
        BOOST_CHECK(bob.balance.amount == 475);
        BOOST_CHECK(jack.balance.amount == 20);
        BOOST_CHECK(john.balance.amount == 30);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reward_researches_in_discipline)
{
    try
    {
        ACTORS((alice)(alex)(jack)(bob)(john));

        create_discipline_with_weight();
        create_research_contents();
        create_researches();
        create_total_votes();
        create_research_tokens();
        create_research_groups();
        create_research_group_tokens();
        create_reviews();
        create_review_votes();

        share_type reward = 1000;

        auto& discipline = db.get<discipline_object, by_discipline_name>("Test Discipline With Weight");

        share_type used_reward = 0;
        BOOST_CHECK_NO_THROW(used_reward = db.reward_researches_in_discipline(discipline, reward));

        auto& group_1 = db.get<research_group_object>(31);
        auto& group_2 = db.get<research_group_object>(32);

        BOOST_CHECK(group_1.balance.amount == 180);
        BOOST_CHECK(group_2.balance.amount == 122);

        BOOST_CHECK(alice.balance.amount == 11300);
        BOOST_CHECK(bob.balance.amount == 15575);
        BOOST_CHECK(john.balance.amount == 2625);

        auto alice_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alice", 10));
        auto alex_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alex", 10));

        BOOST_CHECK(alice_expert_token.amount == 61700);
        BOOST_CHECK(alex_expert_token.amount == 27500);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(distribute_reward)
{
    try
    {
        ACTORS((alice)(alex)(jack)(bob)(john));

        create_discipline_with_weight();
        create_research_contents();
        create_reviews();
        create_researches();
        create_votes();
        create_total_votes();
        create_research_tokens();
        create_research_groups();
        create_research_group_tokens();

        share_type reward = 10000000;

        BOOST_CHECK_NO_THROW(db.distribute_reward(reward));

        BOOST_CHECK(db.get<research_group_object>(31).balance.amount == 4940000);
        BOOST_CHECK(db.get<research_group_object>(32).balance.amount == 1330000);

        BOOST_CHECK(db.get_account("alice").balance.amount == 1053550);
        BOOST_CHECK(db.get_account("bob").balance.amount == 1449700);
        BOOST_CHECK(db.get_account("john").balance.amount == 249375);

        auto john_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alice", 10));
        auto alex_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alex", 10));

        BOOST_CHECK(john_expert_token.amount == 5861500);
        BOOST_CHECK(alex_expert_token.amount == 2612500);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(grant_researches_in_discipline)
{
    try
    {
        ACTORS((alice)(alex)(jack)(bob)(john));

        create_research_contents();
        create_researches();
        create_votes();
        create_total_votes();
        create_research_groups();
        create_research_group_tokens();
        create_grant_test_case();

        share_type grant = 100000;

        BOOST_CHECK_NO_THROW(db.grant_researches_in_discipline(db.get<discipline_object, by_discipline_name>("Test Discipline For Grant With Weight").id, grant));

        BOOST_CHECK(db.get<research_group_object>(31).balance.amount == util::calculate_share(grant, db.get<total_votes_object>(1).total_weight, db.get<discipline_object, by_discipline_name>("Test Discipline For Grant With Weight").total_active_weight - db.get<total_votes_object>(3).total_weight));
        BOOST_CHECK(db.get<research_group_object>(32).balance.amount == util::calculate_share(grant, db.get<total_votes_object>(2).total_weight, db.get<discipline_object, by_discipline_name>("Test Discipline For Grant With Weight").total_active_weight - db.get<total_votes_object>(3).total_weight));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(process_grants)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: process_grants");

       ACTORS((alice)(alex)(jack)(bob)(john));

       generate_block();

       create_discipline_with_weight();
       create_research_contents();
       create_researches();
       create_votes();
       create_total_votes();
       create_research_groups();
       create_research_group_tokens();
       create_grant();

       BOOST_CHECK_NO_THROW(db.process_grants());

       BOOST_CHECK(db.get<research_group_object>(31).balance.amount == util::calculate_share(100, db.get<total_votes_object>(1).total_weight, db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").total_active_weight));
       BOOST_CHECK(db.get<research_group_object>(32).balance.amount == util::calculate_share(100, db.get<total_votes_object>(2).total_weight, db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").total_active_weight));

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
