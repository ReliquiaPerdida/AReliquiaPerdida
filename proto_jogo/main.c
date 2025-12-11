//gcc -o reliquia main.c $(pkg-config --cflags --libs sdl2 SDL2_image SDL2_ttf)
#include <assert.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> 
#include <time.h>   
#include <string.h> 

// --- MACROS E CONSTANTES DE RENDERIZAÇÃO ---
const int WINDOW_WIDTH = 1920; 
const int WINDOW_HEIGHT = 1080;
const int MUMIA_FRAME_W = 48;
const int MUMIA_FRAME_H = 64; 
const int DANCA_FRAME_W = 23; 
const int DANCA_FRAME_H = 35; 

// --- CONFIGURAÇÕES DO JOGO IMERSIVO ---
#define MAP_WIDTH_TILES 30
#define MAP_HEIGHT_TILES 30
#define TILE_SIZE 80
#define RELIC_TILE 4
#define MUMMY_COUNT 6
#define DANCER_COUNT 6
#define PLAYER_WIDTH 60
#define PLAYER_HEIGHT 60
#define MIN_SPAWN_DIST_TILES 5 
#define NUM_AREAS 3 
#define NUM_TREASURES_TO_SPAWN 2
#define MAX_LIVES 2

// --- DEFINIÇÕES DE ESTADO DO JOGO ---
typedef enum {
    GAME_MENU,      // Tela inicial com botões
    GAME_PLAYING,   // O jogo está rodando
    GAME_OVER,      // Tela de Game Over
    GAME_VICTORY,   // Tela de Vitória
    GAME_EXITING    // Prepara para fechar
} GameState;

// --- ENUM DIDÁTICO E DE ESTADOS ---
typedef enum {
    DIREITA = 1, ESQUERDA = -1
} DirecaoVisual;
typedef enum {
    PARADO, ANDANDO, CORRENDO, PULANDO, ATACANDO
} EstadoMovimento;
typedef enum {
    DANCANDO, HIPNOTIZANDO, ATORDOADA, PARADA
} EstadoDancarina;
typedef enum enum_mumia {
    MUMIA_DORMINDO, MUMIA_PERSEGUINDO, MUMIA_CONFUSA, MUMIA_ENROLANDO, MUMIA_ATORDOADA
} EstadoMumia;
typedef enum {
    TESOURO_TOCHA, TESOURO_CALICE_SAGRADO, TESOURO_PERGAMINHO, TESOURO_MALCIDAO, TESOURO_ESTATUETA
} TipoTesouro;


// --- STRUCTS: AS ENTIDADES DO JOGO (Estruturas do código Imersivo) ---
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
    int x, y; int w, h; int vida; int alcanceVisaoQuadrado;  
    int raioHipnoseQuadrado; Uint32 tempoEstado; int deslocamento; int direcaoDanca;
    int dirX, dirY;
    bool alvo_hipnotizado;
} Dancarina; // Nota: Remoção da frameRecorte e direcaoVisual (ficam na função aux_dancarina)

typedef struct {
    EstadoMumia estado;
    int x, y; int w, h; 
    int vida; int alcanceVisao2; int distanciaEnrolar2; int danoAtaque;
    Uint32 tempoEstado; int dirX, dirY; 
} Mumia;

typedef struct {
    int x, y; int w, h;
    bool coletada[NUM_AREAS];
} GlobalReliquia;

typedef struct {
    TipoTesouro tipo;
    int x, y; int w, h;
    bool coletado;
} Tesouro;


// --- VARIÁVEIS GLOBAIS (do código imersivo) ---
int current_phase = 0;
// Note: As arrays de mapas map0, map1, map2 e ALL_MAPS[] e current_map DEVEM SER DEFINIDAS ANTES DE main.
// Aqui estão como funções externas, pois assumo que você as tem em seu arquivo.
extern int (*current_map)[MAP_WIDTH_TILES];
extern int (*ALL_MAPS[NUM_AREAS])[MAP_WIDTH_TILES];

// --- FUNÇÕES AUXILIARES (Assumidas como Externas/Definidas no arquivo) ---
// Estas funções DEVERIAM ESTAR DEFINIDAS ANTES DE main.
extern bool is_colliding(int x, int y, int w, int h);
extern void update_position(int* x, int* y, int dx, int dy);
extern void find_valid_spawn(int* out_x, int* out_y, int entity_w, int entity_h, int player_start_x, int player_start_y);
extern void render_text(SDL_Renderer* ren, TTF_Font* font, const char* text, int x, int y, SDL_Color color);
extern void initialize_phase(int phase_index, Jogador* jogador, Mumia mummies[], Dancarina dancers[], Tesouro treasures[], GlobalReliquia* relic, bool* visaoExtra, char current_message[], Uint32* message_start_time);
extern void reset_game(Jogador* jogador, GlobalReliquia* relic, bool* visaoExtra, char current_message[], Uint32* message_start_time, Uint32* game_start_time, Mumia mummies[], Dancarina dancers[], Tesouro treasures[]);

// --- FUNÇÃO AUXILIAR PARA ATUALIZAR O SPRITE DA DANÇARINA (simplificada) ---
void aux_dancarina(Dancarina* danca) {
    // Esta função precisaria de acesso direto à textura para ser 100% precisa, 
    // mas está aqui para manter a FSM da Dançarina funcional
}


// --- FUNÇÃO PRINCIPAL: O GAME LOOP ---
int main(int argc, char* args[]) {
    srand(time(NULL));

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0 || TTF_Init() == -1) {
        fprintf(stderr, "Erro na Inicializacao SDL/TTF!\n");
        return 1;
    }
    
    SDL_Window* win = SDL_CreateWindow("A Reliquia Perdida",
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    
    // --- Carregamento de Texturas (Completo do Bruno) ---
    SDL_Texture* img = IMG_LoadTexture(ren, "anim2.png");
    SDL_Texture* img2 = IMG_LoadTexture(ren, "escuridao.png");
    SDL_Texture* img3 = IMG_LoadTexture(ren, "escuridao2.png");
    SDL_Texture* img4 = IMG_LoadTexture(ren, "bloco.png");
    SDL_Texture* img_fundo = IMG_LoadTexture(ren, "fundo.png");
    SDL_Texture* img_menu_fundo = IMG_LoadTexture(ren, "fundomenu.png"); // NOVO FUNDO DE MENU
    SDL_Texture* img_mumia = IMG_LoadTexture(ren, "mumia.png");
    SDL_Texture* img_dancarina = IMG_LoadTexture(ren, "dancarina.png");
    SDL_Texture* img_tocha = IMG_LoadTexture(ren, "tocha.png");
    SDL_Texture* img_calice = IMG_LoadTexture(ren, "calice.png");
    SDL_Texture* img_vida = IMG_LoadTexture(ren, "vida.png");
    SDL_Texture* img_vida2 = IMG_LoadTexture(ren, "vida2.png");
    
    TTF_Font* font_small = TTF_OpenFont("tiny.ttf", 24); 
    TTF_Font* font_large = TTF_OpenFont("tiny.ttf", 48); 
    
    SDL_Cursor* cursor_mao = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    SDL_Cursor* cursor_seta = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    
    // Asserções para garantir que os arquivos foram carregados
    assert(img != NULL && img2 != NULL && img3 != NULL && img4 != NULL && img_fundo != NULL && img_menu_fundo != NULL && 
           img_mumia != NULL && img_dancarina != NULL && img_tocha != NULL && img_calice != NULL && 
           img_vida != NULL && img_vida2 != NULL);
    assert(font_small != NULL && font_large != NULL);

    // --- DEFINIÇÃO DOS BOTÕES (Novo Menu de Mouse 1920x1080) ---
    const int BTN_W = 300;
    const int BTN_H = 105;
    
    SDL_Rect btn_opcoes = { 40, 940, BTN_W, BTN_H }; 
    SDL_Rect btn_play = { 820, 946, BTN_W, BTN_H };
    SDL_Rect btn_sair = { 1593, 948, BTN_W, BTN_H };
    SDL_Rect rFundo = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT }; 

    // --- Variáveis de Entidade e Controle do Jogo do Bruno ---
    const int PLAYER_SCREEN_X = (WINDOW_WIDTH - PLAYER_WIDTH) / 2;
    const int PLAYER_SCREEN_Y = (WINDOW_HEIGHT - PLAYER_HEIGHT) / 2;

    SDL_Rect r = {PLAYER_SCREEN_X, PLAYER_SCREEN_Y, PLAYER_WIDTH, PLAYER_HEIGHT};
    SDL_Rect c = {0, 0, 100, 80}; // Frame de recorte do jogador
    int vel = 1; bool noChao = true; int chao = 130; bool subindo = true; bool acertou = false;
    int aux = 2; int k = 0; EstadoMovimento estado = PARADO; // Estado de movimento do jogador
    const Uint8* teclas = SDL_GetKeyboardState(NULL);
    
    // Inicialização das STRUCTS grandes (o reset_game fará a inicialização completa)
    Jogador jogador = {0};
    GlobalReliquia global_relic = {0};
    Mumia mummies[MUMMY_COUNT] = {0};
    Dancarina dancers[DANCER_COUNT] = {0};
    Tesouro treasures[NUM_TREASURES_TO_SPAWN] = {0};
    bool visaoExtra = false;
    char current_message[256] = "";
    Uint32 message_start_time = 0;
    Uint32 game_start_time = 0;
    Uint32 game_end_time = 0;

    GameState game_state = GAME_MENU; 
    bool rodando = true;
    SDL_Event evt;

    while (rodando) {
        
        // 1. PROCESSAR INPUT E EVENTOS
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        SDL_Point mouse_pos = { mouseX, mouseY };

        while (SDL_PollEvent(&evt)) { 
            if (evt.type == SDL_QUIT) rodando = false; 
            
            // --- Lógica do NOVO MENU (Clique do Mouse) ---
            if (game_state == GAME_MENU || game_state == GAME_VICTORY || game_state == GAME_OVER) {
                if (evt.type == SDL_MOUSEBUTTONDOWN) {
                    if (SDL_PointInRect(&mouse_pos, &btn_play)) {
                        // O botão PLAY/JOGAR NOVAMENTE sempre executa o reset_game
                        reset_game(&jogador, &global_relic, &visaoExtra, current_message, &message_start_time, &game_start_time, mummies, dancers, treasures);
                        game_state = GAME_PLAYING;
                    }
                    else if (SDL_PointInRect(&mouse_pos, &btn_sair)) {
                        rodando = false;
                    }
                }
            } 
            // --- Lógica de Input do Jogo (Teclado) ---
            else if (game_state == GAME_PLAYING) {
                if (evt.type == SDL_KEYDOWN) {
                    if(!jogador.hipnotizado && !jogador.enrolado) { 
                        switch (evt.key.keysym.sym) {
                            case SDLK_LSHIFT: if(noChao) estado = CORRENDO; break;
                            case SDLK_SPACE: if(noChao) { chao = jogador.y; estado = PULANDO; noChao = false; } break;
                            case SDLK_k: acertou = false; estado = ATACANDO; break;
                            case SDLK_UP: case SDLK_DOWN: case SDLK_LEFT: case SDLK_RIGHT:
                                if(estado != CORRENDO && estado != PULANDO && noChao) estado = ANDANDO; break;
                        }
                    }
                } else if (evt.type == SDL_KEYUP) {
                    if(!jogador.hipnotizado && !jogador.enrolado && noChao) { 
                        if(evt.key.keysym.sym == SDLK_LSHIFT) {
                            if(teclas[SDL_SCANCODE_LEFT] || teclas[SDL_SCANCODE_RIGHT] || teclas[SDL_SCANCODE_UP] || teclas[SDL_SCANCODE_DOWN]) estado = ANDANDO;
                            else estado = PARADO;
                        }
                        if(evt.key.keysym.sym == SDLK_UP || evt.key.keysym.sym == SDLK_DOWN || teclas[SDL_SCANCODE_LEFT] || teclas[SDL_SCANCODE_RIGHT]) {
                            if(teclas[SDL_SCANCODE_LEFT] || teclas[SDL_SCANCODE_RIGHT] || teclas[SDL_SCANCODE_UP] || teclas[SDL_SCANCODE_DOWN]) {
                                if(teclas[SDL_SCANCODE_LSHIFT]) estado = CORRENDO;
                                else estado = ANDANDO;
                            } else estado = PARADO;
                        }
                    }
                }
            }
        } // Fim do Loop de Eventos

        // 2. LÓGICA DE HOVER / CURSOR
        if (game_state == GAME_MENU || game_state == GAME_VICTORY || game_state == GAME_OVER) {
             if (SDL_PointInRect(&mouse_pos, &btn_play) || SDL_PointInRect(&mouse_pos, &btn_sair) || SDL_PointInRect(&mouse_pos, &btn_opcoes)) {
                SDL_SetCursor(cursor_mao);
            } else {
                SDL_SetCursor(cursor_seta);
            }
        } else {
             SDL_SetCursor(cursor_seta);
        }

        // 3. ATUALIZAÇÃO DO JOGO (UPDATE)
        if (game_state == GAME_PLAYING) {
            // --- Lógica de Transição de Fase / Game Over do Bruno ---
            if (jogador.vida <= 0) {
                // ... (Lógica de Game Over/Respawn dele) ...
            }
            // ... (Lógica de Relíquia, Tesouro, Transição de Fase, etc. dele) ...
            
            // Lógica da Dançarina
            int dx = jogador.x - dancers[0].x; int dy = jogador.y - dancers[0].y; int distQuadrada = (dx*dx) + (dy*dy);
            switch(dancers[0].estadoAtual) {
                case DANCANDO:
                    if(distQuadrada < dancers[0].raioHipnoseQuadrado) { dancers[0].estadoAtual = HIPNOTIZANDO; jogador.hipnotizado = true; dancers[0].tempoEstado = SDL_GetTicks(); estado = PARADO; }
                    if(distQuadrada > dancers[0].alcanceVisaoQuadrado) { dancers[0].estadoAtual = PARADA; }
                    if(dancers[0].deslocamento > 8) dancers[0].direcaoDanca = -1;
                    if(dancers[0].deslocamento < -8) dancers[0].direcaoDanca = 1; 
                    dancers[0].deslocamento += dancers[0].direcaoDanca; dancers[0].x += dancers[0].direcaoDanca;
                    break;
                case HIPNOTIZANDO:
                    if(teclas[SDL_SCANCODE_E]) { dancers[0].estadoAtual = ATORDOADA; dancers[0].tempoEstado = SDL_GetTicks(); jogador.hipnotizado = false; estado = PARADO; }
                    if (SDL_GetTicks() - dancers[0].tempoEstado > 5000) { jogador.hipnotizado = false; estado = PARADO; dancers[0].estadoAtual = ATORDOADA; dancers[0].tempoEstado = SDL_GetTicks(); }
                    break;
                case ATORDOADA: if(SDL_GetTicks() - dancers[0].tempoEstado > 3000) dancers[0].estadoAtual = PARADA; break;
                case PARADA: if(distQuadrada < dancers[0].alcanceVisaoQuadrado) dancers[0].estadoAtual = DANCANDO; break;
            }
            aux_dancarina(&dancers[0]);
            
            // ... (Restante da lógica de Inimigos e Jogador) ...
        }


        // 4. RENDERIZAÇÃO
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        SDL_RenderClear(ren);

        if (game_state == GAME_MENU) {
            // RENDERIZAÇÃO DO NOVO MENU
            SDL_RenderCopy(ren, img_menu_fundo, NULL, &rFundo);
            // Renderização opcional dos nomes dos botões usando SDL_ttf aqui.
            // ...
            
        } else if (game_state == GAME_PLAYING) {
            // RENDERIZAÇÃO DO JOGO (Mapa, Player, Inimigos, HUD)
            
            // ... (Manter toda a renderização do Mapa, Relíquia, Tesouros, Inimigos, HUD do Bruno) ...
            
        } else if (game_state == GAME_VICTORY || game_state == GAME_OVER) {
            // RENDERIZAÇÃO DAS TELAS FINAIS (Usar render_text do Bruno)
            
            // ... (Manter a renderização de Game Over / Vitória do Bruno) ...
        }

        SDL_RenderPresent(ren);
        SDL_Delay(10);
    }

    // --- LIMPEZA DE MEMÓRIA (FINALIZAÇÃO) ---
    SDL_FreeCursor(cursor_mao); 
    SDL_FreeCursor(cursor_seta);
    // ... (Limpar todas as texturas e fontes) ...
    TTF_CloseFont(font_small);
    TTF_CloseFont(font_large);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
