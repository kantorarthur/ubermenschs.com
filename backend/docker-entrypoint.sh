#!/bin/sh
# Starts the backend inside the Docker image.
#
#   Locally (docker compose): runs the given command (normally "server") as the user "app".
#   On Render (EMBEDDED_REDIS=true): first starts a Redis inside the same container that keeps its
#   data on the disk mounted at /data, waits until it answers, then starts the server.
#   SIGTERM (deploy, restart) stops the server first and then Redis, which writes its data to disk.
set -eu

if [ "$#" -eq 0 ]; then
    set -- server
fi

# Runs a program as the user "app": the server does not need root.
AS_APP="setpriv --reuid=app --regid=app --init-groups"

# Some hosts do not let the start script switch users, or signal the processes of another user.
# Then both programs run as the user this script runs as, so they can still be stopped cleanly.
can_switch_user() {
    [ "$(id -u)" -eq 0 ] || return 1
    # A short-lived test process as "app": can it be started, and may this script signal it?
    $AS_APP sleep 1 2>/dev/null &
    probe=$!
    allowed=1
    kill -0 "$probe" 2>/dev/null || allowed=0
    if ! wait "$probe"; then
        allowed=0
    fi
    [ "$allowed" -eq 1 ]
}

if ! can_switch_user; then
    echo "Note: running as $(id -un) (cannot switch to the user app here)" >&2
    AS_APP=""
fi

if [ "${EMBEDDED_REDIS:-false}" != "true" ]; then
    exec $AS_APP "$@"
fi

# True while the process exists and has not exited (a finished child stays as a "zombie" until waited for).
is_running() {
    [ -r "/proc/$1/status" ] && ! grep -q '^State:[[:space:]]*Z' "/proc/$1/status"
}

DATA_DIR="${REDIS_DATA_DIR:-/data/redis}"
mkdir -p "$DATA_DIR"
if [ -n "$AS_APP" ]; then
    chown app:app "$DATA_DIR"
fi

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
    if ! is_running "$REDIS_PID"; then
        code=0
        wait "$REDIS_PID" || code=$?
        echo "Redis stopped while starting (exit code $code, user $(id -un), data in $DATA_DIR)" >&2
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
while is_running "$SERVER_PID" && is_running "$REDIS_PID"; do
    sleep 1
done
echo "The server or Redis stopped; stopping the other one" >&2
stop_all
exit 1
