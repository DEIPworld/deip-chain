/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/schema/deip_objects.hpp>
#include <deip/chain/database/database.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/reflect/variant.hpp>

#include "database_fixture.hpp"

#include <cmath>

using namespace deip;
using namespace deip::chain;
using namespace deip::protocol;

BOOST_FIXTURE_TEST_SUITE(serialization_tests, clean_database_fixture)

/*
BOOST_AUTO_TEST_CASE( account_name_type_test )
{

auto test = []( const string& data ) {
   fixed_string<> a(data);
   std::string    b(data);

   auto ap = fc::raw::pack( empty );
   auto bp = fc::raw::pack( emptystr );
   FC_ASSERT( ap.size() == bp.size() );
   FC_ASSERT( std::equal( ap.begin(), ap.end(), bp.begin() ) );

   auto sfa = fc::raw::unpack<std::string>( ap );
   auto afs = fc::raw::unpack<fixed_string<>>( bp );
}
test( std::string() );
test( "helloworld" );
test( "1234567890123456" );

auto packed_long_string = fc::raw::pack( std::string( "12345678901234567890" ) );
auto unpacked = fc::raw::unpack<fixed_string<>>( packed_long_string );
idump( (unpacked) );
}
*/

BOOST_AUTO_TEST_CASE(serialization_raw_test)
{
    try
    {
        ACTORS((alice)(bob))
        transfer_operation op;
        op.from = "alice";
        op.to = "bob";
        op.amount = asset(100, DEIP_SYMBOL);

        trx.operations.push_back(op);
        auto packed = fc::raw::pack(trx);
        signed_transaction unpacked = fc::raw::unpack<signed_transaction>(packed);
        unpacked.validate();
        BOOST_CHECK(trx.digest() == unpacked.digest());
    }
    catch (fc::exception& e)
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(json_tests)
{
    try
    {
        auto var = fc::json::variants_from_string("10.6 ");
        var = fc::json::variants_from_string("10.5");
    }
    catch (const fc::exception& e)
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(extended_private_key_type_test)
{
    try
    {
        fc::ecc::extended_private_key key
            = fc::ecc::extended_private_key(fc::ecc::private_key::generate(), fc::sha256(), 0, 0, 0);
        extended_private_key_type type = extended_private_key_type(key);
        std::string packed = std::string(type);
        extended_private_key_type unpacked = extended_private_key_type(packed);
        BOOST_CHECK(type == unpacked);
    }
    catch (const fc::exception& e)
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(extended_public_key_type_test)
{
    try
    {
        fc::ecc::extended_public_key key
            = fc::ecc::extended_public_key(fc::ecc::private_key::generate().get_public_key(), fc::sha256(), 0, 0, 0);
        extended_public_key_type type = extended_public_key_type(key);
        std::string packed = std::string(type);
        extended_public_key_type unpacked = extended_public_key_type(packed);
        BOOST_CHECK(type == unpacked);
    }
    catch (const fc::exception& e)
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(version_test)
{
    try
    {
        BOOST_REQUIRE_EQUAL(string(version(1, 2, 3)), "1.2.3");

        fc::variant ver_str("3.0.0");
        version ver;
        fc::from_variant(ver_str, ver);
        BOOST_REQUIRE(ver == version(3, 0, 0));

        ver_str = fc::variant("0.0.0");
        fc::from_variant(ver_str, ver);
        BOOST_REQUIRE(ver == version());

        ver_str = fc::variant("1.0.1");
        fc::from_variant(ver_str, ver);
        BOOST_REQUIRE(ver == version(1, 0, 1));

        ver_str = fc::variant("1_0_1");
        fc::from_variant(ver_str, ver);
        BOOST_REQUIRE(ver == version(1, 0, 1));

        ver_str = fc::variant("12.34.56");
        fc::from_variant(ver_str, ver);
        BOOST_REQUIRE(ver == version(12, 34, 56));

        ver_str = fc::variant("256.0.0");
        DEIP_REQUIRE_THROW(fc::from_variant(ver_str, ver), fc::exception);

        ver_str = fc::variant("0.256.0");
        DEIP_REQUIRE_THROW(fc::from_variant(ver_str, ver), fc::exception);

        ver_str = fc::variant("0.0.65536");
        DEIP_REQUIRE_THROW(fc::from_variant(ver_str, ver), fc::exception);

        ver_str = fc::variant("1.0");
        DEIP_REQUIRE_THROW(fc::from_variant(ver_str, ver), fc::exception);

        ver_str = fc::variant("1.0.0.1");
        DEIP_REQUIRE_THROW(fc::from_variant(ver_str, ver), fc::exception);
    }
    FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE(hardfork_version_test)
{
    try
    {
        BOOST_REQUIRE_EQUAL(string(hardfork_version(1, 2)), "1.2.0");

        fc::variant ver_str("3.0.0");
        hardfork_version ver;
        fc::from_variant(ver_str, ver);
        BOOST_REQUIRE(ver == hardfork_version(3, 0));

        ver_str = fc::variant("0.0.0");
        fc::from_variant(ver_str, ver);
        BOOST_REQUIRE(ver == hardfork_version());

        ver_str = fc::variant("1.0.0");
        fc::from_variant(ver_str, ver);
        BOOST_REQUIRE(ver == hardfork_version(1, 0));

        ver_str = fc::variant("1_0_0");
        fc::from_variant(ver_str, ver);
        BOOST_REQUIRE(ver == hardfork_version(1, 0));

        ver_str = fc::variant("12.34.00");
        fc::from_variant(ver_str, ver);
        BOOST_REQUIRE(ver == hardfork_version(12, 34));

        ver_str = fc::variant("256.0.0");
        DEIP_REQUIRE_THROW(fc::from_variant(ver_str, ver), fc::exception);

        ver_str = fc::variant("0.256.0");
        DEIP_REQUIRE_THROW(fc::from_variant(ver_str, ver), fc::exception);

        ver_str = fc::variant("0.0.1");
        fc::from_variant(ver_str, ver);
        BOOST_REQUIRE(ver == hardfork_version(0, 0));

        ver_str = fc::variant("1.0");
        DEIP_REQUIRE_THROW(fc::from_variant(ver_str, ver), fc::exception);

        ver_str = fc::variant("1.0.0.1");
        DEIP_REQUIRE_THROW(fc::from_variant(ver_str, ver), fc::exception);
    }
    FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_SUITE_END()
#endif
