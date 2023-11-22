/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include "eckit/memory/NonCopyable.h"

//----------------------------------------------------------------------------------------------------------------------

#ifdef __NEC__
#include <cstring>
std::basic_ostream<char, std::char_traits<char>> & operator<<(std::basic_ostream<char, std::char_traits<char>>& out, const std::string & s) {
   out << s.c_str();
   return out;
}
std::ostream & operator<<(std::ostream& out, std::string & s) {
    out << s.c_str();
    return out;
}
#endif

namespace eckit {

//----------------------------------------------------------------------------------------------------------------------

NonCopyable::NonCopyable() {}

NonCopyable::~NonCopyable() {}

//----------------------------------------------------------------------------------------------------------------------

}  // namespace eckit
