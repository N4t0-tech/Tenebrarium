#include "Quest.hpp"
#include <algorithm>

bool Quest::allObjectivesComplete() const {
    return std::all_of(objectives.begin(), objectives.end(),
        [](const QuestObjective& o) { return o.completed; });
}
