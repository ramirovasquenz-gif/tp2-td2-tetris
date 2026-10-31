// Uso:  ./tetris           corre los tests y despues el juego
//       ./tetris --test    corre solo los tests (valgrind --leak-check=full ./tetris --test)

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// ========= CONSTANTES DEL JUEGO =========
#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 800
#define GRID_ROWS 20
#define GRID_COLS 10
#define CELL_SIZE 35
#define OFFSET_X 75
#define OFFSET_Y 50
#define TICK_RATE 30

const int TETROMINOS[7][4][4][4] = {
    { {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}}, {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}}, {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}}, {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}} },
    { {{2,0,0,0},{2,2,2,0},{0,0,0,0},{0,0,0,0}}, {{0,2,2,0},{0,2,0,0},{0,2,0,0},{0,0,0,0}}, {{0,0,0,0},{2,2,2,0},{0,0,2,0},{0,0,0,0}}, {{0,2,0,0},{0,2,0,0},{2,2,0,0},{0,0,0,0}} },
    { {{0,0,3,0},{3,3,3,0},{0,0,0,0},{0,0,0,0}}, {{0,3,0,0},{0,3,0,0},{0,3,3,0},{0,0,0,0}}, {{0,0,0,0},{3,3,3,0},{3,0,0,0},{0,0,0,0}}, {{3,3,0,0},{0,3,0,0},{0,3,0,0},{0,0,0,0}} },
    { {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}} },
    { {{0,5,5,0},{5,5,0,0},{0,0,0,0},{0,0,0,0}}, {{0,5,0,0},{0,5,5,0},{0,0,5,0},{0,0,0,0}}, {{0,0,0,0},{0,5,5,0},{5,5,0,0},{0,0,0,0}}, {{5,0,0,0},{5,5,0,0},{0,5,0,0},{0,0,0,0}} },
    { {{0,6,0,0},{6,6,6,0},{0,0,0,0},{0,0,0,0}}, {{0,6,0,0},{0,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,0,0,0},{6,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,6,0,0},{6,6,0,0},{0,6,0,0},{0,0,0,0}} },
    { {{7,7,0,0},{0,7,7,0},{0,0,0,0},{0,0,0,0}}, {{0,0,7,0},{0,7,7,0},{0,7,0,0},{0,0,0,0}}, {{0,0,0,0},{7,7,0,0},{0,7,7,0},{0,0,0,0}}, {{0,7,0,0},{7,7,0,0},{7,0,0,0},{0,0,0,0}} }
};

SDL_Color colors[8] = {
    {30, 30, 30, 255}, {0, 255, 255, 255}, {0, 0, 255, 255}, {255, 165, 0, 255},
    {255, 255, 0, 255}, {0, 255, 0, 255}, {128, 0, 128, 255}, {255, 0, 0, 255}
};

// ========= ESTRUCTURAS DE DATOS =========

// 1. Nodo para la fila del tablero (Lista Enlazada)
typedef struct RowNode {
    int cells[GRID_COLS]; 
    struct RowNode* next; 
} RowNode;

// 2. Nodo para la cola de próximas piezas (Queue)
typedef struct PieceQueueNode {
    int type; 
    struct PieceQueueNode* next;
} PieceQueueNode;

// 3. Nodo para el sistema de partículas 
typedef struct ParticleNode {
    float x, y;
    float vel_x, vel_y;
    int color;
    int life_timer;
    struct ParticleNode* next;
} ParticleNode;

typedef struct ActivePiece {
    int col, row;
    int type;      
    int rotation;  
    int active;
} ActivePiece;

typedef struct GameBoard {
    RowNode* first_row;             
    PieceQueueNode* queue_head;     
    PieceQueueNode* queue_tail;     
    ParticleNode* first_particle;   
    ActivePiece current_piece;
    int score;
    int tick_timer;
} GameBoard;

// ========= VARIABLES GLOBALES =========
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
GameBoard* game_board = NULL;

// ========= EJERCICIO 1: STRINGS (NO USAR string.h) =========

char* strDuplicate(char* src) {
    // Inicio una variable que va a valre el largo de src, para luego poder pedir memoria con malloc. 
    int len = 0;
    while (src[len] != '\0') {
        len++;
    }
    char* copiaNueva = (char*)malloc((len+1) * sizeof(char));
    if (copiaNueva == NULL) {
        return NULL;
        // Pido memoria del largo de src +1 por el caracter nulo y si no hay memoria devuelve NULL.
    } 
    for(int i=0; i<=len; i++) {
        copiaNueva[i] = src[i];
        // Copio caracter a caracter en la copia nueva y luego la devuelvo.
    }
    return copiaNueva;
}

int strCompare(char* s1, char* s2) {
    int i =0;
    // Va avanzando mientras sean iguales y no se llegue al final.
    while(s1[i]!= '\0' && s1[i] == s2[i]) {
        i++;
    }
    if(s1[i] == s2[i])
        return 0;    // Significa que recorrio ambos, y el caracter final es el mismo para ambos.
    if(s1[i] < s2[i])
        return 1;   // Recorrio ambos, pero como s1 es mas corto, su ultimo caracter, '\0' es menor que cualquier otro y lo hace menor.
    return -1;        // Recorrio ambos, pero s2 es mas corto.
}

char* strConcatenate(char* src1, char* src2) {
    int len1 = 0;
    int len2 = 0; // Creo 2 variables que van a guardar las longitudes de los strings.
    while (src1[len1]!= '\0') {
        len1++;
    }
    while (src2[len2]!= '\0') {
        len2++; 
    }
    char* copiaConcatenada = (char*)malloc((len1 + len2 + 1) * sizeof(char));
    if (copiaConcatenada == NULL) {
        return NULL;
     } // Pido la memoria del tamaño de ambos strings y considerop el '\0'. Devuelvo NULL si no hay memoria.
        for (int i=0; i<len1; i++) {
            copiaConcatenada[i] = src1[i];
        }
        for (int j=0; j<=len2; j++) { // j<=len2 para que considere el '\0' para el nuevo stringg.
            copiaConcatenada[len1 + j] = src2[j];
            // Se copia src1 y src2 inmediatamente despues de src1.
        }
        free(src1);
        free(src2);
        return copiaConcatenada; // Se liberan ambas memorias de los strings y se devuelve la copia concatenada.
}

// ========= EJERCICIO 2: FUNCIONES A IMPLEMENTAR (EL TP) =========

GameBoard* gameBoardNew() {
    GameBoard* board = (GameBoard*)malloc(sizeof(GameBoard)); //Reserva la estructura principal.
    if (board == NULL) {
        return NULL;
    }
    board->first_row = NULL;
    board->queue_head = NULL;
    board->queue_tail = NULL;
    board->first_particle = NULL;
    board->current_piece.active = 0;
    board->current_piece.col = 0;
    board->current_piece.row = 0;
    board->current_piece.type = 0;
    board->current_piece.rotation = 0;
    board->score = 0;
    board->tick_timer = 0; //Inicio los campos, con punteros y null y contadores en 0.
//voy a crear las filas del tablero, estan enlazadas, cada nueva fila va al principio de la lista.
    for(int i =0; i<GRID_ROWS; i++) {
        RowNode* nuevaFila = (RowNode*)malloc(sizeof(RowNode)); //creo la nueva fila.
        if (nuevaFila ==NULL) {
            RowNode* curr = board->first_row;
            while (curr != NULL) {
                RowNode* sig = curr->next;
                free(curr);
                curr = sig;
            }
            free(board);
            return NULL; //Si algun malloc falla, se libera lo reservado y devuelve null.
        }
        for(int j=0; j < GRID_COLS; j++) {
            nuevaFila->cells[j] = 0; 
        }
        nuevaFila->next = board->first_row;
            board->first_row = nuevaFila;
    }
    return board; 
   
}

void gameBoardDelete(GameBoard* board) {
    if(board ==NULL){
        return;
    }
    RowNode* nuevaFila = board->first_row;
    while(nuevaFila != NULL) {
        RowNode* sig = nuevaFila->next;
        free(nuevaFila);
        nuevaFila = sig;
    }
    PieceQueueNode* piezasDeCola = board->queue_head;
    while(piezasDeCola != NULL) {
        PieceQueueNode* sig = piezasDeCola->next;
        free(piezasDeCola);
        piezasDeCola = sig;
    }
    ParticleNode* particulas = board->first_particle;
    while(particulas != NULL){
        ParticleNode* sig = particulas->next;
        free(particulas);
        particulas = sig;
    }
    free(board);//Libero memoria de todas las estructuras y del tablero.
}

void gameBoardAddParticle(GameBoard* board, float x, float y, int color) {
    ParticleNode* nuevaParticula = (ParticleNode*)malloc(sizeof(ParticleNode));
    if (nuevaParticula == NULL) {
        return; //si no hay memoria, no se hace la particula/
    }
    nuevaParticula->x = x;
    nuevaParticula->y = y;
    nuevaParticula->vel_x = (rand() % 100) / 100.0f * 6.0f - 3.0f; //Velocidades aleatorias entre -3 y 3
    nuevaParticula->vel_y = -(rand() % 100) / 100.0f*6.0f - 1.0f;
    nuevaParticula->color = color;
    nuevaParticula->life_timer = 30;
    nuevaParticula->next = board->first_particle;
    board->first_particle = nuevaParticula;
}

int gameBoardCheckAndClearLines(GameBoard* board) {
    int lineasEliminadas =0;
    int indice = 0;
    RowNode* prev =NULL;
    RowNode* curr = board->first_row;

    while(curr!= NULL) {
        int filaLlena = 1;
        for(int i=0; i<GRID_COLS; i++) {
            if(curr->cells[i] == 0) {
                filaLlena = 0;
                break;
            } //reviso si la fila esta llena (ninguna en 0).
        }
    

    RowNode* sig = curr->next; //guardo el siguiente nodo antes de eliminar curr.

    if(filaLlena) {
        for (int j=0; j<GRID_COLS; j++) {
            gameBoardAddParticle(board, OFFSET_X + j * CELL_SIZE, OFFSET_Y + indice * CELL_SIZE, curr->cells[j]);
            //particula por celda.
        }

        if (prev == NULL) {
            board->first_row = sig; //si es la primera fila, cambio a first_row .
        }

        else{
            prev->next = sig; // si no es la primera, pasa al siguiente nodo.
        }
        free(curr);
        lineasEliminadas++;
    }
        else{
            prev = curr;
        }
        curr = sig;
        indice++;
    }
    
    for (int k = 0; k < lineasEliminadas; k++) { //se van a crear nuevas filas vacias por cada una eliminada.
        RowNode* nuevaFila = (RowNode*)malloc(sizeof(RowNode));
        if (nuevaFila ==NULL){
            break; //no hay memoria --> no se crean filas.
        }
        for(int l=0; l<GRID_COLS; l++) {
            nuevaFila->cells[l] =0;
        }
        nuevaFila-> next = board->first_row;
        board-> first_row = nuevaFila; //se agrega una nueva fila al principio de la lista.
    }
    return lineasEliminadas;
}

void gameBoardEnqueuePiece(GameBoard* board, int type) {
    PieceQueueNode* nuevoNodo = (PieceQueueNode*)malloc(sizeof(PieceQueueNode)); //Nuevo nodo para la cola de piezas.
    if (nuevoNodo == NULL) { 
        return; //si malloc falla devuelve null.
    }
    nuevoNodo -> type = type;
    nuevoNodo -> next = NULL; //Apunta a NULL porque es el ultimo de la cola.

    if (board->queue_tail == NULL) {
        board->queue_head = nuevoNodo;
        board->queue_tail = nuevoNodo; //Si la cola es nula, el nodo es al mismo tiempo el principio y el final.
    }
    else{
        board->queue_tail->next = nuevoNodo;
        board->queue_tail = nuevoNodo; //La cola no es nula y se agrega el nuevo nodo al final.
    }
}

int gameBoardDequeuePiece(GameBoard* board, int* out_type) {
    if (board->queue_head == NULL) {
        return 0; // La cola esta vacia, devuelve 0 sin modificar out_type.
    }

    PieceQueueNode* nodoEliminado = board->queue_head; 
    *out_type = nodoEliminado->type; 
    board->queue_head = nodoEliminado->next; //se guarda la cabeza de la cola en nodoEliminado con el tipo de pieeza y se actualiza la cabeza de la cola al siguiente nodo. 

    if (board->queue_head == NULL) {
        board->queue_tail = NULL; //si despues de eliminar el nodo la lista queda vacia, la cola debe ser nula tambien.
    }
    free(nodoEliminado);
    return 1; // se devuelve 1 cuando se elimina el nodo correctamente. Daba 0 si era vacia.

}

// ========= LOGICA DE DIBUJADO Y COLISIONES (NO MODIFICAR) =========

RowNode* getRowAt(GameBoard* board, int index) {
    RowNode* curr = board->first_row;
    for(int i = 0; i < index && curr != NULL; i++) curr = curr->next;
    return curr;
}

int canMove(GameBoard* board, int target_col, int target_row, int target_rot) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (TETROMINOS[board->current_piece.type][target_rot][r][c] != 0) {
                int board_col = target_col + c;
                int board_row = target_row + r;
                if (board_col < 0 || board_col >= GRID_COLS || board_row >= GRID_ROWS) return 0;
                if (board_row >= 0) {
                    RowNode* row_node = getRowAt(board, board_row);
                    if (row_node && row_node->cells[board_col] != 0) return 0;
                }
            }
        }
    }
    return 1;
}

void lockPiece(GameBoard* board) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int val = TETROMINOS[board->current_piece.type][board->current_piece.rotation][r][c];
            if (val != 0) {
                RowNode* row_node = getRowAt(board, board->current_piece.row + r);
                if (row_node) row_node->cells[board->current_piece.col + c] = val;
            }
        }
    }
    board->current_piece.active = 0;
    board->score += gameBoardCheckAndClearLines(board) * 100;
}

// Devuelve 0 si la pieza nueva no entra (GAME OVER), 1 en otro caso.
int spawnNextPiece(GameBoard* board) {
    int type;
    if (gameBoardDequeuePiece(board, &type)) {
        board->current_piece.type = type;
        board->current_piece.rotation = 0;
        board->current_piece.col = GRID_COLS / 2 - 2;
        board->current_piece.row = 0;
        board->current_piece.active = 1;
        gameBoardEnqueuePiece(board, rand() % 7);
        if (!canMove(board, board->current_piece.col, board->current_piece.row, board->current_piece.rotation)) {
            printf("GAME OVER. Puntaje final: %d\n", board->score);
            board->current_piece.active = 0;
            return 0;   // main sale del loop y libera tablero y SDL
        }
    }
    return 1;
}

// Devuelve 0 cuando termina la partida.
int gameBoardUpdate(GameBoard* board) {
    if (!board) return 0;
    if (!board->current_piece.active && !spawnNextPiece(board)) return 0;

    board->tick_timer++;
    if (board->tick_timer >= TICK_RATE) {
        board->tick_timer = 0;
        if (canMove(board, board->current_piece.col, board->current_piece.row + 1, board->current_piece.rotation)) {
            board->current_piece.row++;
        } else {
            lockPiece(board);
        }
    }

    ParticleNode* curr_part = board->first_particle;
    ParticleNode* prev_part = NULL;
    while (curr_part) {
        curr_part->x += curr_part->vel_x;
        curr_part->y += curr_part->vel_y;
        curr_part->vel_y += 0.5f;
        curr_part->life_timer--;

        if (curr_part->life_timer <= 0) {
            ParticleNode* to_delete = curr_part;
            if (prev_part == NULL) {
                board->first_particle = curr_part->next;
                curr_part = board->first_particle;
            } else {
                prev_part->next = curr_part->next;
                curr_part = prev_part->next;
            }
            free(to_delete);
        } else {
            prev_part = curr_part;
            curr_part = curr_part->next;
        }
    }
    return 1;
}

void gameBoardDraw(GameBoard* board) {
    if (!board) return;
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect board_rect = {OFFSET_X - 2, OFFSET_Y - 2, GRID_COLS * CELL_SIZE + 4, GRID_ROWS * CELL_SIZE + 4};
    SDL_RenderDrawRect(renderer, &board_rect);

    RowNode* current_row = board->first_row;
    int r = 0;
    while (current_row) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (current_row->cells[c] > 0) {
                SDL_Color col = colors[current_row->cells[c]];
                SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                SDL_Rect cell_rect = {OFFSET_X + c * CELL_SIZE, OFFSET_Y + r * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1};
                SDL_RenderFillRect(renderer, &cell_rect);
            }
        }
        current_row = current_row->next;
        r++;
    }

    if (board->current_piece.active) {
        for(int pr = 0; pr < 4; pr++) {
            for(int pc = 0; pc < 4; pc++) {
                int cell_val = TETROMINOS[board->current_piece.type][board->current_piece.rotation][pr][pc];
                if (cell_val != 0) {
                    SDL_Color col = colors[cell_val];
                    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                    SDL_Rect p_rect = {OFFSET_X + (board->current_piece.col + pc) * CELL_SIZE, 
                                       OFFSET_Y + (board->current_piece.row + pr) * CELL_SIZE, 
                                       CELL_SIZE - 1, CELL_SIZE - 1};
                    SDL_RenderFillRect(renderer, &p_rect);
                }
            }
        }
    }

    ParticleNode* curr_part = board->first_particle;
    while (curr_part) {
        SDL_Color col = colors[curr_part->color];
        int alpha = curr_part->life_timer * 8;   // se satura: SDL recibe un Uint8
        if (alpha > 255) alpha = 255;
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, alpha);
        SDL_Rect part_rect = {(int)curr_part->x, (int)curr_part->y, 6, 6};
        SDL_RenderFillRect(renderer, &part_rect);
        curr_part = curr_part->next;
    }
    SDL_RenderPresent(renderer);
}

int inicializar() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 0;
    window = SDL_CreateWindow("Tetris Base - TP", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) return 0;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) return 0;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    return 1;
}

//TESTS 
void correrTests() {
    // strDuplicate 1. String vacio, 2. String de 1 caracter, 3. String con distintos caracteres
    char* dup1 = strDuplicate("");
    if (dup1 != NULL && dup1[0] == '\0') {
        printf("[OK] strDuplicate: string vacio\n");
    }
    else {
        printf("[FALLO] strDuplicate: string vacio\n");
    }
    free(dup1);

    char* dup2 = strDuplicate("a");
    if (dup2 != NULL && dup2[0] == 'a' && dup2[1] == '\0') {
        printf("[OK] strDuplicate: un caracter\n");
    }
    else {
        printf("[FALLO] strDuplicate: un caracter\n");
    }
    free(dup2);

    char* dup3 = strDuplicate("Hola 123!");
    if (dup3 != NULL && strCompare(dup3, "Hola 123!") == 0) {
        printf("[OK] strDuplicate: distintos caracteres\n");
    }
    else {
        printf("[FALLO] strDuplicate: distintos caracteres\n");
    }
    free(dup3);

    //strCompare 1. 2 strings vacios o nulos, 2. 2 strings de un solo caracter, 3. 2 strings diferentes, en ambos ordenes
    if (strCompare("", "") == 0) {
        printf("[OK] strCompare: dos vacios\n");
    }
    else {
        printf("[FALLO] strCompare: dos vacios\n");
    }

    if (strCompare("x", "x") == 0) {
        printf("[OK] strCompare: un caracter igual\n");
    }
    else {
        printf("[FALLO] strCompare: un caracter igual\n");
    }

    if (strCompare("a", "b") == 1) {
        printf("[OK] strCompare: a vs b\n");
    }
    else {
        printf("[FALLO] strCompare: a vs b\n");
    }

    if (strCompare("b", "a") == -1) {
        printf("[OK] strCompare: b vs a\n");
    }
    else {
        printf("[FALLO] strCompare: b vs a\n");
    }

    //strConcatenate 1. String vacio y string de 3 caracteres, 2. Dos strings de 5 caracteres
    char* conc1a = strDuplicate("");
    char* conc1b = strDuplicate("abc");
    char* res1 = strConcatenate(conc1a, conc1b);
    if (res1 != NULL && strCompare(res1, "abc") == 0) {
        printf("[OK] strConcatenate: vacio + abc\n");
    }
    else {
        printf("[FALLO] strConcatenate: vacio + abc\n");
    }
    free(res1);

    char* conc2a = strDuplicate("hola!");
    char* conc2b = strDuplicate("mundo");
    char* res2 = strConcatenate(conc2a, conc2b);
    if (res2 != NULL && strCompare(res2, "hola!mundo") == 0) {
        printf("[OK] strConcatenate: hola! + mundo\n");
    }
    else {
        printf("[FALLO] strConcatenate: hola! + mundo\n");
    }
    free(res2);

//Tablero: colas (FIFO)
    GameBoard* tablero = gameBoardNew();
    int tipo[5] = {3, 1, 2, 1, 5};
    int salida = -1;

    //5 push.
    for(int i=0; i < 5; i++) {
        gameBoardEnqueuePiece(tablero, tipo[i]);
    }

    //6 pop.
    int bienOrdenado =1;
    for(int i =0; i<5; i++) {
        int res = gameBoardDequeuePiece(tablero, &salida);
        if (res != 1 || salida != tipo[i]) {
            bienOrdenado = 0;
        }
    }
        if (bienOrdenado) {
            printf("[OK] cola: Orden FIFO de 5 piezas\n");
        }
        else {
            printf("[FALLO] cola: Orden FIFO de 5 piezas\n");
        }
    
        salida = 99;
        int sextoPop = gameBoardDequeuePiece(tablero, &salida);
        if (sextoPop ==0 && salida == 99 && tablero->queue_head ==NULL && tablero->queue_tail ==NULL) {
            printf("[OK] cola: sexto pop da 0 y cabeza/cola son nulas\n");
        }
        else {
            printf("[FALLO] cola: el sexto pop no da 0 o bien la cabeza o la cola no son nulas\n");
        }
        //un push mas para ver si se puede volver a extraer.
        gameBoardEnqueuePiece(tablero, 6);
        int otroPop =gameBoardDequeuePiece(tablero, &salida);
        if (otroPop ==1 && salida ==6) {
            printf("[OK] cola: se puede volver a extraer despues de hacer pop\n");
        }
        else {
            printf("[FALLO] cola: no se puede extraer de vuelta luego de hacer pop\n");
        }
        gameBoardDelete(tablero);
    

    //test de fila 19.
    GameBoard* tablero2 = gameBoardNew();
    RowNode* fila0Primera = tablero2->first_row;
    RowNode* filaUltima = getRowAt(tablero2, 19);
    for (int i=0; i<GRID_COLS; i++) {
        filaUltima->cells[i]=1; 
    }

    int lineasEliminadas = gameBoardCheckAndClearLines(tablero2);

    //ahora la fila0 nueva tiene que ser vacia y diferente a fila0Primera
    int fila0Nueva = 1;
    for (int j =0; j<GRID_COLS; j++) {
        if (tablero2->first_row->cells[j] !=0) {
            fila0Nueva = 0;
        }
    }

    //ahora cuento cuantas filas hay y si la fila0 nueva es diferente a la primera.
    int filasTotales = 0;
    RowNode* curr = tablero2->first_row;
    while (curr != NULL) {
        filasTotales++;
        curr = curr->next;
    }
    if(lineasEliminadas == 1 && tablero2->first_row != fila0Primera && fila0Nueva && filasTotales ==20) {
        printf("[OK] tablero: se pudo eliminar la ultima fila. Se agrego una nueva al principio, y el total de filas es de 20\n");
    }
    else {
        printf("[FALLO] tablero: no se pudo eliminar la ultima fila correctamente\n");
    }
    gameBoardDelete(tablero2);

    //test llenar filas 0 y 1.

    GameBoard* tablero3 = gameBoardNew();
    RowNode* fila0 = getRowAt(tablero3, 0);
    RowNode* fila1 = getRowAt (tablero3, 1);
    for (int i=0; i<GRID_COLS; i++) {
        fila0->cells[i] = 1;
        fila1->cells[i] = 1;
    }

    int lineasEliminadas2 = gameBoardCheckAndClearLines(tablero3);

    //nuevamente, contar filas.
    int filasTotales2 = 0;
    RowNode* curr2 = tablero3->first_row;
    while (curr2 != NULL) {
        filasTotales2++;
        curr2 = curr2->next;
    }
    if(lineasEliminadas2 ==2 && filasTotales2 ==20) {
        printf("[OK] tablero: se eliminaron 2 filas y el total es de 20\n");
    }
    else {
        printf("[FALLO] tablero: no se eliminaron correctamente 2 filas\n");
    }
    gameBoardDelete(tablero3);
}

// Compara contra "--test" sin depender de string.h ni del Ejercicio 1.
int esFlagTest(char* arg) {
    char* flag = "--test";
    int i = 0;
    while (arg[i] != '\0' && arg[i] == flag[i]) i++;
    return arg[i] == '\0' && flag[i] == '\0';
}

int main(int argc, char* args[]) {
    srand(time(NULL));

    // Los tests corren antes de SDL. Con --test termina aca, asi valgrind solo
    // reporta la memoria del TP (SDL tiene bloques propios que no se liberan).
    correrTests();
    if (argc > 1 && esFlagTest(args[1])) return 0;

    if (!inicializar()) return 1;

    game_board = gameBoardNew();
    if (game_board == NULL) {
        printf("ADVERTENCIA: Implementa gameBoardNew() para iniciar el juego.\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    for(int i=0; i<3; i++) gameBoardEnqueuePiece(game_board, rand() % 7);

    SDL_Event e;
    int game_over = 0;

    while (!game_over) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) game_over = 1;
            if (e.type == SDL_KEYDOWN && game_board->current_piece.active) {
                if (e.key.keysym.sym == SDLK_LEFT && canMove(game_board, game_board->current_piece.col - 1, game_board->current_piece.row, game_board->current_piece.rotation)) game_board->current_piece.col--;
                if (e.key.keysym.sym == SDLK_RIGHT && canMove(game_board, game_board->current_piece.col + 1, game_board->current_piece.row, game_board->current_piece.rotation)) game_board->current_piece.col++;
                if (e.key.keysym.sym == SDLK_DOWN && canMove(game_board, game_board->current_piece.col, game_board->current_piece.row + 1, game_board->current_piece.rotation)) game_board->current_piece.row++;
                if (e.key.keysym.sym == SDLK_UP) {
                    int next_rot = (game_board->current_piece.rotation + 1) % 4;
                    if (canMove(game_board, game_board->current_piece.col, game_board->current_piece.row, next_rot)) game_board->current_piece.rotation = next_rot;
                }
                if (e.key.keysym.sym == SDLK_SPACE) {
                     while(canMove(game_board, game_board->current_piece.col, game_board->current_piece.row + 1, game_board->current_piece.rotation)) game_board->current_piece.row++;
                     lockPiece(game_board);
                }
            }
        }
        if (!gameBoardUpdate(game_board)) game_over = 1;
        gameBoardDraw(game_board);
        SDL_Delay(16);
    }
    gameBoardDelete(game_board);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
