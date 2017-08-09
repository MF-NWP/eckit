/*
 * (C) Copyright 1996-2017 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */


#include "eckit/value/Value.h"
#include "eckit/types/FloatCompare.h"

#define ECKIT_TESTING_SELF_REGISTER_CASES 0
#include "eckit/testing/Test.h"

// Disable warnings for old-style casts in these tests. They are intentional
#pragma clang diagnostic ignored "-Wold-style-cast"

using namespace std;
using namespace eckit;
using namespace eckit::testing;


namespace eckit {
namespace test {

typedef std::vector<Test> Tests;

namespace unequal_arrays {
    std::vector<int> a = { 1, 2, 3 };
    std::vector<int> b = { 1, 2, 3, 4 };
    int arr[3] = { 1, 2, 4 };
    std::vector<string> s = { "1", "22", "333" };
    std::string arr_s[3] = { "1", "22", "444" };
}

namespace equal_arrays {
    std::vector<int> a = { 1, 2, 3 };
    std::vector<int> b = { 1, 2, 3 };
    int arr[3] = { 1, 2, 3 };
    std::vector<string> s = { "1", "22", "333" };
    std::string arr_s[3] = { "1", "22", "333" };
}

//----------------------------------------------------------------------------------------------------------------------

// These unit tests test the eckit testing framework itself.

//----------------------------------------------------------------------------------------------------------------------

void ThrowStd() {
    throw std::exception();
}

Tests tests = {
    { CASE( "CASE with no EXPECT passes" ) {

    }},

    { CASE( "EXPECT macros are defined correctly" ) {
        EXPECT( true );
        EXPECT_NOT( false );
        EXPECT_NO_THROW( true );
        EXPECT_THROWS ( throw std::exception() );
        EXPECT_THROWS_AS( throw std::exception(), std::exception );
    }},

    { CASE( "EXPECT does not throw exception on success" ) {
        Test pass = { CASE( "P" ) { EXPECT( true  ); } };

        try { pass.run(); }
        catch(...) { throw eckit::testing::TestException("Unexpected exception thrown in EXPECT.", Here()); }
    }},

    { CASE( "EXPECT creates an exception on failure" ) {
        Test fail = { CASE("F") { EXPECT( false ); } };
        
        do {
            try { fail.run(); } catch ( const TestException& e ) { break; }
            throw eckit::testing::TestException("No exception thrown in EXPECT.", Here());
        } while (false);
    }},

    { CASE( "EXPECT succeeds for success (true) and failure (false)" ) {
        Tests pass = {{ CASE("P") { EXPECT( true  ); } }};
        Tests fail = {{ CASE("F") { EXPECT( false ); } }};
        
        EXPECT( 0 == run(pass, TestVerbosity::Silent) );
        EXPECT( 1 == run(fail, TestVerbosity::Silent) );
    }},

    { CASE( "EXPECT succeeds for integer comparison" ) {
        EXPECT( 7 == 7 );
        EXPECT( 7 != 8 );
        EXPECT( 7 >= 6 );
        EXPECT( 7 <= 8 );
        EXPECT( 7 >  6 );
        EXPECT( 7 < 8 );
        EXPECT_NOT( 7 == 8 );
        EXPECT_NOT( 7 != 7 );
        EXPECT_NOT( 7 <= 6 );
        EXPECT_NOT( 7 >= 8 );
        EXPECT_NOT( 7 <  6 );
        EXPECT_NOT( 7 > 8 );
    }},
    { CASE( "Expect succeeds for integer vs. real comparison" ) {
        EXPECT( 7.0 == 7   );
        EXPECT( 7.0 != 8   );
        EXPECT( 7   == 7.0 );
        EXPECT( 7   != 8.0 );

        EXPECT_NOT( 7.0 == 8   );
        EXPECT_NOT( 7  !=  7.0 );
    }},

    { CASE( "Expect succeeds for string comparison" ) {
        std::string a("a"); std::string b("b");

        EXPECT( a == a ); EXPECT( a != b );
        EXPECT( b >= a ); EXPECT( a <= b );
        EXPECT( b >  a ); EXPECT( a <  b );

        EXPECT_NOT( a == b );
        EXPECT_NOT( a != a );
        EXPECT_NOT( b <= a );
        EXPECT_NOT( a >= b );
        EXPECT_NOT( b <  a );
        EXPECT_NOT( a >  b );
    }},

    { CASE( "Expect expression RHS can use * / % + -" ) {
        EXPECT( 7 == 1 * 7 );
        EXPECT( 7 == 7 / 1 );
        EXPECT( 0 == 7 % 1 );
        EXPECT( 7 == 1 + 6 );
        EXPECT( 7 == 8 - 1 );
    }},

    { CASE( "Expect expression LHS can use * / % + -" ) {
        EXPECT( 1 * 7 == 7 );
        EXPECT( 7 / 1 == 7 );
        EXPECT( 7 % 1 == 0 );
        EXPECT( 1 + 6 == 7 );
        EXPECT( 8 - 1 == 7 );
    }},
    { CASE( "run() returns the correct failure count" ) {
        Tests pass   = {{ CASE( "P"  ) { EXPECT( 1==1 ); } }};
        Tests fail_1 = {{ CASE( "F1" ) { EXPECT( 0==1 ); } }};
        Tests fail_3 = {{ CASE( "F1" ) { EXPECT( 0==1 ); } },
                         { CASE( "F2" ) { EXPECT( 0==1 ); } },
                         { CASE( "F3" ) { EXPECT( 0==1 ); } }};

        EXPECT( 0 == run( pass   , TestVerbosity::Silent ) );
        EXPECT( 1 == run( fail_1 , TestVerbosity::Silent ) );
        EXPECT( 3 == run( fail_3 , TestVerbosity::Silent ) );
    }},
    { CASE( "run() returns -1 if no CASE() is found" ) {
        Tests empty   = {};
        EXPECT( -1 == run( empty , TestVerbosity::Silent ) );
    }},
    { CASE( "Setup creates a fresh fixture for each section" ) {
        
        SETUP("Context") {
            int i = 7;

            SECTION("S1") { 
                i = 42;
            }
            SECTION("S2") {
                EXPECT( i == 7 );
            }
        }
    }},
    { CASE( "Setup runs as many times as there are sections" ) {

        int i = 0;
        SETUP("Context") {
            ++i;
            SECTION("S1") { }
            SECTION("S2") { }
        }
        EXPECT( i == 2 );
    }},
    { CASE( "Setup runs as many times as there are sections, with for-loop" ) {

        int i = 0;
        SETUP("Context") {
            ++i;
            for (int j = 0; j < 10; j++)
                SECTION("S1") { }
        }
        EXPECT( i == 10 );
    }},
    { CASE( "Expect runs multiple times in for-loop" ) {

        int i = 0;
        for (int j = 0; j < 10; j++)
            EXPECT( i++ == j );
        EXPECT( i == 10);

    }},
    { CASE( "Collections compare correctly" ) {

        SETUP("Create Collections") {
            std::vector<int> a = { 1, 2, 3 };
            std::vector<int> b = { 1, 2, 3 };

            SECTION("Compare Collections") {
                EXPECT( a == b );
                EXPECT( a <= b );
                EXPECT( a >= b );
                EXPECT_NOT ( a != b );
                EXPECT_NOT ( a > b );
                EXPECT_NOT ( a < b );
                a[0] = 0;
                EXPECT_NOT( a == b );
                EXPECT_NOT( a >= b );
                EXPECT ( a <= b );
                EXPECT ( a < b );
                EXPECT ( a != b );
                a[0] = 2;
                EXPECT_NOT ( a == b );
                EXPECT_NOT ( a <= b );
                EXPECT ( a >= b );
                EXPECT ( a >  b );
                EXPECT ( a != b );
            }
        }

    }},
    { CASE( "EXPECT_ALL_EQUAL compares equal arrays correctly" ) {
        using namespace eckit::test::equal_arrays;
        EXPECT_ALL_EQUAL( &a[0], &a[0] + 3,   &b[0], &b[0] + 3 );
        EXPECT_ALL_EQUAL( a.begin(), a.end(), &b[0], &b[0] + 3 );
        EXPECT_ALL_EQUAL( a.begin(), a.end(), b.begin(), b.end() );
        EXPECT_ALL_EQUAL( &a[0], &a[0] + 3, arr, arr + 3 );
        EXPECT_ALL_EQUAL( a.begin(), a.end(), arr, arr + 3 );

        EXPECT_ALL_EQUAL( s.begin(), s.end(), arr_s, arr_s + 3 );
        EXPECT_ALL_EQUAL( &s[0], &s[0] + 3, arr_s, arr_s + 3 );
    }},
    { CASE("EXPECT_ALL_EQUAL compares unequal arrays correctly") {
        // -- There is no "EXPECT_NOT_ALL_EQUAL" or equivalent, so the best way to test
        //    correct failures is to put each test inside a case and check that the correct
        //    number of cases fail.
        Tests tests = {
            { CASE( "Should pass" ) {
                using namespace eckit::test::unequal_arrays;
                EXPECT_ALL_EQUAL( &a[0], &a[0] + 3,   &b[0], &b[0] + 3 ); // Should pass
                EXPECT_ALL_EQUAL( a.begin(), a.end(), &b[0], &b[0] + 3 ); // Should pass
            }},
            { CASE ( "Should fail, wrong values (1 != 2) ") {
                using namespace eckit::test::unequal_arrays;
                EXPECT_ALL_EQUAL( a.begin(), a.end(), &b[1], &b[1] + 3 );
            }},
            { CASE ( "Should fail, wrong size") {
                using namespace eckit::test::unequal_arrays;
                EXPECT_ALL_EQUAL( &a[0], &a[0] + 3,   b.begin(), b.end() );
            }},
            { CASE ( "Should fail, wrong size") {
                using namespace eckit::test::unequal_arrays;
                EXPECT_ALL_EQUAL( a.begin(), a.end(), b.begin(), b.end() );
            }},
            { CASE ( "Should fail, wrong values (3 != 4)") {
                using namespace eckit::test::unequal_arrays;
                EXPECT_ALL_EQUAL( &a[0], &a[0] + 3, arr, arr + 3 );
            }},
            { CASE ( "Should fail, wrong values (3 != 4)") {
                using namespace eckit::test::unequal_arrays;
                EXPECT_ALL_EQUAL( a.begin(), a.end(), arr, arr + 3 );
            }},
            { CASE ( "Should fail, wrong values (333 != 444)") {
                using namespace eckit::test::unequal_arrays;
                EXPECT_ALL_EQUAL( s.begin(), s.end(), arr_s, arr_s + 3 );
            }},
            { CASE ( "Should fail, wrong values (333 != 444)") {
                using namespace eckit::test::unequal_arrays;
                EXPECT_ALL_EQUAL( &s[0], &s[0] + 3, arr_s, arr_s + 3 );
            }},
        };
        EXPECT( 7 == run( tests , TestVerbosity::Silent ) ); // Total of 7 tests ^ should fail.
    }},
};

//----------------------------------------------------------------------------------------------------------------------

} // namespace test
} // namespace eckit



int main(int argc, char* argv[]) {
    eckit::Main::initialise(argc, argv);
    return run_tests(eckit::test::tests, argc, argv);
}
