#pragma once

// Sistema de misiones simple basado en contadores globales (enemiesKilled_, chestsOpened_, etc.).
// Las misiones se definen en Game::initQuests() y se comprueban cada frame en checkQuestProgress().
// No hay misiones de fallo actualmente (Failed existe para uso futuro).
// Para añadir una misión: agregar entrada en initQuests() y el caso en checkQuestProgress().

#include <string>
#include <vector>
#include <functional>

struct QuestObjective {
    std::string description;
    bool completed;
};

enum class QuestStatus {
    InProgress,
    Completed,
    Failed,    // reservado para uso futuro
};

struct Quest {
    std::string id;           // identificador único usado en checkQuestProgress()
    std::string title;
    std::string description;
    QuestStatus status;
    std::vector<QuestObjective> objectives;
    int xpReward;
    int goldReward;

    bool allObjectivesComplete() const;
};
