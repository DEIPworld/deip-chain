#include <deip/chain/dbs_research.hpp>
#include <deip/chain/database.hpp>


namespace deip{
namespace chain{

dbs_research::dbs_research(database &db) : _base_type(db)
{
}

const research_object& dbs_research::create(const string &title, const string &abstract, const string &permlink,
                                            const research_group_id_type &research_group_id, const uint16_t review_share,
                                            const uint16_t dropout_compensation)
{
    const auto& new_research = db_impl().create<research_object>([&](research_object& r) {
        fc::from_string(r.title, title);
        fc::from_string(r.abstract, abstract);
        fc::from_string(r.permlink, permlink);
        r.research_group_id = research_group_id;
        r.review_share = review_share;
        r.dropout_compensation = dropout_compensation;
        r.is_finished = false;
        r.owned_tokens = DEIP_100_PERCENT;
        r.created_at = db_impl().head_block_time();
        r.last_update_time = db_impl().head_block_time();
        r.review_share_last_update = db_impl().head_block_time();
    });

    return new_research;
}

dbs_research::research_refs_type dbs_research::get_researches() const
{
    research_refs_type ret;

    const auto& idx = db_impl().get_index<research_index>().indicies().get<by_id>();
    auto it = idx.lower_bound(0);
    const auto it_end = idx.cend();
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_research::research_refs_type dbs_research::get_researches_by_research_group(const research_group_id_type& research_group_id) const
{
    research_refs_type ret;

    auto it_pair
        = db_impl().get_index<research_index>().indicies().get<by_research_group>().equal_range(research_group_id);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const research_object& dbs_research::get_research(const research_id_type& id) const
{
    return db_impl().get<research_object>(id);
}

const research_object& dbs_research::get_research_by_permlink(const string& permlink) const
{
    const auto& idx = db_impl().get_index<research_index>().indices().get<by_permlink>();
    auto itr = idx.find(permlink, fc::strcmp_less());
    FC_ASSERT(itr != idx.end(), "Research by permlink ${n} is not found", ("n", permlink));
    return *itr;
}

void dbs_research::check_research_existence(const research_id_type& id) const
{
    auto research = db_impl().find<research_object, by_id>(id);
    FC_ASSERT(research != nullptr, "Research with id \"${1}\" must exist.", ("1", id));
}

void dbs_research::decrease_owned_tokens(const research_object& research, const share_type delta)
{
    FC_ASSERT((research.owned_tokens - delta > 0), "Cannot update research owned tokens (result amount < 0)");
    db_impl().modify(research, [&](research_object& r_o) { r_o.owned_tokens -= delta; });
}

void dbs_research::increase_owned_tokens(const research_object& research, const share_type delta)
{
    FC_ASSERT(delta >= 0, "Cannot update research owned tokens (delta < 0)");
    db_impl().modify(research, [&](research_object& r_o) { r_o.owned_tokens += delta; });
}

void dbs_research::change_research_review_share_percent(const research_id_type& research_id,
                                                        const uint16_t review_share)
{
    check_research_existence(research_id);
    const research_object& research = get_research(research_id);
    db_impl().modify(research, [&](research_object& r) {
        r.review_share = review_share;
        r.review_share_last_update = db_impl().head_block_time();
    });
}
}
}