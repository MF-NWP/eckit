/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#ifndef eckit_NonCopyable_h
#define eckit_NonCopyable_h

#include <iostream>
extern std::basic_ostream<char, std::char_traits<char>> & operator<<(std::basic_ostream<char, std::char_traits<char>>& out, const std::string & s);
extern std::ostream & operator<<(std::ostream& out, std::string & s);

namespace eckit {

/// Inherit from this class to make a NonCopyable class

class NonCopyable {
protected:
    NonCopyable();
    ~NonCopyable();

private:  // No copy allowed
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

}  // namespace eckit

#endif
