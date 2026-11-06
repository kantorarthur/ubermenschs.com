#include "redis_client.hpp"

#include <crow/logging.h>

#include <chrono>
#include <sstream>
#include <string>

std::unique_ptr<sw::redis::Redis> makeRedisClient(const Config& config)
{
    sw::redis::ConnectionOptions connection;
    connection.host = config.redisHost;
    connection.port = config.redisPort;
    if(!config.redisPassword.empty())
        connection.password = config.redisPassword;

    // Short timeouts: a request should fail fast instead of hanging when Redis is unreachable.
    connection.connect_timeout = std::chrono::milliseconds(500);
    connection.socket_timeout = std::chrono::milliseconds(1000);

    sw::redis::ConnectionPoolOptions pool;
    pool.size = 4;
    pool.wait_timeout = std::chrono::milliseconds(1000);

    return std::make_unique<sw::redis::Redis>(connection, pool);
}

int connectedReplicas(sw::redis::Redis& redis)
{
    // INFO replication answers with lines like "connected_slaves:1".
    const std::string info = redis.info("replication");
    const std::string key = "connected_slaves:";

    std::istringstream lines(info);
    std::string line;
    while(std::getline(lines, line))
    {
        if(line.rfind(key, 0) != 0)
            continue;

        try
        {
            return std::stoi(line.substr(key.size()));
        }
        catch(const std::exception&)
        {
            return -1;
        }
    }
    return -1;
}

void waitForReplica(sw::redis::Redis connection)
{
    try
    {
        const long long confirmed = connection.command<long long>("WAIT", "1", "100");
        if(confirmed < 1)
            CROW_LOG_WARNING << "WAIT: the replica did not confirm the write within 100 ms";
    }
    catch(const sw::redis::Error& error)
    {
        CROW_LOG_WARNING << "WAIT failed: " << error.what();
    }
}
