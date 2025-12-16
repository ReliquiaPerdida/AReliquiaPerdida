#!/bin/bash

# Este script automatiza a instalacao de dependencias e a compilação do jogo.

echo "--- 1. Atualizando listas de pacotes e instalando dependencias do SDL2 (sera solicitada a sua senha se necessario) ---"

# Instala as bibliotecas de desenvolvimento SDL2 e suas extensoes.
# O -y aceita automaticamente a instalacao para nao precisar de confirmacao.
sudo apt update
sudo apt install -y libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev

if [ $? -ne 0 ]; then
    echo "ERRO: A instalacao de dependencias falhou. Verifique sua conexao e se o 'sudo' esta configurado corretamente."
    exit 1
fi

echo "--- 2. Compilando o codigo C (reliquia.c) ---"

# Comando de compilacao com pkg-config que funcionou para o seu projeto.
gcc -o reliquia reliquia.c $(pkg-config --cflags --libs sdl2 SDL2_image SDL2_ttf SDL2_mixer)

# Verifica se a compilacao foi bem-sucedida (o $? armazena o codigo de saida do ultimo comando)
if [ $? -ne 0 ]; then
    echo "ERRO: Falha na compilacao. Verifique se o arquivo reliquia.c existe e se o comando 'gcc' esta disponivel."
    exit 1
fi

echo "--- 3. Executando o jogo (./reliquia) ---"

# Executa o jogo compilado.
./reliquia

echo "--- Processo finalizado. ---"
