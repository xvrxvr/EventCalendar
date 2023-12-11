#include "common.h"

#include "hadrware.h"
#include "setup_data.h"
#include "wifi_module.h"
#include "activity.h"
#include "interactive.h"
#include "challenge_list.h"

void start_http_servers();

extern "C" void app_main(void)
{
    hw_init();
    init_or_load_setup();
    wifi_init();
    start_http_servers();
    Activity::start();

//    uint32_t door = open_door(0);
//    printf("OpenDoor returns %lX\n", door);

//    printf("EQuest = %d\n", EQuest::run_challenge());
//    printf("Riddle = %d\n", Riddle::run_challenge());
//    printf("15 Game = %d\n", Game15::run_challenge());
    printf("Tile Game = %d\n", TileGame::run_challenge());

    for(;;) {Interactive::entry(); vTaskDelay(100);}
}