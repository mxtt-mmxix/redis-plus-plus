/**************************************************************************
   Copyright (c) 2017 sewenew

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 *************************************************************************/

#ifndef SEWENEW_REDISPLUSPLUS_REPLY_H
#define SEWENEW_REDISPLUSPLUS_REPLY_H

#include <cassert>
#include <string>
#include <functional>
#include <hiredis/hiredis.h>
#include "exceptions.h"
#include "utils.h"

namespace sw {

namespace redis {

struct ReplyDeleter {
    void operator()(redisReply *reply) const {
        if (reply != nullptr) {
            freeReplyObject(reply);
        }
    }
};

using ReplyUPtr = std::unique_ptr<redisReply, ReplyDeleter>;

class DummyReplyFunctor {
public:
    void operator()(redisReply & /*reply*/) {}
};

class StatusReplyFunctor {
public:
    template <typename StringReplyCallback>
    explicit StatusReplyFunctor(StringReplyCallback callback) : _callback(callback) {}

    void operator()(redisReply &reply);

private:
    std::function<void (const std::string &)> _callback;
};

class StringReplyFunctor {
public:
    template <typename StringReplyCallback>
    explicit StringReplyFunctor(StringReplyCallback callback) : _callback(callback) {}

    void operator()(redisReply &reply);

private:
    std::function<void (const std::string &)> _callback;
};

class IntegerReplyFunctor {
public:
    template <typename IntegerReplyCallback>
    explicit IntegerReplyFunctor(IntegerReplyCallback callback) : _callback(callback) {}

    void operator()(redisReply &reply);

private:
    std::function<void (long long)> _callback;
};

namespace reply {

inline bool is_error(redisReply &reply) {
    return reply.type == REDIS_REPLY_ERROR;
}

inline bool is_nil(redisReply &reply) {
    return reply.type == REDIS_REPLY_NIL;
}

inline bool is_string(redisReply &reply) {
    return reply.type == REDIS_REPLY_STRING;
}

inline bool is_status(redisReply &reply) {
    return reply.type == REDIS_REPLY_STATUS;
}

inline bool is_integer(redisReply &reply) {
    return reply.type == REDIS_REPLY_INTEGER;
}

inline bool is_array(redisReply &reply) {
    return reply.type == REDIS_REPLY_ARRAY;
}

std::string to_error(redisReply &reply);

std::string to_status(redisReply &reply);

std::string to_string(redisReply &reply);

OptionalString to_optional_string(redisReply &reply);

long long to_integer(redisReply &reply);

template <typename Iter>
void to_string_array(redisReply &reply, Iter output) {
    if (!reply::is_array(reply)) {
        throw RException("Expect ARRAY reply.");
    }

    for (std::size_t idx = 0; idx != reply.elements; ++idx) {
        auto *sub_reply = reply.element[idx];
        if (sub_reply == nullptr) {
            throw RException("Null string array reply.");
        }

        *output = to_string(*sub_reply);

        ++output;
    }
}

template <typename Iter>
void to_string_pair_array(redisReply &reply, Iter output) {
    if (!reply::is_array(reply)) {
        throw RException("Expect ARRAY reply.");
    }

    if (reply.elements % 2 != 0) {
        throw RException("Not string pair array reply");
    }

    for (std::size_t idx = 0; idx != reply.elements; idx += 2) {
        auto *key_reply = reply.element[idx];
        auto *val_reply = reply.element[idx + 1];
        if (key_reply == nullptr || val_reply == nullptr) {
            throw RException("Null string array reply.");
        }

        *output = std::make_pair(to_string(*key_reply), to_string(*val_reply));

        ++output;
    }
}

bool status_ok(redisReply &reply);

}

// Inline implementations.

inline void StatusReplyFunctor::operator()(redisReply &reply) {
    _callback(reply::to_status(reply));
}

inline void StringReplyFunctor::operator()(redisReply &reply) {
    _callback(reply::to_string(reply));
}

inline void IntegerReplyFunctor::operator()(redisReply &reply) {
    _callback(reply::to_integer(reply));
}

}

}

#endif // end SEWENEW_REDISPLUSPLUS_REPLY_H