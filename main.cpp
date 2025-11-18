#include <SDL.h>
#include <vector>
#include <iostream>
#include <ctime>

const int LATIME_ECRAN = 800;
const int INALTIME_ECRAN = 600;
const int NR_OBIECTE = 10000;

struct Entitate {
    float x, y;
    float vitezaX, vitezaY;
};
std::vector<Entitate> listaEntitatiOOP;

struct DateLume {
    std::vector<float> x;
    std::vector<float> y;
    std::vector<float> vitezaX;
    std::vector<float> vitezaY;
} dateDOD;

void InitializeazaDate() {
    srand((unsigned int)time(0));

    dateDOD.x.resize(NR_OBIECTE);
    dateDOD.y.resize(NR_OBIECTE);
    dateDOD.vitezaX.resize(NR_OBIECTE);
    dateDOD.vitezaY.resize(NR_OBIECTE);
    listaEntitatiOOP.resize(NR_OBIECTE);

    for (int i = 0; i < NR_OBIECTE; i++) {
        float pozX = (float)(rand() % LATIME_ECRAN);
        float pozY = (float)(rand() % INALTIME_ECRAN);
        float vX = (float)((rand() % 200) - 100);
        float vY = (float)((rand() % 200) - 100);

        listaEntitatiOOP[i] = { pozX, pozY, vX, vY };

        dateDOD.x[i] = pozX;
        dateDOD.y[i] = pozY;
        dateDOD.vitezaX[i] = vX;
        dateDOD.vitezaY[i] = vY;
    }
}

void ActualizareOOP(float deltaTimp) {
    for (int i = 0; i < NR_OBIECTE; i++) {
        listaEntitatiOOP[i].x += listaEntitatiOOP[i].vitezaX * deltaTimp;
        listaEntitatiOOP[i].y += listaEntitatiOOP[i].vitezaY * deltaTimp;

        if (listaEntitatiOOP[i].x < 0 || listaEntitatiOOP[i].x > LATIME_ECRAN)
            listaEntitatiOOP[i].vitezaX *= -1;
        if (listaEntitatiOOP[i].y < 0 || listaEntitatiOOP[i].y > INALTIME_ECRAN)
            listaEntitatiOOP[i].vitezaY *= -1;

        if (i < NR_OBIECTE - 1) {
            float dx = listaEntitatiOOP[i].x - listaEntitatiOOP[i + 1].x;
            float dy = listaEntitatiOOP[i].y - listaEntitatiOOP[i + 1].y;
            if (dx * dx + dy * dy < 16.0f) {
                listaEntitatiOOP[i].vitezaX *= -1;
                listaEntitatiOOP[i].vitezaY *= -1;
            }
        }
    }
}

void ActualizareDOD(float deltaTimp) {
    for (int i = 0; i < NR_OBIECTE; i++) {
        dateDOD.x[i] += dateDOD.vitezaX[i] * deltaTimp;
        dateDOD.y[i] += dateDOD.vitezaY[i] * deltaTimp;

        if (dateDOD.x[i] < 0 || dateDOD.x[i] > LATIME_ECRAN)
            dateDOD.vitezaX[i] *= -1;
        if (dateDOD.y[i] < 0 || dateDOD.y[i] > INALTIME_ECRAN)
            dateDOD.vitezaY[i] *= -1;

        if (i < NR_OBIECTE - 1) {
            float dx = dateDOD.x[i] - dateDOD.x[i + 1];
            float dy = dateDOD.y[i] - dateDOD.y[i + 1];
            if (dx * dx + dy * dy < 16.0f) {
                dateDOD.vitezaX[i] *= -1;
                dateDOD.vitezaY[i] *= -1;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "Eroare SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* fereastra = SDL_CreateWindow("Proiect Faza 1: OOP vs DOD",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        LATIME_ECRAN, INALTIME_ECRAN, SDL_WINDOW_SHOWN);
    SDL_Renderer* randator = SDL_CreateRenderer(fereastra, -1, SDL_RENDERER_ACCELERATED);

    InitializeazaDate();

    bool programulRuleaza = true;
    bool folosesteDOD = false;
    Uint64 timpAnterior = SDL_GetPerformanceCounter();

    std::cout << "--- PROIECT PORNIT ---" << std::endl;
    std::cout << "Apasa SPACE pentru a schimba modul (OOP / DOD)." << std::endl;

    while (programulRuleaza) {
        SDL_Event eveniment;
        while (SDL_PollEvent(&eveniment)) {
            if (eveniment.type == SDL_QUIT) {
                programulRuleaza = false;
            }
            if (eveniment.type == SDL_KEYDOWN) {
                if (eveniment.key.keysym.sym == SDLK_SPACE) {
                    folosesteDOD = !folosesteDOD;
                    std::cout << "MOD SCHIMBAT: " << (folosesteDOD ? ">>> DOD (Rapid)" : ">>> OOP (Clasic)") << std::endl;
                }
            }
        }

        Uint64 timpCurent = SDL_GetPerformanceCounter();
        float deltaTimp = (float)(timpCurent - timpAnterior) / SDL_GetPerformanceFrequency();
        timpAnterior = timpCurent;

        if (folosesteDOD) {
            ActualizareDOD(deltaTimp);
        }
        else {
            ActualizareOOP(deltaTimp);
        }

        SDL_SetRenderDrawColor(randator, 0, 0, 0, 255);
        SDL_RenderClear(randator);

        SDL_SetRenderDrawColor(randator, 255, 255, 255, 255);

        for (int i = 0; i < NR_OBIECTE; i++) {
            if (folosesteDOD) {
                SDL_RenderDrawPoint(randator, (int)dateDOD.x[i], (int)dateDOD.y[i]);
            }
            else {
                SDL_RenderDrawPoint(randator, (int)listaEntitatiOOP[i].x, (int)listaEntitatiOOP[i].y);
            }
        }

        SDL_RenderPresent(randator);
    }

    SDL_DestroyRenderer(randator);
    SDL_DestroyWindow(fereastra);
    SDL_Quit();

    return 0;
}
