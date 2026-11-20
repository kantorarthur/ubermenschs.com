#pragma once

#include <sw/redis++/redis++.h>

#include <cstddef>
#include <string>
#include <vector>

// The same limits the frontend uses (src/data/studyPlan.ts).
namespace limits
{
    inline constexpr std::size_t studyBookId = 60;
    inline constexpr std::size_t studyBooksRead = 100;
}

// Progress through the philosophical study plan: which books the user marked as read.
// The plan itself (stages, books and their order) lives in the frontend, in src/data/studyPlan.ts;
// here a book is only its id, such as "meditations".
//
//   user:{id}:study   set of the ids of the books read
class StudyStore
{
public:
    explicit StudyStore(sw::redis::Redis& redis);

    // The ids of the books read, sorted.
    std::vector<std::string> readBooks(const std::string& userId);

    // Marks a book as read or not read. The id must be 1-60 characters of a-z, 0-9 and "-".
    void setRead(const std::string& userId, const std::string& bookId, bool read);

    // Forgets the whole progress (when the account is deleted).
    void removeAll(const std::string& userId);

private:
    sw::redis::Redis& redis_;
};
