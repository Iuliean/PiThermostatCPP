#include "database.hpp"
#include <cassert>
#include <expected>
#include <filesystem>
#include <sqlite3.h>
#include <utility>

namespace pi
{

    std::expected<sqlite_connection, sqlite_error> sqlite_connection::create(std::filesystem::path path, sqlite_access_mode mode) noexcept
    {
        sqlite3* obj = nullptr;

        // https://sqlite.org/c3ref/open.html
        const int result = ::sqlite3_open_v2(
            std::filesystem::absolute(path).c_str(),
            &obj,
            std::to_underlying(mode),
            nullptr
        );

        if (result != SQLITE_OK)
            return std::unexpected{
                sqlite_error{
                    sqlite_result{result},
                    ::sqlite3_errmsg(obj)
                }
            };

        return sqlite_connection{obj};
    }

    sqlite_connection::sqlite_connection(sqlite_connection&& other) noexcept
        : m_sqlite_obj(other.m_sqlite_obj)
    {
        other.m_sqlite_obj = nullptr;
    }

    sqlite_connection& sqlite_connection::operator=(sqlite_connection&& other) noexcept
    {
        m_sqlite_obj = other.m_sqlite_obj;
        other.m_sqlite_obj = nullptr;
        return *this;
    }

    sqlite_connection::~sqlite_connection()
    {
        //For now like this untill i decide how to handle it
        if (::sqlite3_close(m_sqlite_obj) != SQLITE_OK)
        {
            assert(false);
        }
    }

    std::expected<sqlite_statement, sqlite_error> sqlite_connection::prepare_statement(std::string_view query) noexcept
    {
        sqlite3_stmt* statement = nullptr;
        const char* remainder = nullptr;
        const int result = ::sqlite3_prepare_v3(m_sqlite_obj, query.data(), query.size(), 0, &statement, &remainder);

        if ( result != SQLITE_OK)
            return std::unexpected{
                sqlite_error{
                    sqlite_result{result},
                    std::format("Failed to prepare statement. Stopped at {}. {}", query.substr(remainder - query.begin()), error_message())
                }
            };

        return sqlite_statement{
            statement,
            this
        };
    }

    std::string_view sqlite_connection::error_message()const noexcept
    {
        const char* msg = ::sqlite3_errmsg(m_sqlite_obj);

        return msg == nullptr ? msg : "Sqlite errmsg is not available";
    }

    /**********
    * PRIVATE *
    **********/
    sqlite_connection::sqlite_connection(sqlite3* obj) noexcept : m_sqlite_obj(obj) { }

    /*******************
    * SQLITE STATEMENT *
    *******************/

    sqlite_statement::sqlite_statement(sqlite3_stmt* statement, sqlite_connection* db) noexcept
        : m_db(db),
          m_statement(statement)
    { }

    sqlite_statement::sqlite_statement(sqlite_statement&& other) noexcept
        : m_db(other.m_db),
          m_statement(other.m_statement)
    {
        other.m_statement = nullptr;
        other.m_db = nullptr;
    }

    sqlite_statement& sqlite_statement::operator=(sqlite_statement&& other) noexcept
    {
        m_statement = other.m_statement;
        other.m_statement = nullptr;

        m_db = other.m_db;
        other.m_db = nullptr;

        return *this;
    }

    sqlite_statement::~sqlite_statement() noexcept
    {
        ::sqlite3_finalize(m_statement);
    }


    std::expected<void, sqlite_error> sqlite_statement::bind_parameter(const char* name, int value) noexcept
    {
        const int result = ::sqlite3_bind_int(
            m_statement, 
            index_of(name),
            value
        );

        if (result != SQLITE_OK)
            return std::unexpected{
                sqlite_error{
                    sqlite_result{result},
                    m_db->error_message()
                }
            };

        return {};
    }

    std::expected<void, sqlite_error> sqlite_statement::bind_parameter(const char* name, std::int64_t value) noexcept
    {
        const int result = ::sqlite3_bind_int(
            m_statement, 
            index_of(name),
            value
        );

        if (result != SQLITE_OK)
            return std::unexpected{
                sqlite_error{
                    sqlite_result{result},
                    m_db->error_message()
                }
            };

        return {};
    }

    std::expected<void, sqlite_error> sqlite_statement::bind_parameter(const char* name, double value) noexcept
    {
        const int result = ::sqlite3_bind_double(
            m_statement, 
            index_of(name),
            value
        );

        if (result != SQLITE_OK)
            return std::unexpected{
                sqlite_error{
                    sqlite_result{result},
                    m_db->error_message()
                }
            };

        return {};
    }

    std::expected<void, sqlite_error> sqlite_statement::bind_parameter(const char* name, std::string_view value) noexcept
    {
        const int result = ::sqlite3_bind_text(
            m_statement, 
            index_of(name),
            value.data(),
            value.size(),
            SQLITE_TRANSIENT
        );

        if (result != SQLITE_OK)
            return std::unexpected{
                sqlite_error{
                    sqlite_result{result},
                    m_db->error_message()
                }
            };

        return {};
    }

    std::expected<std::optional<sqlite_statement::table_result>, sqlite_error> sqlite_statement::execute() noexcept
    {
        table_result out;
        for (int result = ::sqlite3_step(m_statement); result != SQLITE_DONE; ::sqlite3_step(m_statement))
        {
            switch (result)
            {
                case SQLITE_ERROR:
                    return std::unexpected{
                        sqlite_error{
                            sqlite_result::sqlite_error,
                            m_db->error_message()
                        }
                    };
                case SQLITE_BUSY:
                    return std::unexpected{
                        sqlite_error{
                            sqlite_result::sqlite_busy,
                            m_db->error_message()
                        }
                    };
                case SQLITE_MISUSE:
                    return std::unexpected{
                        sqlite_error{
                            sqlite_result::sqlite_misuse,
                            m_db->error_message()
                        }
                    };
                case SQLITE_ROW:
                {
                    const int col_count = column_count();
                    for (int col_idx = 0; col_idx < col_count; ++col_idx)
                    {
                        switch (::sqlite3_column_type(m_statement,col_idx))
                        {
                            case SQLITE_INTEGER:
                            {
                                out[column_name(col_idx)]
                                    .push_back(
                                        ::sqlite3_column_int64(m_statement, col_idx)
                                    );
                                break;
                            }
                            case SQLITE_FLOAT:
                            {
                                out[column_name(col_idx)]
                                    .push_back(
                                        ::sqlite3_column_double(m_statement, col_idx)
                                    );
                                break;
                            }
                            case SQLITE_TEXT:
                            {
                                out[column_name(col_idx)]
                                    .push_back(
                                        reinterpret_cast<const char*>(::sqlite3_column_text(m_statement, col_idx))
                                    );
                                break;
                            }
                        }
                    }
                }
            }
        }

        return !out.empty() ? out : std::expected<std::optional<table_result>, sqlite_error>{};
    }


    int sqlite_statement::index_of(const char* name) const noexcept
    {
        return sqlite3_bind_parameter_index(m_statement, name);
    }

    int sqlite_statement::column_count() const noexcept
    {
        return sqlite3_column_count(m_statement);
    }

    std::string sqlite_statement::column_name(int index) noexcept
    {
        return ::sqlite3_column_name(m_statement, index);
    }

    /**********
    * PRIVATE *
    **********/

    /*******************
    * DATABASE MANAGER *
    *******************/
    database_manager::database_manager(std::filesystem::path db_path)

        : m_db_path(std::move(db_path))

    { }

    std::expected<sqlite_connection, sqlite_error> database_manager::get_connection()const noexcept
    {
        return sqlite_connection::create(m_db_path, sqlite_access_mode::readwrite);
    }

    /**********
    * PRIVATE *
    **********/


}