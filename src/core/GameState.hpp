#pragma once

// Estados principales de la máquina de estados del juego.
// Las transiciones están centralizadas en Game::setState() y Game::dispatchInput().
// Diagrama resumido:
//   MainMenu → Village → Exploration → Combat → Exploration (o GameOver)
//   Exploration → Inventory / QuestLog / Bestiary / QuitDialog → Exploration
//   Village → Shop / Inventory / QuestLog / Bestiary (Forge/Training son submenús)
//   Exploration → Village (escalera de subida, solo con el piso limpio)
//   GameOver → MainMenu
enum class GameState {
    MainMenu,    // menú inicial + flujo de creación de personaje (ver MenuPhase)
    Village,     // pueblo hub — base segura con tienda, forja y entrenamiento
    Exploration, // movimiento en el mapa, interacción con cofres/escaleras
    Combat,      // sistema de combate por turnos
    Inventory,   // pantalla de mochila y equipo
    QuestLog,    // diario de misiones
    Bestiary,    // bestiario
    GameOver,    // derrota o victoria (victory_ distingue cuál)
    Shop,        // tienda del pueblo
    QuitDialog,  // confirmación de salida (ESC cancela sin regenerar el mapa)
};
