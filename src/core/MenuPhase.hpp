#pragma once

// Sub-fases del estado MainMenu. El flujo normal es lineal:
//   Title → NameInput → ClassSelect → HudSelect → (inicia partida)
// Credits es accesible desde Title y vuelve a Title con ESC.
// ESC en cualquier fase retrocede a la fase anterior.
enum class MenuPhase {
    Title,       // pantalla de inicio con opciones (Continuar/Nueva/Créditos/Salir)
    NameInput,   // campo de texto para el nombre del personaje (máx 16 chars)
    ClassSelect, // selección de clase con retrato (Guerrero/Mago/Ranger)
    HudSelect,   // selección de layout de HUD (Sidebar o Bottom bar)
    Credits,     // pantalla de créditos
};
