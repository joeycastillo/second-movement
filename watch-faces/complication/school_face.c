
#include <stdlib.h>
#include <string.h>
#include "school_face.h"
#include "filesystem.h"
#include "watch.h"
#include "watch_utility.h"
#include "watch_common_display.h"


static void persist_schedule_to_filesystem(schedule_for_filesave *schedule, uint8_t schedule_index) {

    char filename[13];
    sprintf(filename, "%s%02d.%03d", SCHOOLFACE_FILENAME_HEADER,schedule_index,SCHOOLFACE_FILENAME_EXTENTION);
    filesystem_write_file(filename, (char *) schedule , sizeof(schedule_for_filesave));
}


// Check the integrity of the loaded data, and set default values if errors are found
// 読み込んだデータの整合性をチェックし、エラーがあればデフォルト値をセットする
static void schedule_data_check(schedule_for_filesave *schedule) {

    if (schedule->start_hour > 23) {
        schedule->start_hour = 0;
    }

    if (schedule->start_minute > 59) {
        schedule->start_minute = 0;
    }
    
    if (schedule->end_hour > 23) {
        schedule->end_hour = 0;
    }

    if (schedule->end_minute > 59) {
        schedule->end_minute = 0;
    }

    // If End time is earlier than Start time, adjust End to match Start
    // Start時間よりEnd時間が早い場合は、End時間をStart時間に合わせる
    if (schedule->end_hour < schedule->start_hour ||    
        (schedule->end_hour == schedule->start_hour && schedule->end_minute < schedule->start_minute)) {        
        schedule->end_hour = schedule->start_hour;
        schedule->end_minute = schedule->start_minute;
    }
    

    for (size_t i = 0; i < 3; i++){
        schedule->main_label[i] = (schedule->main_label[i] >= 0x20 && schedule->main_label[i] <= 0x7E) ? schedule->main_label[i] : ' ';
    }

    for (size_t i = 0; i < 2; i++){
        schedule->sub_label[i] = (schedule->sub_label[i] >= 0x20 && schedule->sub_label[i] <= 0x7E) ? schedule->sub_label[i] : ' ';
    }
    
}


static bool load_schedule_from_filesystem(schedule_for_filesave *schedule, uint8_t schedule_index) {

    char filename[13];

    sprintf(filename, "%s%02d.%03d", SCHOOLFACE_FILENAME_HEADER,schedule_index,SCHOOLFACE_FILENAME_EXTENTION);

    bool read_successful = filesystem_read_file(filename, (char *)schedule, sizeof(schedule_for_filesave));

    if (read_successful) {
        // Perform error checking when data is read successfully
        // 正常に読み込めた場合はデータのエラーチェックを行う
        schedule_data_check(schedule);

        return true;
    }
    else  {
        // If unable to load, set default values
        // 読み込めない場合はデフォルト値を読み込む
        
        //printf("Failed to load schedule %d, using default values.\n", schedule_index);

        schedule->main_label[0] = 'A';
        schedule->main_label[1] = 'A';
        schedule->main_label[2] = 'A';

        schedule->sub_label[0] = 'A';
        schedule->sub_label[1] = 'A';

        schedule->start_hour = 0;
        schedule->start_minute = 0;
        schedule->end_hour = 0;
        schedule->end_minute = 0;

        return false;
    }
    
}


// Advance character to the next character code
// 文字を次の文字コードに進める
static void advance_character_at_position(char *character, uint8_t position) {

    //printf("advance_character_at_position pos=%d \n",position);

    bool is_custom_lcd = watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM;
    if (is_custom_lcd || position == 0) {
        // On custom LCD and at position 0 of classic LCD, all characters are supported.
        // Only need to jump to useful characters in the ASCII table
        //
        // カスタムLCDおよびクラシックLCDの位置0では、すべての文字がサポートされています。
        // 必要なのは、ASCIIテーブル上で有用な文字へジャンプすることだけです。
        switch (*character) {
            case ' ':
                *character = 'A';
                break;
            case 'z':
                *character = '0';
                break;
            case '9':
                *character = '{';
                break;
            case '}':
                *character = '*';
                break;
            case '.':
                *character = '/';
                break;
            case '/':
            case 0x7F: // failsafe: if they've broken out of the intended rotation, return them to 0x20
                *character = ' ';
                break;
            default:
                *character += 1;
                break;
        }
    } else {
        //クラシックディスプレイの1文字目のとき
        // otherwise we have to do some wacky shit
        // それ以外の場合は、ちょっと厄介なことをしなければなりません
        switch (*character) {
            case ' ':
                *character = 'A';
                break;
            case 'F':
            case 'J':
            case 'L':
            case 'R':
            case '1':
                *character += 2;
                break;
            case 'H':
                *character = 'l';
                break;
            case 'l':
                *character = 'J';
                break;
            case 'O':
                *character = 'R';
                break;
            case 'U':
                *character = 'X';
                break;
            case 'X':
                *character = '0';
                break;
            case '3':
                *character = '7';
                break;
            case '8':
                *character = '{';
                break;
            case '{':
            case 0x7F: // failsafe: if they've broken out of the intended rotation, return them to 0x20
                *character = ' ';
                break;
            default:
                *character += 1;
                break;
        }
    }
}

// Migrated code from clock_face
// clocl_faceからの移植コード

//from clock face
static watch_date_time_t clock_24h_to_12h(watch_date_time_t date_time) {
    date_time.unit.hour %= 12;

    if (date_time.unit.hour == 0) {
        date_time.unit.hour = 12;
    }

    return date_time;
}


static  void clock_indicate(watch_indicator_t indicator, bool on) {
    if (on) {
        watch_set_indicator(indicator);
    } else {
        watch_clear_indicator(indicator);
    }
}

static  void clock_indicate_alarm() {
    clock_indicate(WATCH_INDICATOR_SIGNAL, movement_alarm_enabled());
}

static  void clock_indicate_time_signal(school_state_t *state) {
    clock_indicate(WATCH_INDICATOR_BELL, state->time_signal_enabled);
}

static  void clock_indicate_24h() {
    clock_indicate(WATCH_INDICATOR_24H, !!movement_clock_mode_24h());
}

static  bool clock_is_pm(watch_date_time_t date_time) {
    return date_time.unit.hour >= 12;
}

static  void clock_indicate_pm(watch_date_time_t date_time) {
    if (movement_clock_mode_24h()) { return; }
    clock_indicate(WATCH_INDICATOR_PM, clock_is_pm(date_time));
}

//clock_faceからの移植
static  void clock_display_all(watch_date_time_t date_time) {
    char buf[8 + 1];

    snprintf(
        buf,
        sizeof(buf),
        movement_clock_mode_24h() == MOVEMENT_CLOCK_MODE_024H ? "%02d%02d%02d%02d" : "%2d%2d%02d%02d",
        date_time.unit.day,
        date_time.unit.hour,
        date_time.unit.minute,
        date_time.unit.second
    );

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, watch_utility_get_long_weekday(date_time), watch_utility_get_weekday(date_time));
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    watch_display_text(WATCH_POSITION_BOTTOM, buf + 2);
}


static  void clock_display_clock(watch_date_time_t current) {
    if (movement_clock_mode_24h() == MOVEMENT_CLOCK_MODE_12H) {
        clock_indicate_pm(current);
        current = clock_24h_to_12h(current);
    }
    clock_display_all(current);
}


// Add a schedule to the structure
// 構造体にスケジュールを追加する
static void addDefaultSchedule( int8_t i , bool isActive, school_state_t* state,char* labelLCustom,char* labelLClassic,char* labelR, int s_hour,int s_minute,int e_hour,int e_minute)
{
    //int i = state->scheduleCount;

    if ( (i+1) > SCHEDULE_LIMIT ){
        // If the limit is exceeded, raise an error and do not add
        // リミットを超えた場合はエラーを発生させ追加させない
        printf("ERROR -> over schedule limit error\n");
    }
    else{
        //printf("schedule[%d] \n",i);
        state->schedule[i].is_active = isActive;

        strncpy(state->schedule[i].labelLCustom  ,labelLCustom  ,LABEL_L_LIMIT );
        strncpy(state->schedule[i].labelLClassic ,labelLClassic ,LABEL_LS_LIMIT );
        strncpy(state->schedule[i].labelR  ,labelR  ,LABEL_R_LIMIT );

        state->schedule[i].start.unit.hour = s_hour;
        state->schedule[i].start.unit.minute = s_minute;
        state->schedule[i].start.unit.second = 00;

        state->schedule[i].end.unit.hour = e_hour;
        state->schedule[i].end.unit.minute = e_minute;
        state->schedule[i].end.unit.second = 00;
    }
}

// Copy values from memory to file schedule structure
// メモリからファイルスケジュール構造体に値をコピーする
static void copyFromMemToStructure(school_state_t *state, int i)
{
    //printf("copyFromMemToStructure %d \n",i);

    state->file_schedule[i].is_active = state->schedule[i].is_active;
    
    strncpy(state->file_schedule[i].main_label, state->schedule[i].labelLCustom , LABEL_L_LIMIT);
    strncpy(state->file_schedule[i].sub_label, state->schedule[i].labelR , LABEL_R_LIMIT);
    state->file_schedule[i].start_hour = state->schedule[i].start.unit.hour;
    state->file_schedule[i].start_minute = state->schedule[i].start.unit.minute;
    state->file_schedule[i].end_hour = state->schedule[i].end.unit.hour;
    state->file_schedule[i].end_minute = state->schedule[i].end.unit.minute;

    //printf("add schedule %d %s %s  %02d:%02d-%02d:%02d\n",
    //     i,
    //     state->file_schedule[i].main_label,
    //     state->file_schedule[i].sub_label,
    //     state->file_schedule[i].start_hour,
    //     state->file_schedule[i].start_minute,
    //     state->file_schedule[i].end_hour,
    //     state->file_schedule[i].end_minute);

    //debug
    //persist_schedule_to_filesystem(&state->schedule[0]);
}


// Reload schedules
// スケジュールを読み込み直す
void loadScheduleFromMemoryAndFile(school_state_t *state) {


    // Create schedules up to the specified number
    // スケジュールを指定個数分作成する
    for (int i = 0; i < SCHEDULE_LIMIT; i++) {
        addDefaultSchedule(i, false, state, "   ", "  ", "  ", 0, 0, 0, 0);
    }

    // Initialize default schedules
    // デフォルトスケジュールの初期化    
    addDefaultSchedule( 0, true,state,"CLS","CL"," 1", 9,20,10,50);
    addDefaultSchedule( 1, true,state,"CLS","CL"," 2",11, 0,12,30);
    addDefaultSchedule( 2, true,state,"CLS","CL"," 3",13,20,14,50);
    addDefaultSchedule( 3, true,state,"CLS","CL"," 4",15, 0,16,30);
    addDefaultSchedule( 4,false,state,"FIN","FN","  ",16,30,18, 0);

    //debug code
    // addDefaultSchedule( 3, true,state,"CLS","CL"," 4",16,19 ,16,20);
    // addDefaultSchedule( 4, true,state,"FIN","FN","  ",16,20,16,21);
    // addDefaultSchedule( 5, true,state,"FI2","F2","  ",16,21,16,22);

    for (int i = 0; i <  SCHEDULE_LIMIT ; i++)
    {
        //printf("読み込みテスト\n");

        bool isReadSuccess = load_schedule_from_filesystem(&state->file_schedule[i], i);

        // On successful read, overwrite state->schedule with values from file_schedule
        //読み込み成功時は、file_scheduleからstate->scheduleに値を上書きする
        
        
        if (isReadSuccess) {
            //printf("Schedule %d loaded successfully.\n", i);

            state->schedule[i].is_active = state->file_schedule[i].is_active;

            strncpy(state->schedule[i].labelLCustom, state->file_schedule[i].main_label, LABEL_L_LIMIT);
            strncpy(state->schedule[i].labelLClassic, state->file_schedule[i].main_label, LABEL_LS_LIMIT);
            strncpy(state->schedule[i].labelR, state->file_schedule[i].sub_label, LABEL_R_LIMIT);
            state->schedule[i].start.unit.hour = state->file_schedule[i].start_hour;
            state->schedule[i].start.unit.minute = state->file_schedule[i].start_minute;
            state->schedule[i].end.unit.hour = state->file_schedule[i].end_hour;
            state->schedule[i].end.unit.minute = state->file_schedule[i].end_minute;
        }
        else {  

            // If read fails, use default values as-is
            // 読み込み失敗時は、デフォルト値をそのまま使用する
            
            //printf("Schedule %d loaded failed.\n", i);
            copyFromMemToStructure(state, i);
        }
    
    }
}

// Entry point: allocate memory. Executed only once.
// エントリーポイント  メモリの割当 起動後、本当に１回しか実行されない。
void school_face_setup(uint8_t watch_face_index, void ** context_ptr) {

    // Explicitly mark as unused
    // 未使用であることを明示
    (void) watch_face_index; 

    //printf("school_face_setup  entry point \n");

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(school_state_t));
        memset(*context_ptr, 0, sizeof(school_state_t));

        //half_time_alarm_enabledにのみ、初期値をセットする。
        //この値はファイル保存されない
        school_state_t *state = (school_state_t *)(*context_ptr);
        state->half_time_alarm_enabled = false; 

        // File loading happens only here. Once is enough; the data remains in memory.
        //ファイル読み込みはココだけ。１回で良いはず。あとはメモリ中に常に残っている。
        loadScheduleFromMemoryAndFile(state);
    }


    // //debug code 
    // watch_date_time_t datetime = movement_get_local_date_time();
    // datetime.unit.year  = 2025 - WATCH_RTC_REFERENCE_YEAR;
    // datetime.unit.month = 7;
    // datetime.unit.day   = 24;
    // datetime.unit.hour  = 16;
    // datetime.unit.minute= 19;
    // datetime.unit.second= 0;
    // movement_set_local_date_time(datetime);

    // Data migration process: delete all possible old files
    // データのマイグレーション処理: 作成されている可能性のあるすべてのファイルを削除する
    for (int i = 0; i < SCHEDULE_LIMIT ; i++)
    {
        char filename[13];
        sprintf(filename, "scface%02d.u32", i);
        filesystem_rm(filename);
    }

}

// Copy values from file_schedule to memory schedule
// file_schedule から schedule に値をコピーする 
static void copyFromFScheduleToMemSchedule(school_state_t *state) {

    for (int i = 0; i < SCHEDULE_LIMIT ; i++){   
        state->schedule[i].is_active = state->file_schedule[i].is_active;
        strncpy(state->schedule[i].labelLCustom, state->file_schedule[i].main_label, LABEL_L_LIMIT);
        strncpy(state->schedule[i].labelLClassic, state->file_schedule[i].main_label, LABEL_LS_LIMIT);
        strncpy(state->schedule[i].labelR, state->file_schedule[i].sub_label, LABEL_R_LIMIT);
        state->schedule[i].start.unit.hour = state->file_schedule[i].start_hour;
        state->schedule[i].start.unit.minute = state->file_schedule[i].start_minute;
        state->schedule[i].end.unit.hour = state->file_schedule[i].end_hour;
        state->schedule[i].end.unit.minute = state->file_schedule[i].end_minute;
    }
}

void school_face_activate(void *context) {
    
    //printf("school_face_activate に来ました \n");

    school_state_t *state = (school_state_t *)context;

    if (watch_sleep_animation_is_running()) watch_stop_sleep_animation();

    state->label_pos = 0;
    state->schedule_index = 0;
    state->active = false;

    // Reset counter when outside of schedules
    // スケジュール外カウントをリセットする
    state->nothingCount = 0;                          

    state->displayMode = SCHOOLFACE_MODE_NORMAL;
    state->showNomalModeCountDown = 0;
}


// Initial screen
// スプラッシュ画面
static void _school_face_update_lcd(school_state_t *state) {
    (void)state;

    watch_clear_colon();

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "SCL", "SC");
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "_2"); //version

    watch_display_text(WATCH_POSITION_BOTTOM, "------"); 
}


static uint32_t convertTotalSec(watch_date_time_t date_time){

    uint32_t sec = 
                date_time.unit.hour   * 60 * 60 +
                date_time.unit.minute * 60 +
                date_time.unit.second ; 

    return sec;
}


static void displayTotalSecToTimeString(char* labelLCustom,char* labelLClassic,char* labelR, uint32_t sec,bool powerflag){

    uint32_t hour = sec / 3600;
    sec -= hour * 3600;

    uint32_t minute = sec / 60;
    sec -= minute * 60;

    char buf[20];

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT,labelLCustom ,labelLClassic);

    watch_display_text(WATCH_POSITION_TOP_RIGHT, labelR);

    //hour
    sprintf(buf, "%02u",hour);
    watch_display_text(WATCH_POSITION_HOURS, buf);

    //minutes
    sprintf(buf, "%02u",minute);
    watch_display_text(WATCH_POSITION_MINUTES, buf);


    if(powerflag == true){
        // Power saving mode
        // 省エネモード
        sprintf(buf, "%02u",sec);
        watch_display_text(WATCH_POSITION_SECONDS, buf);
    }
    else{
        watch_display_text(WATCH_POSITION_SECONDS, "  ");
    }

}

// Check which schedule the current time is within
// どのイベントの範囲内に居るのか調べる
int inSchedule(watch_date_time_t now, school_state_t* state){

    uint32_t nowsec;
    uint32_t startsec;
    uint32_t endsec;

    nowsec = convertTotalSec(now);
    
    for( int n=0 ; n < SCHEDULE_LIMIT ; n++){

        if (state->schedule[n].is_active == false) {
            // Skip if schedule is inactive
            // スケジュールが無効な場合はスキップ
            continue; 
        }
        
        startsec = convertTotalSec(state->schedule[n].start);
        endsec   = convertTotalSec(state->schedule[n].end);

        if( (startsec < nowsec) && (nowsec < endsec) ){
            //printf("in the schedule = %d\n",n);

            // Reset counter when outside of schedules
            // スケジュール外カウントをリセットする
            state->nothingCount = 0;  
            return n;
        }
    }
    
    return -1;
}


// Find the next schedule if current time is outside any schedule
// Return value: index of next schedule, -1 if none, -2 if all ended
// 
// 現在時刻がスケジュール内でない場合、次に発生するスケジュールはどれになるのかを調べる
// スケジュールがすでにすべて終了している場合は、最初に実施されるスケジュールのインデックスを返す。
// スケジュールは開始時間が早い順にならんでいるとは限らないので注意すること。
//  戻り値 int ・・・次に発生するスケジュールのインデックス
//  戻り値 -1 ・・・スケジュールが存在しない場合
//  戻り値 -2 ・・・スケジュールがすべて終了している場合
int searchNextSchedule(watch_date_time_t now, school_state_t* state){

    uint32_t nowsec = convertTotalSec(now);



    //すべてのスケジュールの開始時間と終了時間をチェックし、有効なスケジュールを探す。
    //無効なスケジュールの条件
    // ・ 開始時間が終了時間よりも大きな値である
    // ・ 開始時間と終了時間が同じである
    //有効なスケジュールの中から、一番最後になる終了時間を探し、そのインデックスを取得する。
    //現在時刻は考慮しない。
    //ここでいう最後のインデックスとは、添字の値が一番大きいインデックスのことではなく、
    //終了時間が一番大きいスケジュールのインデックスを指す。
    int8_t lastValidScheduleIndex = -1;

    //現在の一番大きい終了時間
    uint32_t lastEndSec = 0;
    
    for (int n = 0; n < SCHEDULE_LIMIT ; n++) {

        if (state->schedule[n].is_active == false) {
            continue; //スケジュールが無効な場合はスキップ
        }

        //開始時間と終了時間を取得

        uint32_t startsec = convertTotalSec(state->schedule[n].start);
        uint32_t endsec = convertTotalSec(state->schedule[n].end);

        // Skip invalid schedules
        // 無効なスケジュールをスキップ
        if (startsec >= endsec) {
            continue;
        }

        // Update index of the schedule that ends the latest among valid schedules
        // 有効なスケジュールの中で、最後に終了するスケジュールのインデックスを更新
        if (endsec > lastEndSec) {
            lastEndSec = endsec;
            lastValidScheduleIndex = n;
        }
    }

    // If no valid schedules exist, return -1
    // 有効なスケジュールが存在しない場合は、-1を返す
    if (lastValidScheduleIndex < 0) {
        //printf("No valid schedule found.\n");
        return -1;
    }

    // If all schedules have ended
    // スケジュールがすべて終了している場合
    if ( nowsec > convertTotalSec(state->schedule[ lastValidScheduleIndex ].end)) {
        //printf("All schedules have ended.\n");
        return -2;
    }

    // Find the next schedule
    // 次のスケジュールを探す
    int8_t firstValidScheduleIndex = -1;

    // Current smallest start time
    // 現在の一番小さい開始時間
    uint32_t minStartSec = UINT32_MAX; 

    // 以下のコードではこのような不具合がある
    // 現在時刻がスケジュールの開始時間よりも前にある場合、次のスケジュールを探す
    // 現在時刻がスケジュールの終了時間よりも後にある場合、次のスケジュールを探す
    // 現在時刻がスケジュールの開始時間と終了時間の間にある場合、スケジュールは存在しない

    for (int n = 0; n < SCHEDULE_LIMIT ; n++) {
        
        //スケジュールの開始時刻と終了時刻を表示する
        // printf("schedule %d start = %02d:%02d, end = %02d:%02d \n",
        //       n,
        //       state->schedule[n].start.unit.hour,
        //       state->schedule[n].start.unit.minute,
        //       state->schedule[n].end.unit.hour,
        //       state->schedule[n].end.unit.minute);
        

        if (state->schedule[n].is_active == false) {
            //printf("skip invalid schedule 2 %d \n",n);
            continue; //スケジュールが無効な場合はスキップ
        }

        uint32_t startsec = convertTotalSec(state->schedule[n].start);
        uint32_t endsec = convertTotalSec(state->schedule[n].end);

        // Skip invalid schedules
        // 無効なスケジュールをスキップ
        if (startsec >= endsec) {
            //printf("skip invalid schedule 2 %d \n",n);
            continue;
        }

        // If the current time is before the schedule's start time, look for the next schedule
        // 現在時刻がスケジュールの開始時間よりも前にある場合、次のスケジュールを探す
        if (nowsec < startsec) {
            //printf("next schedule index = %d \n",n);
            if (firstValidScheduleIndex < 0 || startsec < convertTotalSec(state->schedule[firstValidScheduleIndex].start)) {
                firstValidScheduleIndex = n; // 次のスケジュールのインデックスを返す
            }
        }
        else if (nowsec < endsec) {
            // This case should not occur because inSchedule() has already been called before this function
            //現在時刻がスケジュールの終了時間よりも後にある場合、次のスケジュールを探す

            //printf("skip schedule %d \n",n);
            continue;
        }
        else {
            // This case should not occur because inSchedule() has already been called before this function
            // 現在時刻がスケジュールの開始時間と終了時間の間にある場合、スケジュールは存在しない

            //printf("skip schedule %d \n",n);
            continue;
        }
        

        //printf("schedule %d is valid \n",n);
        //printf("startsec = %d, endsec = %d \n",startsec,endsec);
    }

    return firstValidScheduleIndex;   
}


// Normal mode processing branch
// 通常時の処理分岐
static bool school_face_do_normal_mode_loop(movement_event_t event,school_state_t *state) {

    //printf("NORMAL LOOP \n");

    watch_date_time_t date_time;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _school_face_update_lcd(state);

            break;
        case EVENT_MODE_BUTTON_UP:
            movement_move_to_next_face();
            break;
            

        case EVENT_LOW_ENERGY_UPDATE:
        case EVENT_TICK:

            date_time = movement_get_local_date_time();


            watch_clear_indicator(WATCH_INDICATOR_PM);

            // If half-time alarm is enabled, sound chime exactly at half time
            // 設定のハーフタイムアラームが有効な場合は、ベルのインジケーターを表示する

            if (state->half_time_alarm_enabled) {
                watch_set_indicator(WATCH_INDICATOR_BELL);
            } else {
                watch_clear_indicator(WATCH_INDICATOR_BELL);
            }

            

            int scheduleNum = inSchedule(date_time,state);

            if (scheduleNum >= 0){
                
                // 
                // 現在時刻がスケジュール内である場合の処理
                // 

                watch_set_colon();                
                
                uint32_t nowSec = convertTotalSec(date_time);
                uint32_t ends = convertTotalSec(state->schedule[scheduleNum].end);

                bool isLowEnergyUpdate = (event.event_type == EVENT_LOW_ENERGY_UPDATE);

                if((event.event_type == EVENT_LOW_ENERGY_UPDATE) && !watch_sleep_animation_is_running()) {
                    watch_start_sleep_animation(1000);
                }

                
               
                displayTotalSecToTimeString( state->schedule[scheduleNum].labelLCustom,
                                          state->schedule[scheduleNum].labelLClassic,
                                          state->schedule[scheduleNum].labelR,
                                          ends - nowSec,
                                          !isLowEnergyUpdate);

                // If half-time alarm is enabled, sound chime exactly at half time
                //ハーフタイムアラームが有効の場合は、時間がちょうど半分の場合にチャイムを鳴らす
                if (state->half_time_alarm_enabled &&
                    (ends - nowSec) == (convertTotalSec(state->schedule[scheduleNum].end) - convertTotalSec(state->schedule[scheduleNum].start)) / 2) {
                    //printf("half time alarm \n");
                    movement_play_alarm_beeps(1, BUZZER_NOTE_G7);
                }
                

            }
            else{
                
                // スケジュール外
                // 

                //printf("not in the schedule \n");

                int8_t nextScheduleIndex = searchNextSchedule(date_time, state);
                
                //printf("nextScheduleIndex = %d \n",nextScheduleIndex);

                if (nextScheduleIndex < 0) {

                    // If no schedules exist or all have ended, return to initial face
                    //スケジュールがすべて終わっているとか、スケジュールがそもそもゼロ個の場合

                    state->nothingCount++;


                    if ( state->nothingCount > 60 ){
                        //スケジュール内でなければ最初の画面に戻る
                        
                        //printf("nextScheduleIndex = %d \n",nextScheduleIndex);

                        movement_move_to_face(0);
                        return true;
                    }

                    //もしLEモード中に計測終了になったら表示がおかしくなるのでLEアニメーションを辞める
                    //if (watch_tick_animation_is_running()) watch_stop_tick_animation();
                    if (watch_sleep_animation_is_running()) watch_stop_sleep_animation();

                    _school_face_update_lcd(NULL);
                    return true;
                }
                else{
                    // If a schedule exists, display the next schedule
                    //スケジュールが存在する場合は、次のスケジュールを表示する

                    watch_set_colon();                
                    
                    uint32_t nowSec = convertTotalSec(date_time);
                    uint32_t ends = convertTotalSec(state->schedule[nextScheduleIndex].start);

                    //printf("%d\n",ends-nowSec);

                    bool isLowEnergyUpdate = (event.event_type == EVENT_LOW_ENERGY_UPDATE);

                    if((event.event_type == EVENT_LOW_ENERGY_UPDATE) && !watch_sleep_animation_is_running()) {
                        watch_start_sleep_animation(1000);
                    }

                    displayTotalSecToTimeString( "BRK",
                            "BK",
                            "  ",
                            ends - nowSec,
                            !isLowEnergyUpdate);

                }


            }
            
            break;
        case EVENT_LIGHT_BUTTON_UP:
            //printf("EVENT_LIGHT_BUTTON_UP \n"); 
            break;
        
        //Enable Light button
        //ライトボタンは有効化する        
        //case EVENT_LIGHT_BUTTON_DOWN:
            //  Including this process prevents the light from turning on
            // この処理をいれることでライトが光らなくなる                                                   
            // break;
        case EVENT_LIGHT_LONG_PRESS:
            // 長押しで設定モードに入る
            // 

            // printf("EVENT_LIGHT_LONG_PRESS \n"); 

            watch_set_colon();
            watch_clear_indicator(WATCH_INDICATOR_BELL);

            movement_request_tick_frequency(4);

            state->displayMode = SCHOOLFACE_MODE_SETTING;
            state->setting_mode = SCHOOLFACE_SETTING_MAIN_LABEL;

            break;

        case EVENT_ALARM_BUTTON_UP:
            // 時計モードへ移行

            watch_clear_indicator(WATCH_INDICATOR_BELL);

            date_time = movement_get_local_date_time();
            clock_display_clock(date_time);

            state->displayMode = SCHOOLFACE_MODE_CLOCK;
            state->showNomalModeCountDown = SCHOOLFACE_GLANCE_SEC;

            break;

        case EVENT_ALARM_BUTTON_DOWN:
            break;

        case EVENT_ALARM_LONG_PRESS:
            // printf("EVENT_ALARM_LONG_PRESS \n");        

            // Switch half-time alarm on/off
            // ハーフタイムアラームの有効/無効を切り替える
            state->half_time_alarm_enabled = !state->half_time_alarm_enabled;

            break;
        case EVENT_TIMEOUT:

            date_time = movement_get_local_date_time();


            // If not in schedule and no next schedule is found, return to face number 0
            // スケジュール内ではなく、かつ、次のスケジュールも見つからない場合はフェイス番号０の画面に戻る
            
            if ( (inSchedule(date_time,state) < 0) &&  (searchNextSchedule(date_time, state) < 0) ) { 
                // もしスケジュール内でなければ最初の画面に戻る
                movement_move_to_face(0);
            }

            break;

        default:
        
            //return movement_default_loop_handler(event, settings);
            return movement_default_loop_handler(event);
    
    }

    return true;
}


// Setting mode processing branch
// 時計モードの処理分岐
static bool school_face_do_clock_mode_loop(movement_event_t event,school_state_t *state) {

    //printf("CLOCK LOOP \n");

    watch_date_time_t date_time;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            // Show your initial UI here.

            _school_face_update_lcd(state);

            break;
        case EVENT_MODE_BUTTON_UP:
            movement_move_to_next_face();
            break;
            

        case EVENT_LOW_ENERGY_UPDATE:
        case EVENT_TICK:

            //printf("EVENT_TICK \n");

            // Display current time mode
            // 現在時刻表示モード
            
            date_time = movement_get_local_date_time();

            state->showNomalModeCountDown -= 1; //一秒ずつ減算
            
 
            clock_display_clock(date_time);

            if(state->showNomalModeCountDown == 0){
                state->displayMode = SCHOOLFACE_MODE_NORMAL;
            }

            break;

        case EVENT_LIGHT_BUTTON_UP:
            //printf("EVENT_LIGHT_BUTTON_UP \n"); 
            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            //printf("EVENT_LIGHT_BUTTON_DOWN \n"); //これでライトが止まる
            break;

        case EVENT_LIGHT_LONG_PRESS:
            //printf("EVENT_LIGHT_LONG_PRESS \n"); 
            break;

        case EVENT_ALARM_BUTTON_DOWN:

            // Extend the peek function time
            // チラ見機能の時間延長

            state->nothingCount = 0;

            // Reset the peek function processing (may not be called with UP)
            // チラ見機能をリセットする処理 UPが 呼ばれないこともあるので不要かも

            movement_request_tick_frequency(1);
            break;

        case EVENT_ALARM_BUTTON_UP:
            break;

        case EVENT_ALARM_LONG_PRESS:
            //printf("EVENT_ALARM_LONG_PRESS \n");        

            break;
        case EVENT_TIMEOUT:
            //printf("time out !\n");
            break;            
        default:
            return movement_default_loop_handler(event);
    
    }

    return true;
}


//設定モードの処理分岐
static bool school_face_do_setting_mode_loop(movement_event_t event,school_state_t *state) {

    bool is_custom_lcd = watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM;

    
    uint8_t main_length = is_custom_lcd ? 3 : 2 ;
    uint8_t sub_length = 2 ;


    switch (event.event_type) {

        case EVENT_LIGHT_LONG_PRESS:  
            
            schedule_data_check(&state->file_schedule[state->schedule_index]);
            persist_schedule_to_filesystem(&state->file_schedule[state->schedule_index], state->schedule_index);
            copyFromFScheduleToMemSchedule(state);

            state->schedule_index++;
            state->label_pos = 0;                

            state->setting_mode = SCHOOLFACE_SETTING_MAIN_LABEL;

            if (state->schedule_index >= SCHEDULE_LIMIT)
            {
                //If the limit of schedules is exceeded, return to the initial screen
                //スケジュールのリミットを超えた場合は、最初の画面に戻る

                movement_request_tick_frequency(1);
                state->displayMode = SCHOOLFACE_MODE_NORMAL;
                event.event_type = EVENT_ACTIVATE;

                state->schedule_index = 0;
                state->label_pos = 0;
                state->setting_mode = SCHOOLFACE_SETTING_MAIN_LABEL;

                // Hide the indicator
                // インジケーターを非表示にする

                watch_clear_indicator(WATCH_INDICATOR_SIGNAL);

                return school_face_do_normal_mode_loop(event, state);
            }

            break;

        case EVENT_MODE_BUTTON_UP:
            // MODE button during setting is forced termination of setting.
            // 設定中の MODEボタン は設定の強制終了。
            
            schedule_data_check(&state->file_schedule[state->schedule_index]);
            persist_schedule_to_filesystem(&state->file_schedule[state->schedule_index], state->schedule_index);
            copyFromFScheduleToMemSchedule(state);

            state->label_pos = 0;
            
            state->schedule_index = 0;
            movement_request_tick_frequency(1);
            state->displayMode = SCHOOLFACE_MODE_NORMAL;
            state->setting_mode = SCHOOLFACE_SETTING_MAIN_LABEL;
            event.event_type = EVENT_ACTIVATE;

            watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
            return false;

        case EVENT_LIGHT_BUTTON_UP:

            is_custom_lcd = watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM;

            // After registering the specified number of schedules, it ends.
            // 指定個数分スケジュールを登録したら終わり

            if (state->schedule_index > (SCHEDULE_LIMIT - 1)) {
            
                state->schedule_index = 0;

                movement_request_tick_frequency(1);

                state->displayMode = SCHOOLFACE_MODE_NORMAL;
                event.event_type = EVENT_ACTIVATE;

                //インジケーターを非表示にする
                watch_clear_indicator(WATCH_INDICATOR_SIGNAL);

                return school_face_do_normal_mode_loop(event, state);
            }
            else if(state->setting_mode == SCHOOLFACE_SETTING_MAIN_LABEL){
                //printf("Now Main label mode\n");

                state->label_pos++;
                
                if (state->label_pos >= main_length) {
                    //printf("Goto sub label mode\n");
                    state->label_pos = 0;
                    //次の設定モードに移行
                    state->setting_mode = SCHOOLFACE_SETTING_SUB_LABEL;
                }

            }
            else if(state->setting_mode == SCHOOLFACE_SETTING_SUB_LABEL){
                //printf("Now Sub label mode\n");

                state->label_pos++;

                if (state->label_pos  >= sub_length) {
                    //printf("Goto start hour mode\n");
                    state->setting_mode = SCHOOLFACE_SETTING_START_HOUR;
                }
            }
            else if (state->setting_mode == SCHOOLFACE_SETTING_START_HOUR){
                //printf("Goto start minute  mode\n");
                state->setting_mode = SCHOOLFACE_SETTING_START_MINUTE;
            }
            else  if (state->setting_mode == SCHOOLFACE_SETTING_START_MINUTE){
                //printf("Goto end hour mode\n");
                state->setting_mode = SCHOOLFACE_SETTING_END_HOUR;
            } 
            else if (state->setting_mode == SCHOOLFACE_SETTING_END_HOUR){
                //printf("Goto end minute mode\n");
                state->setting_mode = SCHOOLFACE_SETTING_END_MINUTE;
            }
            else  if (state->setting_mode == SCHOOLFACE_SETTING_END_MINUTE){
                //printf("Goto Main label mode\n");
                
                //データ保存
                schedule_data_check(&state->file_schedule[state->schedule_index]);
                persist_schedule_to_filesystem(&state->file_schedule[state->schedule_index], state->schedule_index);
                copyFromFScheduleToMemSchedule(state);

                state->schedule_index++;
                state->label_pos = 0;                

                state->setting_mode = SCHOOLFACE_SETTING_MAIN_LABEL;

                if (state->schedule_index >= SCHEDULE_LIMIT)
                {
                    //スケジュールのリミットを超えた場合は、最初の画面に戻る
                    movement_request_tick_frequency(1);
                    state->schedule_index = 0;
                    state->label_pos = 0;
                    state->displayMode = SCHOOLFACE_MODE_NORMAL;
                    event.event_type = EVENT_ACTIVATE;

                    //インジケーターを非表示にする
                    watch_clear_indicator(WATCH_INDICATOR_SIGNAL);

                    return school_face_do_normal_mode_loop(event, state);
                }
            } 

            break;
        case EVENT_ALARM_LONG_PRESS:

            //printf("EVENT_ALARM_LONG_PRESS \n");
            
            //スケジュールの反転
            state->file_schedule[state->schedule_index].is_active = !state->file_schedule[state->schedule_index].is_active;
            break;

        case EVENT_ALARM_BUTTON_UP:
            //printf("EVENT_ALARM_BUTTON_UP \n");

            //文字変更とカウントアップ処理

            if(state->setting_mode == SCHOOLFACE_SETTING_MAIN_LABEL){

                advance_character_at_position(&state->file_schedule[state->schedule_index].main_label[state->label_pos], state->label_pos);
            }
            else if(state->setting_mode == SCHOOLFACE_SETTING_SUB_LABEL){

                advance_character_at_position(&state->file_schedule[state->schedule_index].sub_label[state->label_pos],
                     state->label_pos + main_length);

            }
            else if (state->setting_mode == SCHOOLFACE_SETTING_START_HOUR){
            
                state->file_schedule[state->schedule_index].start_hour++;
                if (state->file_schedule[state->schedule_index].start_hour > 23) {
                    state->file_schedule[state->schedule_index].start_hour = 0;
                }
                
            }
            else  if (state->setting_mode == SCHOOLFACE_SETTING_START_MINUTE){
            
                state->file_schedule[state->schedule_index].start_minute++;

                if (state->file_schedule[state->schedule_index].start_minute > 59) {
                    state->file_schedule[state->schedule_index].start_minute = 0;
                }   
               
            } 
            else if (state->setting_mode == SCHOOLFACE_SETTING_END_HOUR){
            
                state->file_schedule[state->schedule_index].end_hour++;
                
                if (state->file_schedule[state->schedule_index].end_hour > 23) {
                    state->file_schedule[state->schedule_index].end_hour = 0;
                }   

            }
            else  if (state->setting_mode == SCHOOLFACE_SETTING_END_MINUTE){
            
                state->file_schedule[state->schedule_index].end_minute++;

                if (state->file_schedule[state->schedule_index].end_minute > 59) {
                    state->file_schedule[state->schedule_index].end_minute = 0;
                }   
            } 

            break;

        case EVENT_TIMEOUT:        
            movement_move_to_face(0);
            break;

        default:
            break;
    }

    char buf[13];

    
    //watch_set_colon();

    //スケジュールの有効無効を示すベルフラグ
    if (state->file_schedule[state->schedule_index].is_active) {
        watch_set_indicator(WATCH_INDICATOR_SIGNAL);
    }
    else{
        watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
    }


    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM)
    {
        // Custom LCD

        if (state->setting_mode ==  SCHOOLFACE_SETTING_END_HOUR ||
            state->setting_mode == SCHOOLFACE_SETTING_END_MINUTE) {

            // Second page: end time input
            // 2ページ目 終了時間入力

            sprintf(buf, "%c%c%c%c%02d%02d%2d%c",
                'E',
                'N',
                ' ',
                ' ',
                state->file_schedule[state->schedule_index].end_hour,
                state->file_schedule[state->schedule_index].end_minute,
                state->schedule_index + 1,
                'D'  //  for Custom LCD
            );
        }
        else{
            // First page
            // 1ページ目
            sprintf(buf, "%c%c%c%c%02d%02d%2d%c",
                state->file_schedule[state->schedule_index].main_label[0],
                state->file_schedule[state->schedule_index].main_label[1],
                state->file_schedule[state->schedule_index].sub_label[0],
                state->file_schedule[state->schedule_index].sub_label[1],
                state->file_schedule[state->schedule_index].start_hour,
                state->file_schedule[state->schedule_index].start_minute,
                state->schedule_index + 1,
                state->file_schedule[state->schedule_index].main_label[2]  // for Custom LCD
            );
        }

    }
    else{
        // Classic LCD

        if (state->setting_mode ==  SCHOOLFACE_SETTING_END_HOUR ||
            state->setting_mode == SCHOOLFACE_SETTING_END_MINUTE) {

            // Second page: end time input
            // 2ページ目 終了時間入力
            sprintf(buf, "%c%c%c%c%02d%02d%2d",
                'E',
                'N',
                ' ',
                ' ',
                state->file_schedule[state->schedule_index].end_hour,
                state->file_schedule[state->schedule_index].end_minute,
                state->schedule_index + 1
            );
        }
        else{
            // First page
            // 1ページ目
            sprintf(buf, "%c%c%c%c%02d%02d%2d",
                state->file_schedule[state->schedule_index].main_label[0],
                state->file_schedule[state->schedule_index].main_label[1],
                state->file_schedule[state->schedule_index].sub_label[0],
                state->file_schedule[state->schedule_index].sub_label[1],
                state->file_schedule[state->schedule_index].start_hour,
                state->file_schedule[state->schedule_index].start_minute,
                state->schedule_index + 1
            );
        }

    }
    

    watch_clear_indicator(WATCH_INDICATOR_PM);

    // Blinking process
    // Blinking effect: replace current input position with space
    //
    // チカチカ処理
    // 点滅させる処理 出来上がったBufの入力中の箇所を空白に変える
    if (event.subsecond % 2) {

        if(state->setting_mode == SCHOOLFACE_SETTING_MAIN_LABEL){

            // When using custom LCD, the 3rd character is at a separate position, so handle specially
            // CultomLCDのときは、3文字目は離れた場所にあるので特別なコードを書いて対応する
            if ( is_custom_lcd && (state->label_pos == 2) ) { 
                
                if (buf[10] == ' ') {
                    buf[10 ] = '_';
                }
                else{
                   buf[10 ] = ' ';
                }

            }
            else{

                if (buf[state->label_pos] == ' ') {
                    buf[state->label_pos ] = '_';
                }
                else{
                   buf[state->label_pos ] = ' ';
                }
                
            }
        }
        else if(state->setting_mode == SCHOOLFACE_SETTING_SUB_LABEL){
            
            if (buf[2 + state->label_pos] == ' ') {
                buf[2 + state->label_pos ] = '_';
            }
            else{
                buf[2 + state->label_pos ] = ' ';
            }
        }
        else if (state->setting_mode == SCHOOLFACE_SETTING_START_HOUR){
            buf[ 2 + sub_length ] = ' ';
            buf[ 2 + sub_length + 1 ] = ' ';
        }
        else  if (state->setting_mode == SCHOOLFACE_SETTING_START_MINUTE){
            buf[ 2 + sub_length + 2 ] = ' ';
            buf[ 2 + sub_length + 3 ] = ' ';
        } 
        else if (state->setting_mode == SCHOOLFACE_SETTING_END_HOUR){
            buf[ 2 + sub_length ] = ' ';
            buf[ 2 + sub_length + 1 ] = ' ';
        }
        else  if (state->setting_mode == SCHOOLFACE_SETTING_END_MINUTE){
            buf[ 2 + sub_length + 2 ] = ' ';
            buf[ 2 + sub_length + 3 ] = ' ';
        } 

    }

    watch_display_text(WATCH_POSITION_FULL, buf);

    return true;
}

// Distribute processing to each mode inside the loop
// ループ内で各モードに処理を振り分ける
bool school_face_loop(movement_event_t event, void *context) {

    school_state_t *state = (school_state_t *)context;

    if (state->displayMode == SCHOOLFACE_MODE_NORMAL) {
        return school_face_do_normal_mode_loop(event, state);
    } 
    else if (state->displayMode == SCHOOLFACE_MODE_CLOCK) {
        return school_face_do_clock_mode_loop(event, state);
    } 
    else if (state->displayMode == SCHOOLFACE_MODE_SETTING) {
        return school_face_do_setting_mode_loop(event, state);
    }
    
    printf("ERROR!!!!!!  -> Unknown display mode\n");
    return school_face_do_normal_mode_loop(event, state);
}

// Processing at exit 
// 終了時の処理 
void school_face_resign(void *context) {
    (void) context;
}

