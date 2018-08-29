#include <deip/chain/services/dbs_vote.hpp>
#include <deip/chain/database/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_vote::dbs_vote(database &db)
        : _base_type(db)
{
}

dbs_vote::vote_refs_type dbs_vote::get_votes_by_discipline(const discipline_id_type& discipline_id) const
{
    vote_refs_type ret;

    auto it_pair = db_impl().get_index<vote_index>().indicies().get<by_discipline_id>().equal_range(discipline_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_vote::vote_refs_type dbs_vote::get_votes_by_voter(const account_name_type& voter) const
{
    vote_refs_type ret;

    auto it_pair = db_impl().get_index<vote_index>().indicies().get<by_voter>().equal_range(voter);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_vote::vote_refs_type dbs_vote::get_votes_by_research(const research_id_type &research_id) const
{
    vote_refs_type ret;

    auto it_pair = db_impl().get_index<vote_index>().indicies().get<by_research_id>().equal_range(research_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_vote::vote_refs_type dbs_vote::get_votes_by_research_content(const research_content_id_type &research_content_id) const
{
    vote_refs_type ret;

    auto it_pair = db_impl().get_index<vote_index>().indicies().get<by_research_content_id>().equal_range(research_content_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_vote::vote_refs_type dbs_vote::get_votes_by_research_and_discipline(const research_id_type &research_id,
                                                                        const discipline_id_type &discipline_id) const
{
    vote_refs_type ret;

    auto it_pair = db_impl().get_index<vote_index>().indicies().get<by_research_and_discipline>()
            .equal_range(std::make_tuple(research_id, discipline_id));
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_vote::vote_refs_type dbs_vote::get_votes_by_research_content_and_discipline(
        const research_content_id_type &research_content_id, const discipline_id_type &discipline_id) const
{
    vote_refs_type ret;

    auto it_pair = db_impl().get_index<vote_index>().indicies().get<by_content_and_discipline>()
            .equal_range(std::make_tuple(research_content_id, discipline_id));
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const vote_object& dbs_vote::create_vote(const discipline_id_type &discipline_id, const account_name_type &voter,
                                         const research_id_type &research_id,
                                         const research_content_id_type &research_content_id,
                                         const share_type &tokens_amount, const int16_t &weight,
                                         const uint16_t &voting_power, const time_point_sec &voting_time)
{
    FC_ASSERT(tokens_amount != 0, "Cannot vote with 0 tokens");
    FC_ASSERT(weight != 0, "Cannot vote with 0 weight");
    FC_ASSERT(voting_power > 0, "Cannot vote without voting power");
    FC_ASSERT(voting_time <= db_impl().head_block_time(), "Voting time cannot be in future");

    const auto& new_vote = db_impl().create<vote_object>([&](vote_object& v) {
        v.discipline_id = discipline_id;
        v.research_id = research_id;
        v.research_content_id = research_content_id;
        v.voter = voter;
        v.tokens_amount = tokens_amount;
        v.weight = weight;
        v.voting_power = voting_power;
        v.voting_time = voting_time;
    });

    return new_vote;
}

const total_votes_object& dbs_vote::create_total_votes(const discipline_id_type& discipline_id,
                                                       const research_id_type& research_id,
                                                       const research_content_id_type& research_content_id)
{
    const auto& new_total_votes = db_impl().create<total_votes_object>([&](total_votes_object& v_o) {
        v_o.discipline_id = discipline_id;
        v_o.research_id = research_id;
        v_o.research_content_id = research_content_id;
        v_o.total_weight = 0;
    });

    return new_total_votes;
}

const total_votes_object& dbs_vote::get_total_votes_by_content_and_discipline(const research_content_id_type& research_content_id,
                                                                                     const discipline_id_type& discipline_id) const
{
    try {
        return db_impl().get<total_votes_object, by_content_and_discipline>(
                boost::make_tuple(research_content_id, discipline_id));
    }
    FC_CAPTURE_AND_RETHROW((research_content_id)(discipline_id));
}

dbs_vote::total_votes_refs_type dbs_vote::get_total_votes_by_research_and_discipline(const research_id_type& research_id,
                                                                              const discipline_id_type& discipline_id) const
{
    total_votes_refs_type ret;

    auto it_pair = db_impl().get_index<total_votes_index>().indicies().get<by_research_and_discipline>().equal_range(
        std::make_tuple(research_id, discipline_id));
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}

dbs_vote::total_votes_refs_type dbs_vote::get_total_votes_by_research(const research_id_type& research_id) const
{
    total_votes_refs_type ret;

    auto it_pair = db_impl().get_index<total_votes_index>().indicies().get<by_research_id>().equal_range(research_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}

dbs_vote::total_votes_refs_type dbs_vote::get_total_votes_by_discipline(const discipline_id_type &discipline_id) const {
    total_votes_refs_type ret;

    auto it_pair = db_impl().get_index<total_votes_index>().indicies().get<by_discipline_id>().equal_range(discipline_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}

const total_votes_object& dbs_vote::update_total_votes(const total_votes_object& total_votes, const share_type total_weight)
{
    db_impl().modify(total_votes, [&](total_votes_object& tv_o) { tv_o.total_weight = total_weight; });
    return total_votes;
}

dbs_vote::total_votes_refs_type dbs_vote::get_total_votes_by_content(const research_content_id_type& research_content_id) const
{
    total_votes_refs_type ret;

    auto it_pair = db_impl().get_index<total_votes_index>().indicies().get<by_research_content_id>().equal_range(research_content_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}

const review_vote_object & dbs_vote::create_review_vote(const review_id_type &review_id, const discipline_id_type &discipline_id,
                                                        const account_name_type &voter, const int16_t &weight,
                                                        const time_point_sec &voting_time)
{
    FC_ASSERT(weight != 0, "Cannot vote with 0 weight");
    FC_ASSERT(voting_time <= db_impl().head_block_time(), "Voting time cannot be in future");

    const auto& vote = db_impl().create<review_vote_object>([&](review_vote_object& v) {
        v.discipline_id = discipline_id;
        v.review_id = review_id;
        v.voter = voter;
        v.weight = weight;
        v.voting_time = voting_time;
    });

    return vote;
}

dbs_vote::review_vote_refs_type dbs_vote::get_review_votes(const review_id_type &review_id) const
{
    review_vote_refs_type ret;

    auto it_pair = db_impl().get_index<review_vote_index>().indicies().get<by_review_id>().equal_range(review_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}

dbs_vote::review_vote_refs_type dbs_vote::get_review_votes_by_voter(const account_name_type& voter) const
{
    review_vote_refs_type ret;

    auto it_pair = db_impl().get_index<review_vote_index>().indicies().get<by_voter>().equal_range(voter);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}

dbs_vote::review_vote_refs_type dbs_vote::get_review_votes_by_review_and_discipline(const review_id_type &review_id,
                                                                                    const discipline_id_type &discipline_id) const
{
    review_vote_refs_type ret;

    auto it_pair = db_impl().get_index<review_vote_index>().indicies().get<by_review_and_discipline>()
            .equal_range(std::make_tuple(review_id, discipline_id));
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}

dbs_vote::review_vote_refs_type dbs_vote::get_review_votes_by_discipline(const discipline_id_type &discipline_id) const
{
    review_vote_refs_type ret;

    auto it_pair = db_impl().get_index<review_vote_index>().indicies().get<by_discipline_id>().equal_range(discipline_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}

bool dbs_vote::is_exists_by_research_and_discipline(const research_content_id_type &research_content_id,
                                                    const discipline_id_type &discipline_id)
{
    const auto& idx = db_impl().get_index<total_votes_index>().indices().get<by_content_and_discipline>();

    if (idx.find(boost::make_tuple(research_content_id, discipline_id)) != idx.cend())
        return true;
    else
        return false;
}

} //namespace chain
} //namespace deip