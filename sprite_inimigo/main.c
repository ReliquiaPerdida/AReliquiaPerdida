#include <assert.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> 

// --- MACROS E CONSTANTES DE RENDERIZAÇÃO DA MÚMIA ---
const int MUMIA_FRAME_W = 48;
const int MUMIA_FRAME_H = 64; 

// --- FUNÇÃO AUXILIAR DO BRUNO ---
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

// --- ENUMS: OS ESTADOS DAS ENTIDADES ---
typedef enum {
    PARADO, ANDANDO, CORRENDO, PULANDO, ATACANDO
} EstadoMovimento;
typedef enum {
    DANCANDO, HIPNOTIZANDO, ATORDOADA, PARADA
} EstadoDancarina;
typedef enum enum_mumia { // Alterei para enum_mumia para evitar conflitos de nome.
    MUMIA_DORMINDO, MUMIA_PERSEGUINDO, MUMIA_CONFUSA, MUMIA_ENROLANDO, MUMIA_ATORDOADA
} EstadoMumia;

// --- STRUCTS: AS ENTIDADES DO JOGO ---
typedef struct {
    int x, y; int vida; int alcanceAtaque; int danoAtaque; bool hipnotizado; bool enrolado; Uint32 tempoEstado;
} Jogador;
typedef struct {
    EstadoDancarina estadoAtual; int x, y; int w, h; int vida; int alcanceVisaoQuadrado;  
    int raioHipnoseQuadrado; Uint32 tempoEstado; int deslocamento; int direcaoDanca;
} Dancarina;
typedef struct {
    EstadoMumia estado; int x, y; int w, h; // Largura e Altura na TELA (escala)
    int vida; int alcanceVisao2; int distanciaEnrolar2; int danoAtaque;
    Uint32 tempoEstado; int dirX, dirY; 
} Mumia;


// --- FUNÇÃO PRINCIPAL: O GAME LOOP ---
int main(int argc, char* args[]) {
    // --- INICIALIZAÇÃO SDL ---
    SDL_Init(SDL_INIT_EVERYTHING);

    const int WINDOW_WIDTH = 600;
    const int WINDOW_HEIGHT = 400;

    SDL_Window* win = SDL_CreateWindow("Protótipo do Personagem",
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    
    // --- Carregando as Texturas ---
    SDL_Texture* img = IMG_LoadTexture(ren, "anim2.png"); 
    assert(img != NULL);
    SDL_Texture* mumia_img = IMG_LoadTexture(ren, "mumia48x64.png"); // Múmia (Sprite)
    assert(mumia_img != NULL);

    // --- Variáveis de Controle e Entidades ---
    const int PLAYER_WIDTH = 70;
    const int PLAYER_HEIGHT = 70;
    const int PLAYER_SCREEN_X = (WINDOW_WIDTH - PLAYER_WIDTH) / 2;
    const int PLAYER_SCREEN_Y = (WINDOW_HEIGHT - PLAYER_HEIGHT) / 2;

    SDL_Rect r = {PLAYER_SCREEN_X, PLAYER_SCREEN_Y, PLAYER_WIDTH, PLAYER_HEIGHT}; 
    SDL_Rect c = {0, 0, 100, 80}; 
    int vel = 1; bool noChao = true; int chao = 130; bool subindo = true; bool acertou = false;
    int aux = 2; int k = 0; EstadoMovimento estado = PARADO; Uint32 tempoHipnoseInicio = 0;
    const Uint8* teclas = SDL_GetKeyboardState(NULL); 
    
    Dancarina danca = {
        .estadoAtual = PARADA, .x = 400, .y = 100, .w = 50, .h = 50, .vida = 100,
        .alcanceVisaoQuadrado = 200 * 200, .raioHipnoseQuadrado = 70 * 70, .tempoEstado = 0, 
        .deslocamento = 0, .direcaoDanca = 1
    };
    
    Mumia mumia = {
        .estado = MUMIA_DORMINDO, .x = 260, .y = 400, .w = 70, .h = 90, .vida = 100,
        .danoAtaque = 20, .alcanceVisao2 = 200*200, .distanciaEnrolar2 = 50*50,
        .tempoEstado = 0, .dirX = 1, .dirY = 1
    };

    Jogador jogador = {
        .x = 180, .y = 130, .vida = 100, .danoAtaque = 20,
        .alcanceAtaque = 100 * 100, .hipnotizado = false, .enrolado = false, .tempoEstado = 0
    };


    // --- O GAME LOOP ---
    bool rodando = true;
    SDL_Event evt;
    Uint32 espera = 10; 

    while (rodando) {
        // ... (1. Processar Input) ...
        teclas = SDL_GetKeyboardState(NULL); 
        int isevt = AUX_WaitEventTimeout(&evt, &espera);
        
        if (isevt) {
            if (evt.type == SDL_QUIT) rodando = false;
            else if (evt.type == SDL_KEYDOWN) {
                if(!jogador.hipnotizado && !jogador.enrolado) { 
                    switch (evt.key.keysym.sym) {
                        case SDLK_LSHIFT: if(noChao) estado = CORRENDO; break;
                        case SDLK_SPACE: 
                            if(noChao) { chao = jogador.y; estado = PULANDO; noChao = false; }
                            break;
                        case SDLK_k: acertou = false; estado = ATACANDO; break;
                        case SDLK_UP: case SDLK_DOWN: case SDLK_LEFT: case SDLK_RIGHT:
                            if(estado != CORRENDO && estado != PULANDO && noChao) estado = ANDANDO;
                            break;
                    }
                }
            } else if (evt.type == SDL_KEYUP) {
                if(!jogador.hipnotizado && !jogador.enrolado && noChao) { 
                    if(evt.key.keysym.sym == SDLK_LSHIFT) {
                        if(teclas[SDL_SCANCODE_LEFT] || teclas[SDL_SCANCODE_RIGHT] || teclas[SDL_SCANCODE_UP] || teclas[SDL_SCANCODE_DOWN])
                            estado = ANDANDO;
                        else estado = PARADO;
                    }
                    if(evt.key.keysym.sym == SDLK_UP || evt.key.keysym.sym == SDLK_DOWN ||
                       evt.key.keysym.sym == SDLK_LEFT || evt.key.keysym.sym == SDLK_RIGHT) {
                        if(teclas[SDL_SCANCODE_LEFT] || teclas[SDL_SCANCODE_RIGHT] || teclas[SDL_SCANCODE_UP] || teclas[SDL_SCANCODE_DOWN]) {
                            if(teclas[SDL_SCANCODE_LSHIFT]) estado = CORRENDO;
                            else estado = ANDANDO;
                        } else estado = PARADO;
                    }
                }
            }
        }

        // 2. Lógica e Atualização de Entidades (UPDATE)
        
        // --- Lógica da Dançarina (NPC 1 - FSM) ---
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
            case MUMIA_DORMINDO:
                if(dist2 < mumia.alcanceVisao2) {
                    mumia.estado = MUMIA_PERSEGUINDO; 
                    printf("A múmia acordou!\n");
                }
                break;
                
            case MUMIA_PERSEGUINDO:
                if(dist2 < mumia.distanciaEnrolar2) {
                    mumia.estado = MUMIA_ENROLANDO; 
                    mumia.tempoEstado = SDL_GetTicks();
                    printf("Mumia está enrolando o jogador, jogador tomou 20 de dano!\n");
                } else if(dist2 > mumia.alcanceVisao2) {
                    mumia.estado = MUMIA_CONFUSA; 
                    mumia.tempoEstado = SDL_GetTicks();
                    printf("Múmia perdeu o jogador de vista e ficou confusa.\n");
                } else {
                    static int frame = 0;
                    frame++;
                    if(frame % 2 == 0) {
                        if(mx > 0) {
                            mumia.x++;
                            mumia.dirX = 1; // Múmia movendo-se para a DIREITA
                        }
                        else {
                            mumia.x--;
                            mumia.dirX = -1; // Múmia movendo-se para a ESQUERDA
                        }
                        if(my > 0) mumia.y++;
                        else mumia.y--;
                    }
                }
                break;

            case MUMIA_CONFUSA:
                if(SDL_GetTicks() - mumia.tempoEstado > 1000) {
                    mumia.estado = MUMIA_DORMINDO; 
                    printf("A múmia voltou a dormir.\n");
                } else {
                    mumia.x += mumia.dirX;
                    mumia.y += mumia.dirY;
                    if(rand() % 40 == 0) mumia.dirX = -mumia.dirX;
                    if(rand() % 40 == 0) mumia.dirY = -mumia.dirY;
                    if(dist2 < mumia.alcanceVisao2) mumia.estado = MUMIA_PERSEGUINDO;
                }
                break;

            case MUMIA_ENROLANDO:
                jogador.enrolado = true;
                jogador.vida -= mumia.danoAtaque;
                if (SDL_GetTicks() - mumia.tempoEstado > 3000) {
                    mumia.estado = MUMIA_ATORDOADA;
                    mumia.tempoEstado = SDL_GetTicks();
                    jogador.enrolado = false;
                    printf("Múmia soltou o jogador!\n");
                }
                break;

            case MUMIA_ATORDOADA:
                if(SDL_GetTicks() - mumia.tempoEstado > 2000) {
                    mumia.estado = MUMIA_DORMINDO;
                    printf("A múmia se recuperou.\n");
                }
                break;
        }

        // --- LÓGICA DE MOVIMENTO DO JOGADOR (CORRIGIDA) ---
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
                    // Logica de ataque (dano e transicao)
                    int mx_atk = jogador.x - mumia.x;
                    int my_atk = jogador.y - mumia.y;
                    int dist2_mumia = mx_atk*mx_atk + my_atk*my_atk;

                    if (dist2_mumia < jogador.alcanceAtaque) {
                        mumia.vida -= jogador.danoAtaque;  acertou = true;
                        if(mumia.estado != MUMIA_ATORDOADA) {
                            mumia.estado = MUMIA_ATORDOADA; mumia.tempoEstado = SDL_GetTicks();
                            if(jogador.enrolado) jogador.enrolado = false;
                        }
                        printf("Múmia atingida. Dano: %d. Vida restante (Múmia): %d\n", jogador.danoAtaque, mumia.vida);
                    } 
                    
                    int dx_atk = jogador.x - danca.x;
                    int dy_atk = jogador.y - danca.y;
                    int dist2_danca = dx_atk*dx_atk + dy_atk*dy_atk;

                    if (dist2_danca < jogador.alcanceAtaque) {
                        danca.vida -= jogador.danoAtaque; acertou = true;
                        if(danca.estadoAtual != ATORDOADA) {
                            danca.estadoAtual = ATORDOADA; danca.tempoEstado = SDL_GetTicks();
                            jogador.hipnotizado = false;
                        }
                        printf("Dançarina atingida. Dano: %d. Vida restante (Dançarina): %d\n", jogador.danoAtaque, danca.vida);
                    }

                    if (!acertou) { printf("Ataque do Jogador: Nenhum alvo no alcance.\n"); }
                    
                    estado=(teclas[SDL_SCANCODE_LEFT]|| teclas[SDL_SCANCODE_RIGHT] || teclas[SDL_SCANCODE_UP] || teclas[SDL_SCANCODE_DOWN] )? ANDANDO : PARADO;
                    break;
                
                default: break;
            }
        } else if (jogador.hipnotizado) {
            c = (SDL_Rect){680,0,100,80};
        } else if (jogador.enrolado) {
            c = (SDL_Rect){0,0,100,80};
        }

        // 3. Renderização (Desenho na Tela)
        int camera_offset_x = jogador.x - PLAYER_SCREEN_X;
        int camera_offset_y = jogador.y - PLAYER_SCREEN_Y;
        
        SDL_SetRenderDrawColor(ren, 255,255,255,255);
        SDL_RenderClear(ren);

        // Renderizar o Jogador
        SDL_RenderCopy(ren, img, &c, &r);

        // --- Dançarina (Retângulo Colorido) ---
        SDL_Rect rDanca = {
            danca.x - camera_offset_x, danca.y - camera_offset_y, danca.w, danca.h
        };
        SDL_Color corDanca;
        switch(danca.estadoAtual) {
            case DANCANDO: case HIPNOTIZANDO: corDanca=(SDL_Color){128,0,128,255}; break;
            case ATORDOADA: corDanca=(SDL_Color){128,128,128,255}; break;
            case PARADA: corDanca=(SDL_Color){0,255,0,255}; break;
        }
        SDL_SetRenderDrawColor(ren, corDanca.r, corDanca.g, corDanca.b, 255);
        SDL_RenderFillRect(ren, &rDanca);


        // --- Múmia (CORREÇÃO DO SPRITE DE ESQUERDA!) ---
        SDL_Rect rMumia = {
            mumia.x - camera_offset_x, mumia.y - camera_offset_y, mumia.w, mumia.h
        };

        SDL_Rect cMumia;
        cMumia.w = MUMIA_FRAME_W; // 48
        cMumia.h = MUMIA_FRAME_H; // 64
        cMumia.x = 0; // Posição X fixa no sprite sheet
        
        switch(mumia.estado) {
            case MUMIA_DORMINDO: 
                cMumia.y = 0 * MUMIA_FRAME_H; // LINHA 0
                break;
                
            case MUMIA_PERSEGUINDO: 
                if (mumia.dirX < 0) {
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

        // Renderizar a Textura da Múmia
        SDL_RenderCopy(ren, mumia_img, &cMumia, &rMumia);

        SDL_RenderPresent(ren);
        espera = 10;
    }

    // Limpeza de Memória
    SDL_DestroyTexture(img);
    SDL_DestroyTexture(mumia_img); 
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

//Direitos autorais de imagem:
//Múmia
//Created by Svetlana Kushnariova (Cabbit) & Jordan Irwin (AntumDeluge)
//Dançarina
//Svetlana Kushnariova

