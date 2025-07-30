# Analyse der Nutzerfreundlichkeit & Usability - WinVimTiler

Dieses Dokument ergänzt die technische Performance-Analyse und konzentriert sich auf Optimierungen der Nutzerfreundlichkeit (User Friendliness) und allgemeinen Benutzbarkeit (Usability).

## 💡 Usability-Verbesserungen

### 1. **Konfigurationsdatei für Flexibilität**
- **Problem**: Hotkeys und Verhalten sind fest im Code verankert (`hardcoded`). Der Nutzer kann keine Anpassungen vornehmen.
- **Lösung**: Einführung einer Konfigurationsdatei (z.B. `config.ini` oder `config.json`), in der Nutzer folgendes anpassen können:
    - **Hotkeys**: Die Tastenkombinationen für alle Aktionen.
    - **Ignorierte Anwendungen**: Eine Liste von Prozessen (z.B. `Spotify.exe`, `widgets.exe`), die vom Tiling ausgeschlossen werden sollen.
    - **Fenster-Abstände (Gaps)**: Eine Option, um einen kleinen Abstand zwischen den Fenstern zu lassen.
- **Nutzen**: Enorme Steigerung der Flexibilität und Personalisierbarkeit.

### 2. **System-Tray-Icon für bessere Kontrolle**
- **Problem**: Die Anwendung läuft unsichtbar im Hintergrund. Es gibt keine einfache Möglichkeit, sie zu beenden oder zu pausieren, außer über den Task-Manager.
- **Lösung**: Implementierung eines Icons im System-Tray (Benachrichtigungsfeld der Taskleiste) mit einem Kontextmenü, das folgende Optionen bietet:
    - **Tiling aktivieren/deaktivieren**: Zum temporären Pausieren der Funktionalität.
    - **Einstellungen bearbeiten**: Öffnet die Konfigurationsdatei.
    - **Hilfe**: Zeigt eine kurze Übersicht der Hotkeys an.
    - **Beenden**: Schließt die Anwendung ordnungsgemäß.
- **Nutzen**: Macht die Anwendung greifbarer und einfacher zu verwalten.

### 3. **Persistenz der Fensterzustände**
- **Problem**: Wenn die Anwendung neu gestartet wird, gehen alle Fensterpositionen und -zustände verloren.
- **Lösung**: Speichern der `windowStates`-Map in einer Datei beim Beenden und Laden beim Start.
- **Nutzen**: Verbessert die Kontinuität der Arbeitsumgebung des Nutzers.

### 4. **Verbessertes Fehler-Handling**
- **Problem**: Fehler werden über `MessageBoxW` angezeigt. Dies ist bei einem Hintergrund-Tool sehr störend und unterbricht den Workflow.
- **Lösung**: Fehler in eine Log-Datei schreiben (z.B. `wintile.log`). Kritische Fehler könnten einmalig als Tray-Benachrichtigung angezeigt werden.
- **Nutzen**: Weniger aufdringlich, professionelleres Verhalten der Anwendung.

### 5. **Visuelles Feedback**
- **Problem**: Das Verschieben des Cursors ist das einzige Feedback. Bei schnellen Aktionen ist es nicht immer klar, ob der Befehl erfolgreich war.
- **Lösung**: Optionales, kurzes Aufleuchten eines farbigen Rahmens um das betroffene Fenster, um die Snap-Aktion visuell zu bestätigen. Die Farbe und Dauer könnte in der Konfigurationsdatei einstellbar sein.
- **Nutzen**: Gibt dem Nutzer eine sofortige und klare Rückmeldung.

### 6. **Onboarding für neue Nutzer**
- **Problem**: Ein neuer Nutzer weiß nicht, wie die Anwendung funktioniert oder welche Hotkeys er verwenden muss.
- **Lösung**: Eine einmalige Willkommensnachricht beim ersten Start, die die Grundfunktionen und die Standard-Hotkeys erklärt. Diese könnte auch auf die Konfigurationsdatei und das Tray-Icon hinweisen.
- **Nutzen**: Erleichtert den Einstieg und die Einarbeitung erheblich.

## 📊 Prioritäten-Ranking (Usability)

| Feature | Geschätzter Nutzen | Implementierungs-Aufwand |
|-----------------------------|----------------------|------------------------|
| Konfigurationsdatei         | Sehr Hoch            | Mittel                 |
| System-Tray-Icon            | Sehr Hoch            | Mittel                 |
| Verbessertes Fehler-Handling| Hoch                 | Niedrig                |
| Visuelles Feedback          | Mittel               | Mittel                 |
| Onboarding                  | Mittel               | Niedrig                |
| Persistenz der Zustände     | Niedrig              | Mittel                 |

### **Empfohlene nächste Schritte:**
1.  **Konfigurationsdatei**: Dies ist die wichtigste Verbesserung, da sie die Grundlage für viele andere Anpassungen legt.
2.  **System-Tray-Icon**: Bietet die notwendige Kontrolle über die Anwendung.
3.  **Fehler-Logging**: Eine schnelle und einfache Verbesserung mit großem Nutzen.
