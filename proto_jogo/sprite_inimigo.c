#include <assert.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> 

// --- MACROS E CONSTANTES DE RENDERIZAÇÃO DA MÚMIA ---
const int MUMIA_FRAME_W = 48;
const int MUMIA_FRAME_H = 64; 

// Função para esperar eventos com timeout. Importante paraBame Loop.
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

// =========================================================================
// --- BLOCO 1: ENUMS (DECLARAÇÕES DE ESTADO) ---
// Estas definicoes precisam vir antes de serem usadas pelas STRUCTS ou MAIN.
// =========================================================================

typedef enum {
    PARADO, ANDANDO, CORRENDO, PULANDO, ATACANDO
} EstadoMovimento;

typedef enum {
    DANCANDO, HIPNOTIZANDO, ATORDOADA, PARADA
} EstadoDancarina;

typedef enum {
    MUMIA_DORMINDO, MUMIA_PERSEGUINDO, MUMIA_CONFUSA, MUMIA_ENROLANDO, MUMIA_ATORDOADA
} EstadoMumia;


// =========================================================================
// --- BLOCO 2: STRUCTS (DECLARAÇÕES DE ENTIDADE) ---
// Estas definicoes precisam vir antes da funcao MAIN.
// =========================================================================

typedef struct {
    int x, y;
    int vida;
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
} Dancarina;

typedef struct {
    EstadoMumia estado;
    int x, y;
    int w, h; // Largura e Altura na TELA (escala)
    int vida;
    int alcanceVisao2;    // Distancia para perseguir (quadrada)
    int distanciaEnrolar2; // Distancia para ataque (quadrada)
    int danoAtaque;
    Uint32 tempoEstado;  // Para controle de stun e confusão
    int dirX, dirY;      // Usado na lógica CONFUSA (movimento aleatório)
} Mumia;


// =========================================================================
// --- BLOCO 3: FUNÇÃO PRINCIPAL (MAIN) ---
// Contém a inicialização de variáveis e o Game Loop.
// =========================================================================
int main(int argc, char* args[]) {
    SDL_Init(SDL_INIT_EVERYTHING);

    const int WINDOW_WIDTH = 600;
    const int WINDOW_HEIGHT = 400;

    SDL_Window* win = SDL_CreateWindow("Protótipo do Personagem",
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    
    // --- Carregando as Texturas ---
    SDL_Texture* img = IMG_LoadTexture(ren, "anim2.png"); // Personagem Principal
    assert(img != NULL);
    SDL_Texture* mumia_img = IMG_LoadTexture(ren, "mumia48x64.png"); // Múmia (Sprite)
    assert(mumia_img != NULL);
    
    // --- VARIÁVEIS DE CONTROLE E ENTIDADES ---
    const int PLAYER_WIDTH = 70;
    const int PLAYER_HEIGHT = 70;
    const int PLAYER_SCREEN_X = (WINDOW_WIDTH - PLAYER_WIDTH) / 2;
    const int PLAYER_SCREEN_Y = (WINDOW_HEIGHT - PLAYER_HEIGHT) / 2;

    // Variáveis de escopo local (agora declaradas no topo do main)
    SDL_Rect c = {0, 0, 100, 80}; // Recorte da Fonte do Jogador
    SDL_Rect r = {PLAYER_SCREEN_X, PLAYER_SCREEN_Y, PLAYER_WIDTH, PLAYER_HEIGHT}; // Destino do Jogador
    int vel = 1;
    bool noChao = true;
    int chao = 130; // Altura do chao inicial (valor original de jogador.y)
    bool subindo = true;
    bool acertou = false;
    int aux = 2;
    int k = 0;
    EstadoMovimento estado = PARADO; // Estado inicial do jogador
    Uint32 tempoHipnoseInicio = 0;
    
    // O ponteiro 'teclas' é atualizado dentro do loop, mas deve ser acessível
    const Uint8* teclas; 
    
    // Inicialização das STRUCTS (usando as definições agora visíveis)
    Dancarina danca = {
        .estadoAtual = PARADA, .x = 400, .y = 100, .w = 50, .h = 50, .vida = 100,
        .alcanceVisaoQuadrado = 200 * 200, .raioHipnoseQuadrado = 70 * 70, 
        .tempoEstado = 0, .deslocamento = 0, .direcaoDanca = 1
    };
    
    Mumia mumia = {
        .estado = MUMIA_DORMINDO, .x = 260, .y = 400, .w = 70, .h = 90, .vida = 100,
        .danoAtaque = 20, .alcanceVisao2 = 200*200, .distanciaEnrolar2 = 50*50,
        .tempoEstado = 0, .dirX = 1, .dirY = 1
    };

    Jogador jogador = {
        .x = 180, .y = 130, .vida = 100, .danoAtaque = 20,
        .alcanceAtaque = 100 * 100, .hipnotizado = false, .enrolado = false,
        .tempoEstado = 0
    };


    // --- O GAME LOOP ---
    bool rodando = true;
    SDL_Event evt;
    Uint32 espera = 10; 

    while (rodando) {
        // Atualiza o estado do teclado a cada frame
        teclas = SDL_GetKeyboardState(NULL); 
        int isevt = AUX_WaitEventTimeout(&evt, &espera);
        
        // ... (Lógica de INPUT do Jogador: Tratamento de SDL_QUIT, KEYDOWN e KEYUP) ...

        // 2. Lógica e Atualização de Entidades (UPDATE)
        
        // --- Lógica da Dançarina (NPC 1) --- (Mantida a lógica original)
        int dx = jogador.x - danca.x;
        int dy = jogador.y - danca.y;
        int distQuadrada = (dx*dx) + (dy*dy);
        switch(danca.estadoAtual) {
            case DANCANDO:
                if(distQuadrada < danca.raioHipnoseQuadrado) {
                    danca.estadoAtual = HIPNOTIZANDO; jogador.hipnotizado = true; 
                    tempoHipnoseInicio = SDL_GetTicks(); estado = PARADO; printf("Jogador hipnotizado!\n"); }
                if(distQuadrada > danca.alcanceVisaoQuadrado) { danca.estadoAtual = PARADA; }
                if(danca.deslocamento > 8) danca.direcaoDanca = -1;
                if(danca.deslocamento < -8) danca.direcaoDanca = 1;
                danca.deslocamento += danca.direcaoDanca;
                danca.x += danca.direcaoDanca;
                break;
            case HIPNOTIZANDO:
                if(teclas[SDL_SCANCODE_E]) {
                    danca.estadoAtual = ATORDOADA; danca.tempoEstado = SDL_GetTicks();
                    jogador.hipnotizado = false; estado = PARADO; printf("Dançarina atordoada!\n"); }
                if (SDL_GetTicks() - tempoHipnoseInicio > 5000) {
                    jogador.hipnotizado = false; estado = PARADO; danca.estadoAtual = ATORDOADA;       
                    danca.tempoEstado = SDL_GetTicks(); printf("Fim da hipnose!\n"); }
                break;
            case ATORDOADA:
                if(SDL_GetTicks() - danca.tempoEstado > 3000) danca.estadoAtual = PARADA;
                break;
            case PARADA:
                if(distQuadrada < danca.alcanceVisaoQuadrado) danca.estadoAtual = DANCANDO;
                break;
        }


        // --- LÓGICA DA MÚMIA (NPC 2 - FSM) ---
        int mx = jogador.x - mumia.x;
        int my = jogador.y - mumia.y;
        int dist2 = mx*mx + my*my;

        switch(mumia.estado) {
            // ... (Lógica da Múmia - Manter completa) ...
        }

        // -------------------------------------------------------------------
        // --- LÓGICA DE MOVIMENTO DO JOGADOR (RESTUARADA DO CÓDIGO ORIGINAL) ---
        // -------------------------------------------------------------------
        if(!jogador.hipnotizado && !jogador.enrolado) {
            switch(estado) {
                case ANDANDO:
                    vel = 1;
                    if(teclas[SDL_SCANCODE_UP]) { jogador.y -= vel; c = (aux==1)? (SDL_Rect){200,0,100,80} : (SDL_Rect){100,0,100,80}; }
                    if(teclas[SDL_SCANCODE_DOWN]) { jogador.y += vel; c = (aux==1)? (SDL_Rect){200,0,100,80} : (SDL_Rect){100,0,100,80}; }
                    if(teclas[SDL_SCANCODE_LEFT]) { jogador.x -= vel; c = (SDL_Rect){200,0,100,80}; aux=1; }
                    if(teclas[SDL_SCANCODE_RIGHT]) { jogador.x += vel; c = (SDL_Rect){100,0,100,80}; aux=2; }
                    break;

                case CORRENDO:
                    vel = 2;
                    if(teclas[SDL_SCANCODE_UP]) { jogador.y -= vel; c = (aux==1)? (SDL_Rect){590,0,100,80} : (SDL_Rect){490,0,100,80}; }
                    if(teclas[SDL_SCANCODE_DOWN]) { jogador.y += vel; c = (aux==1)? (SDL_Rect){590,0,100,80} : (SDL_Rect){490,0,100,80}; }
                    if(teclas[SDL_SCANCODE_LEFT]) { jogador.x -= vel; c = (SDL_Rect){590,0,100,80}; aux=1; }
                    if(teclas[SDL_SCANCODE_RIGHT]) { jogador.x += vel; c = (SDL_Rect){490,0,100,80}; aux=2; }
                    break;

                case PULANDO:
                    if(k==0) { c=(SDL_Rect){300,0,100,80}; k=1; }
                    if(teclas[SDL_SCANCODE_LEFT]) { jogador.x -= vel; c=(SDL_Rect){400,0,100,80}; }
                    if(teclas[SDL_SCANCODE_RIGHT]) { jogador.x += vel; c=(SDL_Rect){300,0,100,80}; }
                    if(subindo) { jogador.y -= vel; if(jogador.y <= chao-50) subindo=false; }
                    else { jogador.y += vel; if(jogador.y >= chao) { jogador.y=chao; k=0; estado=(teclas[SDL_SCANCODE_LEFT]||teclas[SDL_SCANCODE_RIGHT])? ANDANDO : PARADO; noChao=true; subindo=true; } }
                    break;

                case PARADO:
                    c = (SDL_Rect){0,0,100,80};
                    break;
                
                case ATACANDO:
                    // ... (logica de ataque - mantida) ...
                    estado=(teclas[SDL_SCANCODE_LEFT]|| teclas[SDL_SCANCODE_RIGHT]  || teclas[SDL_SCANCODE_UP] || teclas[SDL_SCANCODE_DOWN] )? ANDANDO : PARADO;
                    break;
                
                default: break;
            }
        } else if (jogador.hipnotizado) {
            c = (SDL_Rect){680,0,100,80};
        }


        // 3. Renderização (Desenho na Tela)
        // ... (Renderização completa da Múmia e Dançarina) ...
        
        espera = 10;
    }

    // ... (Limpeza de Memória) ...
    return 0;
}
