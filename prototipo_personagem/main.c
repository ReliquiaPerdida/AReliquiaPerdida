#include <assert.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <time.h>    


#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
// --- Configurações do Mapa ---
#define MAP_WIDTH_TILES 60
#define MAP_HEIGHT_TILES 20
#define TILE_SIZE 80
#define RELIC_TILE 4
#define MUMMY_COUNT 6
#define DANCER_COUNT 6
#define PLAYER_WIDTH 60
#define PLAYER_HEIGHT 60
#define MIN_SPAWN_DIST_TILES 10 // Mínimo de 10 tiles de distância do jogador

// --- Configurações dos Tesouros ---
#define NUM_TREASURES_TO_SPAWN 2

// Configurações de Vidas (2 vidas)
#define MAX_LIVES 2 


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
    TESOURO_PERGAMINHO,
    TESOURO_MALCIDAO,
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
    bool coletada;
} Reliquia;

typedef struct {
    TipoTesouro tipo;
    int x, y;
    int w, h;
    bool coletado;
} Tesouro;


int map[MAP_HEIGHT_TILES][MAP_WIDTH_TILES] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,2,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,0,0,0,0,1,0,1,0,1,0,0,0,0,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,4,1},
    {1,0,1,1,1,1,1,0,1,1,1,0,1,1,1,1,0,1,0,1,0,1,0,1,1,1,0,1,0,1,0,1,0,1,1,1,0,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,1},
    {1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,0,1,1,0,1,0,1,1,1,0,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};


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
            if (map[i][j] == 1) {
                return true; 
            }
        }
    }
    return false;
}

void update_position(int* x, int* y, int dx, int dy) {
    // Tenta mover em X
    int new_x = *x + dx;
    if (!is_colliding(new_x, *y, PLAYER_WIDTH, PLAYER_HEIGHT)) { 
        *x = new_x;
    }

    // Tenta mover em Y
    int new_y = *y + dy;
    if (!is_colliding(*x, new_y, PLAYER_WIDTH, PLAYER_HEIGHT)) { 
        *y = new_y;
    }
}


// --- Encontrar Posição de Spawn Válida ---
void find_valid_spawn(int* out_x, int* out_y, int entity_w, int entity_h, int player_start_x, int player_start_y) {
    int max_attempts = 1000;
    int attempts = 0;

    while (attempts < max_attempts) {
        // Escolhe um tile aleatório (evita bordas de parede 1)
        int rand_tile_x = 1 + rand() % (MAP_WIDTH_TILES - 2);
        int rand_tile_y = 1 + rand() % (MAP_HEIGHT_TILES - 2);

        // Se o tile for caminho livre (0)
        if (map[rand_tile_y][rand_tile_x] == 0) {
            
            // Calcula a posição real em pixels
            int potential_x = rand_tile_x * TILE_SIZE + (TILE_SIZE - entity_w) / 2;
            int potential_y = rand_tile_y * TILE_SIZE + (TILE_SIZE - entity_h) / 2;

            // 1. Verifica se não está dentro de uma parede
            if (!is_colliding(potential_x, potential_y, entity_w, entity_h)) {
                
                // 2. Verifica a distância mínima do jogador
                int dx = potential_x - player_start_x;
                int dy = potential_y - player_start_y;
                int dist2 = dx*dx + dy*dy;
                
                // Distância mínima em pixels
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


int main(int argc, char* args[]) {
    srand(time(NULL));

    SDL_Init(SDL_INIT_EVERYTHING);
    
    SDL_Window* win = SDL_CreateWindow("A Reliquia Perdida",
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* img = IMG_LoadTexture(ren, "anim2.png");
    SDL_Texture* img2 = IMG_LoadTexture(ren, "escuridao.png");
    SDL_Texture* img3 = IMG_LoadTexture(ren, "escuridao2.png");
    SDL_Texture* img4 = IMG_LoadTexture(ren, "bloco.png");
    SDL_Texture* img_fundo = IMG_LoadTexture(ren, "fundo.png");
    SDL_Texture* img_mumia = IMG_LoadTexture(ren, "mumia.png");
    SDL_Texture* img_dancarina = IMG_LoadTexture(ren, "dancarina.png");
    SDL_Texture* img_tocha = IMG_LoadTexture(ren, "tocha.png");
    SDL_Texture* img_calice = IMG_LoadTexture(ren, "calice.png");
    SDL_Texture* img_vida = IMG_LoadTexture(ren, "vida.png");
    SDL_Texture* img_vida2 = IMG_LoadTexture(ren, "vida2.png");
    assert(img != NULL);
    assert(img2 != NULL);
    assert(img3 != NULL);
    assert(img4 != NULL);
    assert(img_fundo != NULL);
    assert(img_mumia != NULL);
    assert(img_dancarina != NULL);

    // --- Personagem ---
    const int PLAYER_SCREEN_X = (WINDOW_WIDTH - PLAYER_WIDTH) / 2;
    const int PLAYER_SCREEN_Y = (WINDOW_HEIGHT - PLAYER_HEIGHT) / 2;

    SDL_Rect r = {PLAYER_SCREEN_X, PLAYER_SCREEN_Y, PLAYER_WIDTH, PLAYER_HEIGHT};
    SDL_Rect c = {0, 0, 100, 80};
    SDL_Rect visao = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_Rect rFundo = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};

    // --- ENCONTRAR Posição Inicial do Jogador ('2' no mapa) ---
    int start_x = 0, start_y = 0;
    for (int i = 0; i < MAP_HEIGHT_TILES; i++) {
        for (int j = 0; j < MAP_WIDTH_TILES; j++) {
            if (map[i][j] == 2) {
                start_x = j * TILE_SIZE + (TILE_SIZE - PLAYER_WIDTH) / 2;
                start_y = i * TILE_SIZE + (TILE_SIZE - PLAYER_HEIGHT) / 2;
                break;
            }
        }
    }
    
    // Posição de Respawn
    const int respawn_x = start_x;
    const int respawn_y = start_y;


    // --- Jogador ---
    Jogador jogador = {
        .x = start_x, 
        .y = start_y, 
        .vida = 100,
        .vidas = MAX_LIVES, 
        .danoAtaque = 20,
        .alcanceAtaque = 100 * 100,
        .hipnotizado = false,
        .enrolado = false,
        .tempoEstado = 0
    };


    // --- RELÍQUIA ---
    Reliquia relic = {0};
    // Encontra o tile 4 
    for (int i = 0; i < MAP_HEIGHT_TILES; i++) {
        for (int j = 0; j < MAP_WIDTH_TILES; j++) {
            if (map[i][j] == 4) {
                relic.x = j * TILE_SIZE + (TILE_SIZE - 30) / 2;
                relic.y = i * TILE_SIZE + (TILE_SIZE - 30) / 2;
                relic.w = 30;
                relic.h = 30;
                relic.coletada = false;
                map[i][j] = 0;
                break;
            }
        }
    }

    // --- MÚMIAS ---
    Mumia mummies[MUMMY_COUNT] = {0};

    for(int i = 0; i < MUMMY_COUNT; i++) {
        int mx, my;
        // encontrar uma posição segura
        find_valid_spawn(&mx, &my, 40, 40, jogador.x, jogador.y); 

        mummies[i] = (Mumia){
            .estado = MUMIA_DORMINDO,
            .x = mx, // Posição segura
            .y = my, // Posição segura
            .w = 60,
            .h = 60,
            .vida = 100,
            .danoAtaque = 20,
            .alcanceVisao2 = 200*200,
            .distanciaEnrolar2 = 50*50,
            .tempoEstado = 0,
            .dirX = (rand() % 2 == 0) ? 1 : -1,
            .dirY = (rand() % 2 == 0) ? 1 : -1
        };
    }

    // --- DANÇARINAS ---
    Dancarina dancers[DANCER_COUNT] = {0};

    for(int i = 0; i < DANCER_COUNT; i++) {
        int dx, dy;
        find_valid_spawn(&dx, &dy, 50, 50, jogador.x, jogador.y);

        dancers[i] = (Dancarina){
            .estadoAtual = PARADA,
            .x = dx, // Posição segura
            .y = dy, // Posição segura
            .w = 60,
            .h = 60,
            .vida = 100,
            .alcanceVisaoQuadrado = 200 * 200,
            .raioHipnoseQuadrado = 100 * 100,
            .tempoEstado = 0,
            .deslocamento = 0,
            .direcaoDanca = 1,
            .alvo_hipnotizado = false,
            .dirX = (rand() % 2 == 0) ? 1 : -1, 
            .dirY = 0
        };
    }

    // --- TESOUROS ---
    Tesouro treasures[NUM_TREASURES_TO_SPAWN] = {0};

    for(int i = 0; i < NUM_TREASURES_TO_SPAWN; i++) {
        int tx, ty;
        // Chama a função para encontrar uma posição segura
        find_valid_spawn(&tx, &ty, 30, 30, jogador.x, jogador.y); 

        TipoTesouro tipo;
        // Para garantir que os dois tesouros apareçam:
        if (i == 0) tipo = TESOURO_TOCHA;
        else tipo = TESOURO_CALICE_SAGRADO;

        treasures[i] = (Tesouro){
            .tipo = tipo,
            .x = tx,
            .y = ty,
            .w = 30,
            .h = 30,
            .coletado = false
        };
    }

    int vel = 1;
    bool noChao = true;
    int chao = jogador.y; 
    bool subindo = true;
    bool acertou = false;
    int aux = 2;
    int k = 0;
    bool visaoExtra = false;
    EstadoMovimento estado = PARADO;
    Uint32 tempoHipnoseInicio = 0;
    
    bool game_over = false; 
    bool rodando = true;
    SDL_Event evt;
    Uint32 espera = 10;

    while (rodando) {
        int isevt = AUX_WaitEventTimeout(&evt, &espera);
        const Uint8* teclas = SDL_GetKeyboardState(NULL);

        if (isevt) {
            if (evt.type == SDL_QUIT) rodando = false;
            else if (evt.type == SDL_KEYDOWN) {
                if(!jogador.hipnotizado && !jogador.enrolado) {
                    switch (evt.key.keysym.sym) {
                        case SDLK_LSHIFT: if(noChao) estado = CORRENDO; break;
                        case SDLK_SPACE:
                            if(noChao) {
                                chao = jogador.y;
                                estado = PULANDO;
                                noChao = false;
                            }
                            break;
                        case SDLK_k:
                            acertou = false;
                            estado = ATACANDO;
                            break;
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
        
        // --- Lógica de Dano, Respawn e Game Over ---
        if (jogador.vida <= 0) {
            jogador.vidas--; // Perde uma vida
            
            if (jogador.vidas < 0) {
                // FIM DO JOGO
                game_over = true;
                rodando = false; 
                printf("\n============================================\n");
                printf("!! GAME OVER !!\n");
                printf("============================================\n");
            } else {
                // RESPONS
                printf("\nVocê perdeu uma vida! Vidas restantes: %d\n", jogador.vidas);
                
                // Reinicia a vida e teletransporta para o spawn
                jogador.vida = 100;
                jogador.x = respawn_x;
                jogador.y = respawn_y;
                jogador.hipnotizado = false; 
                jogador.enrolado = false;
                estado = PARADO; 
                
                // Os itens coletados não são resetados.
            }
        }

        // --- LÓGICA DA RELÍQUIA E CONDIÇÃO DE VITÓRIA ---
        if (!relic.coletada) {
        // Checa colisão entre Jogador e Relíquia
           if (jogador.x < relic.x + relic.w &&
             jogador.x + PLAYER_WIDTH > relic.x &&
             jogador.y < relic.y + relic.h &&
             jogador.y + PLAYER_HEIGHT > relic.y) 
          {
            relic.coletada = true;
            printf("Relíquia Coletada! Agora encontre a Saída (Tile Vermelho)!\n");
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
                
                switch(t->tipo) {
                    case TESOURO_TOCHA: 
                        printf("Tesouro Coletado: TOCHA! Sua visão parece mais clara. (Efeito Futuro: Aumentar visao)\n");
                        visaoExtra = true;
                        break;
                    case TESOURO_CALICE_SAGRADO: 
                        printf("Tesouro Coletado: CÁLICE SAGRADO! Você sente uma nova força. (Efeito Futuro: Vida Extra)\n");
                        break;
                    case TESOURO_PERGAMINHO: 
                        printf("Tesouro Coletado: PERGAMINHO! O mapa se clareia. (Efeito Futuro: Indica Saída)\n");
                        break;
                    case TESOURO_MALCIDAO: 
                        printf("Tesouro Coletado: MALDIÇÃO! Você recebeu 20 de dano. (Efeito Futuro: Causa Dano)\n");
                        jogador.vida -= 20; // Aplica o dano imediatamente
                        break;
                    case TESOURO_ESTATUETA: 
                        printf("Tesouro Coletado: ESTATUETA! Você se sente desorientado. (Efeito Futuro: Teleporta)\n");
                        break;
                }
            }
        }


// Condição de Vitória: Pegar a Relíquia E Tocar no Tile de Saída (3)
int player_center_x = jogador.x + PLAYER_WIDTH / 2;
int player_center_y = jogador.y + PLAYER_HEIGHT / 2;
int tile_exit_x = player_center_x / TILE_SIZE;
int tile_exit_y = player_center_y / TILE_SIZE;

if (tile_exit_y >= 0 && tile_exit_y < MAP_HEIGHT_TILES &&
    tile_exit_x >= 0 && tile_exit_x < MAP_WIDTH_TILES) 
{
    if (map[tile_exit_y][tile_exit_x] == 3) {
        if (relic.coletada) {
            printf("\nPARABÉNS! Você escapou com a Relíquia!\n");
            rodando = false; // Fim do jogo
        } else {
            printf("\nA Saída está bloqueada! Você deve primeiro coletar a Relíquia!\n");
        }
    }
}
        
        // --- LÓGICA DAS DANÇARINAS ---
        for(int i = 0; i < DANCER_COUNT; i++) {
            Dancarina* danca = &dancers[i];
            if (danca->vida <= 0) continue; 

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
                        // Patrulhamento em Corredor
                        if (danca->dirX == 0 && danca->dirY == 0) {
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
                         tempoHipnoseInicio = SDL_GetTicks();
                         danca->alvo_hipnotizado = true;
                    }
                    
                    if (SDL_GetTicks() - danca->tempoEstado > 3000) {
                        danca->tempoEstado = SDL_GetTicks();
                        danca->estadoAtual = ATORDOADA;
                        jogador.hipnotizado = false; 
                    }
                    break;
                case ATORDOADA:
                    if (SDL_GetTicks() - danca->tempoEstado > 2000) {
                        danca->estadoAtual = PARADA;
                    }
                    break;
            }

            // Aplica o movimento da dançarina com COLISÃO
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
        }
        
        // --- LÓGICA DAS MÚMIAS ---
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
                        jogador.enrolado = true; 
                        jogador.vida -= mumia->danoAtaque;
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
                    if(SDL_GetTicks() - mumia->tempoEstado > 3000) {
                        mumia->estado = MUMIA_ATORDOADA;
                        mumia->tempoEstado = SDL_GetTicks();
                        jogador.enrolado = false; 
                    }
                    break;

                case MUMIA_ATORDOADA:
                    if(SDL_GetTicks() - mumia->tempoEstado > 2000) {
                        mumia->estado = MUMIA_DORMINDO;
                    }
                    break;
            }
            
            // Aplica o movimento da múmia com colisão
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
                    if(teclas[SDL_SCANCODE_UP]) { move_y = -vel; c = (aux==1)? (SDL_Rect){200,0,100,80} : (SDL_Rect){100,0,100,80}; }
                    if(teclas[SDL_SCANCODE_DOWN]) { move_y = vel; c = (aux==1)? (SDL_Rect){200,0,100,80} : (SDL_Rect){100,0,100,80}; }
                    if(teclas[SDL_SCANCODE_LEFT]) { move_x = -vel; c = (SDL_Rect){200,0,100,80}; aux=1; }
                    if(teclas[SDL_SCANCODE_RIGHT]) { move_x = vel; c = (SDL_Rect){100,0,100,80}; aux=2; }
                    break;

                case CORRENDO:
                    vel = 2;
                    if(teclas[SDL_SCANCODE_UP]) { move_y = -vel; c = (aux==1)? (SDL_Rect){590,0,100,80} : (SDL_Rect){490,0,100,80}; }
                    if(teclas[SDL_SCANCODE_DOWN]) { move_y = vel; c = (aux==1)? (SDL_Rect){590,0,100,80} : (SDL_Rect){490,0,100,80}; }
                    if(teclas[SDL_SCANCODE_LEFT]) { move_x = -vel; c = (SDL_Rect){590,0,100,80}; aux=1; }
                    if(teclas[SDL_SCANCODE_RIGHT]) { move_x = vel; c = (SDL_Rect){490,0,100,80}; aux=2; }
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
                            acertou = true;
                            
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
                            acertou = true;

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
            
            // Aplica o movimento do jogador com colisão
            update_position(&jogador.x, &jogador.y, move_x, move_y);

        } else if (jogador.hipnotizado) {
            c = (SDL_Rect){680,0,100,80};
        } else if (jogador.enrolado) {
            c = (SDL_Rect){0,0,100,80};
        }
        
        // --- CÂMERA e RENDERIZAÇÃO ---
        int camera_offset_x = jogador.x - PLAYER_SCREEN_X;
        int camera_offset_y = jogador.y - PLAYER_SCREEN_Y;

        SDL_SetRenderDrawColor(ren, 255,255,255,255);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, img_fundo, NULL, &rFundo); 
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
                        
                    if (map[i][j] == 1) { 
                        SDL_SetRenderDrawColor(ren, 50, 50, 50, 255);
                        SDL_RenderCopy(ren, img4, NULL, &rTile); 
                    } else if (map[i][j] == 2) { 
                        SDL_SetRenderDrawColor(ren, 0, 255, 0, 255);
                        SDL_RenderFillRect(ren, &rTile); 
                    } else if (map[i][j] == 3) { 
                        SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
                        SDL_RenderFillRect(ren, &rTile); 
                    } else { 
                        SDL_SetRenderDrawColor(ren, 80, 50, 20, 255);
                        SDL_RenderFillRect(ren, &rTile);
                    }
                }
            }
        }

        // --- Renderizar a Relíquia ---
        if (!relic.coletada) {
            SDL_Rect rRelic = {
                relic.x - camera_offset_x,
                relic.y - camera_offset_y,
                relic.w,
                relic.h
            };
            // Cor da Relíquia (Ouro)
            SDL_SetRenderDrawColor(ren, 255, 215, 0, 255); 
            SDL_RenderFillRect(ren, &rRelic);
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
            
            SDL_Color corTesouro;
            
            // Cores baseadas no tipo de tesouro
            switch(t_render->tipo) {
                case TESOURO_TOCHA: SDL_RenderCopy(ren, img_tocha, NULL, &rTesouro); break;
                case TESOURO_CALICE_SAGRADO: SDL_RenderCopy(ren, img_calice, NULL, &rTesouro);break; // Amarelo (Ouro)
                case TESOURO_PERGAMINHO: corTesouro=(SDL_Color){245,222,179,255}; break; // Bege (Pergaminho)
                case TESOURO_MALCIDAO: corTesouro=(SDL_Color){100,0,100,255}; break; // Roxo (Maldição)
                case TESOURO_ESTATUETA: corTesouro=(SDL_Color){139,69,19,255}; break; // Marrom (Estatueta)
            }
            
        }
        

        // 2. Renderizar o Jogador
        SDL_RenderCopy(ren, img, &c, &r);
        
        // 3. Renderizar os inimigos
        for(int i = 0; i < DANCER_COUNT; i++) {
            Dancarina* danca_render = &dancers[i];
            if (danca_render->vida <= 0) continue; 

            SDL_Rect rDanca = {
                danca_render->x - camera_offset_x,
                danca_render->y - camera_offset_y,
                danca_render->w,
                danca_render->h
            };
            SDL_RenderCopy(ren, img_dancarina, NULL, &rDanca); 
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
            SDL_RenderCopy(ren, img_mumia, NULL, &rMumia); 
        }
        
        // 4. Renderizar o Efeito de Visão
        if (visaoExtra) SDL_RenderCopy(ren, img3, NULL, &visao);
        else SDL_RenderCopy(ren, img2, NULL, &visao);

        // =======================================================
        // --- Lógica de Renderização do HUD ---
        // =======================================================
        
        // --- Barra de Vida (Fundo Cinza) ---
        SDL_Rect rVidaFundo = { 20, 20, 300, 30 };
        SDL_SetRenderDrawColor(ren, 50, 50, 50, 255); // Cinza Escuro
        SDL_RenderFillRect(ren, &rVidaFundo);

        // --- Barra de Vida (Preenchimento) ---
        // A largura é proporcional à vida atual
        int vida_atual_w = (int)((jogador.vida / 100.0) * 300);
        if (vida_atual_w < 0) vida_atual_w = 0;
        
        SDL_Rect rVidaAtual = { 20, 20, vida_atual_w, 30 };
        
        // Define a cor da barra de vida com base na saúde atual
        if (jogador.vida > 50) {
            SDL_SetRenderDrawColor(ren, 0, 200, 0, 255); // Verde (Saúde Alta)
        } else if (jogador.vida > 20) {
            SDL_SetRenderDrawColor(ren, 255, 255, 0, 255); // Amarelo (Saúde Média)
        } else {
            SDL_SetRenderDrawColor(ren, 200, 0, 0, 255); // Vermelho (Saúde Baixa)
        }
        SDL_RenderFillRect(ren, &rVidaAtual);
        
        // --- Vidas Restantes ---
        const int SQUARE_SIZE = 25;
        const int SPACING = 5;
        const int START_X_LIVES = 340; 
        
        for (int i = 0; i < MAX_LIVES; i++) {
            SDL_Rect rLife = {
                START_X_LIVES + i * (SQUARE_SIZE + SPACING), 
                25, // Posição Y alinhada com o meio da barra
                40, 
                40
            };
            
            if (i < jogador.vidas) {
                SDL_RenderCopy(ren, img_vida, NULL, &rLife);
            } else {
                // Vida perdida
                SDL_RenderCopy(ren, img_vida2, NULL, &rLife);
            }
        }
        // =======================================================
        
        SDL_RenderPresent(ren);

        espera = 10;
    }
    
    // Mensagem de Game Over (nao está finalizada)
    if (game_over) {
        printf("\nFim de jogo, você perdeu todas as suas vidas!\n");
    }


    SDL_DestroyTexture(img);
    SDL_DestroyTexture(img2);
    SDL_DestroyTexture(img3);
    SDL_DestroyTexture(img4);
    SDL_DestroyTexture(img_fundo);
    SDL_DestroyTexture();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
