#ifndef CONFIGCLASSHELPER_H
#define CONFIGCLASSHELPER_H

#include <algorithm>

#include <lwip/sockets.h>
#include <arpa/inet.h>


//-------------------------------------------------------------------------------------
// CONFIG CLASS HELPER FUNCTIONS
//-------------------------------------------------------------------------------------
namespace configClassHelper
{
// Find a sequence: id == -1 looks up by name (new sequence being filled in), otherwise matches by id
template <typename Vec> typename Vec::value_type *findSequenceByIdOrName(Vec &vec, int id, const std::string &name)
{
    for (auto &el : vec) {
        if (id == -1) {
            if (el.sequenceName == name) {
                return &el;
            }
        }
        else if (id == el.sequenceId) {
            return &el;
        }
    }
    return nullptr;
}

// Find a sequence: Match by name only
template <typename Vec> typename Vec::value_type *findSequenceByName(Vec &vec, const std::string &name)
{
    for (auto &el : vec) {
        if (el.sequenceName == name) {
            return &el;
        }
    }
    return nullptr;
}

// Sync sequence vector accross sections
template <typename... Vecs> void syncSequenceVectors(const std::vector<SequenceList> &master, Vecs &...vecs)
{
    auto removeStale = [&](auto &vec) {
        vec.erase(std::remove_if(vec.begin(), vec.end(),
                                 [&](const auto &el) {
                                     return std::none_of(master.begin(), master.end(),
                                                         [&](const SequenceList &m) { return m.sequenceId == el.sequenceId; });
                                 }),
                  vec.end());
    };
    (removeStale(vecs), ...);

    auto addOrUpdate = [&](auto &vec) {
        for (const auto &m : master) {
            auto it = std::find_if(vec.begin(), vec.end(), [&](const auto &el) { return el.sequenceId == m.sequenceId; });
            if (it != vec.end()) {
                it->sequenceName = m.sequenceName;
            }
            else {
                typename std::decay_t<decltype(vec)>::value_type newEl{};
                newEl.sequenceId = m.sequenceId;
                newEl.sequenceName = m.sequenceName;
                vec.push_back(newEl);
            }
        }
    };
    (addOrUpdate(vecs), ...);

    auto sortById = [](auto &vec) {
        using ElType = typename std::decay_t<decltype(vec)>::value_type;
        std::sort(vec.begin(), vec.end(), [](const ElType &x, const ElType &y) { return x.sequenceId < y.sequenceId; });
    };
    (sortById(vecs), ...);
}


// Validate path formatting: normalizing slashes and leading/trailing separators
inline void validatePath(std::string &path, bool withFile = false)
{
    if (path.empty()) {
        return;
    }

    // Replace backslashes
    std::replace(path.begin(), path.end(), '\\', '/');

    if (!withFile) {
        // Remove trailing slash
        if (path.back() == '/') {
            path.pop_back();
        }
    }

    // Ensure leading slash
    if (!path.empty() && path.front() != '/') {
        path.insert(path.begin(), '/');
    }
}


// Validate structure formatting: removing leading/trailing slashes
inline void validateStructure(std::string &structureName)
{
    if (structureName.empty()) {
        return;
    }

    // Replace backslashes
    std::replace(structureName.begin(), structureName.end(), '\\', '/');

    // Remove leading slash
    if (structureName.front() == '/') {
        structureName.erase(structureName.begin());
    }

    // Remove trailing slash
    if (!structureName.empty() && structureName.back() == '/') {
        structureName.pop_back();
    }
}


inline bool isValidIpAddress(const char *ipAddress)
{
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ipAddress, &(sa.sin_addr)) == 1;
}

} // namespace configClassHelper

#endif // CONFIGCLASSHELPER_H
