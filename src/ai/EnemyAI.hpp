#pragma once

// EnemyAI — thread de inteligencia artificial de los enemigos.
// run() se ejecuta en un hilo separado (iniciado en Game::aiLoop()).
// Ciclo: duerme 600ms → mueve enemigos → activa pendingRedraw_.
// Toda lectura/escritura de worldEnemies_ y map_ va bajo worldMutex_.
// Si un enemigo alcanza al jugador, escribe pendingCombatEnemy_ y el hilo
// principal inicia el combate en el siguiente frame.

class Game;

class EnemyAI {
public:
    static void run(Game& g);
};
