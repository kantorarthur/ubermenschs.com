#include "study_store.hpp"

#include "errors.hpp"

#include <algorithm>
#include <iterator>

namespace
{
    std::string studyKey(const std::string& userId)
    {
        return "user:" + userId + ":study";
    }

    bool isBookId(const std::string& bookId)
    {
        if(bookId.empty() || bookId.size() > limits::studyBookId)
            return false;
        return std::all_of(bookId.begin(), bookId.end(), [](char c)
        {
            return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
        });
    }
}

StudyStore::StudyStore(sw::redis::Redis& redis)
    : redis_(redis)
{
}

std::vector<std::string> StudyStore::readBooks(const std::string& userId)
{
    std::vector<std::string> books;
    redis_.smembers(studyKey(userId), std::back_inserter(books));
    std::sort(books.begin(), books.end());
    return books;
}

void StudyStore::setRead(const std::string& userId, const std::string& bookId, bool read)
{
    if(!isBookId(bookId))
        throw ValidationError("This book does not exist.");

    if(!read)
    {
        redis_.srem(studyKey(userId), bookId);
        return;
    }

    if(!redis_.sismember(studyKey(userId), bookId) &&
       redis_.scard(studyKey(userId)) >= static_cast<long long>(limits::studyBooksRead))
        throw ValidationError("Too many books are marked as read.");
    redis_.sadd(studyKey(userId), bookId);
}

void StudyStore::removeAll(const std::string& userId)
{
    redis_.del(studyKey(userId));
}
