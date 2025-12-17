#include <assert.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h> 
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <time.h>    
#include <string.h> 
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const int WINDOW_WIDTH = 1366; 
const int WINDOW_HEIGHT = 768;
// --- Configurações do Mapa ---
#define MAP_WIDTH_TILES 30
#define MAP_HEIGHT_TILES 30
#define TILE_SIZE 90
#define RELIC_TILE 4
#define MUMMY_COUNT 3
#define DANCER_COUNT 99
#define PLAYER_WIDTH 90
#define PLAYER_HEIGHT 70
#define MIN_SPAWN_DIST_TILES 5 

// --- Configurações de Fases ---
#define NUM_AREAS 3 

// --- Configurações dos Tesouros ---
#define NUM_TREASURES_TO_SPAWN 6 

// Configurações de Vidas (2 vidas)
int max_lives_total = 2;

// --- CONSTANTES PARA ANIMAR JOGADOR ---
const int PLAYER_FRAME_W = 100;
const int PLAYER_FRAME_H = 80;  

// --- CONSTANTES DE COLISAO DO JOGADOR ---
#define HITBOX_OFFSET_X 0
#define PLAYER_HITBOX_W 40
#define PLAYER_HITBOX_H 70 
#define HITBOX_OFFSET_Y 0

// --- MACROS E CONSTANTES DE RENDERIZAÇÃO ---
const int MUMIA_FRAME_W = 48;
const int MUMIA_FRAME_H = 64; 
const int DANCA_FRAME_W = 23; // Largura do frame da Dançarina
const int DANCA_FRAME_H = 35; // Altura do frame da Dançarina

typedef enum {
    DIREITA = 1,
    ESQUERDA = -1
} DirecaoVisual;

// --- ESTADO DO JOGO ---
typedef enum {
    GAME_MENU,      
    GAME_PLAYING,
    GAME_OVER,
    GAME_VICTORY,
    GAME_EXITING
} GameState;

typedef enum {
    PARADO,
    ANDANDO,
    CORRENDO,
    PULANDO,
    ATACANDO
} EstadoMovimento;

typedef enum {
    DANCANDO,
    HIPNOTIZANDO,
    ATORDOADA,
    PARADA
} EstadoDancarina;

typedef enum {
    MUMIA_DORMINDO,
    MUMIA_PERSEGUINDO,
    MUMIA_CONFUSA,
    MUMIA_ENROLANDO,
    MUMIA_ATORDOADA
} EstadoMumia;

typedef enum {
    TESOURO_TOCHA,
    TESOURO_CALICE_SAGRADO,
    TESOURO_BUSSOLA, 
    TESOURO_ESTATUETA
} TipoTesouro;

typedef struct {
    int x, y;
    int vida;
    int vidas;
    int alcanceAtaque;
    int danoAtaque;
    bool hipnotizado;
    bool enrolado;
    Uint32 tempoEstado;
} Jogador;

typedef struct {
    EstadoDancarina estadoAtual;
    int x, y;
    int w, h;
    int vida;
    int alcanceVisaoQuadrado;
    int raioHipnoseQuadrado;
    Uint32 tempoEstado;
    int deslocamento;
    int direcaoDanca;
    bool alvo_hipnotizado;
    int dirX, dirY; 
    SDL_Rect frameRecorte; 
    int direcaoVisual;
} Dancarina;

typedef struct {
    EstadoMumia estado;
    int x, y;
    int w, h;
    int vida;
    int alcanceVisao2;
    int distanciaEnrolar2;
    int danoAtaque;
    Uint32 tempoEstado;
    int dirX, dirY;
} Mumia;

typedef struct {
    int x, y;
    int w, h;
    bool coletada[NUM_AREAS]; 
} GlobalReliquia;

typedef struct {
    TipoTesouro tipo;
    int x, y;
    int w, h;
    bool coletado;
} Tesouro;

// --- MAPAS ---

// Mapa 0 
int map0[15][30] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,2,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,3,1},
    {1,1,1,0,1,1,1,0,1,0,1,1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,0,0,0,0,1,0,1},
    {1,0,1,1,1,1,1,0,1,1,1,0,1,1,1,1,0,1,0,1,0,1,0,1,1,1,0,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,1,0,1},
    {1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,0,1,1,0,1,0,1,1,1,0,1,0,1,0,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1},
    {1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,4,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// Mapa 1 
int map1[15][30] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,1},
    {1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1},
    {1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1},
    {1,0,1,0,1,1,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1},
    {1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,0,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// Mapa 2
int map2[15][30] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,1},
    {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1},
    {1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1},
    {1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1},
    {1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// Array de ponteiros para os mapas
int (*ALL_MAPS[NUM_AREAS])[MAP_WIDTH_TILES] = {map0, map1, map2}; 

// Variável que armazena o mapa atualmente em uso
int (*current_map)[MAP_WIDTH_TILES];

// Variável para a fase atual do jogo
int current_phase = 0;


// --- FUNÇÕES AUXILIARES COM O MAPA ---

bool is_colliding(int x, int y, int w, int h) {
    int left_tile = (x + 5) / TILE_SIZE;
    int right_tile = (x + w - 5) / TILE_SIZE;
    int top_tile = (y + 5) / TILE_SIZE;
    int bottom_tile = (y + h - 5) / TILE_SIZE;

    for (int i = top_tile; i <= bottom_tile; i++) {
        for (int j = left_tile; j <= right_tile; j++) {
            if (i < 0 || i >= MAP_HEIGHT_TILES || j < 0 || j >= MAP_WIDTH_TILES) {
                return true; 
            }
            if (current_map[i][j] == 1) { // Usa o mapa atual
                return true; 
            }
        }
    }
    return false;
}

void update_position(int* x, int* y, int dx, int dy) {
    // Tenta mover em X
    int new_x = *x + dx;
    if (!is_colliding(new_x + HITBOX_OFFSET_X, *y + HITBOX_OFFSET_Y, PLAYER_HITBOX_W, PLAYER_HITBOX_H)) { 
        *x = new_x;
    }

    // Tenta mover em Y
    int new_y = *y + dy;
    if (!is_colliding(*x + HITBOX_OFFSET_X, new_y + HITBOX_OFFSET_Y, PLAYER_HITBOX_W, PLAYER_HITBOX_H)) { 
        *y = new_y;
    }
}

// animar a dancarina
void aux_dancarina(Dancarina* danca) {
    if (danca->direcaoDanca > 0) {
        danca->direcaoVisual = DIREITA;
    } else if (danca->direcaoDanca < 0) {
        danca->direcaoVisual = ESQUERDA;
    }
    
    danca->frameRecorte.w = DANCA_FRAME_W;
    danca->frameRecorte.h = DANCA_FRAME_H;
    danca->frameRecorte.x = 0; 

    switch (danca->estadoAtual) {
        case HIPNOTIZANDO:
            danca->frameRecorte.y = 2 * DANCA_FRAME_H; // Linha 2: Centralizada/Hipnose
            break;
            
        case DANCANDO:
            if (danca->direcaoVisual == DIREITA) {
                danca->frameRecorte.y = 1 * DANCA_FRAME_H; // Linha 1: Direita
            } else { 
                danca->frameRecorte.y = 3 * DANCA_FRAME_H; // Linha 3: Esquerda
            }
            break;

        case ATORDOADA:
            danca->frameRecorte.y = 2 * DANCA_FRAME_H; // Linha 2: Atordoada
            break;

        case PARADA:
        default:
            danca->frameRecorte.y = 0 * DANCA_FRAME_H; // Linha 0: Costas/Parada
            break;
    }
}


// --- Encontrar Posição de Spawn Válida (usa current_map implicitamente) ---
void find_valid_spawn(int* out_x, int* out_y, int entity_w, int entity_h, int player_start_x, int player_start_y) {
    int max_attempts = 1000;
    int attempts = 0;

    while (attempts < max_attempts) {
        // Escolhe um tile aleatório (evita bordas externas de parede 1)
        int rand_tile_x = 1 + rand() % (MAP_WIDTH_TILES - 2);
        int rand_tile_y = 1 + rand() % (MAP_HEIGHT_TILES - 2);

        // Se o tile for caminho livre (0)
        if (current_map[rand_tile_y][rand_tile_x] == 0) { // Usa o mapa atual
            
            // Calcula a posição real em pixels
            int potential_x = rand_tile_x * TILE_SIZE + (TILE_SIZE - entity_w) / 2;
            int potential_y = rand_tile_y * TILE_SIZE + (TILE_SIZE - entity_h) / 2;

            //  Verifica se não está dentro de uma parede
            if (!is_colliding(potential_x, potential_y, entity_w, entity_h)) {
                
                //  Verifica a distância mínima do jogador
                int dx = potential_x - player_start_x;
                int dy = potential_y - player_start_y;
                int dist2 = dx*dx + dy*dy;
                
                // Distância mínima em pixels (MIN_SPAWN_DIST_TILES * TILE_SIZE)^2
                if (dist2 > (MIN_SPAWN_DIST_TILES * TILE_SIZE) * (MIN_SPAWN_DIST_TILES * TILE_SIZE)) {
                    *out_x = potential_x;
                    *out_y = potential_y;
                    return;
                }
            }
        }
        attempts++;
    }
    *out_x = player_start_x + 500; 
    *out_y = player_start_y;
}

// Função auxiliar para renderizar texto
void render_text(SDL_Renderer* ren, TTF_Font* font, const char* text, int x, int y, SDL_Color color) {
    if (font == NULL || text == NULL || strlen(text) == 0) return;

    SDL_Surface* surface = TTF_RenderText_Blended(font, text, color);
    if (surface == NULL) {
        fprintf(stderr, "Erro ao renderizar texto: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(ren, surface);
    if (texture == NULL) {
        fprintf(stderr, "Erro ao criar textura de texto: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect dstRect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(ren, texture, NULL, &dstRect);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

// --- FUNÇÃO DE BÚSSOLA ---
const char* obter_direcao_reliquia(int px, int py, int rx, int ry) {
    int dx = rx - px;
    int dy = ry - py;

    // Calcula o ângulo em graus (-180 a 180)
    double angulo = atan2(dy, dx) * (180.0 / M_PI);

    // Ajuste para 8 direções (45 graus cada fatia, centradas)
    if (angulo >= -22.5 && angulo < 22.5) return "Leste (Direita)";
    if (angulo >= 22.5 && angulo < 67.5) return "Sudeste";
    if (angulo >= 67.5 && angulo < 112.5) return "Sul (Baixo)";
    if (angulo >= 112.5 && angulo < 157.5) return "Sudoeste";
    if (angulo >= -67.5 && angulo < -22.5) return "Nordeste";
    if (angulo >= -112.5 && angulo < -67.5) return "Norte (Cima)";
    if (angulo >= -157.5 && angulo < -112.5) return "Noroeste";
    
    return "Oeste (Esquerda)"; // Restante do círculo
}

// --- FUNÇÃO DE INICIALIZAÇÃO DE FASE ---
void initialize_phase(int phase_index, Jogador* jogador, Mumia mummies[], Dancarina dancers[], Tesouro treasures[], GlobalReliquia* relic, bool* visaoExtra, bool* possuiBussola, char current_message[], Uint32* message_start_time) {
    
    current_map = ALL_MAPS[phase_index]; // Define o mapa atual
    
    // 1. Encontrar a Posição Inicial (Tile '2') do NOVO mapa
    int start_x = 0, start_y = 0;
    bool found_start = false;
    for (int i = 0; i < MAP_HEIGHT_TILES; i++) {
        for (int j = 0; j < MAP_WIDTH_TILES; j++) {
            if (current_map[i][j] == 2) {
                start_x = j * TILE_SIZE + (TILE_SIZE - PLAYER_WIDTH) / 2;
                start_y = i * TILE_SIZE + (TILE_SIZE - PLAYER_HEIGHT) / 2;
                found_start = true;
                break;
            }
        }
        if (found_start) break;
    }

    // 2. Teleportar Jogador (e resetar status temporários)
    jogador->x = start_x; 
    jogador->y = start_y;
    jogador->hipnotizado = false;
    jogador->enrolado = false;
    // Não cura totalmente para manter a progressão de dano, mas garante que não está morto.
    if (jogador->vida <= 20) jogador->vida = 50; 
    
    // 3. Inicializar Relíquia da Fase
    // Encontra o tile 4 NO NOVO MAPA
    bool found_relic = false;
    for (int i = 0; i < MAP_HEIGHT_TILES; i++) {
        for (int j = 0; j < MAP_WIDTH_TILES; j++) {
            if (current_map[i][j] == RELIC_TILE) {
                relic->x = j * TILE_SIZE + (TILE_SIZE - 30) / 2;
                relic->y = i * TILE_SIZE + (TILE_SIZE - 30) / 2;
                found_relic = true;
                break;
            }
        }
        if (found_relic) break;
    }

    // 4. Repopular/Respawnar Inimigos e Tesouros
    for(int i = 0; i < MUMMY_COUNT; i++) {
        int mx, my;
        find_valid_spawn(&mx, &my, 60, 60, jogador->x, jogador->y); 
        mummies[i] = (Mumia){
            .estado = MUMIA_DORMINDO, .x = mx, .y = my, .w = 60, .h = 60, .vida = 100,
            .danoAtaque = 20, .alcanceVisao2 = 200*200, .distanciaEnrolar2 = 50*50, .tempoEstado = 0,
            .dirX = (rand() % 2 == 0) ? 1 : -1, .dirY = (rand() % 2 == 0) ? 1 : -1
        };
    }

    for(int i = 0; i < DANCER_COUNT; i++) {
        int dx, dy;
        find_valid_spawn(&dx, &dy, 60, 60, jogador->x, jogador->y);
        dancers[i] = (Dancarina){
            .estadoAtual = PARADA, .x = dx, .y = dy, .w = 50, .h = 50, .vida = 100,
            .alcanceVisaoQuadrado = 200 * 200, .raioHipnoseQuadrado = 90 * 90,
            .tempoEstado = 0, .deslocamento = 0, .direcaoDanca = 1, .frameRecorte = {0, 0, DANCA_FRAME_W, DANCA_FRAME_H},
            .alvo_hipnotizado = false, .direcaoVisual = DIREITA, .dirX = (rand() % 2 == 0) ? 1 : -1, .dirY = 0
        };
    }
    
    // Tesouros são redistribuídos 
    for(int i = 0; i < NUM_TREASURES_TO_SPAWN; i++) {
        int tx, ty;
        find_valid_spawn(&tx, &ty, 30, 30, jogador->x, jogador->y); 
        treasures[i].x = tx;
        treasures[i].y = ty;
        treasures[i].coletado = false; // Reinicia a coleta
    }

    // Define os tipos dos 3 tesouros únicos
    treasures[0].tipo = TESOURO_TOCHA;
    treasures[0].w = 30; treasures[0].h = 30;
    
    treasures[1].tipo = TESOURO_CALICE_SAGRADO;
    treasures[1].w = 30; treasures[1].h = 30;

    treasures[2].tipo = TESOURO_BUSSOLA;
    treasures[2].w = 30; treasures[2].h = 30;

    // Configura os slots restantes (3, 4, 5) como Estatuetas
    for (int k = 3; k < NUM_TREASURES_TO_SPAWN; k++) {
        treasures[k].tipo = TESOURO_ESTATUETA;
        treasures[k].w = 30; 
        treasures[k].h = 30;
    }

    *visaoExtra = false; // Visão de tocha é resetada por fase.
    *possuiBussola = false; // Bússola é perdida ao mudar de fase
    max_lives_total = 2;
    
    snprintf(current_message, 256, "Entrando na Area %d! Colete a reliquia para avancar.", phase_index + 1);
    *message_start_time = SDL_GetTicks();
}

// --- Função de Reinicialização de Jogo ---
void reset_game(Jogador* jogador, GlobalReliquia* relic, bool* visaoExtra, bool* possuiBussola, char current_message[], Uint32* message_start_time, Uint32* game_start_time, Mumia mummies[], Dancarina dancers[], Tesouro treasures[]) {
    // Resetar variáveis de jogo
    current_phase = 0;
    jogador->vidas = max_lives_total;
    jogador->vida = 100;
    
    // Reinicializar relíquias
    for (int i = 0; i < NUM_AREAS; i++) {
        relic->coletada[i] = false;
    }
    
    // Reinicializa a primeira fase (que vai popular inimigos/tesouros)
    initialize_phase(current_phase, jogador, mummies, dancers, treasures, relic, visaoExtra, possuiBussola, current_message, message_start_time);
    
    // Reinicia o cronômetro do jogo
    *game_start_time = SDL_GetTicks();
}


int AUX_WaitEventTimeout(SDL_Event* evt, Uint32* ms) {
    Uint32 antes = SDL_GetTicks();
    int ret = SDL_WaitEventTimeout(evt, *ms);
    Uint32 depois = SDL_GetTicks();
    Uint32 d = depois - antes;
    
    if (ret) {
        if (d >= *ms)
            *ms = 0;
        else
            *ms -= d;
    }
    return ret;
}

int main(int argc, char* args[]) {
    srand(time(NULL));


    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
        fprintf(stderr, "SDL_mixer nao pode ser inicializado! Mix Error: %s\n", Mix_GetError());
    }

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        fprintf(stderr, "SDL não pôde ser inicializado! SDL Error: %s\n", SDL_GetError());
        return 1;
    }
    
    if (TTF_Init() == -1) {
        fprintf(stderr, "SDL_ttf não pôde ser inicializado! SDL_ttf Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    
    SDL_Window* win = SDL_CreateWindow("A Reliquia Perdida",
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    
    // Carregamento de Texturas 
    SDL_Texture* img = IMG_LoadTexture(ren, "anim2.png");
    SDL_Texture* img2 = IMG_LoadTexture(ren, "escuridao.png");
    SDL_Texture* img3 = IMG_LoadTexture(ren, "escuridao2.png");
    SDL_Texture* img4 = IMG_LoadTexture(ren, "bloco.png");
    SDL_Texture* img_fundo = IMG_LoadTexture(ren, "fundo.png");
    SDL_Texture* img_menu_fundo = IMG_LoadTexture(ren, "fundomenu.png");
    SDL_Texture* img_mumia = IMG_LoadTexture(ren, "mumia.png");
    SDL_Texture* img_dancarina = IMG_LoadTexture(ren, "dancarina.png");
    SDL_Texture* img_tocha = IMG_LoadTexture(ren, "tocha.png");
    SDL_Texture* img_calice = IMG_LoadTexture(ren, "calice.png");
    SDL_Texture* img_vida = IMG_LoadTexture(ren, "vida.png");
    SDL_Texture* img_vida2 = IMG_LoadTexture(ren, "vida2.png");
    SDL_Texture* img_estatueta = IMG_LoadTexture(ren, "estatueta.png");
    SDL_Texture* img_bussola = IMG_LoadTexture(ren, "bussola.png");
    SDL_Texture* img_reliquia = IMG_LoadTexture(ren, "reliquia.png");
    SDL_Texture* img_entrada = IMG_LoadTexture(ren, "entrada.png");
    SDL_Texture* mumia_img = IMG_LoadTexture(ren, "mumia48x64.png");
    SDL_Texture* danca_img = IMG_LoadTexture(ren, "dancarina23x35.png");
    SDL_Texture* img_menu_alt = IMG_LoadTexture(ren, "fundomenu2.png");

    Mix_Music* bgm_musica = Mix_LoadMUS("areliquiaperdida-davinunes.mp3");
    if (bgm_musica == NULL) {
        fprintf(stderr, "Falha ao carregar musica: %s\n", Mix_GetError());
    } else {
        Mix_VolumeMusic(25); // Define o volume para ~25% (Range: 0 a 128)
    }
    
    TTF_Font* font_small = TTF_OpenFont("tiny.ttf", 24); 
    TTF_Font* font_large = TTF_OpenFont("tiny.ttf", 48); 
    
    SDL_Cursor* cursor_mao = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    SDL_Cursor* cursor_seta = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);

    // const int BTN_W = 300;
    //const int BTN_H = 105;

   // SDL_Rect btn_opcoes = { 40, 940, BTN_W, BTN_H }; 
   // SDL_Rect btn_play = { 820, 946, BTN_W, BTN_H };
   // SDL_Rect btn_sair = { 1593, 948, BTN_W, BTN_H };
   const float ORIGIN_W = 1920.0f;
   const float ORIGIN_H = 1080.0f;
   float scale_x = (float)WINDOW_WIDTH / ORIGIN_W;
   float scale_y = (float)WINDOW_HEIGHT / ORIGIN_H;
   
   int btn_w_scaled = (int) (300 * scale_x);
   int btn_h_scaled = (int) (105 * scale_y);
  
   SDL_Rect btn_opcoes = { (int)(40 * scale_x), (int)(940 * scale_y), btn_w_scaled, btn_h_scaled};
   
      SDL_Rect btn_play = { (int)(820 * scale_x), (int)(946 * scale_y), btn_w_scaled, btn_h_scaled };
      SDL_Rect btn_sair = { (int)(1593 * scale_x), (int)(948 * scale_y), btn_w_scaled, btn_h_scaled };


    // Verificação de texturas e fontes
    assert(img != NULL && img2 != NULL && img3 != NULL && img4 != NULL && 
           img_fundo != NULL && img_menu_fundo != NULL && 
           img_mumia != NULL && img_dancarina != NULL &&
           img_tocha != NULL && img_calice != NULL && 
           img_vida != NULL && danca_img && danca_img && img_vida2 != NULL && img_entrada != NULL && img_estatueta != NULL && img_bussola != NULL && img_reliquia != NULL && img_menu_alt != NULL);
    assert(font_small != NULL && font_large != NULL);


    // --- Configuração da Câmera / HUD ---
    const int PLAYER_SCREEN_X = (WINDOW_WIDTH - PLAYER_WIDTH) / 2;
    const int PLAYER_SCREEN_Y = (WINDOW_HEIGHT - PLAYER_HEIGHT) / 2;
    // 0=Baixo, 1=Cima, 2=Esquerda, 3=Direita
    int direcaoOlhar = 0;

    SDL_Rect r = {PLAYER_SCREEN_X, PLAYER_SCREEN_Y, PLAYER_WIDTH, PLAYER_HEIGHT};
    SDL_Rect c = {0, 0, 100, 80};
    SDL_Rect visao = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_Rect rFundo = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};

    // --- Jogador ---
    Jogador jogador = {
        .x = 0, .y = 0, 
        .vida = 100,
        .vidas = max_lives_total, 
        .danoAtaque = 20,
        .alcanceAtaque = 100 * 100,
        .hipnotizado = false,
        .enrolado = false,
        .tempoEstado = 0
    };


    // --- RELÍQUIA Global ---
    GlobalReliquia global_relic = {0};
    global_relic.w = 30;
    global_relic.h = 30;
    
    // --- INIMIGOS ---
    Mumia mummies[MUMMY_COUNT] = {0};
    Dancarina dancers[DANCER_COUNT] = {0};

    // --- TESOUROS ---
    Tesouro treasures[NUM_TREASURES_TO_SPAWN] = {0};
    
    // Variáveis de Controle
    int vel = 1;
    bool noChao = true;
    int chao = jogador.y; 
    bool subindo = true;
    int aux = 2;
    int k = 0;
    bool visaoExtra = false;
    bool possuiBussola = false; // NOVA VARIÁVEL
    EstadoMovimento estado = PARADO;
    
    // --- VARIÁVEIS DE ESTADO E TEMPO ---
    GameState game_state = GAME_MENU; 
    Uint32 game_start_time = 0; 
    Uint32 game_end_time = 0; 
    int selected_option = 0; // 0: Novo Jogo/Jogar Novamente, 1: Sair do Jogo
    
    char current_message[256] = ""; 
    Uint32 message_start_time = 0; 
    const Uint32 MESSAGE_DURATION = 3000; 

    bool rodando = true;
    SDL_Event evt;
    Uint32 espera = 10;

    while (rodando) {

        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        SDL_Point mouse_pos = { mouseX, mouseY };
        int isevt;



        if (game_state == GAME_PLAYING) { isevt = AUX_WaitEventTimeout(&evt, &espera); } else { isevt = SDL_PollEvent(&evt); }
        const Uint8* teclas = SDL_GetKeyboardState(NULL);

        
        if (isevt) {
            if (evt.type == SDL_QUIT) rodando = false;

             if (game_state == GAME_MENU) {
                if (evt.type == SDL_MOUSEBUTTONDOWN) {
                   if (SDL_PointInRect(&mouse_pos, &btn_play)) {
                      reset_game(&jogador, &global_relic, &visaoExtra, &possuiBussola, current_message, &message_start_time, &game_start_time, mummies, dancers, treasures);
                      game_state = GAME_PLAYING;
                      
                      // A música começa a tocar em loop AQUI, quando o jogo é iniciado.
                      if (bgm_musica != NULL && !Mix_PlayingMusic()) {
                          Mix_PlayMusic(bgm_musica, -1); // -1 é para loop infinito
                    }
                    
                }
                else if (SDL_PointInRect(&mouse_pos, &btn_sair)) { rodando = false; }
             }
           } 
            
            if (game_state == GAME_PLAYING) {
                if (evt.type == SDL_KEYDOWN) {if (evt.key.keysym.sym == SDLK_F1) {
                        game_end_time = SDL_GetTicks();
                        game_state = GAME_VICTORY;
                    }
                    if(!jogador.hipnotizado && !jogador.enrolado) {
                        switch (evt.key.keysym.sym) {
                            case SDLK_LSHIFT: if(noChao) estado = CORRENDO; break;
                            case SDLK_UP: case SDLK_DOWN: case SDLK_LEFT: case SDLK_RIGHT:
                                if(estado != CORRENDO && estado != PULANDO && noChao)
                                    estado = ANDANDO;
                                break;
                        }
                    }
                } else if (evt.type == SDL_KEYUP) {
                    if(!jogador.hipnotizado && !jogador.enrolado && noChao) {
                        if(evt.key.keysym.sym == SDLK_LSHIFT) {
                            if(teclas[SDL_SCANCODE_LEFT] || teclas[SDL_SCANCODE_RIGHT] ||
                               teclas[SDL_SCANCODE_UP] || teclas[SDL_SCANCODE_DOWN])
                                 estado = ANDANDO;
                            else
                                 estado = PARADO;
                        }
                        if(evt.key.keysym.sym == SDLK_UP || evt.key.keysym.sym == SDLK_DOWN ||
                           evt.key.keysym.sym == SDLK_LEFT || evt.key.keysym.sym == SDLK_RIGHT) {
                            if(teclas[SDL_SCANCODE_LEFT] || teclas[SDL_SCANCODE_RIGHT] ||
                               teclas[SDL_SCANCODE_UP] || teclas[SDL_SCANCODE_DOWN]) {
                                if(teclas[SDL_SCANCODE_LSHIFT])
                                    estado = CORRENDO;
                                else
                                    estado = ANDANDO;
                            } else
                                estado = PARADO;
                        }
                    }
                }
            }
            else if (game_state == GAME_MENU) {
                if (SDL_PointInRect(&mouse_pos, &btn_play) || SDL_PointInRect(&mouse_pos, &btn_sair) || SDL_PointInRect(&mouse_pos, &btn_opcoes)) {
                    SDL_SetCursor(cursor_mao);
                } else {
                    SDL_SetCursor(cursor_seta);
                }
            }
            
            else if (game_state == GAME_VICTORY || game_state == GAME_OVER) {
                // Lógica de input para TELA DE MENU/VITÓRIA/GAME OVER
                if (evt.type == SDL_KEYDOWN) {
                    switch (evt.key.keysym.sym) {
                        case SDLK_UP:
                            selected_option = (selected_option + 1) % 2; 
                            break;
                        case SDLK_DOWN:
                            selected_option = (selected_option + 1) % 2; 
                            break;
                        case SDLK_RETURN:
                        case SDLK_SPACE:
                            if (selected_option == 0) { // Novo Jogo / Jogar Novamente
                                reset_game(&jogador, &global_relic, &visaoExtra, &possuiBussola, current_message, &message_start_time, &game_start_time, mummies, dancers, treasures);
                                game_state = GAME_PLAYING;
                            } else { // Sair do Jogo
                                rodando = false;
                            }
                            break;
                    }
                }
            }
        }
        
        // --- ATUALIZAÇÃO DO JOGO (SOMENTE SE GAME_PLAYING) ---
        if (game_state == GAME_PLAYING) {
        
            // --- Lógica de Dano, Respawn e Game Over ---
            if (jogador.vida <= 0) {
                jogador.vidas--; // Perde uma vida
                
                if (jogador.vidas < 0) {
                    // FIM DO JOGO
                    game_state = GAME_OVER;
                    snprintf(current_message, 256, "!! GAME OVER !! Voce perdeu todas as vidas!");
                    message_start_time = SDL_GetTicks();
                } else {
                    snprintf(current_message, 256, "Voce perdeu uma vida! Vidas restantes: %d", jogador.vidas);
                    message_start_time = SDL_GetTicks();
                    // Reinicia a vida e reinicializa a fase atual (respawn no tile '2' do mapa atual)
                    jogador.vida = 100;
                    initialize_phase(current_phase, &jogador, mummies, dancers, treasures, &global_relic, &visaoExtra, &possuiBussola, current_message, &message_start_time);
                }
            }

            // --- LÓGICA DA RELÍQUIA (Coleta) ---
            if (!global_relic.coletada[current_phase]) {
            // Checa colisão entre Jogador e Relíquia da fase atual
               if (jogador.x < global_relic.x + global_relic.w &&
                 jogador.x + PLAYER_WIDTH > global_relic.x &&
                 jogador.y < global_relic.y + global_relic.h &&
                 jogador.y + PLAYER_HEIGHT > global_relic.y) 
              {
                global_relic.coletada[current_phase] = true;
                snprintf(current_message, 256, "Reliquia da Area %d Coletada! Encontre a Saida!", current_phase + 1);
                message_start_time = SDL_GetTicks();
              }
           }

            // --- LÓGICA DE COLETA DOS TESOUROS ---
            for(int i = 0; i < NUM_TREASURES_TO_SPAWN; i++) {
                Tesouro* t = &treasures[i];
                if (t->coletado) continue;

                // Checa colisão entre Jogador e Tesouro
                if (jogador.x < t->x + t->w &&
                    jogador.x + PLAYER_WIDTH > t->x &&
                    jogador.y < t->y + t->h &&
                    jogador.y + PLAYER_HEIGHT > t->y) 
                {
                    t->coletado = true;
                    
                    // LÓGICA DE EFEITO
                    switch(t->tipo) {
                        case TESOURO_TOCHA: 
                            snprintf(current_message, 256, "TOCHA Coletada! Voce sente seu campo de visao aumentar.");
                            visaoExtra = true;
                            break;
                        case TESOURO_CALICE_SAGRADO: 
                            snprintf(current_message, 256, "CALICE SAGRADO Coletado! Voce foi abencoado com uma nova vida.");
                            jogador.vidas++;
                            max_lives_total++;
                            break;
                        case TESOURO_BUSSOLA:
                            snprintf(current_message, 256, "BUSSOLA ENCONTRADA! Siga a bussola para encontrar a reliquia!");
                            possuiBussola = true;
                            break;
                        case TESOURO_ESTATUETA: 
                            // Recupera 40 de vida
                            jogador.vida += 40;
                            if (jogador.vida > 100) jogador.vida = 100; // Limita a 100
                            
                            snprintf(current_message, 256, "ESTATUETA ANTIGA! Voce recuperou 40 de vida.");
                            break;
                    }
                    message_start_time = SDL_GetTicks(); // Inicia o timer da mensagem
                }
            }


            // --- LÓGICA DE TRANSIÇÃO DE FASE / CONDIÇÃO DE VITÓRIA ---
            int player_center_x = jogador.x + PLAYER_WIDTH / 2;
            int player_center_y = jogador.y + PLAYER_HEIGHT / 2;
            int tile_exit_x = player_center_x / TILE_SIZE;
            int tile_exit_y = player_center_y / TILE_SIZE;

            if (tile_exit_y >= 0 && tile_exit_y < MAP_HEIGHT_TILES &&
                tile_exit_x >= 0 && tile_exit_x < MAP_WIDTH_TILES) 
            {
                if (current_map[tile_exit_y][tile_exit_x] == 3) {
                    if (global_relic.coletada[current_phase]) {
                        
                        // Avança para a próxima fase
                        current_phase++;
                        
                        if (current_phase < NUM_AREAS) {
                            // Transição para a próxima área
                            initialize_phase(current_phase, &jogador, mummies, dancers, treasures, &global_relic, &visaoExtra, &possuiBussola, current_message, &message_start_time);
                        } else {
                            // FIM DO JOGO (Vitória final)
                            game_end_time = SDL_GetTicks(); // Captura o tempo final
                            game_state = GAME_VICTORY;
                            snprintf(current_message, 256, "PARABENS! Voce escapou com as reliquias!");
                            message_start_time = SDL_GetTicks();
                        }
                    } else {
                        // Relíquia NÃO coletada
                        snprintf(current_message, 256, "Saida Bloqueada! Colete a Reliquia da Area %d primeiro.", current_phase + 1);
                        message_start_time = SDL_GetTicks();
                    }
                }
            }
                    
            // --- LÓGICA DAS DANÇARINAS ---
            for(int i = 0; i < DANCER_COUNT; i++) {
                Dancarina* danca = &dancers[i];
                if (danca->vida <= 0) continue; 
                // ... (Lógica da Dançarina)
                int dx = jogador.x - danca->x;
                int dy = jogador.y - danca->y;
                int distQuadrada = (dx*dx) + (dy*dy);
                
                int move_x = 0;
                int move_y = 0;
                int vel_parada = 1;

                switch(danca->estadoAtual) {
                    case PARADA:
                        if (distQuadrada < danca->alcanceVisaoQuadrado) {
                            danca->estadoAtual = DANCANDO;
                            danca->tempoEstado = SDL_GetTicks();
                        } else {
                            if (danca->dirX == 0 && danca->dirY == 0 || rand() % 300 == 0) {
                                if (rand() % 2 == 0) {
                                    danca->dirX = (rand() % 2 == 0) ? 1 : -1;
                                    danca->dirY = 0;
                                } else {
                                    danca->dirX = 0;
                                    danca->dirY = (rand() % 2 == 0) ? 1 : -1;
                                }
                            }

                            move_x = danca->dirX * vel_parada;
                            move_y = danca->dirY * vel_parada;
                        }
                        break;
                    case DANCANDO:
                        danca->deslocamento += danca->direcaoDanca;
                        if (danca->deslocamento > 20 || danca->deslocamento < -20) {
                            danca->direcaoDanca = -danca->direcaoDanca;
                        }
                        move_x = danca->direcaoDanca;
                        
                        if (distQuadrada < danca->raioHipnoseQuadrado) {
                            danca->estadoAtual = HIPNOTIZANDO;
                            danca->tempoEstado = SDL_GetTicks();
                            danca->alvo_hipnotizado = false; 
                        } else if (distQuadrada > danca->alcanceVisaoQuadrado) {
                            danca->estadoAtual = PARADA;
                        }
                        break;
                    case HIPNOTIZANDO:
                        if (!danca->alvo_hipnotizado) {
                             jogador.hipnotizado = true; 
                             danca->alvo_hipnotizado = true;
                        }
                        
                        if (SDL_GetTicks() - danca->tempoEstado > 3000) {
                            danca->tempoEstado = SDL_GetTicks();
                            danca->estadoAtual = ATORDOADA;
                            jogador.hipnotizado = false; 
                        }
                        estado = PARADO;
                        break;
                    case ATORDOADA:
                        if (SDL_GetTicks() - danca->tempoEstado > 2000) {
                            danca->estadoAtual = PARADA;
                        }
                        break;
                }

                if (move_x != 0 || move_y != 0) {
                    int new_dx = danca->x + move_x;
                    int new_dy = danca->y + move_y;

                    if (!is_colliding(new_dx, danca->y, danca->w, danca->h)) {
                        danca->x = new_dx;
                    } else {
                        danca->dirX = 0;
                        danca->dirY = (rand() % 2 == 0) ? 1 : -1; 
                    }

                    if (!is_colliding(danca->x, new_dy, danca->w, danca->h)) {
                        danca->y = new_dy;
                    } else {
                        danca->dirY = 0;
                        danca->dirX = (rand() % 2 == 0) ? 1 : -1; 
                    }
                }
             aux_dancarina(danca);
            }
            
            // --- LÓGICA DAS MÚMIAS  ---
            for(int i = 0; i < MUMMY_COUNT; i++) {
                Mumia* mumia = &mummies[i];

                if (mumia->vida <= 0) continue; 

                int mx = jogador.x - mumia->x;
                int my = jogador.y - mumia->y;
                int dist2 = mx*mx + my*my;
                
                int move_x = 0;
                int move_y = 0;

                switch(mumia->estado) {
                    case MUMIA_DORMINDO:
                        if(dist2 < mumia->alcanceVisao2) {
                            mumia->estado = MUMIA_PERSEGUINDO;
                        }
                        break;
                    
                    case MUMIA_PERSEGUINDO:
                        if(dist2 < mumia->distanciaEnrolar2) {
                            mumia->estado = MUMIA_ENROLANDO;
                            mumia->tempoEstado = SDL_GetTicks();
                            if(!jogador.enrolado) { // Aplica o dano apenas uma vez no início do enrolar
                                 jogador.enrolado = true; 
                                 jogador.vida -= mumia->danoAtaque;
                            }
                        } else if(dist2 > mumia->alcanceVisao2) {
                            mumia->estado = MUMIA_CONFUSA;
                            mumia->tempoEstado = SDL_GetTicks();
                        } else {
                                if(mx > 0) move_x = 1; else move_x = -1;
                                if(my > 0) move_y = 1; else move_y = -1;
                        }
                        break;

                    case MUMIA_CONFUSA:
                        if(SDL_GetTicks() - mumia->tempoEstado > 4000) {
                            mumia->estado = MUMIA_DORMINDO;
                        } else {
                            move_x = mumia->dirX;
                            move_y = mumia->dirY;

                            if(rand() % 40 == 0) mumia->dirX = -mumia->dirX;
                            if(rand() % 40 == 0) mumia->dirY = -mumia->dirY;
                            if(dist2 < mumia->alcanceVisao2) mumia->estado = MUMIA_PERSEGUINDO;
                        }
                        break;

                    case MUMIA_ENROLANDO:
                        if(SDL_GetTicks() - mumia->tempoEstado > 800) {
                            mumia->estado = MUMIA_ATORDOADA;
                            mumia->tempoEstado = SDL_GetTicks();
                            jogador.enrolado = false; 
                        }
                        break;

                    case MUMIA_ATORDOADA:
                        if(SDL_GetTicks() - mumia->tempoEstado > 3000) {
                            mumia->estado = MUMIA_DORMINDO;
                        }
                        break;
                }
                
                if (move_x != 0 || move_y != 0) {
                    int new_mx = mumia->x + move_x;
                    int new_my = mumia->y + move_y;

                    if (!is_colliding(new_mx, mumia->y, mumia->w, mumia->h)) {
                        mumia->x = new_mx;
                    } else {
                        mumia->dirX = -mumia->dirX;
                    }
                    if (!is_colliding(mumia->x, new_my, mumia->w, mumia->h)) {
                        mumia->y = new_my;
                    } else {
                        mumia->dirY = -mumia->dirY;
                    }
                }
            }

            // --- Lógica do personagem ---
            if(!jogador.hipnotizado && !jogador.enrolado) {
                int move_x = 0;
                int move_y = 0;

                switch(estado) {
                    case ANDANDO:
                        vel = 1;
                        if(teclas[SDL_SCANCODE_UP]) { move_y = -vel; direcaoOlhar = 1; }
                        if(teclas[SDL_SCANCODE_DOWN]) { move_y = vel; direcaoOlhar = 0; }
                        if(teclas[SDL_SCANCODE_LEFT]) { move_x = -vel; direcaoOlhar = 2; }
                        if(teclas[SDL_SCANCODE_RIGHT]) { move_x = vel; direcaoOlhar = 3; }
                        break;

                    case CORRENDO:
                        vel = 2;
                        if(teclas[SDL_SCANCODE_UP]) { move_y = -vel; direcaoOlhar = 1; }
                        if(teclas[SDL_SCANCODE_DOWN]) { move_y = vel; direcaoOlhar = 0; }
                        if(teclas[SDL_SCANCODE_LEFT]) { move_x = -vel; direcaoOlhar = 2;  }
                        if(teclas[SDL_SCANCODE_RIGHT]) { move_x = vel; direcaoOlhar = 3; }
                        break;

                    case PULANDO:
                        if(k==0) { c=(SDL_Rect){300,0,100,80}; k=1; }
                        if(teclas[SDL_SCANCODE_LEFT]) { move_x = -vel; c=(SDL_Rect){400,0,100,80}; }
                        if(teclas[SDL_SCANCODE_RIGHT]) { move_x = vel; c=(SDL_Rect){300,0,100,80}; }
                        
                        if(subindo) { 
                            move_y = -vel; 
                            if(jogador.y + move_y <= chao-50) { move_y = chao-50 - jogador.y; subindo=false; }
                        } else { 
                            move_y = vel; 
                            if(jogador.y + move_y >= chao) { 
                                move_y = chao - jogador.y;
                                jogador.y=chao; 
                                k=0; 
                                estado=(teclas[SDL_SCANCODE_LEFT]||teclas[SDL_SCANCODE_RIGHT] || teclas[SDL_SCANCODE_UP] || teclas[SDL_SCANCODE_DOWN])? ANDANDO : PARADO; 
                                noChao=true; 
                                subindo=true; 
                            } 
                        }
                        break;

                    case PARADO:
                        c = (SDL_Rect){0,0,100,80};
                        break;
                    case ATACANDO:
                        for(int i = 0; i < MUMMY_COUNT; i++) {
                            Mumia* mumia = &mummies[i];
                            if (mumia->vida <= 0) continue;

                            int mx_atk = jogador.x - mumia->x;
                            int my_atk = jogador.y - mumia->y;
                            int dist2_mumia = mx_atk*mx_atk + my_atk*my_atk;

                            if (dist2_mumia < jogador.alcanceAtaque) {
                                mumia->vida -= jogador.danoAtaque;
                                
                                if(mumia->estado != MUMIA_ATORDOADA) {
                                    mumia->estado = MUMIA_ATORDOADA;
                                    mumia->tempoEstado = SDL_GetTicks();
                                    if(jogador.enrolado) jogador.enrolado = false; 
                                }
                            }
                        }

                        for(int i = 0; i < DANCER_COUNT; i++) {
                            Dancarina* danca_atk = &dancers[i];
                            if (danca_atk->vida <= 0) continue;

                            int dx_atk = jogador.x - danca_atk->x;
                            int dy_atk = jogador.y - danca_atk->y;
                            int dist2_danca = dx_atk*dx_atk + dy_atk*dy_atk;

                            if (dist2_danca < jogador.alcanceAtaque) {
                                danca_atk->vida -= jogador.danoAtaque;

                                if(danca_atk->estadoAtual != ATORDOADA) {
                                    danca_atk->estadoAtual = ATORDOADA;
                                    danca_atk->tempoEstado = SDL_GetTicks();
                                    jogador.hipnotizado = false; 
                                }
                            }
                        }

                        estado=(teclas[SDL_SCANCODE_LEFT]|| teclas[SDL_SCANCODE_RIGHT]  || teclas[SDL_SCANCODE_UP] || teclas[SDL_SCANCODE_DOWN] )? ANDANDO : PARADO;
                        break;
                    
                    default: break;
                }
                
                update_position(&jogador.x, &jogador.y, move_x, move_y);

            } else if (jogador.hipnotizado) {
                c = (SDL_Rect){680,0,100,80};
            } else if (jogador.enrolado) {
                c = (SDL_Rect){0,0,100,80};
            }
        } // Fim de if (game_state == GAME_PLAYING)

        // --- RENDERIZAÇÃO ---
        SDL_SetRenderDrawColor(ren, 255,255,255,255);
        SDL_RenderClear(ren);
        
        // Renderiza o fundo específico para o Menu

        if (game_state == GAME_MENU && img_menu_fundo != NULL) {
            SDL_RenderCopy(ren, img_menu_fundo, NULL, &rFundo);
        } else if (game_state == GAME_OVER || game_state == GAME_VICTORY)  {
            SDL_RenderCopy(ren, img_menu_alt, NULL, &rFundo);
        }
            else {
            SDL_RenderCopy(ren, img_fundo, NULL, &rFundo); 
        }
        if (game_state == GAME_PLAYING) {
            
            int camera_offset_x = jogador.x - PLAYER_SCREEN_X;
            int camera_offset_y = jogador.y - PLAYER_SCREEN_Y;
            c.y = 0; 
            c.w = PLAYER_FRAME_W;
            c.h = PLAYER_FRAME_H;
            
            // 1. Renderizar o MAPA
            for (int i = 0; i < MAP_HEIGHT_TILES; i++) {
                for (int j = 0; j < MAP_WIDTH_TILES; j++) {
                    SDL_Rect rTile = {
                        j * TILE_SIZE - camera_offset_x,
                        i * TILE_SIZE - camera_offset_y,
                        TILE_SIZE,
                        TILE_SIZE
                    };

                    if (rTile.x + TILE_SIZE > 0 && rTile.x < WINDOW_WIDTH &&
                        rTile.y + TILE_SIZE > 0 && rTile.y < WINDOW_HEIGHT) {
                            
                        if (current_map[i][j] == 1) { // Parede
                            SDL_SetRenderDrawColor(ren, 50, 50, 50, 255);
                            SDL_RenderCopy(ren, img4, NULL, &rTile); 
                        } else if (current_map[i][j] == 2) { // Spawn
                            SDL_SetRenderDrawColor(ren, 50, 50, 50, 255);
                            SDL_RenderCopy(ren, img_entrada, NULL, &rTile); 
                        } else if (current_map[i][j] == 3) { // Saída
                            SDL_RenderCopy(ren, img_entrada, NULL, &rTile);
                        } else { // Caminho
                            SDL_SetRenderDrawColor(ren, 80, 50, 20, 255);
                            SDL_RenderFillRect(ren, &rTile);
                        }
                    }
                }
            }

            // --- Renderizar a Relíquia da fase atual ---
            if (!global_relic.coletada[current_phase]) {
                SDL_Rect rRelic = {
                    global_relic.x - camera_offset_x,
                    global_relic.y - camera_offset_y,
                    global_relic.w,
                    global_relic.h
                };
                SDL_SetRenderDrawColor(ren, 255, 215, 0, 255); 
                SDL_RenderCopy(ren, img_reliquia, NULL, &rRelic); 
            }
            
            // --- Renderizar Tesouros ---
            for(int i = 0; i < NUM_TREASURES_TO_SPAWN; i++) {
                Tesouro* t_render = &treasures[i];
                if (t_render->coletado) continue; 

                SDL_Rect rTesouro = {
                    t_render->x - camera_offset_x,
                    t_render->y - camera_offset_y,
                    t_render->w,
                    t_render->h
                };
                
                switch(t_render->tipo) {
                    case TESOURO_TOCHA: 
                        SDL_RenderCopy(ren, img_tocha, NULL, &rTesouro); 
                        break; 
                    case TESOURO_CALICE_SAGRADO: 
                        SDL_RenderCopy(ren, img_calice, NULL, &rTesouro);
                        break;
                    case TESOURO_BUSSOLA:
                        SDL_RenderCopy(ren, img_bussola, NULL, &rTesouro);
                        break;
                    case TESOURO_ESTATUETA: 
                        SDL_RenderCopy(ren, img_estatueta, NULL, &rTesouro);
                        break;
                }
            }
            

            // 2. Renderizar o Jogador


            int animOffset = ((SDL_GetTicks() / 150) % 2) * 130; // Será 0 ou 100

            if (jogador.hipnotizado || jogador.enrolado) {
               c.x = 930; // Posição 7: Hipnotizado
            } 
            else if (estado == PARADO) {
               c.x = 0;   // Posição 0: Parado
            } 
            else {
               switch (direcaoOlhar) {
                  case 0: // BAIXO -> Posições 1 e 2
                      c.x = 130 + animOffset;
                    break;
                  case 3: // DIREITA -> Posições 3 e 4 
                      int frameDir = (SDL_GetTicks() / 150) % 3; 
                    
                      if (frameDir == 0) c.x = 390;
                      else if (frameDir == 1) c.x = 390 + 130; // 520
                      else c.x = 1060; // O novo corte solicitado
                    break;
                  case 1: // CIMA -> Posições 5 e 6 
                      c.x = 660 + animOffset;
                    break;
                  case 2: // ESQUERDA -> Posições 8 e 9 
                      int frameDir1 = (SDL_GetTicks() / 150) % 3; 
                    
                      if (frameDir1 == 0) c.x = 1190;
                      else if (frameDir1 == 1) c.x = 1190 + 130;
                      else c.x = 1460; 
                    break;
            }
        }
            SDL_RenderCopy(ren, img, &c, &r);
            
            // 3. Renderizar os inimigos
            for(int i = 0; i < DANCER_COUNT; i++) {
                Dancarina* danca_render = &dancers[i];
                if (danca_render->vida <= 0) continue; 

            SDL_Rect rDanca = {
                   danca_render->x - camera_offset_x, 
                   danca_render->y - camera_offset_y, 
                   danca_render->w, danca_render->h
            };
        // Usa o frameRecorte calculado na função auxiliar
        SDL_RenderCopy(ren, danca_img, &danca_render->frameRecorte, &rDanca); 
            }

            for(int i = 0; i < MUMMY_COUNT; i++) {
                Mumia* mumia = &mummies[i];
                if (mumia->vida <= 0) continue;

                SDL_Rect rMumia = {
                    mumia->x - camera_offset_x,
                    mumia->y - camera_offset_y,
                    mumia->w,
                    mumia->h
                };
                
                SDL_Rect cMumia;
                    cMumia.w = MUMIA_FRAME_W; // 48
                    cMumia.h = MUMIA_FRAME_H; // 64
                    cMumia.x = 0; // Posição X fixa no sprite sheet
        
             switch(mumia->estado) {
                 case MUMIA_DORMINDO: 
                      cMumia.y = 0 * MUMIA_FRAME_H; // LINHA 0
                 break;
                
                 case MUMIA_PERSEGUINDO: 
                     if (mumia->dirX > 0) {
                         cMumia.y = 3 * MUMIA_FRAME_H; // LINHA 3: OLHANDO ESQUERDA (192 pixels de offset)
                     } else {
                         cMumia.y = 1 * MUMIA_FRAME_H; // LINHA 1: OLHANDO DIREITA
                     }
                     break;
                
                     case MUMIA_CONFUSA:
                     case MUMIA_ENROLANDO:
                     case MUMIA_ATORDOADA: 
                         cMumia.y = 2 * MUMIA_FRAME_H; // Linha 2: ACORDADA/PARADA/STUN
                     break;
              }
              SDL_RenderCopy(ren, mumia_img, &cMumia, &rMumia);
            }
            
            // 4. Renderizar o Efeito de Visão
            if (visaoExtra) SDL_RenderCopy(ren, img3, NULL, &visao);
            else SDL_RenderCopy(ren, img2, NULL, &visao);

            // =======================================================
            // --- Lógica de Renderização do HUD (Barra de Vida e Vidas) ---
            // =======================================================
            
            // Barra de Vida (Fundo Cinza)
            SDL_Rect rVidaFundo = { 20, 20, 300, 30 };
            SDL_SetRenderDrawColor(ren, 50, 50, 50, 255); 
            SDL_RenderFillRect(ren, &rVidaFundo);

            // Barra de Vida (Preenchimento)
            int vida_atual_w = (int)((jogador.vida / 100.0) * 300);
            if (vida_atual_w < 0) vida_atual_w = 0;
            
            SDL_Rect rVidaAtual = { 20, 20, vida_atual_w, 30 };
            
            if (jogador.vida > 50) {
                SDL_SetRenderDrawColor(ren, 0, 200, 0, 255);
            } else if (jogador.vida > 20) {
                SDL_SetRenderDrawColor(ren, 255, 255, 0, 255);
            } else {
                SDL_SetRenderDrawColor(ren, 200, 0, 0, 255);
            }
            SDL_RenderFillRect(ren, &rVidaAtual);
            
            // Vidas Restantes
            const int SQUARE_SIZE = 25;
            const int SPACING = 5;
            const int START_X_LIVES = 340; 
            
            for (int i = 0; i < max_lives_total; i++) {
                SDL_Rect rLife = {
                    START_X_LIVES + i * (SQUARE_SIZE + SPACING), 
                    25, 
                    40, 
                    40
                };
                
                if (i < jogador.vidas) {
                    if (img_vida) SDL_RenderCopy(ren, img_vida, NULL, &rLife);
                    else { SDL_SetRenderDrawColor(ren, 255, 0, 0, 255); SDL_RenderFillRect(ren, &rLife); }
                } else {
                    if (img_vida2) SDL_RenderCopy(ren, img_vida2, NULL, &rLife);
                    else { SDL_SetRenderDrawColor(ren, 120, 120, 120, 255); SDL_RenderDrawRect(ren, &rLife); }
                }
            }
            
            // Mostra a fase atual
            char phase_info[64];
            snprintf(phase_info, 64, "Area: %d / %d", current_phase + 1, NUM_AREAS);
            render_text(ren, font_small, phase_info, WINDOW_WIDTH - 200, 25, (SDL_Color){255, 255, 255, 255});
            
            // Tempo de jogo
            Uint32 tempo_corrido = SDL_GetTicks() - game_start_time;
            char time_info[64];
            snprintf(time_info, 64, "Tempo: %02u:%02u", (tempo_corrido / 60000) % 60, (tempo_corrido / 1000) % 60);
            render_text(ren, font_small, time_info, WINDOW_WIDTH - 200, 60, (SDL_Color){255, 255, 255, 255});

            // --- HUD DA BÚSSOLA ---
            if (possuiBussola && !global_relic.coletada[current_phase]) {
                const char* direcao = obter_direcao_reliquia(jogador.x, jogador.y, global_relic.x, global_relic.y);
                
                char bussola_info[64];
                snprintf(bussola_info, 64, "Reliquia: %s", direcao);
                
                // Renderiza em Ciano para destacar
                render_text(ren, font_small, bussola_info, WINDOW_WIDTH - 350, 95, (SDL_Color){0, 255, 255, 255});
            }
            
            // --- Lógica de Renderização de Mensagens HUD ---
            if (font_small != NULL && SDL_GetTicks() - message_start_time < MESSAGE_DURATION) {
                SDL_Color corMensagem = {255, 255, 255, 255}; // Branco
                int text_w, text_h;
                TTF_SizeText(font_small, current_message, &text_w, &text_h);
                
                // Renderiza no centro superior
                render_text(ren, font_small, current_message, (WINDOW_WIDTH - text_w) / 2, 80, corMensagem);
            }
            // =======================================================
        } else if (game_state == GAME_VICTORY || game_state == GAME_OVER) {
            // --- RENDERIZAÇÃO DA TELA DE MENU / VITÓRIA / GAME OVER ---
            
            SDL_Color corTitulo;
            char title_text[100];
            char option1_text[64];
            
            if (game_state == GAME_VICTORY) {
                corTitulo = (SDL_Color){0, 255, 0, 255};
                snprintf(title_text, 100, "PARABENS! Voce Escapou!");
                snprintf(option1_text, 64, "-> Jogar Novamente <-");
            } else { // GAME_OVER
                corTitulo = (SDL_Color){255, 0, 0, 255}; // Vermelho para Game Over fica melhor
                snprintf(title_text, 100, "Voce perdeu todas as vidas! Fim de jogo.");
                snprintf(option1_text, 64, "-> Jogar Novamente <-");
            }

            SDL_Color corNormal = {255, 255, 255, 255};
            SDL_Color corSelecionado = {255, 215, 0, 255}; // Dourado

            // Variáveis auxiliares para medir o tamanho do texto
            int w_text, h_text;

            // 1. Título
            TTF_SizeText(font_large, title_text, &w_text, &h_text);
            render_text(ren, font_large, title_text, (WINDOW_WIDTH - w_text) / 2, WINDOW_HEIGHT / 4, corTitulo);

            // 2. Tempo (apenas na vitória)
            if (game_state == GAME_VICTORY) {
                Uint32 total_time = game_end_time - game_start_time;
                char time_text[64];
                snprintf(time_text, 64, "Tempo de Solucao: %02u:%02u", (total_time / 60000) % 60, (total_time / 1000) % 60);
                
                TTF_SizeText(font_small, time_text, &w_text, &h_text);
                render_text(ren, font_small, time_text, (WINDOW_WIDTH - w_text) / 2, WINDOW_HEIGHT / 4 + 80, corNormal);
            }
            
            // Opções de Menu
            int menu_y = WINDOW_HEIGHT / 2;
            SDL_Color corOpcao1 = (selected_option == 0) ? corSelecionado : corNormal;
            SDL_Color corOpcao2 = (selected_option == 1) ? corSelecionado : corNormal;
            
            // 3. Opção 1 (Jogar Novamente)
            TTF_SizeText(font_large, option1_text, &w_text, &h_text);
            render_text(ren, font_large, option1_text, (WINDOW_WIDTH - w_text) / 2, menu_y, corOpcao1);

            // 4. Opção 2 (Sair)
            TTF_SizeText(font_large, "Sair do Jogo", &w_text, &h_text);
            render_text(ren, font_large, "Sair do Jogo", (WINDOW_WIDTH - w_text) / 2, menu_y + 80, corOpcao2);
        }

        // =======================================================
        
        SDL_RenderPresent(ren);

        espera = 10;
    }
    
    TTF_CloseFont(font_small);
    TTF_CloseFont(font_large);
    TTF_Quit();
    
    SDL_DestroyTexture(img);
    SDL_DestroyTexture(img2);
    SDL_DestroyTexture(img3);
    SDL_DestroyTexture(img4);
    SDL_DestroyTexture(img_fundo);
    SDL_DestroyTexture(img_menu_fundo);
    SDL_DestroyTexture(img_mumia);
    SDL_DestroyTexture(img_dancarina);
    SDL_DestroyTexture(img_tocha);
    SDL_DestroyTexture(img_calice);
    SDL_DestroyTexture(img_vida);
    SDL_DestroyTexture(img_vida2);
    SDL_DestroyTexture(img_estatueta);
    SDL_DestroyTexture(img_bussola);
    SDL_DestroyTexture(mumia_img);
    SDL_DestroyTexture(danca_img);
    SDL_DestroyTexture(img_reliquia);
    SDL_DestroyTexture(img_entrada);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
