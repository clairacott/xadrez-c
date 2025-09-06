// xadrez.c — Xadrez simples de terminal (sem xeque/roque/en passant).
// Brancas: letras maiúsculas | Pretas: letras minúsculas
// Movimentos válidos, captura, passo-duplo inicial de peões, promoção automática para Dama.

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#define BOARD_SIZE 8

static void init_board(char b[BOARD_SIZE][BOARD_SIZE]) {
    const char* back = "RNBQKBNR"; // Torre, Cavalo, Bispo, Dama, Rei...
    for (int r = 0; r < BOARD_SIZE; ++r)
        for (int c = 0; c < BOARD_SIZE; ++c)
            b[r][c] = '.';

    // Pretas no topo (minúsculas)
    for (int c = 0; c < BOARD_SIZE; ++c) b[0][c] = tolower(back[c]);
    for (int c = 0; c < BOARD_SIZE; ++c) b[1][c] = 'p';
    // Brancas na base (maiúsculas)
    for (int c = 0; c < BOARD_SIZE; ++c) b[6][c] = 'P';
    for (int c = 0; c < BOARD_SIZE; ++c) b[7][c] = back[c];
}

static void print_board(const char b[BOARD_SIZE][BOARD_SIZE]) {
    printf("\n    a   b   c   d   e   f   g   h\n");
    printf("  +---+---+---+---+---+---+---+---+\n");
    for (int r = 0; r < BOARD_SIZE; ++r) {
        printf("%d |", 8 - r);
        for (int c = 0; c < BOARD_SIZE; ++c) {
            printf(" %c |", b[r][c]);
        }
        printf(" %d\n", 8 - r);
        printf("  +---+---+---+---+---+---+---+---+\n");
    }
    printf("    a   b   c   d   e   f   g   h\n\n");
}

static bool inside(int r, int c) {
    return r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE;
}

static bool is_white(char p) { return p != '.' && isupper((unsigned char)p); }
static bool is_black(char p) { return p != '.' && islower((unsigned char)p); }

static bool path_clear(const char b[BOARD_SIZE][BOARD_SIZE], int r1, int c1, int r2, int c2) {
    int dr = (r2 > r1) ? 1 : (r2 < r1) ? -1 : 0;
    int dc = (c2 > c1) ? 1 : (c2 < c1) ? -1 : 0;
    int r = r1 + dr, c = c1 + dc;
    while (r != r2 || c != c2) {
        if (b[r][c] != '.') return false;
        r += dr; c += dc;
    }
    return true;
}

static bool rook_like_ok(const char b[BOARD_SIZE][BOARD_SIZE], int r1, int c1, int r2, int c2) {
    if (r1 != r2 && c1 != c2) return false;
    return path_clear(b, r1, c1, r2, c2);
}

static bool bishop_like_ok(const char b[BOARD_SIZE][BOARD_SIZE], int r1, int c1, int r2, int c2) {
    if (abs(r2 - r1) != abs(c2 - c1)) return false;
    return path_clear(b, r1, c1, r2, c2);
}

static bool knight_ok(int r1, int c1, int r2, int c2) {
    int dr = abs(r2 - r1), dc = abs(c2 - c1);
    return (dr == 2 && dc == 1) || (dr == 1 && dc == 2);
}

static bool king_ok(int r1, int c1, int r2, int c2) {
    int dr = abs(r2 - r1), dc = abs(c2 - c1);
    return dr <= 1 && dc <= 1 && !(dr == 0 && dc == 0);
}

static bool pawn_ok(const char b[BOARD_SIZE][BOARD_SIZE], int r1, int c1, int r2, int c2, bool white_turn) {
    int dir = white_turn ? -1 : 1; // brancas sobem (linha diminui), pretas descem (linha aumenta)
    int start_row = white_turn ? 6 : 1;
    char target = b[r2][c2];

    // movimento reto
    if (c1 == c2) {
        if (r2 == r1 + dir && target == '.') return true;
        if (r1 == start_row && r2 == r1 + 2*dir && target == '.' && b[r1 + dir][c1] == '.')
            return true;
        return false;
    }
    // captura diagonal
    if (abs(c2 - c1) == 1 && r2 == r1 + dir) {
        if (target != '.' && ((white_turn && is_black(target)) || (!white_turn && is_white(target))))
            return true;
    }
    return false;
}

static bool can_move_piece(const char b[BOARD_SIZE][BOARD_SIZE], int r1, int c1, int r2, int c2, bool white_turn) {
    if (!inside(r1,c1) || !inside(r2,c2)) return false;
    char p = b[r1][c1];
    if (p == '.') return false;

    // peça correta do turno?
    if (white_turn && !is_white(p)) return false;
    if (!white_turn && !is_black(p)) return false;

    // destino não pode ter peça da mesma cor
    char dest = b[r2][c2];
    if (dest != '.') {
        if (is_white(p) && is_white(dest)) return false;
        if (is_black(p) && is_black(dest)) return false;
    }

    char up = (char)toupper((unsigned char)p);
    switch (up) {
        case 'P': return pawn_ok(b, r1, c1, r2, c2, white_turn);
        case 'N': return knight_ok(r1, c1, r2, c2);
        case 'B': return bishop_like_ok(b, r1, c1, r2, c2);
        case 'R': return rook_like_ok(b, r1, c1, r2, c2);
        case 'Q': return bishop_like_ok(b, r1, c1, r2, c2) || rook_like_ok(b, r1, c1, r2, c2);
        case 'K': return king_ok(r1, c1, r2, c2);
        default: return false;
    }
}

static void do_move(char b[BOARD_SIZE][BOARD_SIZE], int r1, int c1, int r2, int c2) {
    b[r2][c2] = b[r1][c1];
    b[r1][c1] = '.';
}

static void promote_if_needed(char b[BOARD_SIZE][BOARD_SIZE]) {
    // promoção simples: peão vira Dama automaticamente
    for (int c = 0; c < BOARD_SIZE; ++c) {
        if (b[0][c] == 'P') b[0][c] = 'Q';
        if (b[7][c] == 'p') b[7][c] = 'q';
    }
}

static bool parse_square(const char* s, int* r, int* c) {
    // formatos aceitos: "e2" (coluna a-h, linha 1-8)
    if (!s || strlen(s) < 2) return false;
    char file = tolower((unsigned char)s[0]);
    char rank = s[1];
    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') return false;
    *c = file - 'a';
    // linha 8 é r=0; linha 1 é r=7
    *r = 8 - (rank - '0');
    return true;
}

static bool parse_move(const char* line, int* r1, int* c1, int* r2, int* c2) {
    // aceita "e2 e4" ou "e2e4"
    char a[3]={0}, b[3]={0};
    int i = 0, j = 0;
    // copiar primeiros dois chars não espaço
    while (line[i] && isspace((unsigned char)line[i])) i++;
    if (!line[i] || !line[i+1]) return false;
    a[0] = line[i]; a[1] = line[i+1];
    i += 2;
    while (line[i] && isspace((unsigned char)line[i])) i++;
    if (!line[i]) {
        // pode estar colado, então usar próximos dois já lidos? Não, precisa mais.
        if (!line[i] && strlen(line) >= 4) {
            // fallback: varrer por 4 alfanuméricos
            int count = 0;
            char sq[5] = {0};
            for (int k = 0; line[k]; ++k) if (!isspace((unsigned char)line[k])) sq[count++] = line[k];
            if (count == 4) {
                a[0]=sq[0]; a[1]=sq[1];
                b[0]=sq[2]; b[1]=sq[3];
                return parse_square(a,r1,c1) && parse_square(b,r2,c2);
            }
            return false;
        }
    }
    if (!line[i] || !line[i+1]) return false;
    b[0] = line[i]; b[1] = line[i+1];
    return parse_square(a,r1,c1) && parse_square(b,r2,c2);
}

int main(void) {
    char board[BOARD_SIZE][BOARD_SIZE];
    init_board(board);
    bool white_turn = true;
    char line[128];

    printf("Xadrez simples (terminal)\n");
    printf("- Digite movimentos como: e2 e4   ou   e2e4\n");
    printf("- 'q' ou 'quit' para sair.\n");
    print_board(board);

    while (1) {
        printf("%s > ", white_turn ? "Brancas" : "Pretas");
        if (!fgets(line, sizeof(line), stdin)) break;

        // sair
        if (strncmp(line, "q", 1) == 0 || strncmp(line, "Q", 1) == 0) {
            printf("Encerrando...\n");
            break;
        }
        int r1,c1,r2,c2;
        if (!parse_move(line, &r1,&c1,&r2,&c2)) {
            printf("Entrada inválida. Ex: e2 e4\n");
            continue;
        }
        if (!inside(r1,c1) || !inside(r2,c2)) {
            printf("Casa fora do tabuleiro.\n");
            continue;
        }
        if (!can_move_piece(board, r1,c1, r2,c2, white_turn)) {
            printf("Movimento ilegal para essa peça/turno.\n");
            continue;
        }
        // Executa
        do_move(board, r1,c1, r2,c2);
        promote_if_needed(board);
        print_board(board);
        white_turn = !white_turn;
    }
    return 0;
}