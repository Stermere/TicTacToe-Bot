#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

// define some constants
#define DEFAULT_BOARD_SIZE 5
#define DEFAULT_WIN_CONDITION 4
#define MAX_DEPTH 20

// struct's that defines the transposition table
struct TranspositionTableEntry {
    long long int hash;
    short depth;
    short eval;
};

struct TranspositionTable {
    struct TranspositionTableEntry *entries;
    int size;
    int used;
};

// a struct to keep track of the tree search
struct SearchInfo {
    long long int hash;
    long long int *hashingNumbers;
    struct TranspositionTable *transpositionTable;
    long long int centerMask;
    int nodes;
    short turn;
    short depth;
    short maxDepth;
    short winCondition;
    short boardSize;
    short totalSquares;
    short eval;
};

// define functions
void playGame(short, short);
void printBoard(long long int, long long int, short);
short getMove(long long int, long long int, struct SearchInfo*);
void makeMove(long long int*, long long int*, short, struct SearchInfo*);
void unmakeMove(long long int*, long long int*, short, struct SearchInfo*);
short checkWin(long long int*, long long int*, struct SearchInfo*, short);
short searchBoard(long long int*, long long int*, struct SearchInfo*);
short evalBoard(long long int*, long long int*, struct SearchInfo*, short, short, short);
long long int buildCenterMask(short);
void freeBoardData(struct SearchInfo*);


int main(int argc, char **argv) {
    //read in args
    short size = DEFAULT_BOARD_SIZE;
    short winCondition = DEFAULT_WIN_CONDITION;
    if (argc != 3) {
        printf("Usage: <size> <winCondition>\n");
        printf("Defaulting to size %d win condition %d\n", DEFAULT_BOARD_SIZE, DEFAULT_WIN_CONDITION);
    } else {
        size = (short) atoi(argv[1]);
        winCondition = (short) atoi(argv[2]);
    }

    // verify the args
    if (size > 8 || size < 3) {
        perror("Size must be between 3 and 8\n");
        return -1;
    }
    if (winCondition > size || winCondition < 3) {
        perror("Win condition must be between 3 and size\n");
        return -1;
    }

    // seed the random number generator
    srand(time(NULL));

    // play the game
    playGame(size, winCondition);
}

// get a entry from the table if it exists
struct TranspositionTableEntry* getEntry(struct TranspositionTable *table, long long int hash) {
    // compute the index
    int index = abs(hash % table->size);

    // check if the entry exists
    if (table->entries[index].hash == hash) {
        return &table->entries[index];
    }

    return NULL;
}

// store a new entry in the table
void storeEntry(struct TranspositionTable *table, long long int hash, struct SearchInfo *boardData, short eval) {
    // compute the index
    int index = abs(hash % table->size);

    // check if the entry exists
    if (table->entries[index].hash == hash && table->entries[index].depth >= boardData->depth) {
        return;
    }

    // store the entry
    table->entries[index].hash = hash;
    table->entries[index].depth = boardData->depth;
    table->entries[index].eval = eval;
}

long long int* generateHashingNumbers(short size) {
    long long int *hashingNumbers = malloc(sizeof(long long int) * size * size);
    int i;
    for (i = 0; i < size * size; i++) {
        hashingNumbers[i] = rand() << 16 | (long long int)rand() << 32 | (long long int)rand() << 48 | (long long int)rand();
    }
    return hashingNumbers;
}

void playGame(short size, short winCondition) {
    // get the board ready
    long long int p1 = 0;
    long long int p2 = 0;
    short playing = 1;

    struct SearchInfo boardData;
    boardData.turn = 1;
    boardData.boardSize = size;
    boardData.winCondition = winCondition;
    boardData.totalSquares = size * size;
    boardData.centerMask = buildCenterMask(size);
    boardData.nodes = 0;
    boardData.maxDepth = MAX_DEPTH;

    // setup the transposition table
    boardData.transpositionTable = malloc(sizeof(struct TranspositionTable));
    boardData.transpositionTable->size = 100000;
    boardData.transpositionTable->used = 0;
    boardData.transpositionTable->entries = malloc(sizeof(struct TranspositionTableEntry) * boardData.transpositionTable->size);



    // set the remaining values in the board to 1
    if (boardData.totalSquares < 64) {
        p1 = ~p1 >> boardData.totalSquares << boardData.totalSquares;
        p2 = ~p2 >> boardData.totalSquares << boardData.totalSquares;
    }

    while (playing) {

        // print the board
        printBoard(p1, p2, boardData.boardSize);

        // get the first move from the user
        short move = getMove(p1, p2, &boardData);

        // make the move
        makeMove(&p1, &p2, move, &boardData);

        // check for a win
        if (checkWin(&p1, &p2, &boardData, move) == 1) {
            printf("Player 1 wins!\n");
            break;
        }

        // for each value in the table update the depth
        for (int i = 0; i < boardData.transpositionTable->size; i++) {
            if (boardData.transpositionTable->entries[i].hash != 0) {
                boardData.transpositionTable->entries[i].depth -= 2;
            }
        }

        // call the bot to get the move
        // TODO add iterative deepening
        // get the time
        struct timeval start, end;
        short winCount = 0;
        gettimeofday(&start, NULL);
        boardData.depth = 2;
        while (1) {
            boardData.depth++;
            move = searchBoard(&p1, &p2, &boardData);
            winCount = (boardData.eval != 0) ? winCount + 1 : 0;
            gettimeofday(&end, NULL);
            if (end.tv_sec - start.tv_sec > 1 || boardData.depth > boardData.maxDepth || winCount > 3 || boardData.depth >= boardData.totalSquares) {
                printf("\n");
                break;
            }
        }

        // make the move
        makeMove(&p1, &p2, move, &boardData);

        // check for a win
        if (checkWin(&p1, &p2, &boardData, move) == -1) {
            printf("Player 2 wins!\n");
            printBoard(p1, p2, boardData.boardSize);
            break;
        }

        // print the nodes
        printf("Nodes: %d\n", boardData.nodes);
        boardData.nodes = 0;
    }
    freeBoardData(&boardData);
} 

void freeBoardData(struct SearchInfo * boardData) {
    free(boardData->transpositionTable->entries);
    free(boardData->transpositionTable);
    free(boardData->hashingNumbers);
}

// print the board
void printBoard(long long int p1, long long int p2, short size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if(p1 >> (i * size + j) & 1) {
                printf("X ");
            } else if (p2 >> (i * size + j) & 1) {
                printf("O ");
            } else {
                printf("- ");
            }
        }

        printf("\t");

        for (int j = 0; j < size; j++) {
            if (i * size + j < 10)
                printf(" ");
            printf("%d ", i * size + j);
        }

        printf("\n");
    }
    printf("\n");
}

// get the move from the user
short getMove(long long int p1, long long int p2, struct SearchInfo *boardData) {
    int x;
    while(1) {
        printf("Enter your move: ");
        scanf("%d", &x);
        if (x < boardData->totalSquares && x >= 0 && !(p1 >> x & 1) && !(p2 >> x & 1))
            break;
        else
            printf("Invalid move\n");
    }
    
    return (short) x;
}

// build the center mask given the board size
long long int buildCenterMask(short size) {
    long long int centerMask = 0;
    short center = size / 2;
    for (int i = 0; i < (size * size); i++) {
        if (i > size * (size / 4) && i < size * (size / 4 * 3) && i % size > size / 4 && i % size < size / 4 * 3)
            centerMask |= (1 << i);    
    }

    return centerMask;
}

// checks if the player that just moved ended the game
// TODO make this more efficient
short checkWin(long long int *p1, long long int *p2, struct SearchInfo *boardData, short lastMove) {
    short player;
    long long int playerBoard;
    
    // starting from the last move check if any dirrections have a line greater than the win
    // condition
    if (*p1 >> lastMove & 1) {
        player = 1;
        playerBoard = *p1;
    } else if (*p2 >> lastMove & 1){
        player = -1;
        playerBoard = *p2;
    }

    // check the horizontal
    short count = 1;
    short index = lastMove - 1;
    while (index % boardData->boardSize != boardData->boardSize - 1 && playerBoard >> index & 1 && index >= 0) {
        count++;
        index--;
    }
    index = lastMove + 1;
    while (index % boardData->boardSize != 0 && playerBoard >> index & 1) {
        count++;
        index++;
    }

    if (count >= boardData->winCondition)
        return player;

    // check the vertical
    count = 1;
    index = lastMove - boardData->boardSize;
    while (index >= 0 && playerBoard >> index & 1) {
        count++;
        index -= boardData->boardSize;
    }
    index = lastMove + boardData->boardSize;
    while (index < boardData->totalSquares && playerBoard >> index & 1) {
        count++;
        index += boardData->boardSize;
    }

    if (count >= boardData->winCondition)
        return player;

    // check the diagonals
    count = 1;
    index = lastMove - (boardData->boardSize + 1);
    while (index >= 0 && (index % boardData->boardSize != boardData->boardSize - 1) && playerBoard >> index & 1) {
        count++;
        index -= (boardData->boardSize + 1);
    }
    index = lastMove + boardData->boardSize + 1;
    while (index < boardData->totalSquares && (index % boardData->boardSize != 0) && playerBoard >> index & 1) {
        count++;
        index += (boardData->boardSize + 1);
    }

    if (count >= boardData->winCondition)
        return player;

    // check the other diagonal
    count = 1;
    index = lastMove - (boardData->boardSize - 1);
    while (index >= 0 && (index % boardData->boardSize != 0) && playerBoard >> index & 1) {
        count++;
        index -= (boardData->boardSize - 1);
    }
    index = lastMove + boardData->boardSize - 1;
    while (index < boardData->totalSquares && (index % boardData->boardSize != boardData->boardSize - 1) && playerBoard >> index & 1) {
        count++;
        index += (boardData->boardSize - 1);
    }

    if (count >= boardData->winCondition)
        return player;


    return 0;
}

// make a move 
void makeMove(long long int *p1, long long int *p2, short move, struct SearchInfo * boardData) {
    boardData->depth--;
    boardData->hash ^= boardData->hashingNumbers[move];
    if (boardData->turn == 1) {
        *p1 ^= 1ll << move;
        boardData->turn *= -1;
    } else {
        *p2 ^= 1ll << move;
        boardData->turn *= -1;
    }
}

// unmake a move
void unmakeMove(long long int *p1, long long int *p2, short move, struct SearchInfo *boardData) {
    boardData->depth++;
    boardData->turn *= -1;
    boardData->hash ^= boardData->hashingNumbers[move];
    if (boardData->turn == 1) {
        *p1 ^= 1ll << move;
    } else {
        *p2 ^= 1ll << move;
    }
}

// get bits on in a number (this is a bithack)
long long int getBitsOn(long long int i){
    i = i - ((i >> 1) & 0x5555555555555555);
    i = (i & 0x3333333333333333) + ((i >> 2) & 0x3333333333333333);
    return (((i + (i >> 4)) & 0xF0F0F0F0F0F0F0F) * 0x101010101010101) >> 56;
}

// evaluate the board in a static position (pieces closer to the center are better)
short evalBoardStatic(long long int *p1, long long int *p2, struct SearchInfo *boardData) {
    // get the center of the board
    short center = boardData->boardSize / 2;
    short centerIndex = center * boardData->boardSize + center;

    // logically and the board with the center mask to get the pieces in the center
    long long int p1Center = *p1 & boardData->centerMask;
    long long int p2Center = *p2 & boardData->centerMask;

    // count the number of pieces in the center (this is a bithack to count the number of 1s in a number)
    short p1CenterCount = getBitsOn(p1Center);
    short p2CenterCount = getBitsOn(p2Center);



    // get the distance from the center for each piece
    return (boardData->turn == -1) ? p1CenterCount - p2CenterCount : p2CenterCount - p1CenterCount;

}

// returns the move to make (starts the search calls evalBoard)
short searchBoard(long long int *p1, long long int *p2, struct SearchInfo *boardData) {
    // generate the possible moves
    long long int moves = ~(*p1 | *p2);
    int bestMove = -1;
    int bestEval = -9999;
    short alpha = -9999;
    short beta = 9999;

    // for each move update the board and call evalBoard
    for (short i = 0; i < boardData->totalSquares; i++) {
        if (moves >> i & 1) {
            makeMove(p1, p2, i, boardData);
            short eval = -evalBoard(p1, p2, boardData, i, -beta, -alpha);
            unmakeMove(p1, p2, i, boardData);

            // if the eval is better than the current best move
            // update the best move
            if (eval > bestEval) {
                bestEval = eval;
                bestMove = i;
            }

            // to inject some variation in the moves 
            // randomly choose a move if the eval is the same
            if (eval == bestEval) {
                if (rand() % 10 == 0) {
                    bestMove = i;
                }
            }

            printf("\rMove: %d Depth: %d Eval: %d   ", i, boardData->depth, bestEval);
        }
    }
    printf("\n");
    boardData->eval = bestEval;
    return (short) bestMove;
}

// returns the eval of the board at this point in the tree
short evalBoard(long long int *p1, long long int *p2, struct SearchInfo *boardData, short lastMove, short alpha, short beta) {
    boardData->nodes++;
    int bestEval = -9999;


    // see if this entry exists in the transposition table
    struct TranspositionTableEntry *entry = getEntry(boardData->transpositionTable, boardData->hash);

    // if the entry exists and the depth is greater than the current depth
    // return the eval
    if (entry != NULL) {
        if (entry->depth >= boardData->depth) {
            bestEval = entry->eval;
        }      
    }

    // check if the game is over or depth is reached
    short win = checkWin(p1, p2, boardData, lastMove);
    if (win != 0) {
        storeEntry(boardData->transpositionTable, boardData->hash, boardData, (short)-1000);
        return -1000;
    }

    // check for a depth cutoff
    if (boardData->depth <= 0) {
        return evalBoardStatic(p1, p2, boardData);
    }

    // generate the possible moves
    long long int moves = ~(*p1 | *p2);

    // if there are no moves left return 0
    if (moves == 0) {
        return 0;
    }

    // for each move update the board and call evalBoard
    for (short i = 0; i < boardData->totalSquares; i++) {
        if (moves >> i & 1) {
            makeMove(p1, p2, i, boardData);
            short eval = -evalBoard(p1, p2, boardData, i, -beta, -alpha);

            // update the eval 
            if (eval != 0) {
                eval = (eval > 0) ? eval - 1 : eval + 1;
            }

            unmakeMove(p1, p2, i, boardData);

            // if the eval is better than the current best move
            // update the best move
            if (eval > bestEval) {
                bestEval = eval;
            }
            // alpha beta pruning
            if (bestEval > alpha) {
                alpha = bestEval;
            }
            if (alpha > beta) {
                //break;
            }
        }
    }

    // store the eval in the transposition table
    storeEntry(boardData->transpositionTable, boardData->hash, boardData, (short)bestEval);

    return (short) bestEval;
}
