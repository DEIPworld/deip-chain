#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/nda_contract_object.hpp>
#include <deip/chain/schema/nda_contract_file_access_object.hpp>

namespace deip {
namespace chain {

class dbs_nda_contract_requests : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_nda_contract_requests() = delete;

protected:
    explicit dbs_nda_contract_requests(database& db);

public:
    using nda_contracts_refs_type = std::vector<std::reference_wrapper<const nda_contract_file_access_object>>;
    using nda_contract_optional_ref_type = fc::optional<std::reference_wrapper<const nda_contract_file_access_object>>;

    const nda_contract_file_access_object& get(const nda_contract_file_access_id_type& request_id);

    const bool request_exists(const nda_contract_file_access_id_type& id) const;

    const nda_contract_optional_ref_type get_request_if_exists(const nda_contract_file_access_id_type& id) const;

    const nda_contract_file_access_object& get_by_contract_id_and_hash(const nda_contract_id_type& contract_id,
                                                                       const fc::string& encrypted_payload_hash);

    const nda_contract_optional_ref_type get_request_by_contract_id_and_hash_if_exists(const nda_contract_id_type& contract_id,
                                                                                       const fc::string& encrypted_payload_hash);

    nda_contracts_refs_type get_by_contract_id(const nda_contract_id_type& contract_id);

    nda_contracts_refs_type get_by_requester(const account_name_type& requester);

    const nda_contract_file_access_object& create_file_access_request(const nda_contract_id_type& contract_id,
                                                                    const account_name_type& requester,
                                                                    const fc::string& encrypted_payload_hash,
                                                                    const fc::string& encrypted_payload_iv);

    void fulfill_file_access_request(const nda_contract_file_access_object& request,
                                     const fc::string& encrypted_payload_encryption_key,
                                     const fc::string& proof_of_encrypted_payload_encryption_key);
};
} // namespace chain
} // namespace deip