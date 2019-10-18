#include "shim.h"
#include "logging.h"
#include <string>
#include "base64/base64.h"
#include "json/parson.h"

#include "battleship-core/battleship.h"

#define OK "OK"
#define UNDEFINED_FUNCTION "undefined function"
#define GAME_STATE_ERROR "GAME_STATE_ERROR"
#define GAME_PARAM_ERROR "GAME_PARAM_ERROR"

#define CHECK_JSON_FAILURE(ret) \
    if(ret == JSONFailure) {    \
        LOG_ERROR("json failed %s:%d", __FUNCTION__, __LINE__); \
    }

extern game_sequencer_t g_game_seq;
extern battleship_game_t* g_p_the_game;

int init(
    uint8_t* response,
    uint32_t max_response_len,
    uint32_t* actual_response_len,
    shim_ctx_ptr_t ctx)
{
    return 0;
}

void display_game() {
    LOG_DEBUG("STATUS: %u\n", g_p_the_game->status);
    LOG_DEBUG("--------- PLAYER 1 -------------\n");
    LOG_DEBUG("token: %s\n", g_p_the_game->player1.assigned_token_string);
    for(unsigned int i=0; i<BATTLESHIP_TABLE_ROWS; i++) {
        for(unsigned int j=0; j<BATTLESHIP_TABLE_COLS; j++) {
            LOG_DEBUG("%u ", (unsigned)g_p_the_game->player1.own_table[i][j]);
        }
        LOG_DEBUG("\t\t\t\t");
        LOG_DEBUG("\t\t\t\t");
        LOG_DEBUG("\t\t\t\t");
        for(unsigned int j=0; j<BATTLESHIP_TABLE_COLS; j++) {
            LOG_DEBUG("%u ", (unsigned)g_p_the_game->player1.guess_table[i][j]);
        }
        LOG_DEBUG("\n");
    }
    LOG_DEBUG("ships: ");
    for(unsigned int i=0; i<TOTAL_SHIPS; i++) {
        LOG_DEBUG("{%u, %u, %u, %u, %u} ",
                g_p_the_game->player1.ships[i].coord.x, g_p_the_game->player1.ships[i].coord.y, g_p_the_game->player1.ships[i].orientation,
                g_p_the_game->player1.ships[i].length, g_p_the_game->player1.ships[i].floating_units);
    }
    LOG_DEBUG("\t\t");
    LOG_DEBUG("adv_ships: ");
    for(unsigned int i=0; i<TOTAL_SHIPS; i++) {
        LOG_DEBUG("{%u, %u, %u, %u, %u} ",
                g_p_the_game->player1.adv_ships[i].coord.x, g_p_the_game->player1.adv_ships[i].coord.y, g_p_the_game->player1.adv_ships[i].orientation,
                g_p_the_game->player1.adv_ships[i].length, g_p_the_game->player1.adv_ships[i].floating_units);
    }
    LOG_DEBUG("\n");
    LOG_DEBUG("Total (remaining) floating units: %u\n", g_p_the_game->player1.total_floating_units);
    LOG_DEBUG("--------------------------------");
    LOG_DEBUG("\n\n");
    LOG_DEBUG("\n\n");
    LOG_DEBUG("--------- PLAYER 2 -------------\n");
    LOG_DEBUG("token: %s\n", g_p_the_game->player2.assigned_token_string);
    for(unsigned int i=0; i<BATTLESHIP_TABLE_ROWS; i++) {
        for(unsigned int j=0; j<BATTLESHIP_TABLE_COLS; j++) {
            LOG_DEBUG("%u ", (unsigned)g_p_the_game->player2.own_table[i][j]);
        }
        LOG_DEBUG("\t\t\t\t");
        LOG_DEBUG("\t\t\t\t");
        LOG_DEBUG("\t\t\t\t");
        for(unsigned int j=0; j<BATTLESHIP_TABLE_COLS; j++) {
            LOG_DEBUG("%u ", (unsigned)g_p_the_game->player2.guess_table[i][j]);
        }
        LOG_DEBUG("\n");
    }
    LOG_DEBUG("ships: ");
    for(unsigned int i=0; i<TOTAL_SHIPS; i++) {
        LOG_DEBUG("{%u, %u, %u, %u, %u} ",
                g_p_the_game->player2.ships[i].coord.x, g_p_the_game->player2.ships[i].coord.y, g_p_the_game->player2.ships[i].orientation,
                g_p_the_game->player2.ships[i].length, g_p_the_game->player2.ships[i].floating_units);
    }
    LOG_DEBUG("\t\t");
    LOG_DEBUG("adv_ships: ");
    for(unsigned int i=0; i<TOTAL_SHIPS; i++) {
        LOG_DEBUG("{%u, %u, %u, %u, %u} ",
                g_p_the_game->player2.adv_ships[i].coord.x, g_p_the_game->player2.adv_ships[i].coord.y, g_p_the_game->player2.adv_ships[i].orientation,
                g_p_the_game->player2.adv_ships[i].length, g_p_the_game->player2.adv_ships[i].floating_units);
    }
    LOG_DEBUG("\n");
    LOG_DEBUG("Total (remaining) floating units: %u\n", g_p_the_game->player2.total_floating_units);

    LOG_DEBUG("--------------------------------");
    LOG_DEBUG("\n");
}


std::string player_response_to_json(player_response_t pr) {
    JSON_Value* root_value = json_value_init_object();
    JSON_Object* root_object = json_value_get_object(root_value);
    json_object_set_number(root_object, "operation_outcome", pr.operation_outcome);
    json_object_set_number(root_object, "game_status", (int)pr.game_status);
    json_object_set_number(root_object, "player_type", pr.player_data.player_type);
    json_object_set_string(root_object, "assigned_token_string", pr.player_data.assigned_token_string);

    json_object_set_value(root_object, "guess_table", json_value_init_array());
    JSON_Array* guess_table_array = json_object_get_array(root_object, "guess_table");
    for(int i=0; i<BATTLESHIP_TABLE_ROWS; i++) {
        for(int j=0; j<BATTLESHIP_TABLE_COLS; j++) {
            json_array_append_number(guess_table_array, pr.player_data.guess_table[i][j]);
        }
    }

    json_object_set_value(root_object, "own_table", json_value_init_array());
    JSON_Array* own_table_array = json_object_get_array(root_object, "own_table");
    for(int i=0; i<BATTLESHIP_TABLE_ROWS; i++) {
        for(int j=0; j<BATTLESHIP_TABLE_COLS; j++) {
            json_array_append_number(own_table_array, pr.player_data.own_table[i][j]);
        }
    }

    json_object_set_value(root_object, "ship_array", json_value_init_array());
    JSON_Array* ship_array = json_object_get_array(root_object, "ship_array");
    for(int i=0; i<TOTAL_SHIPS; i++) {
        CHECK_JSON_FAILURE(json_array_append_number(ship_array, pr.player_data.ships[i].length));
        CHECK_JSON_FAILURE(json_array_append_number(ship_array, pr.player_data.ships[i].coord.x));
        CHECK_JSON_FAILURE(json_array_append_number(ship_array, pr.player_data.ships[i].coord.y));
        CHECK_JSON_FAILURE(json_array_append_number(ship_array, pr.player_data.ships[i].orientation));
        CHECK_JSON_FAILURE(json_array_append_number(ship_array, pr.player_data.ships[i].floating_units));
    }

    json_object_set_number(root_object, "total_floating_units", pr.player_data.total_floating_units);

    json_object_set_value(root_object, "adversary_ship_array", json_value_init_array());
    JSON_Array* adv_ship_array = json_object_get_array(root_object, "adversary_ship_array");
    for(int i=0; i<TOTAL_SHIPS; i++) {
        CHECK_JSON_FAILURE(json_array_append_number(adv_ship_array, pr.player_data.adv_ships[i].length));
        CHECK_JSON_FAILURE(json_array_append_number(adv_ship_array, pr.player_data.adv_ships[i].coord.x));
        CHECK_JSON_FAILURE(json_array_append_number(adv_ship_array, pr.player_data.adv_ships[i].coord.y));
        CHECK_JSON_FAILURE(json_array_append_number(adv_ship_array, pr.player_data.adv_ships[i].orientation));
        CHECK_JSON_FAILURE(json_array_append_number(adv_ship_array, pr.player_data.adv_ships[i].floating_units));
    }

    char* serialized_string = json_serialize_to_string(root_value);
    std::string out(serialized_string);
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
    return out;
}

player_refresh_t json_to_player_refresh(std::string json_in) {
    player_refresh_t pr = {0};
    JSON_Value* root = json_parse_string(json_in.c_str());
    const char* token_string = json_object_get_string(json_object(root), "token_string");
    int max_string_len = (strlen(token_string) >= 32 ? 32 : strlen(token_string));
    strncpy(pr.token_string, token_string, 32);
    return pr;
}

player_shot_t json_to_player_shot(std::string json_in) {
    player_shot_t ps = {0};
    JSON_Value* root = json_parse_string(json_in.c_str());
    const char* token_string = json_object_get_string(json_object(root), "token_string");
    int max_string_len = (strlen(token_string) >= 32 ? 32 : strlen(token_string));
    strncpy(ps.token_string, token_string, max_string_len);
    ps.coord.x = json_object_get_number(json_object(root), "x");
    ps.coord.y = json_object_get_number(json_object(root), "y");
    return ps;
}

player_join_t json_to_player_join(std::string json_in) {
    player_join_t pj = {0};
    JSON_Value* root = json_parse_string(json_in.c_str());
    JSON_Array* ship_array = json_object_get_array(json_object(root), "ship_array");
    int ship_array_len = json_array_get_count(ship_array); // NOTE: ships are serialized as <length, x, y, orientation>
    if(ship_array_len != 4 /*4 numbers in ship structure*/ * TOTAL_SHIPS) {
        LOG_ERROR("wrong number of ships");
        return pj; //empty pj
    }
    for(int ship=0, i=0; ship<TOTAL_SHIPS; ship++) {
        pj.ships[ship].length = json_array_get_number(ship_array, i++);
        pj.ships[ship].coord.x = json_array_get_number(ship_array, i++);
        pj.ships[ship].coord.y = json_array_get_number(ship_array, i++);
        pj.ships[ship].orientation = (ship_orientation_e)json_array_get_number(ship_array, i++);
        //pj.ships[ship].floating_units = json_array_get_number(ship_array, i++);
    }
    return pj;
}

int invoke(
    uint8_t* response,
    uint32_t max_response_len,
    uint32_t* actual_response_len,
    shim_ctx_ptr_t ctx)
{
    LOG_DEBUG("BattleshipCC executing...");
    
    std::string function_name;
    std::vector<std::string> params;
    get_func_and_params(function_name, params, ctx);
    std::string result = UNDEFINED_FUNCTION;

    if(function_name == "NewGame") {
        _new_game();
        result = OK;
    }
    else {
        //load battleship game
        uint32_t battleship_state_len = 0;
        get_state("battleship_game", (uint8_t*)&g_game_seq, sizeof(game_sequencer_t), &battleship_state_len, ctx);
        g_p_the_game = &g_game_seq.the_game;
        result = (battleship_state_len != sizeof(game_sequencer_t) ? GAME_STATE_ERROR : OK);
    }

    if(function_name == "RefreshPlayer") {
        LOG_DEBUG("input string: %s", params[0].c_str());
        player_refresh_t pref = json_to_player_refresh(params[0]);
        //if(pr.token_string[0] == '\0') {
        //    result = GAME_PARAM_ERROR;
        //}
        //else {
            player_response_t pres = player_refresh(pref);
            result = player_response_to_json(pres);
            //result = base64_encode((const unsigned char*)&pr, sizeof(player_response_t));
        //}
    }

    if(function_name == "JoinGame") {
        LOG_DEBUG("input string: %s", params[0].c_str());
        player_join_t pj = json_to_player_join(params[0]);
        player_response_t pres = join(pj);
        result = player_response_to_json(pres);
    }
    if(function_name == "Shoot") {
        LOG_DEBUG("input string: %s", params[0].c_str());
        player_shot_t ps = json_to_player_shot(params[0]);
        LOG_DEBUG("shoot x y -> %u %u", ps.coord.x, ps.coord.y);
        player_response_t pres = play_shot(ps);
        result = player_response_to_json(pres);
    }

    display_game();

    int neededSize = result.size();
    if (max_response_len < neededSize)
    {
        LOG_DEBUG("BattleshipCC: Response buffer too small");
        *actual_response_len = 0;
        return -1;
    }

    //store game only for write operation (exclude refresh, since it is read-only)
    //if(function_name != "RefreshPlayer")
        put_state("battleship_game", (uint8_t*)&g_game_seq, sizeof(game_sequencer_t), ctx);

    memcpy(response, result.c_str(), neededSize);
    *actual_response_len = neededSize;
    LOG_DEBUG("result: %s", result.c_str());
    LOG_DEBUG("BattleshipCC executing...done");
    return 0;
}

