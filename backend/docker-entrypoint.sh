#!/bin/sh
# Starts the backend inside the Docker image.
#
#   Locally (docker compose): runs the given command (normally "server") as the user "app".
#   On Render (EMBEDDED_REDIS=true): first starts a Redis inside the same container that keeps its
#   data on the disk mounted at /data, waits until it answers, then starts the server.
#   SIGTERM (deploy, restart) stops the server first and then Redis, which writes its data to disk.
set -eu

# Runs a program as the user "app" (the server does not need root). Used as a plain command,
# never inside a function, so $! below is the program itself and signals reach it.
AS_APP="setpriv --reuid=app --regid=app --init-groups"

if [ "$#" -eq 0 ]; then
    set -- server
fi

if [ "${EMBEDDED_REDIS:-false}" != "true" ]; then
    exec $AS_APP "$@"
fi

DATA_DIR="${REDIS_DATA_DIR:-/data/redis}"
mkdir -p "$DATA_DIR"
chown app:app "$DATA_DIR"

# Only reachable from inside the container (127.0.0.1). AOF written every second, and RDB snapshots;
# never deletes keys when memory is full (writes fail instead); 512 MB are shared with the server.
$AS_APP redis-server \
    --bind 127.0.0.1 --port 6379 --protected-mode yes \
    --dir "$DATA_DIR" \
    --appendonly yes --appendfsync everysec \
    --save "3600 1 300 100 60 10000" \
    --maxmemory "${REDIS_MAXMEMORY:-200mb}" --maxmemory-policy noeviction \
    --daemonize no &
REDIS_PID=$!

# Loading a large AOF can take a moment: wait up to 60 seconds.
tries=0
until redis-cli -p 6379 ping >/dev/null 2>&1; do
    if ! kill -0 "$REDIS_PID" 2>/dev/null; then
        echo "Redis stopped while starting" >&2
        exit 1
    fi
    tries=$((tries + 1))
    if [ "$tries" -ge 120 ]; then
        echo "Redis did not answer within 60 seconds" >&2
        kill -TERM "$REDIS_PID" 2>/dev/null || true
        exit 1
    fi
    sleep 0.5
done

$AS_APP "$@" &
SERVER_PID=$!

stop_all() {
    kill -TERM "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null || true
    # SHUTDOWN makes Redis write its data before it exits.
    redis-cli -p 6379 shutdown >/dev/null 2>&1 || kill -TERM "$REDIS_PID" 2>/dev/null || true
    wait "$REDIS_PID" 2>/dev/null || true
}

trap 'stop_all; exit 0' TERM INT

# If one of the two stops by itself, stop the other as well, so Render restarts the whole service.
while kill -0 "$SERVER_PID" 2>/dev/null && kill -0 "$REDIS_PID" 2>/dev/null; do
    sleep 1
done
echo "The server or Redis stopped; stopping the other one" >&2
stop_all
exit 1
