#pragma once

#include <string>
#include <vector>
#include <functional>

struct QuestObjective {
    std::string description;
    bool completed;
};

enum class QuestStatus {
    NotStarted,
    InProgress,
    Completed,
    Failed,
};

struct Quest {
    std::string id;
    std::string title;
    std::string description;
    QuestStatus status;
    std::vector<QuestObjective> objectives;
    int xpReward;
    int goldReward;

    bool allObjectivesComplete() const;
};
