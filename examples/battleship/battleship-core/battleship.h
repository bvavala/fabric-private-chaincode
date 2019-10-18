#ifndef BATTLESHIP_H
#define BATTLESHIP_H

#define BATTLESHIP_TABLE_ROWS 10
#define BATTLESHIP_TABLE_COLS 10

#define SHIPS_OF_LENGTH_1 0
#define SHIPS_OF_LENGTH_2 1
#define SHIPS_OF_LENGTH_3 2
#define SHIPS_OF_LENGTH_4 1
#define SHIPS_OF_LENGTH_5 1
#define SHIPS_OF_LENGTH_6 0
#define SHIPS_OF_LENGTH_7 0
#define SHIPS_OF_LENGTH_8 0
#define SHIPS_OF_LENGTH_9 0

#define SHIPS_PER_LENGTH  \
    0,                 /*of length 0*/ \
    SHIPS_OF_LENGTH_1, /*of length 1*/ \
    SHIPS_OF_LENGTH_2, /*of length 2*/ \
    SHIPS_OF_LENGTH_3, /*of length 3*/ \
    SHIPS_OF_LENGTH_4, /*of length 4*/ \
    SHIPS_OF_LENGTH_5, /*of length 5*/ \
    SHIPS_OF_LENGTH_6, /*of length 6*/ \
    SHIPS_OF_LENGTH_7, /*of length 7*/ \
    SHIPS_OF_LENGTH_8, /*of length 8*/ \
    SHIPS_OF_LENGTH_9  /*of length 9*/ 

#define SUM10(a,b,c,d,e,f,g,h,i,l) (a+b+c+d+e+f+g+h+i+l)
#define TOTAL_SHIPS (SUM10(\
            0,\
            SHIPS_OF_LENGTH_1,\
            SHIPS_OF_LENGTH_2,\
            SHIPS_OF_LENGTH_3,\
            SHIPS_OF_LENGTH_4,\
            SHIPS_OF_LENGTH_5,\
            SHIPS_OF_LENGTH_6,\
            SHIPS_OF_LENGTH_7,\
            SHIPS_OF_LENGTH_8,\
            SHIPS_OF_LENGTH_9))

const unsigned ships_per_length[] = {SHIPS_PER_LENGTH};
const unsigned max_ship_length = (sizeof(ships_per_length)/sizeof(ships_per_length[0])) - 1;

typedef enum {
    HORIZONTAL,
    VERTICAL
} ship_orientation_e;

typedef struct {
    unsigned int x,y;
} coordinate_t;
typedef coordinate_t coord_t;

typedef struct {
    unsigned int length;
    coord_t coord;
    ship_orientation_e orientation;
    unsigned int floating_units;
} ship_t;

typedef enum {
    NO_PLAYER,
    PLAYER1,
    PLAYER2,
} player_e;

typedef enum {
    NONE,
    GAME_PLAYER_1_TO_JOIN,
    GAME_PLAYER_2_TO_JOIN,
    GAME_PLAYER_1_TURN,
    GAME_PLAYER_2_TURN,
    GAME_PLAYER_1_WINS,
    GAME_PLAYER_2_WINS,
} game_state_machine_e;

typedef enum {
    NO_OUTCOME,
    REFRESHED,
    NEW_GAME,
    GAME_IN_PROGRESS,
    WRONG_PLAYER,
    EXPECTED_OTHER_PLAYER,
    PLAYER_JOINED,
    MOVE_ACCEPTED,
    OVERSIZED_SHIP,
    BAD_SHIP_ORIENTATION,
    SHIP_OVERLAP,
    CANNOT_DEPLOY_SHIP,
    SHIP_OUT_OF_BOUNDARIES,
    SHOT_OUT_OF_BOUNDARIES,
    SHOT_ALREADY_PLAYED
} operation_outcome_e;

typedef struct {
    ship_t ships[TOTAL_SHIPS];
    //no auth data
} player_join_t;

typedef struct {
    char token_string[33];
} player_refresh_t;

typedef struct {
    coord_t coord;
    char token_string[33];
} player_shot_t;

typedef struct {
    player_e player_type;
    char assigned_token_string[33];
    unsigned char guess_table[BATTLESHIP_TABLE_ROWS][BATTLESHIP_TABLE_COLS];
    unsigned char own_table  [BATTLESHIP_TABLE_ROWS][BATTLESHIP_TABLE_COLS];
    ship_t ships[TOTAL_SHIPS];
    unsigned int total_floating_units;
    ship_t adv_ships[TOTAL_SHIPS];
} player_t;

typedef struct {
    operation_outcome_e operation_outcome;
    game_state_machine_e game_status;
    player_t player_data;
} player_response_t;

typedef struct {
    game_state_machine_e status;
    player_t player1;
    player_t player2;
} battleship_game_t;

typedef struct {
    unsigned int seq;
    battleship_game_t the_game;
} game_sequencer_t;

operation_outcome_e _new_game();
player_response_t   join              (const player_join_t player_join_message);
player_response_t   play_shot         (player_shot_t shot);
player_response_t   player_refresh    (player_refresh_t player_refresh);


#endif //BATTLESHIP_H
