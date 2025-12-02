#include <ncurses.h>
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <thread>
#include <string>

using namespace std;

class asteroid{
public:
    asteroid(int x, int y, int size, int speed) : x(x), y(y), size(size), speed(speed) {}
    int x, y, size, speed;
    // 0 = active, 1 = destroyed 2 = inactive
    int status = 0;

    void update(const vector<asteroid>& astr, double frame){
        if(status) return;
        if(static_cast<int>(frame) % speed == 0 && !status) y += 1;
        if(y >= LINES) {
            y = 0;
            x = rand() % COLS;
        }

        mvprintw(LINES - 2, COLS - 15, "Score: %.0f", frame);
        mvprintw(LINES - 1, COLS - 15, "Asteroids: %lu", astr.size());
    }
};

class bullet{
public:
    bullet(int x, int y, int speed) : x(x), y(y), speed(speed) {}
    int x, y, speed;
    bool hit = false;

    void update(vector<asteroid>& astr){
        if(y >= 0) y -= speed;
        if(check(astr)) y = -1;
    }
private:
    bool check(vector<asteroid>& astr){
        for(auto& a : astr){
            for(int i=0; i<=speed; i++){
                if(a.x == x && a.y == y + i){
                    a.y = LINES + 1;
                    a.status = 1;
                    return true;
                }
            }
        }
        return false;
    }
};

class player {
public:
    player(int x, int y) : x(x), y(y) {}

    void move(int dx, int dy) {
        x += dx;
        y += dy;
    }

    void draw() const {
        mvprintw(y, x, icon == 'V' ? "V" : " ");
    }
    int x, y;
    char icon = 'V';
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

    int mx = 0, my = 0;
    getmaxyx(stdscr, my, mx);

    player p(mx / 2, my / 2);

    vector<asteroid> asteroids;
    vector<bullet> bullets;

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis_x(0, mx - 1);
    uniform_int_distribution<> dis_y(0, my - 3);
    uniform_int_distribution<> dis_ss(1, 3);

    double frame = 0;

    try{
        for (int i = 0; i < 10; ++i) {
            asteroids.emplace_back(dis_x(gen), dis_y(gen), dis_ss(gen), dis_ss(gen));
        }
        refresh();
    }catch (const exception& e) {
        cerr << "Error generating asteroids: " << e.what() << endl;
        return 1;
    }

    try{
        double * ptr = &frame;
        bool game_over = false;
        while(true){
            clear();
            p.draw();

            int index = 0;
            for(auto it = asteroids.begin(); it != asteroids.end(); ){
                if(!it->status){
                    mvprintw(it->y, it->x, "O");
                }else if(it->status == 1){
                    mvprintw(it->y, it->x, "X");
                    it->status = 2;
                }else if(it->status == 2){
                    asteroids.erase(asteroids.begin() + index);
                    frame += 10;
                }
                refresh();
                ++it;
                index++;
            }

            index = 0;
            for (auto& a : asteroids) {
                a.update(asteroids, *ptr);

                for(int i = 0; i <= a.speed; i++){
                    if(p.x == a.x && p.y == a.y - i){
                        game_over = true;
                    }
                }
                index++;
            }

            index = 0;
            for(auto it = bullets.begin(); it != bullets.end(); ){
                it->update(asteroids);
                mvprintw(it->y, it->x, "|");

                if(it->y < 0 || it->hit){
                    bullets.erase(bullets.begin() + index);
                }else{
                    ++it;
                    index++;
                }
                refresh();
            }

            if(game_over) break;

            refresh();

            int ch = getch();
            flushinp();

            if (ch == 'q') break; // Exit on 'q'
            else if (ch == 'w') p.move(0, -1);
            else if (ch == 's') p.move(0, 1);
            else if (ch == 'a') p.move(-1, 0);
            else if (ch == 'd') p.move(1, 0);
            else if (ch == ' '){
                bullets.emplace_back(p.x, p.y - 1, 1);
            }

            if(ch == 'c'){
                /*echo();
                string command = "";
                int newAstr = 0;
                char cmd_char;
                while((cmd_char = getch()) != '\n'){
                    command += cmd_char;
                }

                try{
                    newAstr = stoi(command);
                }catch(const exception& e){
                    newAstr = 0;
                }
                for(int i = 0; i < newAstr; i++){
                    asteroids.emplace_back(dis_x(gen), dis_y(gen), dis_ss(gen), dis_ss(gen));
                }
                noecho();*/
                asteroids.emplace_back(dis_x(gen), dis_y(gen), dis_ss(gen), dis_ss(gen));
            }

            frame++;

            if(static_cast<int>(*ptr) % 500 == 0){
                for(int i = 0; i < 10; ++i) {
                    asteroids.emplace_back(dis_x(gen), dis_y(gen), dis_ss(gen), dis_ss(gen));
                }
            }else if(static_cast<int>(*ptr) % 100 == 0){
                for(int i = 0; i < 5; ++i){
                    asteroids.emplace_back(dis_x(gen), dis_y(gen), dis_ss(gen), dis_ss(gen));
                }
            }

            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }catch (const exception& e) {
        cerr << "Error during game loop: " << e.what() << endl;
    }

    int endGameScreenX = LINES / 2;
    int endGameScreenY = (COLS - 9) / 2;

    clear();
    mvprintw(endGameScreenX, endGameScreenY, "Game Over!");
    mvprintw(endGameScreenX + 1, endGameScreenY, "Final Score: %d", static_cast<int>(frame));
    refresh();

    this_thread::sleep_for(chrono::seconds(2));

    flushinp();
    endwin();
    cout << "Thanks for playing! Your final score was: " << static_cast<int>(frame) << endl;
    return 0;
}