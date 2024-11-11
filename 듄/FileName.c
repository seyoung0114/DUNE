#include <stdio.h>
#include <Windows.h>
#include <conio.h>
#include <string.h>
#include <time.h>

#define N_LAYER 2
#define MAP_WIDTH 60
#define MAP_HEIGHT 18
#define MSG_LOG_SIZE 5

// 화면 배치 좌표
#define STATUS_X 62
#define STATUS_Y 0
#define MSG_X 0
#define MSG_Y 20
#define CMD_X 62
#define CMD_Y 20

// 색상 정의
#define COLOR_EMPTY 7
#define COLOR_PLATE 0
#define COLOR_ROCK 8
#define COLOR_SPICE 6
#define COLOR_BASE 9
#define COLOR_HARVESTER 10
#define COLOR_SANDWORM 14

// 오브젝트 정의
#define EMPTY ' '
#define PLATE 'P'
#define ROCK 'R'
#define SPICE 'S'
#define BASE 'B'
#define AIBASE 'A'
#define HARVESTER 'H'
#define SANDWORM 'W'

#define TICK 10

// 입력키 정의
typedef enum {
    k_none = 0, k_up, k_right, k_left, k_down, K_quit, k_undef
} KEY;
// 이동방향
typedef enum {
    d_stay = 0, d_up, d_right, d_left, d_down, d_quit
} DIRECTION;
typedef enum ColorType {
    BLACK,  	//0
    darkBLUE,	//1
    DarkGreen,	//2
    darkSkyBlue,    //3
    DarkRed,  	//4
    DarkPurple,	//5
    Orange,	//6
    GRAY,		//7
    DarkGray,	//8
    BLUE,		//9
    GREEN,		//10
    SkyBlue,	//11
    RED,		//12
    PURPLE,		//13
    YELLOW,		//14
    WHITE		//15
} BCOLOR;

typedef struct { int row, col; } POSITION;

// 데이터 구조
typedef struct {
    int spice;          // 현재 보유한 스파이스
    int spice_max;      // 스파이스 최대 저장량
    int population;     // 현재 인구수
    int population_max; // 수용 가능한 인구 수
};

typedef struct {
    POSITION pos;
    POSITION dest;
    char repr;
    int move_period;
    int next_move_time;
} OBJECT;

typedef struct {
    int Tcolor;
    int Bcolor;
    char cdata;
} displayvalue;



// 게임 상태 변수
char map[N_LAYER][MAP_HEIGHT][MAP_WIDTH];
displayvalue displayfront[MAP_HEIGHT][MAP_WIDTH];
displayvalue displaybuffer[MAP_HEIGHT][MAP_WIDTH];
char message_log[MSG_LOG_SIZE][MAP_WIDTH + 1] = { "" };
POSITION cursor = { 0, 0 };

// 콘솔 제어 함수
void set_cursor_position(int x, int y) {
    COORD coord;
    coord.X = (SHORT)x;
    coord.Y = (SHORT)y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// 텍스트 칼라 변경
void set_text_color(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

// 텍스트 칼라, 배경색 변경
void color_set(int bcolor, int tcolor) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (bcolor << 4) + tcolor);
}



KEY get_key() {
    if (!_kbhit()) {
        return k_none;
    }
    int byte = _getch();
    switch (byte) {
    case 'q': return K_quit;
    case 224:
        byte = _getch();
        switch (byte) {
        case 72: return k_up;
        case 75: return k_left;
        case 77: return k_right;
        case 80: return k_down;
        default: return k_undef;
        }
    default: return k_undef;
    }
}

// 방향 입력 처리
DIRECTION get_direction_input() {
    KEY key = get_key();
    switch (key) {
    case K_quit:return d_quit;
    case k_up: return d_up;
    case k_left: return d_left;
    case k_right: return d_right;
    case k_down: return d_down;
    case k_undef: return d_stay;
    default:return d_stay;
    }
}

// 커서 이동 처리
void move_cursor(DIRECTION direction) {
    int new_row = cursor.row;
    int new_col = cursor.col;

    switch (direction) {
    case d_up: if (cursor.row > 0) new_row--; break;
    case d_down: if (cursor.row < MAP_HEIGHT - 1) new_row++; break;
    case d_left: if (cursor.col > 0) new_col--; break;
    case d_right: if (cursor.col < MAP_WIDTH - 1) new_col++; break;
    default: break;
    }
    cursor.row = new_row;
    cursor.col = new_col;
}

// 상태창, 명령창, 시스템 메시지 출력 함수
void display_status(const char* status) {
    set_cursor_position(STATUS_X, STATUS_Y);
    printf("상태창");
    set_cursor_position(STATUS_X, STATUS_Y + 1);
    printf("------------");
    set_cursor_position(STATUS_X, STATUS_Y + 2);
    printf("%s", status);

}

void add_system_message(const char* message) {
    for (int i = 0; i < MSG_LOG_SIZE - 1; i++) {
        strcpy_s(message_log[i], sizeof(message_log[i]), message_log[i + 1]);
    }
    strncpy_s(message_log[MSG_LOG_SIZE - 1], sizeof(message_log[MSG_LOG_SIZE - 1]), message, MAP_WIDTH);
}

void display_system_messages() {
    set_cursor_position(MSG_X, MSG_Y);
    printf("시스템 메시지");
    set_cursor_position(MSG_X, MSG_Y + 1);
    printf("------------");
    for (int i = 0; i < MSG_LOG_SIZE; i++) {
        set_cursor_position(MSG_X, MSG_Y + i + 2);
        printf("%s", message_log[i]);
    }
}

void display_command(const char* command) {
    set_cursor_position(CMD_X, CMD_Y);
    printf("명령창");
    set_cursor_position(CMD_X, CMD_Y + 1);
    printf("------------");
    set_cursor_position(CMD_X, CMD_Y + 2);
    printf("%s", command);
}

// 맵 초기화
void initialize_map() {
    for (int L = 0; L < N_LAYER; L++) {
        for (int i = 0; i < MAP_HEIGHT; i++) {
            for (int j = 0; j < MAP_WIDTH; j++) {
                map[L][i][j] = EMPTY;

            }
        }
    }

    // Layer1 등록
    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            // 아트레이스 본진
            if (((i >= MAP_HEIGHT - 3) && (i < MAP_HEIGHT - 1)) && ((j > 0) && (j <= 2))) {
                map[0][i][j] = BASE;

            }

            //하코넨 본진
            if (((i > 0) && (i <= 2)) && ((j >= MAP_WIDTH - 3) && (j < MAP_WIDTH - 1))) {
                map[0][i][j] = AIBASE;

            }

            // 아트레이스 장판
            if (((i >= MAP_HEIGHT - 3) && (i < MAP_HEIGHT - 1)) && ((j > 2) && (j <= 4))) {
                map[0][i][j] = PLATE;

            }

            //하코넨 장판
            if (((i > 0) && (i <= 2)) && ((j >= MAP_WIDTH - 5) && (j < MAP_WIDTH - 3))) {
                map[0][i][j] = PLATE;

            }


            // 스파이스 매장
            if (((i == MAP_HEIGHT - 6) && (j == 1)) || ((i == 5) && j == (MAP_WIDTH - 2))) {
                map[0][i][j] = SPICE;
            }
        }
    }


    // 바위 매장
    map[0][7][MAP_WIDTH - 35] = ROCK;
    map[0][7][MAP_WIDTH - 36] = ROCK;
    map[0][8][MAP_WIDTH - 35] = ROCK;
    map[0][8][MAP_WIDTH - 36] = ROCK;

    map[0][MAP_HEIGHT - 7][35] = ROCK;
    map[0][MAP_HEIGHT - 7][36] = ROCK;
    map[0][MAP_HEIGHT - 8][35] = ROCK;
    map[0][MAP_HEIGHT - 8][36] = ROCK;

    map[0][MAP_HEIGHT - 15][20] = ROCK;
    map[0][15][MAP_WIDTH - 20] = ROCK;

    // Layer2 등록
    // 하베스트 등록
    map[1][MAP_HEIGHT - 4][1] = HARVESTER;
    map[1][3][MAP_WIDTH - 2] = HARVESTER;

    map[1][MAP_HEIGHT - 15][MAP_WIDTH - 35] = SANDWORM;
    map[1][MAP_HEIGHT - 5][MAP_WIDTH - 25] = SANDWORM;

}

// 게임 화면 설정
void init_display() {
    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            set_cursor_position(j, i);
            if ((map[0][i][j] == EMPTY) && (map[1][i][j] == EMPTY))
            {
                displaybuffer[i][j].Bcolor = BLACK;
                displaybuffer[i][j].Tcolor = WHITE;
                displaybuffer[i][j].cdata = map[0][i][j];
            }
            else
            {
                if (map[0][i][j] == BASE)
                {
                    displaybuffer[i][j].Bcolor = BLUE;
                    displaybuffer[i][j].Tcolor = WHITE;
                    displaybuffer[i][j].cdata = map[0][i][j];
                }
                else if (map[0][i][j] == AIBASE)
                {
                    displaybuffer[i][j].Bcolor = RED;
                    displaybuffer[i][j].Tcolor = WHITE;
                    displaybuffer[i][j].cdata = map[0][i][j];
                }
                else if (map[0][i][j] == PLATE)
                {
                    displaybuffer[i][j].Bcolor = BLACK;
                    displaybuffer[i][j].Tcolor = WHITE;
                    displaybuffer[i][j].cdata = map[0][i][j];
                }
                else if (map[0][i][j] == SPICE) {
                    displaybuffer[i][j].Bcolor = Orange;
                    displaybuffer[i][j].Tcolor = BLACK;
                    displaybuffer[i][j].cdata = map[0][i][j];
                }
                else if (map[0][i][j] == ROCK) {
                    displaybuffer[i][j].Bcolor = DarkGray;
                    displaybuffer[i][j].Tcolor = WHITE;
                    displaybuffer[i][j].cdata = map[0][i][j];
                }
                else if (map[0][i][j] == HARVESTER) {
                    displaybuffer[i][j].Bcolor = BLUE;
                    displaybuffer[i][j].Tcolor = WHITE;
                    displaybuffer[i][j].cdata = map[0][i][j];

                }
                else if (map[0][i][j] == SANDWORM) {
                    displaybuffer[i][j].Bcolor = YELLOW;
                    displaybuffer[i][j].Tcolor = BLACK;
                    displaybuffer[i][j].cdata = map[0][i][j];

                }
                else if (map[1][i][j] == BASE)
                {
                    displaybuffer[i][j].Bcolor = BLUE;
                    displaybuffer[i][j].Tcolor = WHITE;
                    displaybuffer[i][j].cdata = map[1][i][j];
                }
                else if (map[1][i][j] == AIBASE)
                {
                    displaybuffer[i][j].Bcolor = RED;
                    displaybuffer[i][j].Tcolor = WHITE;
                    displaybuffer[i][j].cdata = map[1][i][j];
                }
                else if (map[1][i][j] == PLATE)
                {
                    displaybuffer[i][j].Bcolor = BLACK;
                    displaybuffer[i][j].Tcolor = WHITE;
                    displaybuffer[i][j].cdata = map[1][i][j];
                }
                else if (map[1][i][j] == SPICE) {
                    displaybuffer[i][j].Bcolor = Orange;
                    displaybuffer[i][j].Tcolor = BLACK;
                    displaybuffer[i][j].cdata = map[1][i][j];
                }
                else if (map[1][i][j] == ROCK) {
                    displaybuffer[i][j].Bcolor = DarkGray;
                    displaybuffer[i][j].Tcolor = WHITE;
                    displaybuffer[i][j].cdata = map[1][i][j];
                }
                else if (map[1][i][j] == HARVESTER) {
                    displaybuffer[i][j].Bcolor = BLUE;
                    displaybuffer[i][j].Tcolor = WHITE;
                    displaybuffer[i][j].cdata = map[1][i][j];

                }
                else if (map[1][i][j] == SANDWORM) {
                    displaybuffer[i][j].Bcolor = YELLOW;
                    displaybuffer[i][j].Tcolor = BLACK;
                    displaybuffer[i][j].cdata = map[1][i][j];

                }

            }

            // 화면 창 테두리에 '#' 표시를 해준다
            if ((i == 0) || (i == MAP_HEIGHT - 1) || (j == 0) || (j == MAP_WIDTH - 1)) {
                displaybuffer[i][j].Tcolor = RED;
                displaybuffer[i][j].Bcolor = BLACK;
                displaybuffer[i][j].cdata = '#';
            }

        }
    }

}

void display_print(int xPos, int yPos, displayvalue dValue)
{
    set_cursor_position(xPos, yPos);
    color_set(dValue.Bcolor, dValue.Tcolor);
    putchar(dValue.cdata);
}
void buffer_print() {
    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            if (displayfront[i][j].cdata != displaybuffer[i][j].cdata)
            {
                display_print(j, i, displaybuffer[i][j]);
                displayfront[i][j].cdata = displaybuffer[i][j].cdata;
                displayfront[i][j].Tcolor = displaybuffer[i][j].Tcolor;
                displayfront[i][j].Bcolor = displaybuffer[i][j].Bcolor;
            }
        }
    }

}




void cursor_message() {
    char mapdata1, mapdata2;
    mapdata1 = map[0][cursor.row][cursor.col];
    mapdata2 = map[1][cursor.row][cursor.col];
    if (mapdata1 == BASE)
    {
        add_system_message("아트레이더스 본진 발견!");
    }
    else if (mapdata1 == AIBASE)
    {
        add_system_message("하코넨 본진 발견!");
    }
    else if (mapdata1 == PLATE)
    {
        add_system_message("플레이트 발견!");
    }
    else if (mapdata1 == SPICE) {
        add_system_message("스파이스 매장지 발견!");
    }
    else if (mapdata1 == ROCK) {
        add_system_message("바위 매장지 발견!");
    }
    else if (mapdata1 == HARVESTER) {
        add_system_message("하베스터 유닛 발견!");
    }
    else if (mapdata1 == SANDWORM) {
        add_system_message("샌드윔 발견!");
    }
    else if (mapdata2 == BASE)
    {
        add_system_message("아트레이더스 본진 발견!");
    }
    else if (mapdata2 == AIBASE)
    {
        add_system_message("하코넨 본진 발견!");
    }
    else if (mapdata2 == PLATE)
    {
        add_system_message("플레이트 발견!");
    }
    else if (mapdata2 == SPICE) {
        add_system_message("스파이스 매장지 발견!");
    }
    else if (mapdata2 == ROCK) {
        add_system_message("바위 매장지 발견!");
    }
    else if (mapdata2 == HARVESTER) {
        add_system_message("하베스터 유닛 발견!");
    }
    else if (mapdata2 == SANDWORM) {
        add_system_message("샌드윔 발견!");
    }

}






// 게임 루프
void game_loop() {
    char command[50] = " ";
    char status[50] = " ";
    initialize_map();                   // 맵 데이타 초기화
    add_system_message("게임이 시작되었습니다.");
    init_display();                     // 화면 상태 초기화
    buffer_print();                     // 게임 화면 출력
    set_text_color(RED);                // 게임 메시지 화면 텍스트를 빨강색으로
    display_status(status);             // 상태창 메시지 출력
    display_system_messages();          // 시스템 메시지 출력
    display_command(command);           // 명령창 메시지 출력


    set_cursor_position(0, 0);
    while (1) {
        DIRECTION dir = get_direction_input();
        if (dir == d_quit) {
            break;
        }
        else if (dir != d_stay)
        {
            move_cursor(dir);
            buffer_print();                 // 게임 화면 출력 변경된 부분만 출력
            set_text_color(RED);            // 게임 메시지 화면 텍스트를 빨강색으로
            display_status(status);         // 상태창 메시지 출력
            display_system_messages();      // 시스템 메시지 출력
            display_command(command);       // 명령창 메시지 출력

            set_cursor_position(cursor.col, cursor.row);
            // 임의 시스템 메시지
            cursor_message(); //커서 이동 시스템 메시지


        }


        Sleep(TICK);
    }
}

int main() {
    game_loop();
    return 0;
}
