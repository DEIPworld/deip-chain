#pragma once

#include <deip/chain/dbs_base_impl.hpp>
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/review_object.hpp>

namespace deip {
namespace chain {

///** DB service for operations with review_object
// *  --------------------------------------------
// */
class dbs_review : public dbs_base {
    friend class dbservice_dbs_factory;

    dbs_review() = delete;

protected:
    explicit dbs_review(database &db);

public:
    using review_refs_type = std::vector<std::reference_wrapper<const review_object>>;

    const review_object& get(const review_id_type& id);

    review_refs_type get_research_reviews(const research_id_type& research_id) const;
    review_refs_type get_author_reviews(const account_name_type& author) const;

    const review_object& create(const research_id_type& research_id, const string& content, bool is_positive,
                          const account_name_type& author);
};
} // namespace chain
} // namespace deip
