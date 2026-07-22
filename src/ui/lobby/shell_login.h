#pragma once

#include <iosfwd>
#include <string>

namespace session {

struct PlayerCredentials {
    std::string username;
    std::string password;
};

struct TwoPlayerNames {
    std::string white_user;
    std::string white_password;
    std::string black_user;
    std::string black_password;
};

class ShellLogin {
public:
    static PlayerCredentials readPlayerCredentials(std::istream& in, std::ostream& out);
    static TwoPlayerNames readTwoPlayerNames(std::istream& in, std::ostream& out);
};

}  // namespace session
