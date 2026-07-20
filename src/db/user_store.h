#pragma once

#include <optional>
#include <string>

struct sqlite3;

namespace db {

struct UserRecord {
    std::string username;
    std::string password_hash;
    std::string salt;
    int elo = 1200;
};

class UserStore {
public:
    explicit UserStore(const std::string& db_path = "kong_fu_chess.db");
    ~UserStore();

    UserStore(const UserStore&) = delete;
    UserStore& operator=(const UserStore&) = delete;
    UserStore(UserStore&& other) noexcept;
    UserStore& operator=(UserStore&& other) noexcept;

    bool isOpen() const { return db_ != nullptr; }

    std::optional<UserRecord> find(const std::string& username) const;
    bool create(const std::string& username, const std::string& password);
    bool verifyPassword(const std::string& username, const std::string& password) const;
    bool updateElo(const std::string& username, int new_elo);

private:
    void runMigration();

    sqlite3* db_ = nullptr;
};

}  // namespace db
