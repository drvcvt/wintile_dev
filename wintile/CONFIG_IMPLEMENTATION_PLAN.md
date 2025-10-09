# Aufgabenplan: Konfigurierbare Config-Datei & True-Fullscreen

Dieser Aufgabenplan bricht die Implementierung in klar abgegrenzte Tasks auf. Jeder Task kann einzeln gestartet werden und baut – falls notwendig – auf vorherigen Schritten auf.

---

## Task 1 – Infrastruktur vorbereiten
**Ziel:** JSON-Abhängigkeit bereitstellen und Build-System anpassen.

### Teilaufgaben
- [ ] `nlohmann/json` als Header-only-Bibliothek in `third_party/` ablegen oder als Submodule hinzufügen.
- [ ] Sicherstellen, dass `CMakeLists.txt` den Include-Pfad für die JSON-Bibliothek setzt.
- [ ] Optional: Compiler-Definition `-DNOMINMAX` ergänzen, um Konflikte mit Windows-Makros auszuschließen.

### Voraussetzungen
Keine.

---

## Task 2 – Config-Datenmodell anlegen
**Ziel:** Kernstrukturen und Zugriffslogik definieren.

### Teilaufgaben
- [ ] Dateien `Config.h` und `Config.cpp` anlegen und im Projekt registrieren.
- [ ] Datenstrukturen erstellen:
  - `struct BorderConfig { bool enabled; COLORREF focusColor; };`
  - `struct HotkeyBinding { UINT modifiers; UINT key; };`
  - `enum class Action { SnapLeft, SnapRight, SnapUp, SnapDown, MonitorLeft, MonitorRight, TrueFullscreen, ToggleBorders, ... };`
  - `struct Config { BorderConfig border; std::unordered_map<Action, HotkeyBinding> hotkeys; };`
- [ ] Singleton/Accessor implementieren (`Config& GetConfig()`), der beim ersten Zugriff lädt und cached.

### Voraussetzungen
Task 1.

---

## Task 3 – Config-Datei laden & speichern
**Ziel:** `config.json` mit Defaults verwalten.

### Teilaufgaben
- [ ] Programmpfad ermitteln (`GetModuleFileName`, `std::filesystem`) und `config.json` neben der EXE lokalisieren.
- [ ] Default-Werte definieren und beim ersten Start eine Default-Datei schreiben.
- [ ] JSON-Struktur parsen (`{ "border": { ... }, "hotkeys": { ... } }`) und pro Feld auf Defaults zurückfallen, wenn Werte fehlen.
- [ ] (Optional) `ReloadConfig()` vorsehen, um Config neu einzulesen.

### Voraussetzungen
Tasks 1 & 2.

---

## Task 4 – Fokusrahmen an Config koppeln
**Ziel:** Border-Rendering dynamisch anhand der Config steuern.

### Teilaufgaben
- [ ] In `CreateOrUpdateBorder` und `ShouldWindowHaveBorder` prüfen, ob `config.border.enabled` gesetzt ist.
- [ ] `WM_PAINT`-Handler auf `config.border.focusColor` umstellen.
- [ ] Beim Deaktivieren der Borders vorhandene Border-Fenster zerstören (`DestroyWindow`).
- [ ] Optional: Hotkey `Action::ToggleBorders` hinzufügen, der das Flag flippt und `UpdateAllBorders()` ausführt.

### Voraussetzungen
Tasks 2 & 3.

---

## Task 5 – Hotkey-System refaktorieren
**Ziel:** Hotkeys aus der Config laden und dynamisch verwalten.

### Teilaufgaben
- [ ] Feste `HOTKEY_ID_*`-Defines durch dynamische IDs ersetzen (`UINT nextHotkeyId`, `std::unordered_map<UINT, Action>`).
- [ ] Beim Config-Laden Hotkeys parsen, registrieren und in Lookup-Map ablegen; beim Reload zunächst deregistrieren.
- [ ] `WndProc` auf Lookup (`hotkeyLookup`) umstellen und `HandleAction(Action)` aufrufen.
- [ ] Parser `ParseHotkeyString(std::string_view, HotkeyBinding&)` implementieren (unterstützt `CTRL`, `ALT`, `SHIFT`, `WIN`, Funktionstasten, Pfeiltasten, Buchstaben; robustes Fehlerlogging).

### Voraussetzungen
Tasks 2 & 3.

---

## Task 6 – True-Fullscreen implementieren
**Ziel:** Eigenen Fullscreen-Modus unabhängig vom Layout bereitstellen.

### Teilaufgaben
- [ ] `struct FullscreenState { DWORD style; DWORD exStyle; RECT rect; bool active; };` und Mapping `std::unordered_map<HWND, FullscreenState>` einführen.
- [ ] Aktivierung: Fokusfenster ermitteln, Stil & Position sichern, Fenster auf Monitorfläche setzen (`MonitorFromWindow`, `GetMonitorInfo`, `SetWindowLong`, `SetWindowPos` mit `SWP_FRAMECHANGED`).
- [ ] Deaktivierung: ursprüngliche Werte zurücksetzen und Fensterposition wiederherstellen.
- [ ] Border- und Layout-Logik anpassen (True-Fullscreen-Fenster bekommen keine Border und werden vom Auto-Tiling ausgenommen).

### Voraussetzungen
Tasks 3, 4 & 5.

---

## Task 7 – Dokumentation & Beispiel-Config
**Ziel:** Nutzer über neue Config informieren und Start erleichtern.

### Teilaufgaben
- [ ] README um Abschnitt zur `config.json` ergänzen (Pfad, Schema, Hotkey-Aktionen, Farbangaben als Hex).
- [ ] Beispiel-Config (`config.example.json` o. Ä.) mit Defaults bereitstellen und im Code optional beim ersten Start erzeugen.

### Voraussetzungen
Tasks 3, 4 & 5.

---

## Task 8 – Tests & Verifikation
**Ziel:** Regressionen vermeiden und neue Features validieren.

### Teilaufgaben
- [ ] `cmake --build .` ausführen und sicherstellen, dass der Build erfolgreich ist.
- [ ] Manuelle Tests: Hotkeys, Border-Toggle, Farbeinstellungen, True-Fullscreen (An/Aus, Interaktion mit Layouts).
- [ ] Fehlerhafte Config testen (z. B. ungültige Farbe) und Fallback/Logging prüfen.

### Voraussetzungen
Tasks 1–7 (für vollständige Verifikation).

---

## Optionaler Task 9 – Runtime-Reload der Config
**Ziel:** Config ohne Neustart neu laden können.

### Teilaufgaben
- [ ] Aktion `Action::ReloadConfig` definieren und Hotkey anlegen.
- [ ] `ReloadConfig()` implementieren, Hotkeys neu registrieren und Border-Status aktualisieren.
- [ ] Sicherstellen, dass True-Fullscreen-States konsistent bleiben.

### Voraussetzungen
Tasks 3 & 5.

