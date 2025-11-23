#include <assert.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h> // Adicionado para rand() e srand()
#include <stdlib.h> // Adicionado para rand() e srand()
#include <time.h>    // Adicionado para time()


#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
// --- Configurações do Mapa ---
#define MAP_WIDTH_TILES 60
#define MAP_HEIGHT_TILES 20
#define TILE_SIZE 80
#define MUMMY_COUNT 2
#define DANCER_COUNT 2

// Funções Auxiliares
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

// Definições de Estados (Mantidas)
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

// Definições de Structs (Mantidas)
typedef struct {
    int x, y;
    int vida;
    int alcanceAtaque;
    int danoAtaque;
    // O jogador precisa saber se está hipnotizado/enrolado,
    // mas não precisa de um vetor, pois só pode estar sob efeito de uma de cada vez.
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
    bool alvo_hipnotizado; // Novo campo para rastrear se a dançarina hipnotizou alguém
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

// --- Configurações do Mapa ---
// O mapa permanece inalterado
int map[MAP_HEIGHT_TILES][MAP_WIDTH_TILES] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,0,1,1},
    {1,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,1},
    {1,0,1,0,1,1,1,1,1,0,1,0,1,0,1,1,1,1,1,0,1,0,1,0,1,1,1,1,1,0,1,0,1,0,1,1,1,1,1,0,1,0,1,0,1,1,1,1,1,0,1,0,1,0,1,1,1,1,0,1},
    {1,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1},
    {1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,0,1,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// Protótipo da função de verificação de colisão
bool is_colliding(int x, int y, int w, int h);
void update_position(int* x, int* y, int dx, int dy);

// --- Implementação da Função de Colisão (Aprimorada na resposta anterior) ---
bool is_colliding(int x, int y, int w, int h) {
    // 1. Calcula a faixa de tiles que o objeto de (x, y, w, h) abrange
    // Os +5 e -5 são um 'padding' para tornar a colisão um pouco menos pixel-perfect
    int left_tile = (x + 5) / TILE_SIZE;
    int right_tile = (x + w - 5) / TILE_SIZE;
    int top_tile = (y + 5) / TILE_SIZE;
    int bottom_tile = (y + h - 5) / TILE_SIZE;

    // 2. Iterar sobre todos os tiles dentro dessa faixa
    for (int i = top_tile; i <= bottom_tile; i++) {
        for (int j = left_tile; j <= right_tile; j++) {
            
            // 3. Verificar os limites do mapa (Safeguard)
            if (i < 0 || i >= MAP_HEIGHT_TILES || j < 0 || j >= MAP_WIDTH_TILES) {
                return true; // Colisão com limites externos
            }

            // 4. Checar se o tile é de parede (tipo 1)
            if (map[i][j] == 1) {
                return true; // Colisão detectada
            }
        }
    }
    return false;
}

// --- Função para Mover o Jogador com Colisão (Mantida) ---
void update_position(int* x, int* y, int dx, int dy) {
    // Tamanho do jogador 70x70
    const int PLAYER_WIDTH = 70; 
    const int PLAYER_HEIGHT = 70;

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


int main(int argc, char* args[]) {
    // Inicialização da seed do gerador de números aleatórios
    srand(time(NULL));

    SDL_Init(SDL_INIT_EVERYTHING);
    
    SDL_Window* win = SDL_CreateWindow("Protótipo do Personagem",
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* img = IMG_LoadTexture(ren, "anim2.png");
    assert(img != NULL);

    // --- Personagem ---
    const int PLAYER_WIDTH = 70;
    const int PLAYER_HEIGHT = 70;
    const int PLAYER_SCREEN_X = (WINDOW_WIDTH - PLAYER_WIDTH) / 2;
    const int PLAYER_SCREEN_Y = (WINDOW_HEIGHT - PLAYER_HEIGHT) / 2;

    SDL_Rect r = {PLAYER_SCREEN_X, PLAYER_SCREEN_Y, PLAYER_WIDTH, PLAYER_HEIGHT};
    SDL_Rect c = {0, 0, 100, 80};

    // --- ENCONTRAR Posição Inicial do Jogador (Tipo '2' no mapa) ---
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

    // --- MÚMIAS (Vetor de Inimigos) ---
    Mumia mummies[MUMMY_COUNT] = {0};
    int mummy_spawn_coords[3][2] = {{300, 100}, {1500, 500}, {800, 200}}; // Coordenadas de spawn

    for(int i = 0; i < MUMMY_COUNT; i++) {
        mummies[i] = (Mumia){
            .estado = MUMIA_DORMINDO,
            .x = mummy_spawn_coords[i][0],
            .y = mummy_spawn_coords[i][1],
            .w = 40,
            .h = 40,
            .vida = 100,
            .danoAtaque = 20,
            .alcanceVisao2 = 200*200,
            .distanciaEnrolar2 = 50*50,
            .tempoEstado = 0,
            .dirX = 1,
            .dirY = 1
        };
    }

    // --- DANÇARINAS (Vetor de Suporte) ---
    Dancarina dancers[DANCER_COUNT] = {0};
    int dancer_spawn_coords[DANCER_COUNT][2] = {{1800, 700}, {1000, 100}}; 

    for(int i = 0; i < DANCER_COUNT; i++) {
        dancers[i] = (Dancarina){
            .estadoAtual = PARADA,
            .x = dancer_spawn_coords[i][0],
            .y = dancer_spawn_coords[i][1],
            .w = 50,
            .h = 50,
            .vida = 100,
            .alcanceVisaoQuadrado = 200 * 200,
            .raioHipnoseQuadrado = 70 * 70,
            .tempoEstado = 0,
            .deslocamento = 0,
            .direcaoDanca = 1,
            .alvo_hipnotizado = false // Inicializa como false
        };
    }

    // --- Jogador ---
    Jogador jogador = {
        .x = start_x, // Posição do mapa
        .y = start_y, // Posição do mapa
        .vida = 100,
        .danoAtaque = 20,
        .alcanceAtaque = 100 * 100,
        .hipnotizado = false,
        .enrolado = false,
        .tempoEstado = 0
    };

    int vel = 1;
    bool noChao = true;
    int chao = jogador.y; 
    bool subindo = true;
    bool acertou = false;
    int aux = 2;
    int k = 0;
    EstadoMovimento estado = PARADO;
    Uint32 tempoHipnoseInicio = 0; // Usada agora para rastrear a duração do status do jogador

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
                // Lógica de KEYUP (mantida)
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
        
        // --- NOVO: LÓGICA DAS DANÇARINAS (Iterando sobre o vetor) ---
        for(int i = 0; i < DANCER_COUNT; i++) {
            Dancarina* danca = &dancers[i];
            if (danca->vida <= 0) continue; // Dançarina morta

            int dx = jogador.x - danca->x;
            int dy = jogador.y - danca->y;
            int distQuadrada = (dx*dx) + (dy*dy);
            
            // Variável temporária para movimento
            int move_x = 0;
            int move_y = 0;

            switch(danca->estadoAtual) {
                case PARADA:
                    if (distQuadrada < danca->alcanceVisaoQuadrado) {
                        danca->estadoAtual = DANCANDO;
                        danca->tempoEstado = SDL_GetTicks();
                        printf("Dançarina %d começou a dançar.\n", i);
                    }
                    break;
                case DANCANDO:
                    // Move a dançarina um pouco
                    danca->deslocamento += danca->direcaoDanca;
                    if (danca->deslocamento > 20 || danca->deslocamento < -20) {
                        danca->direcaoDanca = -danca->direcaoDanca;
                    }
                    move_x = danca->direcaoDanca;
                    
                    if (distQuadrada < danca->raioHipnoseQuadrado) {
                        danca->estadoAtual = HIPNOTIZANDO;
                        danca->tempoEstado = SDL_GetTicks();
                        danca->alvo_hipnotizado = false; // Reset
                        printf("Dançarina %d está hipnotizando o jogador.\n", i);
                    } else if (distQuadrada > danca->alcanceVisaoQuadrado) {
                        danca->estadoAtual = PARADA;
                        printf("Dançarina %d parou de dançar.\n", i);
                    }
                    break;
                case HIPNOTIZANDO:
                    if (!danca->alvo_hipnotizado) {
                         jogador.hipnotizado = true; // Aplica o efeito no jogador
                         tempoHipnoseInicio = SDL_GetTicks();
                         danca->alvo_hipnotizado = true;
                    }
                    
                    if (SDL_GetTicks() - danca->tempoEstado > 3000) {
                        danca->estadoAtual = DANCANDO;
                        jogador.hipnotizado = false; // Solta o jogador
                        printf("Dançarina %d parou de hipnotizar. Jogador livre.\n", i);
                    } else if (distQuadrada > danca->raioHipnoseQuadrado) {
                        danca->estadoAtual = DANCANDO;
                        jogador.hipnotizado = false;
                        printf("Dançarina %d perdeu o alvo de vista, voltou a dançar.\n", i);
                    }
                    break;
                case ATORDOADA:
                    if (SDL_GetTicks() - danca->tempoEstado > 2000) {
                        danca->estadoAtual = PARADA;
                        printf("Dançarina %d se recuperou do atordoamento.\n", i);
                    }
                    break;
            }

            // Aplica o movimento da dançarina (basicamente só X)
            if (move_x != 0 || move_y != 0) {
                int new_dx = danca->x + move_x;
                if (!is_colliding(new_dx, danca->y, danca->w, danca->h)) {
                    danca->x = new_dx;
                } else {
                    danca->direcaoDanca = -danca->direcaoDanca; // Bateu na parede, inverte
                }
            }
        }
        
        // --- LÓGICA DAS MÚMIAS (Mantida) ---
        for(int i = 0; i < MUMMY_COUNT; i++) {
            Mumia* mumia = &mummies[i];

            if (mumia->vida <= 0) continue; // Múmia morta

            int mx = jogador.x - mumia->x;
            int my = jogador.y - mumia->y;
            int dist2 = mx*mx + my*my;
            
            // Variável temporária para movimento
            int move_x = 0;
            int move_y = 0;

            switch(mumia->estado) {
                case MUMIA_DORMINDO:
                    if(dist2 < mumia->alcanceVisao2) {
                        mumia->estado = MUMIA_PERSEGUINDO;
                        printf("Múmia %d acordou!\n", i);
                    }
                    break;
                
                case MUMIA_PERSEGUINDO:
                    if(dist2 < mumia->distanciaEnrolar2) {
                        mumia->estado = MUMIA_ENROLANDO;
                        mumia->tempoEstado = SDL_GetTicks();
                        printf("Mumia %d está enrolando o jogador, jogador tomou %d de dano!\n", i, mumia->danoAtaque);
                        jogador.enrolado = true; // Aplica o efeito no jogador
                        jogador.vida -= mumia->danoAtaque;
                    } else if(dist2 > mumia->alcanceVisao2) {
                        mumia->estado = MUMIA_CONFUSA;
                        mumia->tempoEstado = SDL_GetTicks();
                        printf("Múmia %d perdeu o jogador de vista e ficou confusa.\n", i);
                    } else {
                        static int frame = 0;
                        frame++;
                        if(frame % 2 == 0) { // Anda só metade dos frames para velocidade lenta
                            if(mx > 0) move_x = 1; else move_x = -1;
                            if(my > 0) move_y = 1; else move_y = -1;
                        }
                    }
                    break;

                case MUMIA_CONFUSA:
                    if(SDL_GetTicks() - mumia->tempoEstado > 1000) {
                        mumia->estado = MUMIA_DORMINDO;
                        printf("Múmia %d voltou a dormir.\n", i);
                    } else {
                        // Anda aleatório
                        move_x = mumia->dirX;
                        move_y = mumia->dirY;

                        if(rand() % 40 == 0) mumia->dirX = -mumia->dirX;
                        if(rand() % 40 == 0) mumia->dirY = -mumia->dirY;
                        if(dist2 < mumia->alcanceVisao2) mumia->estado = MUMIA_PERSEGUINDO;
                    }
                    break;

                case MUMIA_ENROLANDO:
                    // O dano foi aplicado no primeiro frame. 
                    if(SDL_GetTicks() - mumia->tempoEstado > 3000) {
                        mumia->estado = MUMIA_ATORDOADA;
                        mumia->tempoEstado = SDL_GetTicks();
                        jogador.enrolado = false; // Múmia soltou
                        printf("Múmia %d soltou o jogador!\n", i);
                    }
                    break;

                case MUMIA_ATORDOADA:
                    if(SDL_GetTicks() - mumia->tempoEstado > 2000) {
                        mumia->estado = MUMIA_DORMINDO;
                        printf("Múmia %d se recuperou.\n", i);
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
                    // Múmia bateu na parede em X, inverte direção aleatória
                    mumia->dirX = -mumia->dirX;
                }
                if (!is_colliding(mumia->x, new_my, mumia->w, mumia->h)) {
                    mumia->y = new_my;
                } else {
                    // Múmia bateu na parede em Y, inverte direção aleatória
                    mumia->dirY = -mumia->dirY;
                }
            }
        }

        // --- Lógica do personagem (Mantida) ---
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
                    // A movimentação horizontal usa update_position
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
                            estado=(teclas[SDL_SCANCODE_LEFT]||teclas[SDL_SCANCODE_RIGHT])? ANDANDO : PARADO; 
                            noChao=true; 
                            subindo=true; 
                        } 
                    }
                    break;

                case PARADO:
                    c = (SDL_Rect){0,0,100,80};
                    break;
                case ATACANDO:
                    // --- TENTAR ACERTAR AS MÚMIAS ---
                    for(int i = 0; i < MUMMY_COUNT; i++) {
                        Mumia* mumia = &mummies[i];
                        if (mumia->vida <= 0) continue;

                        int mx_atk = jogador.x - mumia->x;
                        int my_atk = jogador.y - mumia->y;
                        int dist2_mumia = mx_atk*mx_atk + my_atk*my_atk;

                        if (dist2_mumia < jogador.alcanceAtaque) {
                            mumia->vida -= jogador.danoAtaque;
                            acertou = true;
                            
                            // Atordoa a múmia no ataque
                            if(mumia->estado != MUMIA_ATORDOADA) {
                                mumia->estado = MUMIA_ATORDOADA;
                                mumia->tempoEstado = SDL_GetTicks();
                                if(jogador.enrolado) jogador.enrolado = false; // Quebra o enrolamento
                            }
                            printf("Múmia %d atingida. Dano: %d. Vida restante: %d\n", i, jogador.danoAtaque, mumia->vida);
                        }
                    }

                    // --- NOVO: TENTAR ACERTAR AS DANÇARINAS ---
                    for(int i = 0; i < DANCER_COUNT; i++) {
                        Dancarina* danca_atk = &dancers[i];
                        if (danca_atk->vida <= 0) continue;

                        int dx_atk = jogador.x - danca_atk->x;
                        int dy_atk = jogador.y - danca_atk->y;
                        int dist2_danca = dx_atk*dx_atk + dy_atk*dy_atk;

                        if (dist2_danca < jogador.alcanceAtaque) {
                            danca_atk->vida -= jogador.danoAtaque;
                            acertou = true;

                            // Atordoa a dançarina no ataque
                            if(danca_atk->estadoAtual != ATORDOADA) {
                                danca_atk->estadoAtual = ATORDOADA;
                                danca_atk->tempoEstado = SDL_GetTicks();
                                jogador.hipnotizado = false; // Quebra a hipnose
                            }
                            printf("Dançarina %d atingida. Dano: %d. Vida restante: %d\n", i, jogador.danoAtaque, danca_atk->vida);
                        }
                    }

                    if (!acertou) {
                        printf("Ataque do Jogador: Nenhum alvo no alcance.\n");
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
        
        // --- CÂMERA (OFFSET) ---
        int camera_offset_x = jogador.x - PLAYER_SCREEN_X;
        int camera_offset_y = jogador.y - PLAYER_SCREEN_Y;
        
        // --- RENDERIZAÇÃO ---
        SDL_SetRenderDrawColor(ren, 255,255,255,255);
        SDL_RenderClear(ren);

        // 1. Renderizar o MAPA
        for (int i = 0; i < MAP_HEIGHT_TILES; i++) {
            for (int j = 0; j < MAP_WIDTH_TILES; j++) {
                SDL_Rect rTile = {
                    j * TILE_SIZE - camera_offset_x,
                    i * TILE_SIZE - camera_offset_y,
                    TILE_SIZE,
                    TILE_SIZE
                };

                // Renderiza apenas tiles visíveis na tela
                if (rTile.x + TILE_SIZE > 0 && rTile.x < WINDOW_WIDTH &&
                    rTile.y + TILE_SIZE > 0 && rTile.y < WINDOW_HEIGHT) {
                        
                    if (map[i][j] == 1) { // Parede
                        SDL_SetRenderDrawColor(ren, 50, 50, 50, 255); // Cinza Escuro
                    } else if (map[i][j] == 2) { // Entrada
                        SDL_SetRenderDrawColor(ren, 0, 255, 0, 255); // Verde
                    } else if (map[i][j] == 3) { // Saída
                        SDL_SetRenderDrawColor(ren, 255, 0, 0, 255); // Vermelho
                        // Lógica de vitória
                    } else { // Caminho Livre (0)
                        SDL_SetRenderDrawColor(ren, 200, 200, 200, 255); // Cinza Claro
                    }
                    SDL_RenderFillRect(ren, &rTile);
                }
            }
        }
        

        // 2. Renderizar o Jogador
        SDL_RenderCopy(ren, img, &c, &r);
        
        // 3. NOVO: Renderizar as Dançarinas (Iterando sobre o vetor)
        for(int i = 0; i < DANCER_COUNT; i++) {
            Dancarina* danca_render = &dancers[i];
            if (danca_render->vida <= 0) continue; // Dançarina morta

            SDL_Rect rDanca = {
                danca_render->x - camera_offset_x,
                danca_render->y - camera_offset_y,
                danca_render->w,
                danca_render->h
            };
            SDL_Color corDanca;
            switch(danca_render->estadoAtual) {
                 case DANCANDO: corDanca=(SDL_Color){128,0,128,255}; break;
                 case HIPNOTIZANDO: corDanca=(SDL_Color){255,105,180,255}; break; // Rosa
                 case ATORDOADA: corDanca=(SDL_Color){128,128,128,255}; break;
                 case PARADA: corDanca=(SDL_Color){0,255,0,255}; break;
            }
            SDL_SetRenderDrawColor(ren, corDanca.r, corDanca.g, corDanca.b, 255);
            SDL_RenderFillRect(ren, &rDanca);
        }


        // 4. Renderizar as Múmias (Mantida)
        for(int i = 0; i < MUMMY_COUNT; i++) {
            Mumia* mumia = &mummies[i];
            if (mumia->vida <= 0) continue;

            SDL_Rect rMumia = {
                mumia->x - camera_offset_x,
                mumia->y - camera_offset_y,
                mumia->w,
                mumia->h
            };

            SDL_Color corMumia;

            switch(mumia->estado) {
                case MUMIA_DORMINDO:      corMumia = (SDL_Color){200,200,0,255}; break;
                case MUMIA_PERSEGUINDO:  corMumia = (SDL_Color){255,150,0,255}; break;
                case MUMIA_CONFUSA:      corMumia = (SDL_Color){0,180,255,255}; break;
                case MUMIA_ENROLANDO:    corMumia = (SDL_Color){255,0,0,255}; break;
                case MUMIA_ATORDOADA:    corMumia = (SDL_Color){120,120,120,255}; break;
            }

            SDL_SetRenderDrawColor(ren, corMumia.r, corMumia.g, corMumia.b, 255);
            SDL_RenderFillRect(ren, &rMumia);
        }

        SDL_RenderPresent(ren);

        espera = 10;
    }

    SDL_DestroyTexture(img);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
