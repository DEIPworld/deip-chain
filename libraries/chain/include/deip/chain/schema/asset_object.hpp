#pragma once
#include "deip_object_types.hpp"
#include <boost/multi_index/composite_key.hpp>
#include <deip/protocol/protocol.hpp>
#include <fc/shared_string.hpp>

namespace deip {
namespace chain {

using deip::protocol::asset_symbol_type;
using deip::protocol::asset;
using deip::protocol::external_id_type;
using fc::shared_string;

enum class asset_type : uint8_t
{
    basic = 1,
    research_security_token = 2
};

class asset_object : public object<asset_object_type, asset_object>
{
    asset_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    asset_object(Constructor&& c, allocator<Allocator> a) : string_symbol(a), description(a)
    {
        c(*this);
    }

    asset_id_type id;

    asset_symbol_type symbol;
    shared_string string_symbol;
    uint8_t precision;

    account_name_type issuer;
    shared_string description;

    uint8_t type = static_cast<uint8_t>(asset_type::basic);
    optional<external_id_type> tokenized_research;

    share_type max_supply = DEIP_MAX_SHARE_SUPPLY;
    share_type current_supply = 0;
};

struct by_symbol;
struct by_string_symbol;
struct by_issuer;

typedef multi_index_container<asset_object,
            indexed_by<ordered_unique<tag<by_id>,
                    member<asset_object,
                            asset_id_type,
                           &asset_object::id>>,
            ordered_unique<tag<by_symbol>,
                    member<asset_object,
                            asset_symbol_type,
                           &asset_object::symbol>>,
            ordered_unique<tag<by_string_symbol>,
                    member<asset_object,
                            fc::shared_string,
                           &asset_object::string_symbol>>,
            ordered_non_unique<tag<by_issuer>,
                    member<asset_object,
                            account_name_type,
                           &asset_object::issuer>>>,
            allocator<asset_object>>
    asset_index;
    }
}

FC_REFLECT( deip::chain::asset_object, (id)(symbol)(string_symbol)(precision)(issuer)(description)(type)(tokenized_research)(max_supply)(current_supply))
CHAINBASE_SET_INDEX_TYPE( deip::chain::asset_object, deip::chain::asset_index )

FC_REFLECT_ENUM(deip::chain::asset_type,
  (basic)
  (research_security_token)
)