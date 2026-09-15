#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <cstdint>
#include<filesystem>
#include <expected>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <variant>

#include "error.hpp"
#include "sqlite3.h"

namespace pi
{
    /* https://sqlite.org/rescode.html */
    enum class sqlite_result
    {
        sqlite_abort      = 4,
        sqlite_auth       = 23,
        sqlite_busy       = 5,
        sqlite_cantopen   = 14,
        sqlite_constraint = 19,
        sqlite_corrupt    = 11,
        sqlite_done       = 101,
        sqlite_empty      = 16,
        sqlite_error      = 1,
        sqlite_format     = 24,
        sqlite_full       = 13,
        sqlite_internal   = 2,
        sqlite_interrupt  = 9,
        sqlite_ioerr      = 10,
        sqlite_locked     = 6,
        sqlite_mismatch   = 20,
        sqlite_misuse     = 21,
        sqlite_nolfs      = 22,
        sqlite_nomem      = 7,
        sqlite_notadb     = 26,
        sqlite_notfound   = 12,
        sqlite_notice     = 27,
        sqlite_perm       = 3,
        sqlite_protocol   = 15,
        sqlite_range      = 25,
        sqlite_readonly   = 8,
        sqlite_row        = 100,
        sqlite_schema     = 17,
        sqlite_toobig     = 18,
        sqlite_warning    = 28
    };

    enum class sqlite_access_mode : int
    {
        readonly = SQLITE_OPEN_READONLY,
        readwrite = SQLITE_OPEN_READWRITE,
        create = SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE
    };

    using sqlite_error = error<sqlite_result>;

    class sqlite_connection;
    class sqlite_statement;

    class sqlite_connection
    {
    public:

        static std::expected<sqlite_connection, sqlite_error> create(std::filesystem::path path, sqlite_access_mode mode) noexcept;

        sqlite_connection(sqlite_connection&& other) noexcept;
        sqlite_connection& operator=(sqlite_connection&& other) noexcept;

        sqlite_connection(const sqlite_connection& ) = delete;
        sqlite_connection& operator=(const sqlite_connection& ) = delete;

        ~sqlite_connection();

        std::expected<sqlite_statement, sqlite_error> prepare_statement(std::string_view query) noexcept;

        std::string_view error_message()const noexcept;

    private:
        sqlite_connection(sqlite3* obj) noexcept ;

        sqlite3* m_sqlite_obj;
    };

    class sqlite_statement
    {
    public:
        using sqlite_value = std::variant<std::int64_t, double, std::string>;
        using table_result = std::unordered_map<std::string, std::vector<sqlite_value>>;

        sqlite_statement(sqlite3_stmt* statement, sqlite_connection* db)noexcept;

        sqlite_statement(const sqlite_statement& ) = delete;
        sqlite_statement& operator=(const sqlite_statement& ) = delete;

        sqlite_statement(sqlite_statement&& other) noexcept;
        sqlite_statement& operator=(sqlite_statement&& other) noexcept;

        ~sqlite_statement() noexcept;

        std::expected<void, sqlite_error> bind_parameter(const char* name, int value) noexcept;
        std::expected<void, sqlite_error> bind_parameter(const char* name, std::int64_t value) noexcept;
        std::expected<void, sqlite_error> bind_parameter(const char* name, double value) noexcept;
        std::expected<void, sqlite_error> bind_parameter(const char* name, std::string_view value) noexcept;

        std::expected<std::optional<table_result>, sqlite_error> execute() noexcept;

        int index_of(const char* name) const noexcept;
        int column_count() const noexcept;
        std::string column_name(int index) noexcept;
    private:

        sqlite_connection* m_db; // mostly for error messages
        sqlite3_stmt* m_statement;
    };

    class database_manager
    {
    public:
        database_manager(std::filesystem::path db_path);
        ~database_manager() = default;

        std::expected<sqlite_connection, sqlite_error> get_connection()const noexcept;

    private:
        std::filesystem::path m_db_path;
    };
}

#endif //DATABASE_HPP