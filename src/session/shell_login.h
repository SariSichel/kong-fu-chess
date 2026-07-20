#pragma once

#include <iosfwd>
#include <string>

namespace session {

struct TwoPlayerNames {
    std::string white_user;
    std::string white_password;
    std::string black_user;
    std::string black_password;
};

class ShellLogin {
public:
    static TwoPlayerNames readTwoPlayerNames(std::istream& in, std::ostream& out);
};

}  // namespace session
