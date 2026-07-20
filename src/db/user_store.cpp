#include "user_store.h"

#include <sqlite3.h>

#include <picosha2.h>

#include <iostream>
#include <random>
#include <vector>

namespace db {

namespace {

constexpr int kDefaultElo = 1200;
constexpr std::size_t kSaltBytes = 16;

std::string generateSalt(std::size_t byte_length = kSaltBytes) {
    static thread_local std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<int> byte_dist(0, 255);

    std::vector<unsigned char> bytes(byte_length);
    for (unsigned char& byte : bytes) {
        byte = static_cast<unsigned char>(byte_dist(rng));
    }
    return picosha2::bytes_to_hex_string(bytes.begin(), bytes.end());
}

std::string hashPassword(const std::string& password, const std::string& salt) {
    return picosha2::hash256_hex_string(password + salt);
}

}  // namespace

UserStore::UserStore(const std::string& db_path) {
    if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "Failed to open database '" << db_path << "': "
                  << (db_ != nullptr ? sqlite3_errmsg(db_) : "unknown error") << '\n';
        sqlite3_close(db_);
        db_ = nullptr;
        return;
    }

    runMigration();
}

UserStore::~UserStore() {
    if (db_ != nullptr) {
        sqlite3_close(db_);
    }
}

UserStore::UserStore(UserStore&& other) noexcept : db_(other.db_) {
    other.db_ = nullptr;
}

UserStore& UserStore::operator=(UserStore&& other) noexcept {
    if (this != &other) {
        if (db_ != nullptr) {
            sqlite3_close(db_);
        }
        db_ = other.db_;
        other.db_ = nullptr;
    }
    return *this;
}

void UserStore::runMigration() {
    static constexpr const char* kCreateTableSql =
        "CREATE TABLE IF NOT EXISTS users ("
        "  username TEXT PRIMARY KEY,"
        "  password_hash TEXT NOT NULL,"
        "  salt TEXT NOT NULL,"
        "  elo INTEGER NOT NULL DEFAULT 1200"
        ");";

    char* error_message = nullptr;
    if (sqlite3_exec(db_, kCreateTableSql, nullptr, nullptr, &error_message) != SQLITE_OK) {
        std::cerr << "Failed to create users table: "
                  << (error_message != nullptr ? error_message : "unknown error") << '\n';
        sqlite3_free(error_message);
    }
}

std::optional<UserRecord> UserStore::find(const std::string& username) const {
    if (db_ == nullptr) {
        return std::nullopt;
    }

    static constexpr const char* kSelectSql =
        "SELECT username, password_hash, salt, elo FROM users WHERE username = ?;";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db_, kSelectSql, -1, &statement, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(statement, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<UserRecord> result;
    if (sqlite3_step(statement) == SQLITE_ROW) {
        UserRecord record;
        record.username = reinterpret_cast<const char*>(sqlite3_column_text(statement, 0));
        record.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
        record.salt = reinterpret_cast<const char*>(sqlite3_column_text(statement, 2));
        record.elo = sqlite3_column_int(statement, 3);
        result = std::move(record);
    }

    sqlite3_finalize(statement);
    return result;
}

bool UserStore::create(const std::string& username, const std::string& password) {
    if (db_ == nullptr || find(username).has_value()) {
        return false;
    }

    const std::string salt = generateSalt();
    const std::string password_hash = hashPassword(password, salt);

    static constexpr const char* kInsertSql =
        "INSERT INTO users (username, password_hash, salt, elo) VALUES (?, ?, ?, ?);";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db_, kInsertSql, -1, &statement, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(statement, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, password_hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, salt.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 4, kDefaultElo);

    const bool success = sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return success;
}

bool UserStore::verifyPassword(const std::string& username, const std::string& password) const {
    const std::optional<UserRecord> record = find(username);
    if (!record.has_value()) {
        return false;
    }

    return hashPassword(password, record->salt) == record->password_hash;
}

bool UserStore::updateElo(const std::string& username, int new_elo) {
    if (db_ == nullptr) {
        return false;
    }

    static constexpr const char* kUpdateSql = "UPDATE users SET elo = ? WHERE username = ?;";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db_, kUpdateSql, -1, &statement, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(statement, 1, new_elo);
    sqlite3_bind_text(statement, 2, username.c_str(), -1, SQLITE_TRANSIENT);

    const bool success = sqlite3_step(statement) == SQLITE_DONE && sqlite3_changes(db_) > 0;
    sqlite3_finalize(statement);
    return success;
}

}  // namespace db
