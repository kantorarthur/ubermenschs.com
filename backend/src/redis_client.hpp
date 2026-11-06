#pragma once

#include "config.hpp"

#include <sw/redis++/redis++.h>

#include <memory>

// Creates the shared Redis client used by every request.
// sw::redis::Redis keeps a pool of connections and is safe to use from several threads.
// It connects lazily: creating it does not fail if Redis is down; the first command does.
std::unique_ptr<sw::redis::Redis> makeRedisClient(const Config& config);

// How many replicas are connected to the primary ("connected_slaves" in INFO replication).
// Returns -1 if the answer cannot be read.
int connectedReplicas(sw::redis::Redis& redis);

// Waits up to 100 ms for the replica to confirm the writes made on this connection
// (pass transaction.redis() right after transaction.exec()).
// If it does not answer in time, the data is still safe on the primary; we only log it.
void waitForReplica(sw::redis::Redis connection);
