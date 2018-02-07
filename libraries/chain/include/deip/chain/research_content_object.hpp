#pragma once
#include <fc/fixed_string.hpp>
#include <fc/shared_string.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/deip_operations.hpp>

#include <deip/chain/deip_object_types.hpp>
#include <deip/chain/shared_authority.hpp>

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <numeric>

#include <vector>

namespace deip {
namespace chain {

enum research_content_type : uint16_t
{
    announcement = 1,
    milestone = 2,
    final_result = 3,
    review = 4,
    
    // make sure to update the bounds while adding new value
    First = announcement,
    Last = review
};

class research_content_object : public object<research_content_object_type, research_content_object>
{
    
    research_content_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    research_content_object(Constructor &&c, allocator<Allocator> a)
    {
        c(*this);
    }

    research_content_id_type id;
    research_id_type research_id;
    research_content_type type;
    fc::string content;
    flat_set<account_name_type> authors;
    time_point_sec created_at;
    std::vector<research_id_type> research_references;
    std::vector<string> research_external_references;
};

struct by_research_id;
struct by_research_id_and_content_type;

typedef multi_index_container<research_content_object,
        indexed_by<ordered_unique<tag<by_id>,
                member<research_content_object,
                        research_content_id_type,
                        &research_content_object::id>>,
                ordered_non_unique<tag<by_research_id>,
                        member<research_content_object,
                                research_id_type,
                                &research_content_object::research_id>>,
                ordered_non_unique<tag<by_research_id_and_content_type>,
                        composite_key<research_content_object,
                                member<research_content_object,
                                        research_id_type,
                                        &research_content_object::research_id>,
                                member<research_content_object,
                                        research_content_type,
                                        &research_content_object::type>>>>,
        allocator<research_content_object>>
        research_content_index;
}
}

FC_REFLECT_ENUM(deip::chain::research_content_type, (announcement)(milestone)(final_result)(review) )

FC_REFLECT(deip::chain::research_content_object, (id)(research_id)(type)(content)(authors)(research_references)(research_external_references))

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_content_object, deip::chain::research_content_index)