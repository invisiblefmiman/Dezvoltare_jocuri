#include <SDL.h>
#include <vector>
#include <iostream>
#include <bitset>
#include <algorithm>

const int LATIME_ECRAN = 800;
const int INALTIME_ECRAN = 600;
const int MAX_ENTITATI = 1000;

enum TipComponenta {
    C_TRANSFORMARE = 0,
    C_CORP_RIGID,
    C_GRAFICA,
    C_CONTROL,
    C_AI_PATRULARE,
    C_COLIZIUNE,
    NUMAR_TIPURI
};

struct Transformare {
    float pozitieX, pozitieY;
};

struct CorpRigid {
    float vitezaX, vitezaY;
    float vitezaDeplasare;
};

struct Grafica {
    int latime, inaltime;
    Uint8 rosu, verde, albastru;
};

struct Coliziune {
    bool esteSolid;
    bool esteDeclansator;
};

// --- REGISTRU ECS ---
class Registru {
public:
    int numarEntitatiActive = 0;
    std::vector<std::bitset<TipComponenta::NUMAR_TIPURI>> semnaturi;

    std::vector<Transformare> transformari;
    std::vector<CorpRigid> corpuriFizice;
    std::vector<Grafica> elementeGrafice;
    std::vector<Coliziune> coliziuni;

    Registru() {
        semnaturi.resize(MAX_ENTITATI);
        transformari.resize(MAX_ENTITATI);
        corpuriFizice.resize(MAX_ENTITATI);
        elementeGrafice.resize(MAX_ENTITATI);
        coliziuni.resize(MAX_ENTITATI);
    }

    int CreazaEntitate() {
        if (numarEntitatiActive >= MAX_ENTITATI) return -1;
        int id = numarEntitatiActive++;
        semnaturi[id].reset();
        return id;
    }

    void AdaugaTransformare(int id, float x, float y) {
        transformari[id] = { x, y };
        semnaturi[id].set(C_TRANSFORMARE);
    }

    void AdaugaCorpRigid(int id, float vx, float vy, float viteza) {
        corpuriFizice[id] = { vx, vy, viteza };
        semnaturi[id].set(C_CORP_RIGID);
    }

    void AdaugaGrafica(int id, int l, int i, Uint8 r, Uint8 g, Uint8 b) {
        elementeGrafice[id] = { l, i, r, g, b };
        semnaturi[id].set(C_GRAFICA);
    }

    void AdaugaColiziune(int id, bool solid) {
        coliziuni[id] = { solid, !solid };
        semnaturi[id].set(C_COLIZIUNE);
    }

    void AdaugaEtichetaControl(int id) { semnaturi[id].set(C_CONTROL); }
    void AdaugaEtichetaAI(int id) { semnaturi[id].set(C_AI_PATRULARE); }
};

// --- SISTEME ---

class SistemControl {
public:
    void Actualizeaza(Registru& reg) {
        const Uint8* stareTaste = SDL_GetKeyboardState(NULL);

        for (int i = 0; i < reg.numarEntitatiActive; i++) {
            if (reg.semnaturi[i].test(C_CONTROL) && reg.semnaturi[i].test(C_CORP_RIGID)) {
                float viteza = reg.corpuriFizice[i].vitezaDeplasare;
                reg.corpuriFizice[i].vitezaX = 0;
                reg.corpuriFizice[i].vitezaY = 0;

                if (stareTaste[SDL_SCANCODE_W] || stareTaste[SDL_SCANCODE_UP])    reg.corpuriFizice[i].vitezaY = -viteza;
                if (stareTaste[SDL_SCANCODE_S] || stareTaste[SDL_SCANCODE_DOWN])  reg.corpuriFizice[i].vitezaY = viteza;
                if (stareTaste[SDL_SCANCODE_A] || stareTaste[SDL_SCANCODE_LEFT])  reg.corpuriFizice[i].vitezaX = -viteza;
                if (stareTaste[SDL_SCANCODE_D] || stareTaste[SDL_SCANCODE_RIGHT]) reg.corpuriFizice[i].vitezaX = viteza;
            }
        }
    }
};

class SistemFizica {
    bool VerificaColiziune(Transformare& t1, Grafica& g1, Transformare& t2, Grafica& g2) {
        return (t1.pozitieX < t2.pozitieX + g2.latime &&
            t1.pozitieX + g1.latime > t2.pozitieX &&
            t1.pozitieY < t2.pozitieY + g2.inaltime &&
            t1.pozitieY + g1.inaltime > t2.pozitieY);
    }

public:
    void Actualizeaza(Registru& reg, float deltaTimp) {
        for (int i = 0; i < reg.numarEntitatiActive; i++) {

            // AI Monstru
            if (reg.semnaturi[i].test(C_AI_PATRULARE) && reg.semnaturi[i].test(C_TRANSFORMARE)) {
                if (reg.transformari[i].pozitieX < 50)
                    reg.corpuriFizice[i].vitezaX = abs(reg.corpuriFizice[i].vitezaX);
                if (reg.transformari[i].pozitieX > LATIME_ECRAN - 100)
                    reg.corpuriFizice[i].vitezaX = -abs(reg.corpuriFizice[i].vitezaX);
            }

            // Aplicare Miscare
            if (reg.semnaturi[i].test(C_TRANSFORMARE) && reg.semnaturi[i].test(C_CORP_RIGID)) {
                float pozitieVecheX = reg.transformari[i].pozitieX;
                float pozitieVecheY = reg.transformari[i].pozitieY;

                reg.transformari[i].pozitieX += reg.corpuriFizice[i].vitezaX * deltaTimp;
                reg.transformari[i].pozitieY += reg.corpuriFizice[i].vitezaY * deltaTimp;

                // Verificare Coliziuni
                if (reg.semnaturi[i].test(C_COLIZIUNE) && reg.semnaturi[i].test(C_GRAFICA)) {
                    for (int j = 0; j < reg.numarEntitatiActive; j++) {
                        if (i == j) continue;

                        if (reg.semnaturi[j].test(C_COLIZIUNE) && reg.semnaturi[j].test(C_TRANSFORMARE) && reg.semnaturi[j].test(C_GRAFICA)) {
                            if (VerificaColiziune(reg.transformari[i], reg.elementeGrafice[i], reg.transformari[j], reg.elementeGrafice[j])) {

                                if (reg.coliziuni[j].esteSolid) {
                                    reg.transformari[i].pozitieX = pozitieVecheX;
                                    reg.transformari[i].pozitieY = pozitieVecheY;
                                }
                                else if (reg.semnaturi[i].test(C_CONTROL) && reg.semnaturi[j].test(C_AI_PATRULARE)) {
                                    std::cout << "GAME OVER!" << std::endl;
                                    reg.transformari[i].pozitieX = 100;
                                    reg.transformari[i].pozitieY = 300;
                                }
                            }
                        }
                    }
                }

                // Limite Ecran
                if (reg.transformari[i].pozitieX < 0) reg.transformari[i].pozitieX = 0;
                if (reg.transformari[i].pozitieY < 0) reg.transformari[i].pozitieY = 0;
                if (reg.transformari[i].pozitieX > LATIME_ECRAN) reg.transformari[i].pozitieX = LATIME_ECRAN;
                if (reg.transformari[i].pozitieY > INALTIME_ECRAN) reg.transformari[i].pozitieY = INALTIME_ECRAN;
            }
        }
    }
};

class SistemDesenare {
public:
    void Deseneaza(Registru& reg, SDL_Renderer* randator) {
        SDL_SetRenderDrawColor(randator, 20, 20, 20, 255);
        SDL_RenderClear(randator);

        for (int i = 0; i < reg.numarEntitatiActive; i++) {
            if (reg.semnaturi[i].test(C_TRANSFORMARE) && reg.semnaturi[i].test(C_GRAFICA)) {
                SDL_Rect dreptunghi;
                dreptunghi.x = (int)reg.transformari[i].pozitieX;
                dreptunghi.y = (int)reg.transformari[i].pozitieY;
                dreptunghi.w = reg.elementeGrafice[i].latime;
                dreptunghi.h = reg.elementeGrafice[i].inaltime;

                SDL_SetRenderDrawColor(randator, reg.elementeGrafice[i].rosu, reg.elementeGrafice[i].verde, reg.elementeGrafice[i].albastru, 255);
                SDL_RenderFillRect(randator, &dreptunghi);
            }
        }
        SDL_RenderPresent(randator);
    }
};

// --- MAIN ---

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return -1;

    SDL_Window* fereastra = SDL_CreateWindow("ECS Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, LATIME_ECRAN, INALTIME_ECRAN, SDL_WINDOW_SHOWN);
    SDL_Renderer* randator = SDL_CreateRenderer(fereastra, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    Registru registru;
    SistemControl sistemControl;
    SistemFizica sistemFizica;
    SistemDesenare sistemDesenare;

    // 1. Jucator (Verde)
    int jucator = registru.CreazaEntitate();
    registru.AdaugaTransformare(jucator, 100, 300);
    registru.AdaugaCorpRigid(jucator, 0, 0, 300.0f);
    registru.AdaugaGrafica(jucator, 30, 30, 0, 255, 0);
    registru.AdaugaEtichetaControl(jucator);
    registru.AdaugaColiziune(jucator, false);

    // 2. Monstru (Rosu)
    int monstru = registru.CreazaEntitate();
    registru.AdaugaTransformare(monstru, 600, 300);
    registru.AdaugaCorpRigid(monstru, -150.0f, 0, 150.0f);
    registru.AdaugaGrafica(monstru, 40, 40, 255, 0, 0);
    registru.AdaugaEtichetaAI(monstru);
    registru.AdaugaColiziune(monstru, true);

    // 3. Platforma (Albastru)
    int platforma = registru.CreazaEntitate();
    registru.AdaugaTransformare(platforma, 300, 400);
    registru.AdaugaGrafica(platforma, 200, 20, 100, 100, 255);
    registru.AdaugaColiziune(platforma, true);

    // 4. Copac (Decor)
    int copac = registru.CreazaEntitate();
    registru.AdaugaTransformare(copac, 700, 100);
    registru.AdaugaGrafica(copac, 50, 80, 0, 100, 0);

    bool ruleaza = true;
    Uint64 timpAnterior = SDL_GetPerformanceCounter();

    while (ruleaza) {
        Uint64 timpCurent = SDL_GetPerformanceCounter();
        float deltaTimp = (float)(timpCurent - timpAnterior) / SDL_GetPerformanceFrequency();
        timpAnterior = timpCurent;

        SDL_Event eveniment;
        while (SDL_PollEvent(&eveniment)) {
            if (eveniment.type == SDL_QUIT) ruleaza = false;
        }

        sistemControl.Actualizeaza(registru);
        sistemFizica.Actualizeaza(registru, deltaTimp);
        sistemDesenare.Deseneaza(registru, randator);
    }

    SDL_DestroyRenderer(randator);
    SDL_DestroyWindow(fereastra);
    SDL_Quit();

    return 0;
}
