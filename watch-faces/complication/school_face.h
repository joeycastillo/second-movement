
#ifndef SCHOOL_FACE_H_
#define SCHOOL_FACE_H_

#include "movement.h"

// Number of schedules
// スケジュールの数
#define SCHEDULE_LIMIT 12

// Character length limits for each class name
// 各授業名の文字数制限
#define LABEL_L_LIMIT 3
#define LABEL_LS_LIMIT 2
#define LABEL_R_LIMIT 2

// Modes
// モード
#define SCHOOLFACE_MODE_NORMAL 0
#define SCHOOLFACE_MODE_CLOCK 1
#define SCHOOLFACE_MODE_SETTING 2

// Setting modes
// 設定モード
#define SCHOOLFACE_SETTING_MAIN_LABEL 1
#define SCHOOLFACE_SETTING_SUB_LABEL 2
#define SCHOOLFACE_SETTING_START_HOUR 3
#define SCHOOLFACE_SETTING_START_MINUTE 4
#define SCHOOLFACE_SETTING_END_HOUR 5
#define SCHOOLFACE_SETTING_END_MINUTE 6

// FILENAME FORMAT 8.3 Style
// HEADER + 00 ～ 999 . EXT
#define SCHOOLFACE_FILENAME_HEADER    "scface"
#define SCHOOLFACE_FILENAME_EXTENTION 1  // 1 -> ".001"

// Seconds for a quick glance at the time
// ちらっと時間を見る秒数
#define SCHOOLFACE_GLANCE_SEC 3


typedef struct{
    // Name of the registered schedule  
    // 登録スケジュールの名前
    char  labelLCustom[LABEL_L_LIMIT + 1];  
    
    // Name of the registered schedule (classic) 
    // 登録スケジュールの名
    char  labelLClassic[LABEL_LS_LIMIT + 1]; 

    // Name of the registered schedule (right)
    // 登録スケジュールの名前
    char  labelR[LABEL_R_LIMIT + 1]; 

    watch_date_time_t start;
    watch_date_time_t end;    
    
    // true: enabled, false: disabled
    bool is_active; 

} myschedule;

// Data to be saved to file
// ファイルに保存すべきデータ
typedef struct {
    char main_label[3];
    char sub_label[2];
    uint8_t start_hour;
    uint8_t start_minute;
    uint8_t end_hour;
    uint8_t end_minute;
    
    // Flag indicating whether this schedule is active
    // このスケジュールが有効かどうかのフラグ
    bool is_active; 
}schedule_for_filesave;



typedef struct {

    bool active;

    // Registered schedules
    // 登録スケジュール
    myschedule schedule[SCHEDULE_LIMIT];    
    
    // For measuring time when countdown is not active
    // カウントダウンが行われていないときの時間計測用
    uint8_t nothingCount;                       
    
    // Seconds to display in normal mode
    // 通常モードで表示する秒数
    uint8_t showNomalModeCountDown;         
    
    // 0: normal, 1: clock, 2: setting
    // 0通常 1時計  2設定
    uint8_t displayMode; 

    struct {
        watch_date_time_t previous;
    } date_time;
    uint8_t last_battery_check;
    uint8_t watch_face_index;
    bool time_signal_enabled;
    bool battery_low;

    schedule_for_filesave file_schedule[SCHEDULE_LIMIT];

    // Current setting mode
    // 現在の設定モード
    uint8_t setting_mode; 

    // Index of the current schedule
    // 現在のスケジュールのインデックス
    uint8_t schedule_index; 

    // Position of the character being entered
    // 何文字目の入力化をカウントする
    uint8_t label_pos; 

    // Whether to sound an alarm when half the time has passed
    // 半分時間が経過したときにアラームを鳴らすかどうか
    bool half_time_alarm_enabled; 

    // File read success flag
    // ファイル読み込み成功フラグ
    bool isReadSuccess; 

} school_state_t;



void school_face_setup(uint8_t watch_face_index, void ** context_ptr);
void school_face_activate(void *context);
bool school_face_loop(movement_event_t event, void *context);
void school_face_resign(void *context);


#define school_face ((const watch_face_t){ \
    school_face_setup, \
    school_face_activate, \
    school_face_loop, \
    school_face_resign, \
    NULL, \
})

#endif // SCHOOL_FACE_H_

