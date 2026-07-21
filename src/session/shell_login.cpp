#include "shell_login.h"

#include <istream>
#include <ostream>
#include <string>

namespace session {

namespace {

std::string trimCopy(const std::string& value) {
    const std::size_t start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }

    const std::size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

std::string readNonEmptyLine(std::istream& in, std::ostream& out, const char* prompt,
                             const char* empty_message) {
    while (true) {
        out << prompt << std::flush;

        std::string line;
        if (!std::getline(in, line)) {
            return "";
        }

        const std::string value = trimCopy(line);
        if (value.empty()) {
            out << empty_message << '\n';
            continue;
        }

        return value;
    }
}

std::string readNonEmptyUsername(std::istream& in, std::ostream& out, const char* prompt) {
    return readNonEmptyLine(in, out, prompt, "Username cannot be empty.");
}

std::string readPassword(std::istream& in, std::ostream& out, const char* prompt) {
    return readNonEmptyLine(in, out, prompt, "Password cannot be empty.");
}

}  // namespace

PlayerCredentials ShellLogin::readPlayerCredentials(std::istream& in, std::ostream& out) {
    PlayerCredentials credentials;
    credentials.username = readNonEmptyUsername(in, out, "Username: ");
    if (credentials.username.empty()) {
        return credentials;
    }

    credentials.password = readPassword(in, out, "Password: ");
    if (credentials.password.empty()) {
        credentials.username.clear();
    }

    return credentials;
}

TwoPlayerNames ShellLogin::readTwoPlayerNames(std::istream& in, std::ostream& out) {
    TwoPlayerNames names;
    names.white_user =
        readNonEmptyUsername(in, out, "Player 1 (White) username: ");
    if (names.white_user.empty()) {
        return names;
    }

    names.white_password = readPassword(in, out, "Password: ");
    if (names.white_password.empty()) {
        names.white_user.clear();
        return names;
    }

    while (true) {
        names.black_user =
            readNonEmptyUsername(in, out, "Player 2 (Black) username: ");
        if (names.black_user.empty()) {
            return names;
        }

        if (names.black_user == names.white_user) {
            out << "Username must differ from Player 1.\n";
            continue;
        }

        break;
    }

    names.black_password = readPassword(in, out, "Password: ");
    if (names.black_password.empty()) {
        names.black_user.clear();
        return names;
    }

    return names;
}

}  // namespace session
