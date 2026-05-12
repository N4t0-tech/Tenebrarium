#pragma once

// Sub-fases del estado MainMenu. El flujo normal es lineal:
//   Title → NameInput → ClassSelect → HudSelect → (inicia partida)
// Credits es accesible desde Title y vuelve a Title con ESC.
// ESC en cualquier fase retrocede a la fase anterior.
enum class MenuPhase {
    Title,
    NameInput,
    ClassSelect,
    HudSelect,
    Credits,
    Settings,
};
