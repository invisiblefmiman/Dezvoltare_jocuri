#include <SDL.h>
#include <vector>
#include <iostream>
#include <bitset>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>

const int LATIME_ECRAN = 800;
const int INALTIME_ECRAN = 600;
const int MAX_ENTITATI = 2000;

enum TipComponenta {
    C_TRANSFORMARE = 0,
    C_CORP_RIGID,
    C_SPRITE,
    C_CONTROL,
    C_AI_PATRULARE,
    C_COLIZIUNE,
    C_STATISTICI,
    C_DURATA_VIATA,
    C_PROIECTIL,
    NUMAR_TIPURI
};

struct Transformare {
    float x, y;
};

struct CorpRigid {
    float vx, vy;
    float vitezaMiscare;
};

struct ComponentaSprite {
    SDL_Texture* textura;
    int latime, inaltime;
    Uint8 r, g, b;
};

struct Coliziune {
    bool esteSolid;
    bool esteDeclansator;
    bool esteInamic;
    bool esteColectabil;
};

struct Statistici {
    int viata;
    int viataMaxima;
    int scor;
};

struct DurataViata {
    float timpRamas;
};

class Registru {
public:
    int indexEntitate = 0;
    std::vector<std::bitset<TipComponenta::NUMAR_TIPURI>> semnaturi;

    std::vector<Transformare> transformari;
    std::vector<CorpRigid> corpuri;
    std::vector<ComponentaSprite> spriteuri;
    std::vector<Coliziune> coliziuni;
    std::vector<Statistici> statistici;
    std::vector<DurataViata> durate;

    Registru() {
        semnaturi.resize(MAX_ENTITATI);
        transformari.resize(MAX_ENTITATI);
        corpuri.resize(MAX_ENTITATI);
        spriteuri.resize(MAX_ENTITATI);
        coliziuni.resize(MAX_ENTITATI);
        statistici.resize(MAX_ENTITATI);
        durate.resize(MAX_ENTITATI);
    }

    int CreazaEntitate() {
        for (int i = 0; i < indexEntitate; i++) {
            if (semnaturi[i].none()) return i;
        }
        if (indexEntitate >= MAX_ENTITATI) return -1;
        int id = indexEntitate++;
        semnaturi[id].reset();
        return id;
    }

    void DistrugeEntitate(int id) {
        if (semnaturi[id].test(C_SPRITE)) {
            SDL_DestroyTexture(spriteuri[id].textura);
            spriteuri[id].textura = nullptr;
        }
        semnaturi[id].reset();
    }

    void AdaugaTransformare(int id, float x, float y) {
        transformari[id] = { x, y };
        semnaturi[id].set(C_TRANSFORMARE);
    }

    void AdaugaCorpRigid(int id, float vx, float vy, float vM) {
        corpuri[id] = { vx, vy, vM };
        semnaturi[id].set(C_CORP_RIGID);
    }

    void AdaugaSprite(int id, SDL_Renderer* ren, int w, int h, int r, int g, int b) {
        SDL_Surface* suprafata = SDL_CreateRGBSurface(0, w, h, 32, 0, 0, 0, 0);
        SDL_FillRect(suprafata, NULL, SDL_MapRGB(suprafata->format, r, g, b));
        SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, suprafata);
        SDL_FreeSurface(suprafata);

        spriteuri[id] = { tex, w, h, (Uint8)r, (Uint8)g, (Uint8)b };
        semnaturi[id].set(C_SPRITE);
    }

    void AdaugaColiziune(int id, bool solid, bool inamic = false, bool colectabil = false) {
        coliziuni[id] = { solid, !solid, inamic, colectabil };
        semnaturi[id].set(C_COLIZIUNE);
    }

    void AdaugaStatistici(int id, int viata) {
        statistici[id] = { viata, viata, 0 };
        semnaturi[id].set(C_STATISTICI);
    }

    void AdaugaDurataViata(int id, float timp) {
        durate[id] = { timp };
        semnaturi[id].set(C_DURATA_VIATA);
    }

    void AdaugaTagControl(int id) { semnaturi[id].set(C_CONTROL); }
    void AdaugaTagAI(int id) { semnaturi[id].set(C_AI_PATRULARE); }
    void AdaugaTagProiectil(int id) { semnaturi[id].set(C_PROIECTIL); }
};

void GenereazaExplozie(Registru& reg, SDL_Renderer* ren, float x, float y) {
    for (int i = 0; i < 10; i++) {
        int particula = reg.CreazaEntitate();
        reg.AdaugaTransformare(particula, x, y);

        float aleatorVx = (rand() % 200 - 100) * 1.5f;
        float aleatorVy = (rand() % 200 - 100) * 1.5f;
        reg.AdaugaCorpRigid(particula, aleatorVx, aleatorVy, 0);

        reg.AdaugaSprite(particula, ren, 5, 5, 255, rand() % 155, 0);
        reg.AdaugaDurataViata(particula, 0.5f);
    }
}

class SistemIntrare {
    float timpReincarcare = 0.0f;
public:
    void Actualizeaza(Registru& reg, SDL_Renderer* ren, float dt) {
        const Uint8* taste = SDL_GetKeyboardState(NULL);
        if (timpReincarcare > 0) timpReincarcare -= dt;

        for (int i = 0; i < reg.indexEntitate; i++) {
            if (reg.semnaturi[i].test(C_CONTROL) && reg.semnaturi[i].test(C_CORP_RIGID)) {
                float viteza = reg.corpuri[i].vitezaMiscare;
                reg.corpuri[i].vx = 0;
                reg.corpuri[i].vy = 0;

                if (taste[SDL_SCANCODE_W] || taste[SDL_SCANCODE_UP])    reg.corpuri[i].vy = -viteza;
                if (taste[SDL_SCANCODE_S] || taste[SDL_SCANCODE_DOWN])  reg.corpuri[i].vy = viteza;
                if (taste[SDL_SCANCODE_A] || taste[SDL_SCANCODE_LEFT])  reg.corpuri[i].vx = -viteza;
                if (taste[SDL_SCANCODE_D] || taste[SDL_SCANCODE_RIGHT]) reg.corpuri[i].vx = viteza;

                if (taste[SDL_SCANCODE_SPACE] && timpReincarcare <= 0) {
                    int proiectil = reg.CreazaEntitate();
                    reg.AdaugaTransformare(proiectil, reg.transformari[i].x + 32, reg.transformari[i].y + 10);
                    reg.AdaugaSprite(proiectil, ren, 10, 5, 255, 255, 255);
                    reg.AdaugaCorpRigid(proiectil, 500.0f, 0, 0);
                    reg.AdaugaColiziune(proiectil, false);
                    reg.AdaugaTagProiectil(proiectil);
                    reg.AdaugaDurataViata(proiectil, 2.0f);

                    timpReincarcare = 0.3f;
                }
            }
        }
    }
};

class SistemLogica {
public:
    void Actualizeaza(Registru& reg, float dt) {
        for (int i = 0; i < reg.indexEntitate; i++) {
            if (reg.semnaturi[i].test(C_DURATA_VIATA)) {
                reg.durate[i].timpRamas -= dt;
                if (reg.durate[i].timpRamas <= 0) {
                    reg.DistrugeEntitate(i);
                }
            }

            if (reg.semnaturi[i].test(C_STATISTICI) && reg.semnaturi[i].test(C_CONTROL)) {
                if (reg.statistici[i].viata <= 0) {
                    reg.statistici[i].viata = reg.statistici[i].viataMaxima;
                    reg.statistici[i].scor = 0;
                    reg.transformari[i].x = 100;
                    reg.transformari[i].y = 300;
                }
            }
        }
    }
};

class SistemFizica {
    bool AABB(Transformare& t1, ComponentaSprite& s1, Transformare& t2, ComponentaSprite& s2) {
        return (t1.x < t2.x + s2.latime && t1.x + s1.latime > t2.x &&
            t1.y < t2.y + s2.inaltime && t1.y + s1.inaltime > t2.y);
    }

public:
    void Actualizeaza(Registru& reg, SDL_Renderer* ren, float dt) {
        for (int i = 0; i < reg.indexEntitate; i++) {
            if (reg.semnaturi[i].none()) continue;

            if (reg.semnaturi[i].test(C_AI_PATRULARE) && reg.semnaturi[i].test(C_TRANSFORMARE)) {
                if (reg.transformari[i].x < 400) reg.corpuri[i].vx = abs(reg.corpuri[i].vx);
                if (reg.transformari[i].x > LATIME_ECRAN - 100) reg.corpuri[i].vx = -abs(reg.corpuri[i].vx);
            }

            if (reg.semnaturi[i].test(C_TRANSFORMARE) && reg.semnaturi[i].test(C_CORP_RIGID)) {
                float vechiX = reg.transformari[i].x;
                float vechiY = reg.transformari[i].y;

                reg.transformari[i].x += reg.corpuri[i].vx * dt;
                reg.transformari[i].y += reg.corpuri[i].vy * dt;

                if (reg.semnaturi[i].test(C_COLIZIUNE) && reg.semnaturi[i].test(C_SPRITE)) {
                    for (int j = 0; j < reg.indexEntitate; j++) {
                        if (i == j || reg.semnaturi[j].none()) continue;

                        if (reg.semnaturi[j].test(C_COLIZIUNE) && reg.semnaturi[j].test(C_SPRITE)) {
                            if (AABB(reg.transformari[i], reg.spriteuri[i], reg.transformari[j], reg.spriteuri[j])) {

                                if (reg.coliziuni[j].esteSolid && !reg.semnaturi[i].test(C_PROIECTIL)) {
                                    reg.transformari[i].x = vechiX;
                                    reg.transformari[i].y = vechiY;
                                }
                                else if (reg.semnaturi[i].test(C_CONTROL) && reg.coliziuni[j].esteColectabil) {
                                    reg.statistici[i].scor += 10;
                                    reg.DistrugeEntitate(j);
                                }
                                else if (reg.semnaturi[i].test(C_CONTROL) && reg.coliziuni[j].esteInamic) {
                                    reg.transformari[i].x = vechiX - 20;
                                    if (reg.semnaturi[i].test(C_STATISTICI)) {
                                        reg.statistici[i].viata -= 10;
                                    }
                                }
                                else if (reg.semnaturi[i].test(C_PROIECTIL) && reg.coliziuni[j].esteInamic) {
                                    GenereazaExplozie(reg, ren, reg.transformari[j].x, reg.transformari[j].y);
                                    reg.DistrugeEntitate(j);
                                    reg.DistrugeEntitate(i);
                                    break;
                                }
                                else if (reg.semnaturi[i].test(C_PROIECTIL) && reg.coliziuni[j].esteSolid) {
                                    reg.DistrugeEntitate(i);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
};

class SistemRandare {
public:
    void Deseneaza(Registru& reg, SDL_Renderer* ren) {
        SDL_SetRenderDrawColor(ren, 30, 30, 30, 255);
        SDL_RenderClear(ren);

        for (int i = 0; i < reg.indexEntitate; i++) {
            if (reg.semnaturi[i].test(C_TRANSFORMARE) && reg.semnaturi[i].test(C_SPRITE)) {
                SDL_Rect destinatie;
                destinatie.x = (int)reg.transformari[i].x;
                destinatie.y = (int)reg.transformari[i].y;
                destinatie.w = reg.spriteuri[i].latime;
                destinatie.h = reg.spriteuri[i].inaltime;

                SDL_RenderCopy(ren, reg.spriteuri[i].textura, NULL, &destinatie);
            }
        }
        SDL_RenderPresent(ren);
    }
};

int main(int argc, char* argv[]) {
    srand((unsigned int)time(0));

    if (SDL_Init(SDL_INIT_VIDEO) < 0) return -1;
    SDL_Window* fereastra = SDL_CreateWindow("Faza 3", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, LATIME_ECRAN, INALTIME_ECRAN, SDL_WINDOW_SHOWN);
    SDL_Renderer* randator = SDL_CreateRenderer(fereastra, -1, SDL_RENDERER_ACCELERATED);

    Registru registru;
    SistemIntrare sisIntrare;
    SistemFizica sisFizica;
    SistemLogica sisLogica;
    SistemRandare sisRandare;

    int jucator = registru.CreazaEntitate();
    registru.AdaugaTransformare(jucator, 100, 300);
    registru.AdaugaCorpRigid(jucator, 0, 0, 300.0f);
    registru.AdaugaSprite(jucator, randator, 32, 32, 0, 255, 0);
    registru.AdaugaColiziune(jucator, false);
    registru.AdaugaTagControl(jucator);
    registru.AdaugaStatistici(jucator, 100);

    for (int k = 0; k < 2; k++) {
        int inamic = registru.CreazaEntitate();
        registru.AdaugaTransformare(inamic, 500 + (k * 100), 300);
        registru.AdaugaCorpRigid(inamic, -100.0f, 0, 0);
        registru.AdaugaSprite(inamic, randator, 40, 40, 255, 0, 0);
        registru.AdaugaColiziune(inamic, true, true, false);
        registru.AdaugaTagAI(inamic);
    }

    int platforma = registru.CreazaEntitate();
    registru.AdaugaTransformare(platforma, 300, 400);
    registru.AdaugaSprite(platforma, randator, 200, 32, 100, 100, 255);
    registru.AdaugaColiziune(platforma, true);

    for (int k = 0; k < 3; k++) {
        int obiect = registru.CreazaEntitate();
        registru.AdaugaTransformare(obiect, 600 + (k * 50), 100);
        registru.AdaugaSprite(obiect, randator, 20, 20, 255, 255, 0);
        registru.AdaugaColiziune(obiect, true, false, true);
    }

    bool ruleaza = true;
    Uint64 timpAnterior = SDL_GetPerformanceCounter();
    double timpAcumulat = 0;
    int cadre = 0;

    while (ruleaza) {
        Uint64 timpCurent = SDL_GetPerformanceCounter();
        float dt = (float)(timpCurent - timpAnterior) / SDL_GetPerformanceFrequency();
        timpAnterior = timpCurent;

        timpAcumulat += dt;
        cadre++;
        if (timpAcumulat >= 0.1) {
            int scorCurent = registru.statistici[jucator].scor;
            int viataCurenta = registru.statistici[jucator].viata;

            std::string titlu = "Faza 3 | Viata: " + std::to_string(viataCurenta) +
                " | Scor: " + std::to_string(scorCurent) +
                " | FPS: " + std::to_string((int)(1.0 / dt));
            SDL_SetWindowTitle(fereastra, titlu.c_str());
            cadre = 0;
            timpAcumulat = 0;
        }

        SDL_Event eveniment;
        while (SDL_PollEvent(&eveniment)) {
            if (eveniment.type == SDL_QUIT) ruleaza = false;
        }

        sisIntrare.Actualizeaza(registru, randator, dt);
        sisLogica.Actualizeaza(registru, dt);
        sisFizica.Actualizeaza(registru, randator, dt);
        sisRandare.Deseneaza(registru, randator);
    }

    for (int i = 0; i < registru.indexEntitate; i++) {
        if (registru.semnaturi[i].test(C_SPRITE) && registru.spriteuri[i].textura)
            SDL_DestroyTexture(registru.spriteuri[i].textura);
    }
    SDL_DestroyRenderer(randator);
    SDL_DestroyWindow(fereastra);
    SDL_Quit();

    return 0;
}
