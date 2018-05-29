#include <deip/chain/deip_evaluator.hpp>
#include <deip/chain/deip_objects.hpp>
#include <deip/chain/witness_objects.hpp>
#include <deip/chain/block_summary_object.hpp>

#include <deip/chain/util/reward.hpp>

#include <deip/chain/database.hpp> //replace to dbservice after _temporary_public_impl remove
#include <deip/chain/dbs_account.hpp>
#include <deip/chain/dbs_witness.hpp>
#include <deip/chain/dbs_grant.hpp>
#include <deip/chain/dbs_discipline.hpp>
#include <deip/chain/dbs_research.hpp>
#include <deip/chain/dbs_research_content.hpp>
#include <deip/chain/dbs_research_discipline_relation.hpp>
#include <deip/chain/dbs_proposal.hpp>
#include <deip/chain/dbs_research_group.hpp>
#include <deip/chain/dbs_research_token_sale.hpp>
#include <deip/chain/dbs_vote.hpp>
#include <deip/chain/dbs_expert_token.hpp>
#include <deip/chain/dbs_research_group_invite.hpp>
#include <deip/chain/dbs_research_token.hpp>
#include <deip/chain/dbs_review.hpp>
#include <deip/chain/dbs_research_group_join_request.hpp>
#include <deip/chain/dbs_vesting_contract.hpp>
#include <deip/chain/dbs_proposal_execution.hpp>

#ifndef IS_LOW_MEM
#include <diff_match_patch.h>
#include <boost/locale/encoding_utf.hpp>

using boost::locale::conv::utf_to_utf;

std::wstring utf8_to_wstring(const std::string& str)
{
    return utf_to_utf<wchar_t>(str.c_str(), str.c_str() + str.size());
}

std::string wstring_to_utf8(const std::wstring& str)
{
    return utf_to_utf<char>(str.c_str(), str.c_str() + str.size());
}

#endif

#include <fc/uint128.hpp>
#include <fc/utf8.hpp>

#include <limits>

namespace deip {
namespace chain {
using fc::uint128_t;

inline void validate_permlink_0_1(const string& permlink)
{
    FC_ASSERT(permlink.size() > DEIP_MIN_PERMLINK_LENGTH && permlink.size() < DEIP_MAX_PERMLINK_LENGTH,
              "Permlink is not a valid size.");

    for (auto ch : permlink)
    {
        if (!std::islower(ch) && !std::isdigit(ch) && !(ch == '-'))
        {
            FC_ASSERT(false, "Invalid permlink character: ${ch}", ("ch", std::string(1, ch)));
        }
    }
}

struct strcmp_equal
{
    bool operator()(const fc::shared_string& a, const string& b)
    {
        return a.size() == b.size() || std::strcmp(a.c_str(), b.c_str()) == 0;
    }
};

void witness_update_evaluator::do_apply(const witness_update_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    account_service.check_account_existence(o.owner);

    FC_ASSERT(o.url.size() <= DEIP_MAX_WITNESS_URL_LENGTH, "URL is too long");
    FC_ASSERT(o.props.account_creation_fee.symbol == DEIP_SYMBOL);

    const auto& by_witness_name_idx = _db._temporary_public_impl().get_index<witness_index>().indices().get<by_name>();
    auto wit_itr = by_witness_name_idx.find(o.owner);
    if (wit_itr != by_witness_name_idx.end())
    {
        _db._temporary_public_impl().modify(*wit_itr, [&](witness_object& w) {
            fc::from_string(w.url, o.url);
            w.signing_key = o.block_signing_key;
            w.props = o.props;
        });
    }
    else
    {
        _db._temporary_public_impl().create<witness_object>([&](witness_object& w) {
            w.owner = o.owner;
            fc::from_string(w.url, o.url);
            w.signing_key = o.block_signing_key;
            w.created = _db.head_block_time();
            w.props = o.props;
            w.hardfork_time_vote = _db.get_genesis_time();
        });
    }
}

void account_create_evaluator::do_apply(const account_create_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();

    const auto& creator = account_service.get_account(o.creator);

    // check creator balance

    FC_ASSERT(creator.balance >= o.fee, "Insufficient balance to create account.",
              ("creator.balance", creator.balance)("required", o.fee));

    // check fee

    const witness_schedule_object& wso = _db.get_witness_schedule_object();
    FC_ASSERT(o.fee >= asset(wso.median_props.account_creation_fee.amount * DEIP_CREATE_ACCOUNT_WITH_DEIP_MODIFIER,
                             DEIP_SYMBOL),
              "Insufficient Fee: ${f} required, ${p} provided.",
              ("f",
               wso.median_props.account_creation_fee
                   * asset(DEIP_CREATE_ACCOUNT_WITH_DEIP_MODIFIER, DEIP_SYMBOL))("p", o.fee));

    // check accounts existence

    account_service.check_account_existence(o.owner.account_auths);

    account_service.check_account_existence(o.active.account_auths);

    account_service.check_account_existence(o.posting.account_auths);

    // write in to DB

    account_service.create_account_by_faucets(o.new_account_name, o.creator, o.memo_key, o.json_metadata, o.owner,
                                              o.active, o.posting, o.fee);

    bool is_personal = true;

    auto& personal_research_group = research_group_service.create_research_group(o.new_account_name,
                                                                                 o.new_account_name,
                                                                                 o.new_account_name,
                                                                                 DEIP_100_PERCENT,
                                                                                 is_personal);
    research_group_service.create_research_group_token(personal_research_group.id, DEIP_100_PERCENT, o.new_account_name);

}

void account_create_with_delegation_evaluator::do_apply(const account_create_with_delegation_operation& o)
{
    const auto& props = _db.get_dynamic_global_properties();

    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();

    const auto& creator = account_service.get_account(o.creator);

    const witness_schedule_object& wso = _db.get_witness_schedule_object();

    // check creator balance

    FC_ASSERT(creator.balance >= o.fee, "Insufficient balance to create account.",
              ("creator.balance", creator.balance)("required", o.fee));

    // check delegation fee

    FC_ASSERT(creator.vesting_shares - creator.delegated_vesting_shares
                      - asset(creator.to_withdraw - creator.withdrawn, VESTS_SYMBOL)
                  >= o.delegation,
              "Insufficient vesting shares to delegate to new account.",
              ("creator.vesting_shares", creator.vesting_shares)(
                  "creator.delegated_vesting_shares", creator.delegated_vesting_shares)("required", o.delegation));

    auto target_delegation
        = asset(wso.median_props.account_creation_fee.amount * DEIP_CREATE_ACCOUNT_WITH_DEIP_MODIFIER
                    * DEIP_CREATE_ACCOUNT_DELEGATION_RATIO,
                DEIP_SYMBOL)
        * props.get_vesting_share_price();

    auto current_delegation
        = asset(o.fee.amount * DEIP_CREATE_ACCOUNT_DELEGATION_RATIO, DEIP_SYMBOL) * props.get_vesting_share_price()
        + o.delegation;

    FC_ASSERT(current_delegation >= target_delegation, "Inssufficient Delegation ${f} required, ${p} provided.",
              ("f", target_delegation)("p", current_delegation)(
                  "account_creation_fee", wso.median_props.account_creation_fee)("o.fee", o.fee)("o.delegation",
                                                                                                 o.delegation));

    FC_ASSERT(o.fee >= wso.median_props.account_creation_fee, "Insufficient Fee: ${f} required, ${p} provided.",
              ("f", wso.median_props.account_creation_fee)("p", o.fee));

    // check accounts existence

    account_service.check_account_existence(o.owner.account_auths);

    account_service.check_account_existence(o.active.account_auths);

    account_service.check_account_existence(o.posting.account_auths);

    account_service.create_account_with_delegation(o.new_account_name, o.creator, o.memo_key, o.json_metadata, o.owner,
                                                   o.active, o.posting, o.fee, o.delegation);


    bool is_personal = true;

    auto& personal_research_group = research_group_service.create_research_group(o.new_account_name,
                                                                                 o.new_account_name,
                                                                                 o.new_account_name,
                                                                                 DEIP_100_PERCENT,
                                                                                 is_personal);
    research_group_service.create_research_group_token(personal_research_group.id, DEIP_100_PERCENT, o.new_account_name);
}

void account_update_evaluator::do_apply(const account_update_operation& o)
{
    if (o.posting)
        o.posting->validate();

    dbs_account& account_service = _db.obtain_service<dbs_account>();

    const auto& account = account_service.get_account(o.account);
    const auto& account_auth = account_service.get_account_authority(o.account);

    if (o.owner)
    {
#ifndef IS_TEST_NET
        FC_ASSERT(_db.head_block_time() - account_auth.last_owner_update > DEIP_OWNER_UPDATE_LIMIT,
                  "Owner authority can only be updated once an hour.");
#endif
        account_service.check_account_existence(o.owner->account_auths);

        account_service.update_owner_authority(account, *o.owner);
    }

    if (o.active)
    {
        account_service.check_account_existence(o.active->account_auths);
    }

    if (o.posting)
    {
        account_service.check_account_existence(o.posting->account_auths);
    }

    account_service.update_acount(account, account_auth, o.memo_key, o.json_metadata, o.owner, o.active, o.posting);
}

/**
 *  Because net_rshares is 0 there is no need to update any pending payout calculations or parent posts.
 */

void transfer_evaluator::do_apply(const transfer_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    const auto& from_account = account_service.get_account(o.from);
    const auto& to_account = account_service.get_account(o.to);

    FC_ASSERT(_db.get_balance(from_account, o.amount.symbol) >= o.amount,
              "Account does not have sufficient funds for transfer.");
    account_service.decrease_balance(from_account, o.amount);
    account_service.increase_balance(to_account, o.amount);
}

void transfer_to_vesting_evaluator::do_apply(const transfer_to_vesting_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    const auto& from_account = account_service.get_account(o.from);
    const auto& to_account = o.to.size() ? account_service.get_account(o.to) : from_account;

    FC_ASSERT(_db.get_balance(from_account, DEIP_SYMBOL) >= o.amount,
              "Account does not have sufficient DEIP for transfer.");
    account_service.decrease_balance(from_account, o.amount);
    account_service.create_vesting(to_account, o.amount);
}

void withdraw_vesting_evaluator::do_apply(const withdraw_vesting_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    const auto& account = account_service.get_account(o.account);

    FC_ASSERT(account.vesting_shares >= asset(0, VESTS_SYMBOL),
              "Account does not have sufficient Deip Power for withdraw.");
    FC_ASSERT(account.vesting_shares - account.delegated_vesting_shares >= o.vesting_shares,
              "Account does not have sufficient Deip Power for withdraw.");

    if (!account.mined)
    {
        const auto& props = _db.get_dynamic_global_properties();
        const witness_schedule_object& wso = _db.get_witness_schedule_object();

        asset min_vests = wso.median_props.account_creation_fee * props.get_vesting_share_price();
        min_vests.amount.value *= 10;

        FC_ASSERT(
            account.vesting_shares > min_vests || o.vesting_shares.amount == 0,
            "Account registered by another account requires 10x account creation fee worth of Deip Power before it "
            "can be powered down.");
    }

    if (o.vesting_shares.amount == 0)
    {
        FC_ASSERT(account.vesting_withdraw_rate.amount != 0,
                  "This operation would not change the vesting withdraw rate.");

        account_service.update_withdraw(account, asset(0, VESTS_SYMBOL), time_point_sec::maximum(), 0);
    }
    else
    {

        // DEIP: We have to decide wether we use 13 weeks vesting period or low it down
        int vesting_withdraw_intervals = DEIP_VESTING_WITHDRAW_INTERVALS; /// 13 weeks = 1 quarter of a year

        auto new_vesting_withdraw_rate = asset(o.vesting_shares.amount / vesting_withdraw_intervals, VESTS_SYMBOL);

        if (new_vesting_withdraw_rate.amount == 0)
            new_vesting_withdraw_rate.amount = 1;

        FC_ASSERT(account.vesting_withdraw_rate != new_vesting_withdraw_rate,
                  "This operation would not change the vesting withdraw rate.");

        account_service.update_withdraw(account, new_vesting_withdraw_rate,
                                        _db.head_block_time() + fc::seconds(DEIP_VESTING_WITHDRAW_INTERVAL_SECONDS),
                                        o.vesting_shares.amount);
    }
}

void set_withdraw_vesting_route_evaluator::do_apply(const set_withdraw_vesting_route_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    try
    {
        const auto& from_account = account_service.get_account(o.from_account);
        const auto& to_account = account_service.get_account(o.to_account);
        const auto& wd_idx
            = _db._temporary_public_impl().get_index<withdraw_vesting_route_index>().indices().get<by_withdraw_route>();
        auto itr = wd_idx.find(boost::make_tuple(from_account.id, to_account.id));

        if (itr == wd_idx.end())
        {
            FC_ASSERT(o.percent != 0, "Cannot create a 0% destination.");
            FC_ASSERT(from_account.withdraw_routes < DEIP_MAX_WITHDRAW_ROUTES,
                      "Account already has the maximum number of routes.");

            _db._temporary_public_impl().create<withdraw_vesting_route_object>(
                [&](withdraw_vesting_route_object& wvdo) {
                    wvdo.from_account = from_account.id;
                    wvdo.to_account = to_account.id;
                    wvdo.percent = o.percent;
                    wvdo.auto_vest = o.auto_vest;
                });

            account_service.increase_withdraw_routes(from_account);
        }
        else if (o.percent == 0)
        {
            _db._temporary_public_impl().remove(*itr);

            account_service.decrease_withdraw_routes(from_account);
        }
        else
        {
            _db._temporary_public_impl().modify(*itr, [&](withdraw_vesting_route_object& wvdo) {
                wvdo.from_account = from_account.id;
                wvdo.to_account = to_account.id;
                wvdo.percent = o.percent;
                wvdo.auto_vest = o.auto_vest;
            });
        }

        itr = wd_idx.upper_bound(boost::make_tuple(from_account.id, account_id_type()));
        uint16_t total_percent = 0;

        while (itr->from_account == from_account.id && itr != wd_idx.end())
        {
            total_percent += itr->percent;
            ++itr;
        }

        FC_ASSERT(total_percent <= DEIP_100_PERCENT,
                  "More than 100% of vesting withdrawals allocated to destinations.");
    }
    FC_CAPTURE_AND_RETHROW()
}

void account_witness_proxy_evaluator::do_apply(const account_witness_proxy_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    const auto& account = account_service.get_account(o.account);
    FC_ASSERT(account.proxy != o.proxy, "Proxy must change.");

    FC_ASSERT(account.can_vote, "Account has declined the ability to vote and cannot proxy votes.");

    optional<account_object> proxy;
    if (o.proxy.size())
    {
        proxy = account_service.get_account(o.proxy);
    }
    account_service.update_voting_proxy(account, proxy);
}

void account_witness_vote_evaluator::do_apply(const account_witness_vote_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_witness& witness_service = _db.obtain_service<dbs_witness>();

    const auto& voter = account_service.get_account(o.account);
    FC_ASSERT(voter.proxy.size() == 0, "A proxy is currently set, please clear the proxy before voting for a witness.");

    if (o.approve)
        FC_ASSERT(voter.can_vote, "Account has declined its voting rights.");

    const auto& witness = _db.get_witness(o.witness);

    const auto& by_account_witness_idx
        = _db._temporary_public_impl().get_index<witness_vote_index>().indices().get<by_account_witness>();
    auto itr = by_account_witness_idx.find(boost::make_tuple(voter.id, witness.id));

    auto expert_tokens_by_account =
            _db._temporary_public_impl().get_index<expert_token_index>().indices().get<by_account_name>().equal_range(voter.name);

    auto it = expert_tokens_by_account.first;
    const auto it_end = expert_tokens_by_account.second;

    share_type total_vote_weight;
    while (it != it_end)
    {
        total_vote_weight += it->amount;
        ++it;
    }

    if (itr == by_account_witness_idx.end())
    {
        FC_ASSERT(o.approve, "Vote doesn't exist, user must indicate a desire to approve witness.");

        FC_ASSERT(voter.witnesses_voted_for < DEIP_MAX_ACCOUNT_WITNESS_VOTES,
                  "Account has voted for too many witnesses."); // TODO: Remove after hardfork 2

        _db._temporary_public_impl().create<witness_vote_object>([&](witness_vote_object& v) {
            v.witness = witness.id;
            v.account = voter.id;
        });

        witness_service.adjust_witness_vote(witness, total_vote_weight);

        account_service.increase_witnesses_voted_for(voter);
    }
    else
    {
        FC_ASSERT(!o.approve, "Vote currently exists, user must indicate a desire to reject witness.");

        witness_service.adjust_witness_vote(witness, -total_vote_weight);

        account_service.decrease_witnesses_voted_for(voter);
        _db._temporary_public_impl().remove(*itr);
    }
}

void vote_evaluator::do_apply(const vote_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_vote& vote_service = _db.obtain_service<dbs_vote>();
    dbs_expert_token& expert_token_service = _db.obtain_service<dbs_expert_token>();
    dbs_discipline& discipline_service = _db.obtain_service<dbs_discipline>();
    dbs_research& research_service = _db.obtain_service<dbs_research>();
    dbs_research_content& research_content_service = _db.obtain_service<dbs_research_content>();

    const auto& dgpo = _db.get_dynamic_global_properties();

    const auto research_reward_curve = curve_id::power1dot5;
    const auto curators_reward_curve = curve_id::power1dot5;

    try
    {
        const auto& voter = account_service.get_account(o.voter);

        FC_ASSERT(voter.can_vote, "Voter has declined their voting rights.");

        research_service.check_research_existence(o.research_id);

        if (o.discipline_id != 0) {
            discipline_service.check_discipline_existence(o.discipline_id);
        }

        expert_token_service.check_expert_token_existence_by_account_and_discipline(o.voter, o.discipline_id);

        const auto& content = research_content_service.get_content_by_id(o.research_content_id);

        std::vector<discipline_id_type> target_disciplines;

        dbs_research_discipline_relation& research_disciplines_service = _db.obtain_service<dbs_research_discipline_relation>();
        const auto& relations = research_disciplines_service.get_research_discipline_relations_by_research(o.research_id);
        for (auto& relation_wrapper : relations) {
            const auto& relation = relation_wrapper.get();
            target_disciplines.push_back(relation.discipline_id);
        }

        const auto& token = expert_token_service.get_expert_token_by_account_and_discipline(o.voter, o.discipline_id);

        // Validate that research has discipline we are trying to vote with
        if (o.discipline_id != 0)
        {
            const auto& itr = std::find(target_disciplines.begin(), target_disciplines.end(), o.discipline_id);
            const bool discipline_found = (itr != target_disciplines.end());
            FC_ASSERT(discipline_found, "Cannot vote with {d} token as research is not in this discipline",
                      ("d", discipline_service.get_discipline(o.discipline_id).name));
        }

        const auto& vote_idx = _db._temporary_public_impl().get_index<vote_index>().indices().get<by_voter_discipline_and_content>();
        const auto& itr = vote_idx.find(std::make_tuple(voter.name, o.discipline_id, o.research_content_id));

        FC_ASSERT(itr == vote_idx.end(), "You have already voted for this research content");

        const auto& total_votes_idx = _db._temporary_public_impl().get_index<total_votes_index>().indices().get<by_content_and_discipline>();
        const auto& total_votes_itr = total_votes_idx.find(std::make_tuple(o.research_content_id, o.discipline_id));

        // Create total_votes_object if it does not exist yet
        if (total_votes_itr == total_votes_idx.end())
        {
            vote_service.create_total_votes(o.discipline_id, o.research_id, o.research_content_id);
        }

        const int64_t elapsed_seconds   = (_db.head_block_time() - token.last_vote_time).to_seconds();

        const int64_t regenerated_power = (DEIP_100_PERCENT * elapsed_seconds) / DEIP_VOTE_REGENERATION_SECONDS;
        const int64_t current_power = std::min(int64_t(token.voting_power + regenerated_power), int64_t(DEIP_100_PERCENT));
        FC_ASSERT(current_power > 0, "Account currently does not have voting power.");

        const int64_t abs_weight = abs(o.weight);
        const int64_t used_power = (current_power * abs_weight) / DEIP_100_PERCENT;
        const int64_t denominated_used_power = used_power / 10;

        FC_ASSERT(used_power <= current_power, "Account does not have enough power to vote.");

        const uint64_t abs_used_tokens
            = ((uint128_t(token.amount.value) * used_power) / (DEIP_100_PERCENT)).to_uint64();

        const auto& tvo = vote_service
                .get_total_votes_by_content_and_discipline(o.research_content_id, o.discipline_id);

        const bool content_is_active = content.activity_state == research_content_activity_state::active;

        FC_ASSERT(o.weight != 0, "Vote weight cannot be 0.");

        _db._temporary_public_impl().modify(token, [&](expert_token_object& t) {
            t.voting_power = current_power - denominated_used_power;
            t.last_vote_time = _db.head_block_time();
        });

        FC_ASSERT(abs_used_tokens > 0, "Cannot vote with 0 rshares.");

        const auto current_weight = tvo.total_weight;
        const uint64_t evaluated_research_rshares = util::evaluate_reward_curve(abs_used_tokens, research_reward_curve).to_uint64();

        _db._temporary_public_impl().modify(tvo, [&](total_votes_object& t) {
            t.total_weight += abs_used_tokens;
            if (content_is_active) {
                t.total_active_weight += abs_used_tokens;
            }

            t.total_research_reward_weight += evaluated_research_rshares;
            if (content_is_active) {
                t.total_active_research_reward_weight += evaluated_research_rshares;
            }
        });

        if (content_is_active) {
            _db._temporary_public_impl().modify(dgpo, [&](dynamic_global_property_object& prop) {
                prop.total_active_disciplines_reward_weight += abs_used_tokens;
            });
        }

        const auto& discipline = discipline_service.get_discipline(o.discipline_id);
        _db._temporary_public_impl().modify(discipline, [&](discipline_object& d) {

            d.total_active_research_reward_weight += evaluated_research_rshares;

            if (content_is_active) {
                d.total_active_reward_weight += abs_used_tokens;
            }
        });

        uint64_t max_vote_weight = 0;

        /** this verifies uniqueness of voter
         *
         *  cv.weight / c.total_vote_weight ==> % of rshares increase that is accounted for by the vote
         *
         *  W(R) = B * R / ( R + 2S )
         *  W(R) is bounded above by B. B is fixed at 2^64 - 1, so all weights fit in a 64 bit integer.
         *
         *  The equation for an individual vote is:
         *    W(R_N) - W(R_N-1), which is the delta increase of proportional weight
         *
         *  c.total_vote_weight =
         *    W(R_1) - W(R_0) +
         *    W(R_2) - W(R_1) + ...
         *    W(R_N) - W(R_N-1) = W(R_N) - W(R_0)
         *
         *  Since W(R_0) = 0, c.total_vote_weight is also bounded above by B and will always fit in a 64 bit
         *integer.
         *
         **/
        auto& vote = _db._temporary_public_impl().create<vote_object>([&](vote_object& v) {
            v.voter = voter.name;
            v.discipline_id = o.discipline_id;
            v.research_id = o.research_id;
            v.research_content_id = o.research_content_id;
            v.weight = o.weight;
            v.voting_power = used_power;
            v.tokens_amount = abs_used_tokens;
            v.voting_time = _db.head_block_time();

            const uint64_t old_weight = util::evaluate_reward_curve(current_weight.value, curators_reward_curve).to_uint64();
            const uint64_t new_weight = util::evaluate_reward_curve(tvo.total_weight.value, curators_reward_curve).to_uint64();
            v.weight = new_weight - old_weight;

            max_vote_weight = v.weight;

            /// discount weight by time
            uint128_t w(max_vote_weight);
            const uint64_t delta_t = std::min(uint64_t((v.voting_time - content.created_at).to_seconds()),
                                        uint64_t(DEIP_REVERSE_AUCTION_WINDOW_SECONDS));

            w *= delta_t;
            w /= DEIP_REVERSE_AUCTION_WINDOW_SECONDS;
            v.weight = w.to_uint64();
        });

        _db._temporary_public_impl().modify(tvo, [&](total_votes_object& t) {
            t.total_curators_reward_weight += vote.weight;
            if (content_is_active) {
                t.total_active_curators_reward_weight += vote.weight;
            }
        });
    }
    FC_CAPTURE_AND_RETHROW((o))
}

void vote_for_review_evaluator::do_apply(const vote_for_review_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_expert_token& expert_token_service = _db.obtain_service<dbs_expert_token>();
    dbs_discipline& discipline_service = _db.obtain_service<dbs_discipline>();
    dbs_review& review_service = _db.obtain_service<dbs_review>();
    dbs_research_content& research_content_service = _db.obtain_service<dbs_research_content>();

    const auto review_reward_curve = curve_id::power1dot5;
    const auto curators_reward_curve = curve_id::power1dot5;

    try
    {
        const auto& voter = account_service.get_account(o.voter);
        FC_ASSERT(voter.can_vote, "Voter has declined their voting rights.");

        const auto& review = review_service.get(o.review_id);

        research_content_service.check_research_content_existence(review.research_content_id);
        auto content = research_content_service.get_content_by_id(review.research_content_id);

        if (o.discipline_id != 0) {
            discipline_service.check_discipline_existence(o.discipline_id);
        }

        expert_token_service.check_expert_token_existence_by_account_and_discipline(o.voter, o.discipline_id);

        std::vector<discipline_id_type> target_disciplines;

        dbs_research_discipline_relation& research_disciplines_service = _db.obtain_service<dbs_research_discipline_relation>();
        const auto& relations = research_disciplines_service.get_research_discipline_relations_by_research(content.research_id);
        for (auto& relation_wrapper : relations) {
            const auto& relation = relation_wrapper.get();
            target_disciplines.push_back(relation.discipline_id);
        }

        const auto& token = expert_token_service.get_expert_token_by_account_and_discipline(o.voter, o.discipline_id);

        // Validate that research has discipline we are trying to vote with
        if (o.discipline_id != 0)
        {
            const auto& itr = std::find(target_disciplines.begin(), target_disciplines.end(), o.discipline_id);
            const bool discipline_found = (itr != target_disciplines.end());
            FC_ASSERT(discipline_found, "Cannot vote with {d} token as research is not in this discipline",
                      ("d", discipline_service.get_discipline(o.discipline_id).name));
        }

        const auto& review_vote_idx = _db._temporary_public_impl().get_index<review_vote_index>().indices().get<by_voter_discipline_and_review>();
        const auto& itr = review_vote_idx.find(std::make_tuple(voter.name, o.discipline_id, o.review_id));

        FC_ASSERT(itr == review_vote_idx.end(), "You have already voted for this review");

        const int64_t elapsed_seconds   = (_db.head_block_time() - token.last_vote_time).to_seconds();

        const int64_t regenerated_power = (DEIP_100_PERCENT * elapsed_seconds) / DEIP_VOTE_REGENERATION_SECONDS;
        const int64_t current_power = std::min(int64_t(token.voting_power + regenerated_power), int64_t(DEIP_100_PERCENT));
        FC_ASSERT(current_power > 0, "Account currently does not have voting power.");

        const int64_t abs_weight = abs(o.weight);
        const int64_t used_power = (current_power * abs_weight) / DEIP_100_PERCENT;
        const int64_t denominated_used_power = used_power / 10;

        FC_ASSERT(used_power <= current_power, "Account does not have enough power to vote.");

        const uint64_t abs_used_tokens
                = ((uint128_t(token.amount.value) * used_power) / (DEIP_100_PERCENT)).to_uint64();

        FC_ASSERT(o.weight != 0, "Vote weight cannot be 0.");

        _db._temporary_public_impl().modify(token, [&](expert_token_object& t) {
            t.voting_power = current_power - denominated_used_power;
            t.last_vote_time = _db.head_block_time();
        });

        FC_ASSERT(abs_used_tokens > 0, "Cannot vote with 0 rshares.");

        const auto& discipline = discipline_service.get_discipline(o.discipline_id);
        const auto current_weight = discipline.total_active_review_reward_weight;
        const uint64_t evaluated_review_vote_weight = util::evaluate_reward_curve(abs_used_tokens, review_reward_curve).to_uint64();

        _db._temporary_public_impl().modify(discipline, [&](discipline_object& d) {
           d.total_active_review_reward_weight += evaluated_review_vote_weight;
        });

        _db._temporary_public_impl().modify(review, [&](review_object& r) {
            r.reward_weights_per_discipline[o.discipline_id] += evaluated_review_vote_weight;
        });

        uint64_t max_vote_weight = 0;

        /** this verifies uniqueness of voter
         *
         *  cv.weight / c.total_vote_weight ==> % of rshares increase that is accounted for by the vote
         *
         *  W(R) = B * R / ( R + 2S )
         *  W(R) is bounded above by B. B is fixed at 2^64 - 1, so all weights fit in a 64 bit integer.
         *
         *  The equation for an individual vote is:
         *    W(R_N) - W(R_N-1), which is the delta increase of proportional weight
         *
         *  c.total_vote_weight =
         *    W(R_1) - W(R_0) +
         *    W(R_2) - W(R_1) + ...
         *    W(R_N) - W(R_N-1) = W(R_N) - W(R_0)
         *
         *  Since W(R_0) = 0, c.total_vote_weight is also bounded above by B and will always fit in a 64 bit
         *integer.
         *
         **/
        auto& review_vote = _db._temporary_public_impl().create<review_vote_object>([&](review_vote_object& v) {
            v.voter = voter.name;
            v.discipline_id = o.discipline_id;
            v.review_id = o.review_id;
            v.weight = o.weight;
            v.voting_power = used_power;
            v.tokens_amount = abs_used_tokens;
            v.voting_time = _db.head_block_time();

            const uint64_t old_weight = util::evaluate_reward_curve(current_weight.value, curators_reward_curve).to_uint64();
            const uint64_t new_weight = util::evaluate_reward_curve(discipline.total_active_review_reward_weight.value, curators_reward_curve).to_uint64();
            v.weight = new_weight - old_weight;

            max_vote_weight = v.weight;

            /// discount weight by time
            uint128_t w(max_vote_weight);
            const uint64_t delta_t = std::min(uint64_t((v.voting_time - review.created_at).to_seconds()),
                                              uint64_t(DEIP_REVERSE_AUCTION_WINDOW_SECONDS));

            w *= delta_t;
            w /= DEIP_REVERSE_AUCTION_WINDOW_SECONDS;
            v.weight = w.to_uint64();
        });

        auto weight_modifier = _db.calculate_review_weight_modifier(review.id, token.discipline_id);

        _db._temporary_public_impl().modify(review, [&](review_object& r) {
            r.curation_reward_weights_per_discipline[o.discipline_id] += review_vote.weight;
            r.weight_modifiers[token.discipline_id] = weight_modifier;
        });
    }
    FC_CAPTURE_AND_RETHROW((o))
}

void request_account_recovery_evaluator::do_apply(const request_account_recovery_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_witness& witness_service = _db.obtain_service<dbs_witness>();

    const auto& account_to_recover = account_service.get_account(o.account_to_recover);

    if (account_to_recover.recovery_account.length()) // Make sure recovery matches expected recovery account
        FC_ASSERT(account_to_recover.recovery_account == o.recovery_account,
                  "Cannot recover an account that does not have you as there recovery partner.");
    else // Empty string recovery account defaults to top witness
        FC_ASSERT(witness_service.get_top_witness().owner == o.recovery_account,
                  "Top witness must recover an account with no recovery partner.");

    account_service.create_account_recovery(o.account_to_recover, o.new_owner_authority);
}

void recover_account_evaluator::do_apply(const recover_account_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    const auto& account_to_recover = account_service.get_account(o.account_to_recover);

    FC_ASSERT(_db.head_block_time() - account_to_recover.last_account_recovery > DEIP_OWNER_UPDATE_LIMIT,
              "Owner authority can only be updated once an hour.");

    account_service.submit_account_recovery(account_to_recover, o.new_owner_authority, o.recent_owner_authority);
}

void change_recovery_account_evaluator::do_apply(const change_recovery_account_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    account_service.check_account_existence(o.new_recovery_account); // Simply validate account exists
    const auto& account_to_recover = account_service.get_account(o.account_to_recover);

    account_service.change_recovery_account(account_to_recover, o.new_recovery_account);
}

void delegate_vesting_shares_evaluator::do_apply(const delegate_vesting_shares_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    const auto& delegator = account_service.get_account(op.delegator);
    const auto& delegatee = account_service.get_account(op.delegatee);
    auto delegation = _db._temporary_public_impl().find<vesting_delegation_object, by_delegation>(
        boost::make_tuple(op.delegator, op.delegatee));

    auto available_shares = delegator.vesting_shares - delegator.delegated_vesting_shares
        - asset(delegator.to_withdraw - delegator.withdrawn, VESTS_SYMBOL);

    const auto& wso = _db.get_witness_schedule_object();
    const auto& gpo = _db.get_dynamic_global_properties();
    auto min_delegation
        = asset(wso.median_props.account_creation_fee.amount * 10, DEIP_SYMBOL) * gpo.get_vesting_share_price();
    auto min_update = wso.median_props.account_creation_fee * gpo.get_vesting_share_price();

    // If delegation doesn't exist, create it
    if (delegation == nullptr)
    {
        FC_ASSERT(available_shares >= op.vesting_shares, "Account does not have enough vesting shares to delegate.");
        FC_ASSERT(op.vesting_shares >= min_delegation, "Account must delegate a minimum of ${v}",
                  ("v", min_delegation));

        _db._temporary_public_impl().create<vesting_delegation_object>([&](vesting_delegation_object& obj) {
            obj.delegator = op.delegator;
            obj.delegatee = op.delegatee;
            obj.vesting_shares = op.vesting_shares;
            obj.min_delegation_time = _db.head_block_time();
        });

        account_service.increase_delegated_vesting_shares(delegator, op.vesting_shares);
        account_service.increase_received_vesting_shares(delegatee, op.vesting_shares);
    }
    // Else if the delegation is increasing
    else if (op.vesting_shares >= delegation->vesting_shares)
    {
        auto delta = op.vesting_shares - delegation->vesting_shares;

        FC_ASSERT(delta >= min_update, "Deip Power increase is not enough of a difference. min_update: ${min}",
                  ("min", min_update));
        FC_ASSERT(available_shares >= op.vesting_shares - delegation->vesting_shares,
                  "Account does not have enough vesting shares to delegate.");

        account_service.increase_delegated_vesting_shares(delegator, delta);
        account_service.increase_received_vesting_shares(delegatee, delta);

        _db._temporary_public_impl().modify(
            *delegation, [&](vesting_delegation_object& obj) { obj.vesting_shares = op.vesting_shares; });
    }
    // Else the delegation is decreasing
    else /* delegation->vesting_shares > op.vesting_shares */
    {
        auto delta = delegation->vesting_shares - op.vesting_shares;

        if (op.vesting_shares.amount > 0)
        {
            FC_ASSERT(delta >= min_update, "Deip Power decrease is not enough of a difference. min_update: ${min}",
                      ("min", min_update));
            FC_ASSERT(op.vesting_shares >= min_delegation,
                      "Delegation must be removed or leave minimum delegation amount of ${v}", ("v", min_delegation));
        }
        else
        {
            FC_ASSERT(delegation->vesting_shares.amount > 0,
                      "Delegation would set vesting_shares to zero, but it is already zero");
        }

        _db._temporary_public_impl().create<vesting_delegation_expiration_object>(
            [&](vesting_delegation_expiration_object& obj) {
                obj.delegator = op.delegator;
                obj.vesting_shares = delta;
                obj.expiration
                    = std::max(_db.head_block_time() + DEIP_CASHOUT_WINDOW_SECONDS, delegation->min_delegation_time);
            });

        account_service.decrease_received_vesting_shares(delegatee, delta);

        if (op.vesting_shares.amount > 0)
        {
            _db._temporary_public_impl().modify(
                *delegation, [&](vesting_delegation_object& obj) { obj.vesting_shares = op.vesting_shares; });
        }
        else
        {
            _db._temporary_public_impl().remove(*delegation);
        }
    }
}

void create_grant_evaluator::do_apply(const create_grant_operation& op)
{
    dbs_grant& grant_service = _db.obtain_service<dbs_grant>();
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_discipline& discipline_service = _db.obtain_service<dbs_discipline>();
    account_service.check_account_existence(op.owner);
    const auto& owner = account_service.get_account(op.owner);
    discipline_service.check_discipline_existence_by_name(op.target_discipline);
    auto& discipline = discipline_service.get_discipline_by_name(op.target_discipline);
    grant_service.create_grant(owner, op.balance, op.start_block, op.end_block, discipline.id);
}

void create_proposal_evaluator::do_apply(const create_proposal_operation& op)
{
    dbs_proposal& proposal_service = _db.obtain_service<dbs_proposal>();
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();

    account_service.check_account_existence(op.creator);
    research_group_service.check_research_group_token_existence(op.creator, op.research_group_id);
    const uint32_t _lifetime_min = DAYS_TO_SECONDS(1);
    const uint32_t _lifetime_max = DAYS_TO_SECONDS(10);

    const auto& props = _db.get_dynamic_global_properties();

    auto sec_till_expiration = op.expiration_time.sec_since_epoch() - props.time.sec_since_epoch();

    FC_ASSERT(sec_till_expiration <= _lifetime_max && sec_till_expiration >= _lifetime_min,
             "Proposal life time is not in range of ${min} - ${max} seconds. The actual value was ${actual}",
             ("min", _lifetime_min)("max", _lifetime_max)("actual", sec_till_expiration));

    auto& research_group = research_group_service.get_research_group(op.research_group_id);
    auto quorum_percent = research_group.quorum_percent;
    // the range must be checked in create_proposal_operation::validate()
    deip::protocol::proposal_action_type action = static_cast<deip::protocol::proposal_action_type>(op.action);

    if (action == deip::protocol::proposal_action_type::invite_member ||
            action == deip::protocol::proposal_action_type::dropout_member ||
            action == deip::protocol::proposal_action_type::change_quorum ||
            action == deip::protocol::proposal_action_type::rebalance_research_group_tokens)
        FC_ASSERT(!research_group.is_personal,
                  "You cannot invite or dropout member, change quorum and rebalance tokens in personal research group");

    // quorum_percent should be taken from research_group_object
    auto& proposal = proposal_service.create_proposal(action, op.data, op.creator, op.research_group_id, op.expiration_time, quorum_percent);

    if (research_group.is_personal)
    {
        auto& proposal_execution_service = _db.obtain_service<dbs_proposal_execution>();
        proposal_execution_service.execute_proposal(proposal);
        proposal_service.complete(proposal);
    }

}

void create_research_group_evaluator::do_apply(const create_research_group_operation& op)
{
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();

    bool is_personal = false;
    const research_group_object& research_group = research_group_service.create_research_group(op.name,
                                                                                               op.permlink,
                                                                                               op.description,
                                                                                               op.quorum_percent,
                                                                                               is_personal);
    
    research_group_service.create_research_group_token(research_group.id, DEIP_100_PERCENT, op.creator);
}

void make_review_evaluator::do_apply(const make_review_operation& op)
{
    dbs_review& review_service = _db.obtain_service<dbs_review>();
    dbs_research& research_service = _db.obtain_service<dbs_research>();
    dbs_research_content& research_content_service = _db.obtain_service<dbs_research_content>();
    dbs_research_discipline_relation& research_discipline_service = _db.obtain_service<dbs_research_discipline_relation>();
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_expert_token& expertise_token_service = _db.obtain_service<dbs_expert_token>();
    dbs_vote& vote_service = _db.obtain_service<dbs_vote>();
    dbs_discipline& discipline_service = _db.obtain_service<dbs_discipline>();

    account_service.check_account_existence(op.author);
    research_content_service.check_research_content_existence(op.research_content_id);
    auto content = research_content_service.get_content_by_id(op.research_content_id);

    auto expertise_tokens = expertise_token_service.get_expert_tokens_by_account_name(op.author);
    auto research_discipline_relations = research_discipline_service.get_research_discipline_relations_by_research(content.research_id);
    std::set<discipline_id_type> review_disciplines;
    std::set<discipline_id_type> research_disciplines_ids;
    for (auto rdr : research_discipline_relations) {
        research_disciplines_ids.insert(rdr.get().discipline_id);
    }

    for (auto expert_token : expertise_tokens)
    {
        auto& token = expert_token.get();
        if (research_disciplines_ids.find(token.discipline_id) != research_disciplines_ids.end())
            review_disciplines.insert(token.discipline_id);
    }

    FC_ASSERT(review_disciplines.size() != 0, "Reviewer does not have enough expertise to make review.");

    auto& review = review_service.create(op.research_content_id, op.content, op.is_positive, op.author, review_disciplines);

    if (review.is_positive) {
        try
        {
            const auto& research_reward_curve = curve_id::power1dot5;
            const auto& curators_reward_curve = curve_id::power1dot5;

            const auto& voter = account_service.get_account(op.author);

            FC_ASSERT(voter.can_vote, "Voter has declined their voting rights.");

            research_service.check_research_existence(content.research_id);

            for (auto& token_wrapper : expertise_tokens) {
                auto& token = token_wrapper.get();
                const auto& vote_idx = _db._temporary_public_impl().get_index<vote_index>().indices().get<by_voter_discipline_and_content>();
                const auto& itr = vote_idx.find(std::make_tuple(voter.name, token.discipline_id, content.id));

                FC_ASSERT(itr == vote_idx.end(), "You have already voted for this research content");

                const auto& total_votes_idx = _db._temporary_public_impl().get_index<total_votes_index>().indices().get<by_content_and_discipline>();
                const auto& total_votes_itr = total_votes_idx.find(std::make_tuple(content.id, token.discipline_id));

                // Create total_votes_object if it does not exist yet
                if (total_votes_itr == total_votes_idx.end())
                {
                    vote_service.create_total_votes(token.discipline_id, content.research_id, content.id);
                }

                const int64_t elapsed_seconds   = (_db.head_block_time() - token.last_vote_time).to_seconds();

                const int64_t regenerated_power = (DEIP_100_PERCENT * elapsed_seconds) / DEIP_VOTE_REGENERATION_SECONDS;
                const int64_t current_power = std::min(int64_t(token.voting_power + regenerated_power), int64_t(DEIP_100_PERCENT));
                FC_ASSERT(current_power > 0, "Account currently does not have voting power.");

                const int64_t abs_weight = DEIP_100_PERCENT;
                const int64_t used_power = (current_power * abs_weight) / DEIP_100_PERCENT;
                const int64_t denominated_used_power = used_power / 10;

                FC_ASSERT(used_power <= current_power, "Account does not have enough power to vote.");

                // TODO: Calculate used tokens
                const uint64_t abs_used_tokens
                        = ((uint128_t(token.amount.value) * used_power) / (DEIP_100_PERCENT)).to_uint64();

                const auto& tvo = vote_service
                        .get_total_votes_by_content_and_discipline(content.id, token.discipline_id);

                const bool content_is_active = content.activity_state == research_content_activity_state::active;

                _db._temporary_public_impl().modify(token, [&](expert_token_object& t) {
                    t.voting_power = current_power - denominated_used_power;
                    t.last_vote_time = _db.head_block_time();
                });

                FC_ASSERT(abs_used_tokens > 0, "Cannot vote with 0 rshares.");

                const auto current_weight = tvo.total_weight;
                const uint64_t evaluated_research_rshares = util::evaluate_reward_curve(abs_used_tokens, research_reward_curve).to_uint64();

                _db._temporary_public_impl().modify(tvo, [&](total_votes_object& t) {
                    t.total_weight += abs_used_tokens;
                    if (content_is_active) {
                        t.total_active_weight += abs_used_tokens;
                    }

                    t.total_research_reward_weight += evaluated_research_rshares;
                    if (content_is_active) {
                        t.total_active_research_reward_weight += evaluated_research_rshares;
                    }
                });

                auto& dgpo = _db.get_dynamic_global_properties();

                if (content_is_active) {
                    _db._temporary_public_impl().modify(dgpo, [&](dynamic_global_property_object& prop) {
                        prop.total_active_disciplines_reward_weight += abs_used_tokens;
                    });
                }

                const auto& discipline = discipline_service.get_discipline(token.discipline_id);
                _db._temporary_public_impl().modify(discipline, [&](discipline_object& d) {

                    d.total_active_research_reward_weight += evaluated_research_rshares;

                    if (content_is_active) {
                        d.total_active_reward_weight += abs_used_tokens;
                    }
                });

                uint64_t max_vote_weight = 0;
                auto& vote = _db._temporary_public_impl().create<vote_object>([&](vote_object& v) {
                    v.voter = voter.name;
                    v.discipline_id = token.discipline_id;
                    v.research_id = content.research_id;
                    v.research_content_id = content.id;
                    v.weight = DEIP_100_PERCENT;
                    v.voting_power = used_power;
                    v.tokens_amount = abs_used_tokens;
                    v.voting_time = _db.head_block_time();

                    const uint64_t old_weight = util::evaluate_reward_curve(current_weight.value, curators_reward_curve).to_uint64();
                    const uint64_t new_weight = util::evaluate_reward_curve(tvo.total_weight.value, curators_reward_curve).to_uint64();
                    v.weight = new_weight - old_weight;

                    max_vote_weight = v.weight;

                    /// discount weight by time
                    uint128_t w(max_vote_weight);
                    const uint64_t delta_t = std::min(uint64_t((v.voting_time - content.created_at).to_seconds()),
                                                      uint64_t(DEIP_REVERSE_AUCTION_WINDOW_SECONDS));

                    w *= delta_t;
                    w /= DEIP_REVERSE_AUCTION_WINDOW_SECONDS;
                    v.weight = w.to_uint64();
                });

                _db._temporary_public_impl().modify(review, [&](review_object& r) {
                    r.expertise_amounts_used[token.discipline_id] = token.amount;
                });

                _db._temporary_public_impl().modify(tvo, [&](total_votes_object& t) {
                    t.total_curators_reward_weight += vote.weight;
                    if (content_is_active) {
                        t.total_active_curators_reward_weight += vote.weight;
                    }
                });
            }
        }
        FC_CAPTURE_AND_RETHROW((op))
    }
}

void contribute_to_token_sale_evaluator::do_apply(const contribute_to_token_sale_operation& op)
{
    dbs_account &account_service = _db.obtain_service<dbs_account>();
    dbs_research_token_sale &research_token_sale_service = _db.obtain_service<dbs_research_token_sale>();

    account_service.check_account_existence(op.owner);

    auto& account = account_service.get_account(op.owner);
    FC_ASSERT(account.balance.amount >= op.amount, "Not enough funds to contribute");

    research_token_sale_service.check_research_token_sale_existence(op.research_token_sale_id);
    fc::time_point_sec contribution_time = _db.head_block_time();

    auto& research_token_sale = research_token_sale_service.get_research_token_sale_by_id(op.research_token_sale_id);
    FC_ASSERT(research_token_sale.status == research_token_sale_status::token_sale_active, "You cannot contribute to finished or expired token sale");

    bool is_hard_cap_reached = research_token_sale.total_amount + op.amount >= research_token_sale.hard_cap;

    share_type amount_to_contribute = op.amount;
    if (is_hard_cap_reached)
        amount_to_contribute = research_token_sale.hard_cap - research_token_sale.total_amount;

    auto research_token_sale_contribution = _db._temporary_public_impl().
            find<research_token_sale_contribution_object, by_owner_and_research_token_sale_id>(boost::make_tuple(op.owner, op.research_token_sale_id));

    if (research_token_sale_contribution != nullptr)
        _db._temporary_public_impl().modify(*research_token_sale_contribution,
                                            [&](research_token_sale_contribution_object& rtsc_o) { rtsc_o.amount += amount_to_contribute; });
    else
        research_token_sale_service.create_research_token_sale_contribution(op.research_token_sale_id, op.owner, contribution_time, amount_to_contribute);

    if (is_hard_cap_reached) {
        account_service.decrease_balance(account_service.get_account(op.owner), asset(amount_to_contribute));
        research_token_sale_service.increase_research_token_sale_tokens_amount(op.research_token_sale_id, amount_to_contribute);
        _db.distribute_research_tokens(op.research_token_sale_id);
    }
    else {
        account_service.decrease_balance(account_service.get_account(op.owner), asset(amount_to_contribute));
        research_token_sale_service.increase_research_token_sale_tokens_amount(op.research_token_sale_id, amount_to_contribute);
    }
}

void approve_research_group_invite_evaluator::do_apply(const approve_research_group_invite_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();
    dbs_research_group_invite &research_group_invite_service = _db.obtain_service<dbs_research_group_invite>();

    auto& research_group_invite = research_group_invite_service.get(op.research_group_invite_id);

    account_service.check_account_existence(research_group_invite.account_name);
    research_group_service.check_research_group_existence(research_group_invite.research_group_id);

    auto amount = research_group_service.decrease_research_group_tokens_amount(research_group_invite.research_group_id, research_group_invite.research_group_token_amount);
    research_group_service.create_research_group_token(research_group_invite.research_group_id,
                                                       amount,
                                                       research_group_invite.account_name);

    _db._temporary_public_impl().remove(research_group_invite);
}

void reject_research_group_invite_evaluator::do_apply(const reject_research_group_invite_operation& op)
{
    dbs_research_group_invite &research_group_invite_service = _db.obtain_service<dbs_research_group_invite>();

    research_group_invite_service.check_research_group_invite_existence(op.research_group_invite_id);

    auto& research_group_invite = research_group_invite_service.get(op.research_group_invite_id);

    _db._temporary_public_impl().remove(research_group_invite);

}

void create_research_group_join_request_evaluator::do_apply(const create_research_group_join_request_operation& op)
{
    dbs_research_group_join_request &research_group_join_request_service = _db.obtain_service<dbs_research_group_join_request>();
    dbs_account &account_service = _db.obtain_service<dbs_account>();
    dbs_research_group &research_group_service = _db.obtain_service<dbs_research_group>();

    account_service.check_account_existence(op.owner);
    research_group_service.check_research_group_existence(op.research_group_id);

    research_group_join_request_service.create(op.owner, op.research_group_id, op.motivation_letter);

}

void reject_research_group_join_request_evaluator::do_apply(const reject_research_group_join_request_operation& op)
{
    dbs_research_group_join_request &research_group_join_request_service = _db.obtain_service<dbs_research_group_join_request>();

    research_group_join_request_service.check_research_group_join_request_existence(op.research_group_join_request_id);

    auto& research_group_join_request = research_group_join_request_service.get(op.research_group_join_request_id);

    _db._temporary_public_impl().remove(research_group_join_request);
}

void transfer_research_tokens_to_research_group_evaluator::do_apply(const transfer_research_tokens_to_research_group_operation& op)
{
    dbs_research_token &research_token_service = _db.obtain_service<dbs_research_token>();
    dbs_research &research_service = _db.obtain_service<dbs_research>();

    research_token_service.check_research_token_existence_by_account_name_and_research_id(op.owner, op.research_id);
    research_service.check_research_existence(op.research_id);

    auto& research_token = research_token_service.get_research_token(op.research_token_id);
    auto& research = research_service.get_research(op.research_id);

    FC_ASSERT(op.amount > 0 && share_type(op.amount) <= research_token.amount, "Amount cannot be negative or greater than research token amount");

    _db._temporary_public_impl().modify(research, [&](research_object& r_o) {
        r_o.owned_tokens += op.amount;
    });

    if (op.amount == research_token.amount)
        _db._temporary_public_impl().remove(research_token);
    else
        _db._temporary_public_impl().modify(research_token, [&](research_token_object& rt_o) {
            rt_o.amount -= op.amount;
        });

}

void add_expertise_tokens_evaluator::do_apply(const add_expertise_tokens_operation& op)
{
    for (auto& discipline_to_add : op.disciplines_to_add)
    {
        FC_ASSERT(discipline_to_add.second > 0, "Amount must be bigger than 0");
        _db._temporary_public_impl().create<expert_token_object>([&](expert_token_object& et_o) {
            et_o.account_name = op.account_name;
            et_o.discipline_id = discipline_to_add.first;
            et_o.amount = discipline_to_add.second;
        });
    }
}

void research_update_evaluator::do_apply(const research_update_operation& op)
{
    dbs_research& research_service = _db.obtain_service<dbs_research>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    account_service.check_account_existence(op.owner);
    research_service.check_research_existence(op.research_id);

    auto& research = research_service.get_research(op.research_id);
    research_group_service.check_research_group_token_existence(op.owner, research.research_group_id);

    _db._temporary_public_impl().modify(research, [&](research_object& r_o) {
        fc::from_string(r_o.title, op.title);
        fc::from_string(r_o.abstract, op.abstract);
        fc::from_string(r_o.permlink, op.permlink);
    });
}

void deposit_to_vesting_contract_evaluator::do_apply(const deposit_to_vesting_contract_operation& op)
{
    dbs_vesting_contract& vesting_contract_service = _db.obtain_service<dbs_vesting_contract>();
    dbs_account &account_service = _db.obtain_service<dbs_account>();

    account_service.check_account_existence(op.sender);
    account_service.check_account_existence(op.receiver);

    vesting_contract_service.create(op.sender, op.receiver, asset(op.balance, DEIP_SYMBOL), op.withdrawal_period, op.contract_duration);

}

void withdraw_from_vesting_contract_evaluator::do_apply(const withdraw_from_vesting_contract_operation& op)
{
    dbs_vesting_contract& vesting_contract_service = _db.obtain_service<dbs_vesting_contract>();
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    account_service.check_account_existence(op.sender);
    account_service.check_account_existence(op.receiver);
    vesting_contract_service.check_vesting_contract_existence_by_sender_and_receiver(op.sender, op.receiver);

    auto& vesting_contract = vesting_contract_service.get_by_sender_and_reviever(op.sender, op.receiver);
    auto now = _db.head_block_time();

    share_type time_since_contract_start = now.sec_since_epoch() - vesting_contract.start_date.sec_since_epoch();
    share_type max_amount_to_withdraw = 0;

    if (now >= vesting_contract.expiration_date) {
        max_amount_to_withdraw = vesting_contract.balance.amount;
    } else {
        share_type current_period = (time_since_contract_start * vesting_contract.withdrawal_periods) / vesting_contract.contract_duration.sec_since_epoch();
        max_amount_to_withdraw = (vesting_contract.balance.amount * current_period) / vesting_contract.withdrawal_periods;
    }

    share_type to_withdraw = max_amount_to_withdraw - vesting_contract.withdrawn;

    if (to_withdraw > 0)
    {
        account_service.increase_balance(_db.get_account(op.receiver), to_withdraw);
        vesting_contract_service.withdraw(vesting_contract.id, to_withdraw);
    }

}

void vote_proposal_evaluator::do_apply(const vote_proposal_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();
    dbs_proposal& proposal_service = _db.obtain_service<dbs_proposal>();
    dbs_proposal_execution& proposal_execution_service = _db.obtain_service<dbs_proposal_execution>();

    research_group_service.check_research_group_token_existence(op.voter, op.research_group_id);
    account_service.check_account_existence(op.voter);
    proposal_service.check_proposal_existence(op.proposal_id);

    const proposal_object& proposal = proposal_service.get_proposal(op.proposal_id);

    FC_ASSERT(proposal.voted_accounts.find(op.voter) == proposal.voted_accounts.end(),
              "Account \"${account}\" already voted", ("account", op.voter));

    FC_ASSERT(!proposal_service.is_expired(proposal), "Proposal '${id}' is expired.", ("id", op.proposal_id));

    proposal_service.vote_for(op.proposal_id, op.voter);

    const research_group_object& research_group = research_group_service.get_research_group(proposal.research_group_id);

    float total_voted_weight = 0;
    auto& votes = proposal_service.get_votes_for(proposal.id);
    for (const proposal_vote_object& vote : votes) {
        total_voted_weight += vote.weight.value;
    }

    if (total_voted_weight  >= research_group.quorum_percent)
    {
        proposal_execution_service.execute_proposal(proposal);
        proposal_service.complete(proposal);
    }

}

} // namespace chain
} // namespace deip 
