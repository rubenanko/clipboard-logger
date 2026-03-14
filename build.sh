#!/bin/bash

# Script de build pour clipboard-logger
# Utilise x86_64-w64-mingw32-gcc pour générer une DLL 64-bit compatible Windows.

# Nom du fichier de sortie
OUTPUT_DLL="clipboard-logger.dll"
SOURCE_FILE="clipboard-logger.c"

# Options de compilation :
# -shared : Générer une DLL
# -s : Retirer les symboles de debug pour réduire la taille
# -ffunction-sections -fdata-sections : Optimiser les sections
# -Wl,--gc-sections : Nettoyer les sections inutilisées
# -luser32 -lshell32 -lkernel32 : Bibliothèques Windows nécessaires
# -static-libgcc : Inclure statiquement la libgcc pour éviter les dépendances externes

echo "Compilation de $SOURCE_FILE vers $OUTPUT_DLL..."

x86_64-w64-mingw32-gcc -shared -o $OUTPUT_DLL $SOURCE_FILE \
    -s -ffunction-sections -fdata-sections -Wl,--gc-sections \
    -luser32 -lshell32 -lkernel32 -luser32 \
    -static-libgcc

if [ $? -eq 0 ]; then
    echo "Build réussi : $OUTPUT_DLL"
else
    echo "Erreur lors de la compilation."
    exit 1
fi
