#include <SDL.h>
#include <vector>
#include <iostream>
#include <string>
#include <algorithm>
#include <cmath>
#include <iomanip>

const int LATIME_ECRAN = 1024;
const int INALTIME_ECRAN = 768;
const int DIMENSIUNE_GRILA = 16;
const int COLOANE_GRILA = LATIME_ECRAN / DIMENSIUNE_GRILA + 1;
const int RANDURI_GRILA = INALTIME_ECRAN / DIMENSIUNE_GRILA + 1;
const int MAX_OBIECTE = 100000;

SDL_Window* fereastra = nullptr;
SDL_Renderer* randator = nullptr;
SDL_Texture* texturaSprite = nullptr;

struct EntitateOOP {
    float x, y;
    float vx, vy;
    bool activ;
};

struct DateDOD {
    std::vector<float> x;
    std::vector<float> y;
    std::vector<float> vx;
    std::vector<float> vy;
    std::vector<bool> activ;
    int dimensiune;
};

std::vector<EntitateOOP> listaOOP;
DateDOD dateDOD;
std::vector<int> grilaColiziune[COLOANE_GRILA][RANDURI_GRILA];

int numarObiecteCurente = 1000;
bool modDOD = false;
bool ruleaza = true;

void InitializareSistem() {
    SDL_Init(SDL_INIT_VIDEO);
    fereastra = SDL_CreateWindow("Proiect Faza 1", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, LATIME_ECRAN, INALTIME_ECRAN, SDL_WINDOW_SHOWN);
    randator = SDL_CreateRenderer(fereastra, -1, SDL_RENDERER_ACCELERATED);

    SDL_Surface* suprafata = SDL_CreateRGBSurface(0, 8, 8, 32, 0, 0, 0, 0);
    SDL_FillRect(suprafata, NULL, SDL_MapRGB(suprafata->format, 255, 100, 100));
    texturaSprite = SDL_CreateTextureFromSurface(randator, suprafata);
    SDL_FreeSurface(suprafata);

    listaOOP.resize(MAX_OBIECTE);
    dateDOD.x.resize(MAX_OBIECTE);
    dateDOD.y.resize(MAX_OBIECTE);
    dateDOD.vx.resize(MAX_OBIECTE);
    dateDOD.vy.resize(MAX_OBIECTE);
    dateDOD.activ.resize(MAX_OBIECTE);
    dateDOD.dimensiune = 0;

    for (int i = 0; i < MAX_OBIECTE; i++) {
        float px = (float)(rand() % LATIME_ECRAN);
        float py = (float)(rand() % INALTIME_ECRAN);
        float v1 = (float)((rand() % 200 - 100) / 10.0f);
        float v2 = (float)((rand() % 200 - 100) / 10.0f);

        listaOOP[i].x = px;
        listaOOP[i].y = py;
        listaOOP[i].vx = v1;
        listaOOP[i].vy = v2;
        listaOOP[i].activ = (i < numarObiecteCurente);

        dateDOD.x[i] = px;
        dateDOD.y[i] = py;
        dateDOD.vx[i] = v1;
        dateDOD.vy[i] = v2;
        dateDOD.activ[i] = (i < numarObiecteCurente);
    }
    dateDOD.dimensiune = numarObiecteCurente;
}

void ProcesareEvenimente() {
    SDL_Event eveniment;
    while (SDL_PollEvent(&eveniment)) {
        if (eveniment.type == SDL_QUIT) {
            ruleaza = false;
        }
        else if (eveniment.type == SDL_KEYDOWN) {
            if (eveniment.key.keysym.sym == SDLK_SPACE) {
                modDOD = !modDOD;
            }
            if (eveniment.key.keysym.sym == SDLK_UP) {
                numarObiecteCurente += 500;
                if (numarObiecteCurente > MAX_OBIECTE) numarObiecteCurente = MAX_OBIECTE;
            }
            if (eveniment.key.keysym.sym == SDLK_DOWN) {
                numarObiecteCurente -= 500;
                if (numarObiecteCurente < 0) numarObiecteCurente = 0;
            }
        }
    }
}

void ActualizareOOP(float delta) {
    for (int x = 0; x < COLOANE_GRILA; x++) {
        for (int y = 0; y < RANDURI_GRILA; y++) {
            grilaColiziune[x][y].clear();
        }
    }

    for (int i = 0; i < numarObiecteCurente; i++) {
        listaOOP[i].x += listaOOP[i].vx * delta;
        listaOOP[i].y += listaOOP[i].vy * delta;

        if (listaOOP[i].x <= 0 || listaOOP[i].x >= LATIME_ECRAN - 8) listaOOP[i].vx *= -1;
        if (listaOOP[i].y <= 0 || listaOOP[i].y >= INALTIME_ECRAN - 8) listaOOP[i].vy *= -1;

        int celulaX = (int)(listaOOP[i].x / DIMENSIUNE_GRILA);
        int celulaY = (int)(listaOOP[i].y / DIMENSIUNE_GRILA);
        
        if (celulaX >= 0 && celulaX < COLOANE_GRILA && celulaY >= 0 && celulaY < RANDURI_GRILA) {
            grilaColiziune[celulaX][celulaY].push_back(i);
        }
    }

    for (int i = 0; i < numarObiecteCurente; i++) {
        int cx = (int)(listaOOP[i].x / DIMENSIUNE_GRILA);
        int cy = (int)(listaOOP[i].y / DIMENSIUNE_GRILA);

        if (cx >= 0 && cx < COLOANE_GRILA && cy >= 0 && cy < RANDURI_GRILA) {
            for (int idVecin : grilaColiziune[cx][cy]) {
                if (i != idVecin) {
                    float dx = listaOOP[i].x - listaOOP[idVecin].x;
                    float dy = listaOOP[i].y - listaOOP[idVecin].y;
                    if (dx * dx + dy * dy < 64.0f) {
                        listaOOP[i].vx *= -1;
                        listaOOP[i].vy *= -1;
                    }
                }
            }
        }
    }
}

void ActualizareDOD(float delta) {
    for (int x = 0; x < COLOANE_GRILA; x++) {
        for (int y = 0; y < RANDURI_GRILA; y++) {
            grilaColiziune[x][y].clear();
        }
    }

    for (int i = 0; i < numarObiecteCurente; i++) {
        dateDOD.x[i] += dateDOD.vx[i] * delta;
        dateDOD.y[i] += dateDOD.vy[i] * delta;

        if (dateDOD.x[i] <= 0 || dateDOD.x[i] >= LATIME_ECRAN - 8) dateDOD.vx[i] *= -1;
        if (dateDOD.y[i] <= 0 || dateDOD.y[i] >= INALTIME_ECRAN - 8) dateDOD.vy[i] *= -1;

        int celulaX = (int)(dateDOD.x[i] / DIMENSIUNE_GRILA);
        int celulaY = (int)(dateDOD.y[i] / DIMENSIUNE_GRILA);

        if (celulaX >= 0 && celulaX < COLOANE_GRILA && celulaY >= 0 && celulaY < RANDURI_GRILA) {
            grilaColiziune[celulaX][celulaY].push_back(i);
        }
    }

    for (int i = 0; i < numarObiecteCurente; i++) {
        int cx = (int)(dateDOD.x[i] / DIMENSIUNE_GRILA);
        int cy = (int)(dateDOD.y[i] / DIMENSIUNE_GRILA);

        if (cx >= 0 && cx < COLOANE_GRILA && cy >= 0 && cy < RANDURI_GRILA) {
            for (int idVecin : grilaColiziune[cx][cy]) {
                if (i != idVecin) {
                    float dx = dateDOD.x[i] - dateDOD.x[idVecin];
                    float dy = dateDOD.y[i] - dateDOD.y[idVecin];
                    if (dx * dx + dy * dy < 64.0f) {
                        dateDOD.vx[i] *= -1;
                        dateDOD.vy[i] *= -1;
                    }
                }
            }
        }
    }
}

void Randare() {
    SDL_SetRenderDrawColor(randator, 0, 0, 0, 255);
    SDL_RenderClear(randator);

    SDL_Rect rectDestinatie;
    rectDestinatie.w = 8;
    rectDestinatie.h = 8;

    if (modDOD) {
        for (int i = 0; i < numarObiecteCurente; i++) {
            rectDestinatie.x = (int)dateDOD.x[i];
            rectDestinatie.y = (int)dateDOD.y[i];
            SDL_RenderCopy(randator, texturaSprite, NULL, &rectDestinatie);
        }
    }
    else {
        for (int i = 0; i < numarObiecteCurente; i++) {
            rectDestinatie.x = (int)listaOOP[i].x;
            rectDestinatie.y = (int)listaOOP[i].y;
            SDL_RenderCopy(randator, texturaSprite, NULL, &rectDestinatie);
        }
    }

    SDL_RenderPresent(randator);
}

int main(int argc, char* argv[]) {
    InitializareSistem();

    Uint64 timpAnterior = SDL_GetPerformanceCounter();
    double timpAcumulat = 0;
    int cadre = 0;

    while (ruleaza) {
        Uint64 timpCurent = SDL_GetPerformanceCounter();
        float delta = (float)(timpCurent - timpAnterior) / (float)SDL_GetPerformanceFrequency();
        timpAnterior = timpCurent;

        ProcesareEvenimente();

        if (modDOD) {
            ActualizareDOD(delta);
        }
        else {
            ActualizareOOP(delta);
        }

        Randare();

        timpAcumulat += delta;
        cadre++;
        if (timpAcumulat >= 1.0) {
            std::string titlu = "Mod: " + std::string(modDOD ? "DOD" : "OOP") + 
                " | Obiecte: " + std::to_string(numarObiecteCurente) + 
                " | FPS: " + std::to_string(cadre);
            SDL_SetWindowTitle(fereastra, titlu.c_str());
            cadre = 0;
            timpAcumulat = 0;
        }
    }

    SDL_DestroyTexture(texturaSprite);
    SDL_DestroyRenderer(randator);
    SDL_DestroyWindow(fereastra);
    SDL_Quit();

    return 0;
}
