#pragma once
#include "crow/logging.h"
#include "sqlite/sqlite3.h"
#include "json_custom.h"

#include <string>

class DataBase
{
public:
    DataBase()
        : connection(nullptr)
    {
        sqlite3_open("data.db", &connection);

        char* msg = nullptr;
        sqlite3_exec(connection, "CREATE TABLE IF NOT EXISTS Temperatures (value REAL NOT NULL, date DATE NOT NULL, time TIME NOT NULL);", nullptr, nullptr, &msg);

        if(msg)
        {
            CROW_LOG_ERROR << "[DataBase]:" << msg;
            sqlite3_free(msg);
        }

        sqlite3_exec(connection, "CREATE TABLE IF NOT EXISTS States (state BOOLEAN NOT NULL, date DATE NOT NULL, time TIME NOT NULL, duration INTEGER NOT NULL);", nullptr, nullptr, &msg);
        
        if(msg)
        {
            CROW_LOG_ERROR << "[DataBase]:" << msg;
            sqlite3_free(msg);
        }
    }

    ~DataBase()
    {
        if (connection)
            sqlite3_close(connection);
    }
    
    void insertTemp(float value)
    {
        char* msg           = nullptr;
        std::string query   = "INSERT INTO Temperatures (value, date, time) VALUES (" + std::to_string(value)+",DATE(),TIME());";
        sqlite3_exec(connection, query.c_str(), nullptr, nullptr, &msg);

        if(msg)
            CROW_LOG_ERROR << "[DataBase]:" << msg;

        sqlite3_free(msg);
    }
    void insertState(bool state, float duration)
    {
        char* msg           = nullptr;
        std::string query   = "INSERT INTO States (state, date, time, duration) VALUES ("+ std::to_string(state)+", DATE(), TIME(),"+ std::to_string(duration)+");";
        sqlite3_exec(connection, query.c_str(), nullptr, nullptr, &msg);

        if(msg)
            CROW_LOG_ERROR << "[DataBase]:" << msg;

        sqlite3_free(msg);
    }

    inline void getTemperatures(const std::string& start, const std::string& end, json& j)
    {
        queryDB("SELECT * FROM Temperatures WHERE julianday(date) >= julianday(\'" + start + "\') AND julianday(date) <= julianday(\'"+ end + "\');", j);
    }

    inline void getTemperaturesPast24h(json& j)
    {
        queryDB("SELECT * FROM Temperatures WHERE DATE('now', '-1 days') < DATE(date || time);", j);
    }

    inline void getAverageTemp(json& j)
    {
        queryDB("SELECT AVG(value) AS averageTemp FROM Temperatures WHERE date = DATE('now');", j);
    }

    inline void getAverageTempPast24h(json& j)
    {
        queryDB("SELECT AVG(value) AS averageTemp FROM Temperatures WHERE DATE('now', '-1 days') < DATE(date || time);", j);
    }

    inline void getAverageTemp(const std::string& start, const std::string& end, json& j)
    {
        queryDB("SELECT AVG(value) AS averageTemp FROM Temperatures WHERE julianday(date) >= julianday(\'" + start + "\') AND julianday(date) <= julianday(\'"+ end + "\');", j);
    }

    inline void getStates(const std::string& state, const std::string& start, const std::string& end, json& j)
    {
        queryDB("SELECT * FROM States WHERE julianday(date) >= julianday(\'" + start + "\') AND julianday(date) <= julianday(\'" + end + "\') AND state ="+ state + ";", j);
    }

    inline void getStates24h(const std::string& state, json& j)
    {
        queryDB("SELECT * FROM States WHERE DATE('now', '-1 days') < DATE(date || time) AND state =" + state + ";", j);
    }

    inline void getAverageStateTime(const std::string& state, json& j)
    {
        queryDB("SELECT AVG(duration) AS averageOnTime FROM States WHERE date = DATE('now') AND state =" + state + ";", j);
    }

    inline void getAverageStateTimePast24h(const std::string& state, json& j)
    { 
        queryDB("SELECT AVG(duration) AS averageOnTime FROM States WHERE DATE('now', '-1 days') < DATE(date || time) AND state =" + state + ";", j);
    }

    inline void getAverageStateTime(const std::string& state, const std::string& start, const std::string& end, json& j)
    {
        queryDB("SELECT AVG(duration) AS averageOnTime FROM States WHERE julianday(date) >= julianday(\'" + start + "\') AND julianday(date) <= julianday(\'" + end + "\') AND state =" + state +";", j);       
    }
private:
    sqlite3* connection;
    
    void queryDB(const std::string& query, json& out)
    {
        out["status"] = 0;
        sqlite3_stmt* statement;
        const char* tail; 
        int error = sqlite3_prepare_v2(connection, query.c_str(), query.size()+1, &statement, &tail);
        if(error != SQLITE_OK)
        {
            CROW_LOG_ERROR << "[DataBase]:" << tail << " Error code: " << error;
            sqlite3_finalize(statement);
            out["status"] = "Database error: " + std::to_string(error);
            return;
        }
        
        std::size_t entryNum = 0;
        for(int result = sqlite3_step(statement); result != SQLITE_DONE;
                result = sqlite3_step(statement))
        {
            switch (result)
            {
            case SQLITE_BUSY:
                {
                    CROW_LOG_ERROR << "[DataBase]: SQLITE_BUSY";
                    sqlite3_finalize(statement);
                    out["status"] = "SQLITE_BUSY";
                    return;
                }
            case SQLITE_ERROR:
                {
                    CROW_LOG_ERROR << "[DataBase]: SQLITE_ERROR";
                    sqlite3_finalize(statement);
                    out["status"] = "SQLITE_ERROR";
                    return;
                }
            case SQLITE_MISUSE:
                {
                    CROW_LOG_ERROR << "[DataBase]: SQLITE_MISUSE";
                    sqlite3_finalize(statement);
                    out["status"] = "SQLITE_MISUSE";
                    return;
                }
            case SQLITE_ROW:
                {
                    int count = sqlite3_column_count(statement);
                    for(int i = 0; i < count; i++)
                    {
                        switch(sqlite3_column_type(statement, i))
                        {
                            case SQLITE_FLOAT:
                            {
                               out["data"][entryNum][sqlite3_column_name(statement, i)] = sqlite3_column_double(statement, i);
                               break;
                            }
                            case SQLITE_INTEGER:
                            {
                                out["data"][entryNum][sqlite3_column_name(statement, i)] = sqlite3_column_int(statement, i);
                                break;
                            }
                            case SQLITE_TEXT:
                            {
                                out["data"][entryNum][sqlite3_column_name(statement, i)] = (const char*)sqlite3_column_text(statement, i);
                            }

                        }                   
                    }
                }
                break;
            default:
                {
                    CROW_LOG_ERROR << "[DataBase]: Error code:" << result;
                    sqlite3_finalize(statement);
                    out["status"] = "Unhandeled error " + std::to_string(result);
                    return;
                }
            }
            entryNum++;
        }

        sqlite3_finalize(statement);
    }
};
