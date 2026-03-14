# Clipboard Logger DLL

Ce projet implémente une bibliothèque dynamique (DLL) pour Windows, conçue pour surveiller les changements du presse-papiers et enregistrer tout contenu textuel dans un fichier log situé sur le bureau de l'utilisateur.

## Caractéristiques

- **Surveillance en temps réel** : Utilise l'API `AddClipboardFormatListener` pour une détection efficace des mises à jour du presse-papiers.
- **Log Automatique** : Concatène le texte capturé dans `clipboard_log.txt` sur le bureau avec un horodatage précis.
- **Compatibilité Injection** : Conçue pour être injectée via mappage manuel (compatible avec `dll-injector`).
- **Légèreté** : Implémentation en C pur avec des dépendances minimales.

## Structure du Projet

- `clipboard-logger.c` : Code source principal de la DLL.
- `build.sh` : Script de compilation utilisant `x86_64-w64-mingw32-gcc`.
- `README.md` : Documentation du projet.

## Compilation

Pour compiler la DLL sous un environnement Linux (comme Ubuntu) avec MinGW-w64 installé :

```bash
./build.sh
```

Cela générera le fichier `clipboard-logger.dll`.

## Utilisation avec dll-injector

Cette DLL peut être injectée dans n'importe quel processus Windows (ex: `notepad.exe`) en utilisant l'outil `dll-injector`. Une fois injectée, elle crée un thread de surveillance invisible qui fonctionne en arrière-plan.

## Compatibilité

- Windows Vista et versions ultérieures (testé pour Windows 11 25H2).
- Architecture x86_64.
