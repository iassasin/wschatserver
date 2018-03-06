#include "redis.hpp"

boost::asio::io_service Redis::io;
redisclient::RedisSyncClient Redis::redisInstance(io);
