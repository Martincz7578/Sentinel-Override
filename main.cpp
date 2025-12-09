#include <ncurses.h>
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <thread>
#include <string>
#include <cstdint>

using namespace std;

class bullet{
public:
    int16_t * x = nullptr, * y = nullptr;
    uint8_t speed;
    bool hit = false;
    bullet(int nx, int ny, uint8_t s) : speed(s) {
        this->x = (int16_t *)malloc((size_t)nx);
        this->y = (int16_t *)malloc((size_t)ny);
        *this->x = (int16_t)nx;
        *this->y = (int16_t)ny;
    }

    ~bullet(){

    }

    void update(){
        if(*y >= 0) (*y) -= speed;
    }
    void reallocate(){
        x = (int16_t *)realloc(x, 1);
        y = (int16_t *)realloc(y, 1);
        if(x == nullptr || y == nullptr){
            cerr << "Memory reallocation failed for bullet coordinates." << endl;
            exit(1);
        }
    }
};

class asteroid{
public:
    int16_t * x, * y;
    uint8_t size, speed;
    // 0 = active, 1 = destroyed 2 = inactive
    int8_t status = 0;
    int8_t t = 0;
    asteroid(int nx, int ny, int8_t s, int8_t sp) : size((uint8_t)s), speed((uint8_t)sp) {
        this->x = (int16_t *)malloc((size_t)nx);
        this->y = (int16_t *)malloc((size_t)ny);
        *this->x = (int16_t)nx;
        *this->y = (int16_t)ny;
    }

    ~asteroid(){
        
    }

    void check(vector<bullet>& bul){
        for(auto& b : bul){
            if(b.x == nullptr || b.y == nullptr) continue;
            if(*b.x == *this->x && *b.y == *this->y){
                status = 1;
                b.hit = true;
                *b.y = -1;
            } 
        }
    }

    void reallocate(){
        x = (int16_t *)realloc(x, 1);
        y = (int16_t *)realloc(y, 1);
        if(x == nullptr || y == nullptr){
            cerr << "Memory reallocation failed for asteroid coordinates." << endl;
            exit(1);
        }
    }

    void update(const vector<asteroid>& astr, uint64_t frame){
        if(status) return;
        if(static_cast<int>(frame) % speed == 0 && !status) (*y) += 1;
        if(*y >= LINES) {
            *y = 0;
            *x = (int16_t)(rand() % COLS);
        }

        mvprintw(LINES - 2, COLS - 15, "Score: %.0lu", frame);
        mvprintw(LINES - 1, COLS - 15, "Asteroids: %lu", astr.size());
    }
};

class player {
public:
    uint16_t * x, * y;
    char icon = 'V';
    player(int nx, int ny) {
        this->x = (uint16_t *)calloc((size_t)nx, 1);
        this->y = (uint16_t *)calloc((size_t)ny, 1);
        *this->x = (uint16_t)nx;
        *this->y = (uint16_t)ny;
    }

    ~player(){

    }

    void move(int dx, int dy) {
        *x += (uint16_t)dx;
        *y += (uint16_t)dy;
    }

    void reallocate(){
        x = (uint16_t *)realloc(x, 1);
        y = (uint16_t *)realloc(y, 1);
        if(x == nullptr || y == nullptr){
            cerr << "Memory reallocation failed for player coordinates." << endl;
            exit(1);
        }
    }

    void draw() const {
        mvprintw(*y, *x, icon == 'V' ? "V" : " ");
    }
};

int main(){
    try{
        initscr();
        noecho();
        curs_set(0);
        nodelay(stdscr, TRUE);
        start_color();
        init_pair(1, COLOR_RED, COLOR_BLACK);
    }catch (const exception& e) {
        cerr << "Error initializing ncurses: " << e.what() << endl;
        return 1;
    }

    int * pmx = (int *)malloc(sizeof(int)), * pmy = (int *)malloc(sizeof(int));
    if(pmx == nullptr || pmy == nullptr){
        cerr << "Memory allocation failed for screen dimensions." << endl;
        endwin();
        return 1;
    }
    getmaxyx(stdscr, *pmy, *pmx);

    const int bmx = (int)ceil(log2((*pmx)));
    const int bmy = (int)ceil(log2((*pmy)));

    int * mx = (int *)malloc((size_t)bmx);
    int * my = (int *)malloc((size_t)bmy);
    if(mx == nullptr || my == nullptr){
        cerr << "Memory allocation failed for bit dimensions." << endl;
        free(pmx);
        free(pmy);
        endwin();
        return 1;
    }

    (*mx) = *pmx;
    (*my) = *pmy;

    free(pmx);
    free(pmy);

    player p(*mx / 2, *my / 2);

    vector<asteroid> asteroids;
    vector<bullet> bullets;

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis_x(0, *mx - 1);
    uniform_int_distribution<> dis_y(0, *my - 3);
    uniform_int_distribution<> dis_ss(1, 3);

    uint64_t * frame = (uint64_t *)malloc(8);
    if(frame == nullptr){
        cerr << "Memory allocation failed for frame." << endl;
        endwin();
        return 1;
    }
    *frame = 0;

    try{
        for (uint8_t i = 0; i < 10; ++i) {
            asteroids.emplace_back(dis_x(gen), dis_y(gen), dis_ss(gen), dis_ss(gen));
            asteroids[i].reallocate();
        }
        refresh();
    }catch (const exception& e) {
        cerr << "Error generating asteroids: " << e.what() << endl;
        return 1;
    }
    //*/

    try{
        bool game_over = false;
        while(true){
            clear();
            p.draw();

            uint8_t * index = (uint8_t *)malloc(1);
            if(index == nullptr){
                endwin();
                cerr << "Memory allocation failed for index." << endl;
                break;
            }
            if(asteroids.size() > 0){
                index = (uint8_t *)realloc(index, asteroids.size() ? asteroids.size() : 1);
                if(index == nullptr){
                    cerr << "Memory reallocation failed for index." << endl;
                    break;
                }
                (*index) = 0;
                for(auto it = asteroids.begin(); it != asteroids.end(); ){
                    if(it->x == nullptr || it->y == nullptr) continue;
                    it->reallocate();
                    if(it->x == nullptr || it->y == nullptr) continue;
                    it->update(asteroids, *frame);
                    if(!it->status){
                        mvprintw(*it->y, *it->x, "O");
                    }else if(it->status == 1){
                        mvprintw(*it->y, *it->x, "X");
                        it->t++;
                        if(it->t > 3) it->status = 2;
                    }else if(it->status == 2){
                        asteroids.erase(asteroids.begin() + (*index));
                        *frame += 10;
                        *it->y = -1;
                    }
                    refresh();
                    ++it;
                    (*index)++;
                }
            }

            if(bullets.size() > 0){
                index = (uint8_t *)realloc(index, bullets.size() ? bullets.size() : 1);
                if(index == nullptr){
                    endwin();
                    cerr << "Memory reallocation failed for index." << endl;
                    break;
                }
                (*index) = 0;
                for(auto it = bullets.begin(); it != bullets.end(); ){
                    if(it->x == nullptr || it->y == nullptr) continue;
                    it->reallocate();
                    if(it->x == nullptr || it->y == nullptr) continue;
                    it->update();
                    mvprintw(*it->y, *it->x, "|");

                    if(*it->y < 0 || it->hit){
                        bullets.erase(bullets.begin() + (*index));
                    }else{
                        ++it;
                        (*index)++;
                    }
                    refresh();
                }
            }

            if(asteroids.size() > 0){
                index = (uint8_t *)realloc(index, asteroids.size() ? asteroids.size() : 1);
                if(index == nullptr){
                    endwin();
                    cerr << "Memory reallocation failed for index." << endl;
                    break;
                }
                (*index) = 0;
                for (auto& a : asteroids) {
                    if(a.x == nullptr || a.y == nullptr) continue;
                    a.check(bullets);
                    if(*a.x == *p.x && *a.y == *p.y){
                        game_over = true;
                    }
                    (*index)++;
                }
            }
            free(index);

            if(game_over) break;

            refresh();

            uint8_t ch = (uint8_t)getch();
            flushinp();

            if (ch == 'q') break; // Exit on 'q'
            else if (ch == 'w') p.move(0, -1);
            else if (ch == 's') p.move(0, 1);
            else if (ch == 'a') p.move(-1, 0);
            else if (ch == 'd') p.move(1, 0);
            else if (ch == ' '){
                bullets.emplace_back((*p.x), (*p.y), 1);
            }

            if(ch == 'c'){
                asteroids.emplace_back(dis_x(gen), dis_y(gen), dis_ss(gen), dis_ss(gen));
            }

            frame = (uint64_t *)realloc(frame, (size_t)ceil((double)(*frame) / 125.0) + 1);
            if(frame == nullptr){
                endwin();
                cerr << "Memory reallocation failed for frame." << endl;
                break;
            }
            (*frame)++;
            if(static_cast<int>(*frame) % 500 == 0){
                for(int i = 0; i < 10; ++i) {
                    asteroids.emplace_back(dis_x(gen), dis_y(gen), dis_ss(gen), dis_ss(gen));
                }
            }else if(static_cast<int>(*frame) % 100 == 0){
                for(int i = 0; i < 5; ++i){
                    asteroids.emplace_back(dis_x(gen), dis_y(gen), dis_ss(gen), dis_ss(gen));
                }
            }
            //*/

            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }catch (const exception& e) {
        endwin();
        cerr << "Error during game loop: " << e.what() << endl;
    }

    const int finalScore = static_cast<int>(*frame);    
    const int endGameScreenX = LINES / 2;
    const int endGameScreenY = (COLS - 9) / 2;

    clear();
    mvprintw(endGameScreenX, endGameScreenY, "Game Over!");
    mvprintw(endGameScreenX + 1, endGameScreenY, "Final Score: %d", finalScore);
    refresh();

    this_thread::sleep_for(chrono::seconds(2));

    flushinp();
    endwin();
    cout << "Thanks for playing! Your final score was: " << finalScore << endl;
    free(frame);
    return 0;
}