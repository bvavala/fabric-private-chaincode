#include <battleship.h>
#include <string.h>

game_sequencer_t g_game_seq = {0};
battleship_game_t* g_p_the_game = &g_game_seq.the_game;

operation_outcome_e _new_game() {
    if(g_p_the_game->status == NONE || g_p_the_game->status == GAME_PLAYER_1_WINS || g_p_the_game->status == GAME_PLAYER_2_WINS) {
        g_game_seq.seq++;
        memset(g_p_the_game, 0, sizeof(battleship_game_t));
        g_p_the_game->status = GAME_PLAYER_1_TO_JOIN;
        return NEW_GAME;
    }
    //else
    return GAME_IN_PROGRESS;    
}

#define POINT_IN_BOUNDARIES(c) (c.x>=0 && c.y>=0 && c.x<BATTLESHIP_TABLE_ROWS && c.y<BATTLESHIP_TABLE_COLS)

#define ITH_SHIP_UNIT_POINT(s, i) \
    (coord_t)({s.coord.x + (s.orientation == HORIZONTAL ? i : 0), s.coord.y + (s.orientation == VERTICAL ? i : 0)})

typedef enum {
    NO_SHOT,
    HIT,
    MISS,
    NUM_RESERVED_OUTCOMES
} shot_outcome_e;

#define FROM_SHIP_INDEX_TO_CELL_INDEX(x) (x + NUM_RESERVED_OUTCOMES)
#define FROM_CELL_INDEX_TO_SHIP_INDEX(x) (x - NUM_RESERVED_OUTCOMES)
#define CELL_INDEX_HAS_SHIP_INDEX(x) (x >= NUM_RESERVED_OUTCOMES)

coord_t ith_ship_unit_coord(ship_t s, unsigned int i) {
    return {s.coord.x + (s.orientation == HORIZONTAL ? 0 : i), s.coord.y + (s.orientation == VERTICAL ? 0 : i)};
}

void _assign_token(char* in, unsigned int in_size, char* out, unsigned int out_size) {
    //TODO implement token with secret keys
    //NOTE: these must be secret keys deterministically derived from the state encryption key
    //      so in the end we need a secret seed, not an encryption key.
    player_e player_type = *((player_e*)in);
    if(player_type == PLAYER1) {
        out[0] = 'A';
        out[1] = 'F';
        out[2] = '\0';
    }
    else {
        out[0] = 'C';
        out[1] = 'D';
        out[2] = '\0';
    }
}

operation_outcome_e _join(const player_join_t player_join_message) {
    unsigned can_deploy_ship[] = {SHIPS_PER_LENGTH};
    player_t* player;
    player_e player_type;

    //what player is joining
    if(g_p_the_game->status == GAME_PLAYER_1_TO_JOIN) {
        player = &g_p_the_game->player1;
        player_type = PLAYER1;
    }
    else if(g_p_the_game->status == GAME_PLAYER_2_TO_JOIN) {
        player = &g_p_the_game->player2;
        player_type = PLAYER2;
    }
    else {
        return GAME_IN_PROGRESS;
    }

    //reset any previous joining attempts
    memset(player, 0, sizeof(player_t));

    for(unsigned int i = 0; i < TOTAL_SHIPS; i++) {
        const ship_t* p_ship = &player_join_message.ships[i];

        //check ship size
        if(p_ship->length > max_ship_length)
            return OVERSIZED_SHIP;

        //check ship can be deployed
        if(can_deploy_ship[p_ship->length] == 0)
            return CANNOT_DEPLOY_SHIP;

        //check ship orientation
        if(p_ship->orientation != HORIZONTAL && p_ship->orientation != VERTICAL)
            return BAD_SHIP_ORIENTATION;

        //check ship is in boundaries (just check if first and last points are)
        coord_t cstart = p_ship->coord;
        coord_t cend = ith_ship_unit_coord(*p_ship, p_ship->length-1);
        if(!POINT_IN_BOUNDARIES(cstart) ||
           !POINT_IN_BOUNDARIES(cend))
            return SHIP_OUT_OF_BOUNDARIES;

        //check ship does not overlap
        for(unsigned int j=0; j<p_ship->length; j++) {
            coord_t c = ith_ship_unit_coord(*p_ship, j);
            if(player->own_table[c.x][c.y] > 0)
                return SHIP_OVERLAP;
        }

        //***all checks passed***

        //reduce deployable ships
        can_deploy_ship[p_ship->length]--;
        //update ship list
        player->ships[i] = *p_ship;
        player->ships[i].floating_units = player->ships[i].length;
        //update total floating units
        player->total_floating_units += player->ships[i].floating_units;
        //update table
        for(unsigned int j=0; j<p_ship->length; j++) {
            coord_t c = ith_ship_unit_coord(*p_ship, j);
            player->own_table[c.x][c.y] = FROM_SHIP_INDEX_TO_CELL_INDEX(i);
        }
    }

    //register player
    player->player_type = player_type;
    //assign authentication token to player
    _assign_token((char*)&player_type, sizeof(player_type), player->assigned_token_string, sizeof(player->assigned_token_string));
    //update status
    if(g_p_the_game->status == GAME_PLAYER_1_TO_JOIN)
        g_p_the_game->status = GAME_PLAYER_2_TO_JOIN;
    else
        g_p_the_game->status = GAME_PLAYER_1_TURN; //player 1 starts!

    return PLAYER_JOINED;
}

operation_outcome_e _play_shot(const player_shot_t shot) {
    //NOTE: player authentication is done by the caller, here it is assumed the right player plays
    player_t* cur_player = (g_p_the_game->status == GAME_PLAYER_1_TURN ? &g_p_the_game->player1 : &g_p_the_game->player2);
    player_t* adv_player = (g_p_the_game->status == GAME_PLAYER_1_TURN ? &g_p_the_game->player2 : &g_p_the_game->player1);

    //check shot in boundaries
    if(!POINT_IN_BOUNDARIES(shot.coord))
        return SHOT_OUT_OF_BOUNDARIES;

    //check shot is new
    if(cur_player->guess_table[shot.coord.x][shot.coord.y] != NO_SHOT)
        return SHOT_ALREADY_PLAYED;

    //**all check passed **
    
    // check if ship was hit (and sunk)
    if(CELL_INDEX_HAS_SHIP_INDEX(adv_player->own_table[shot.coord.x][shot.coord.y])) { // it is a hit
        unsigned int ship_index = FROM_CELL_INDEX_TO_SHIP_INDEX(adv_player->own_table[shot.coord.x][shot.coord.y]);

        //reduce global/local floating units
        adv_player->ships[ship_index].floating_units--;
        adv_player->total_floating_units--;

        //mark ship as hit for player
        cur_player->guess_table[shot.coord.x][shot.coord.y] = HIT;

        //mark ship as hit for adversary
        adv_player->own_table[shot.coord.x][shot.coord.y] = HIT;

        //if ship sunk, reveal ship
        if(adv_player->ships[ship_index].floating_units == 0) {
            cur_player->adv_ships[ship_index] = adv_player->ships[ship_index];
        }
    }
    else { // it is a miss
        //mark a miss for player
        cur_player->guess_table[shot.coord.x][shot.coord.y] = MISS;

        //mark a miss for adversary
        adv_player->own_table[shot.coord.x][shot.coord.y] = MISS;
    }


    //switch turn or declare winner
    g_p_the_game->status = (g_p_the_game->status == GAME_PLAYER_1_TURN ?
            (adv_player->total_floating_units == 0 ? GAME_PLAYER_1_WINS : GAME_PLAYER_2_TURN)
            : 
            (adv_player->total_floating_units == 0 ? GAME_PLAYER_2_WINS : GAME_PLAYER_1_TURN));

    return MOVE_ACCEPTED;
}

player_response_t join(const player_join_t player_join_message) {
    player_response_t pr;

    if(g_p_the_game->status != GAME_PLAYER_1_TO_JOIN && g_p_the_game->status != GAME_PLAYER_2_TO_JOIN) {
        memset(&pr, 0, sizeof(player_response_t));
        pr.operation_outcome = GAME_IN_PROGRESS;
        return pr;
    }

    //**all checks done here**

    player_t* player_to_join = (g_p_the_game->status == GAME_PLAYER_1_TO_JOIN ? &g_p_the_game->player1 : &g_p_the_game->player2);
    
    pr.operation_outcome = _join(player_join_message);
    pr.game_status = g_p_the_game->status;
    pr.player_data = *player_to_join;
    return pr;
}

player_e _authenticate_player(char* token_string) {
    if(memcmp(g_p_the_game->player1.assigned_token_string, token_string, 33) == 0) {
        return PLAYER1;
    }
    if(memcmp(g_p_the_game->player2.assigned_token_string, token_string, 33) == 0) {
        return PLAYER2;
    }
    //else
    return NO_PLAYER;
}

player_response_t play_shot(player_shot_t shot) {
    player_response_t pr;
    player_t* player = (g_p_the_game->status == GAME_PLAYER_1_TURN ? &g_p_the_game->player1 : &g_p_the_game->player2);
    memset(&pr, 0, sizeof(player_response_t));

    //check if somebody can play
    if(g_p_the_game->status != GAME_PLAYER_1_TURN && g_p_the_game->status != GAME_PLAYER_2_TURN) {
        pr.operation_outcome = WRONG_PLAYER;
        return pr;
    }

    //authenticate player
    player_e authenticated_player = _authenticate_player(shot.token_string);
    //check if it is player's turn
    if(authenticated_player != player->player_type) {
        pr.operation_outcome = EXPECTED_OTHER_PLAYER;
        return pr;
    }

    //***all checks passed***
    
    pr.operation_outcome = _play_shot(shot);
    pr.game_status = g_p_the_game->status;
    pr.player_data = *player;
    return pr;
}

player_response_t player_refresh(player_refresh_t player_refresh) {
    player_response_t pr;
    player_e authenticated_player = _authenticate_player(player_refresh.token_string);

    if(authenticated_player == NO_PLAYER) {
        memset(&pr, 0, sizeof(player_response_t));
        pr.operation_outcome = WRONG_PLAYER;
        return pr;
    }
    //else
    pr.operation_outcome = REFRESHED;
    pr.game_status = g_p_the_game->status;
    pr.player_data = (authenticated_player == PLAYER1 ? g_p_the_game->player1 : g_p_the_game->player2);
    return pr;
}

