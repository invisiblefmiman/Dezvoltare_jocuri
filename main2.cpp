#include <SDL.h>
#include <vector>
#include <iostream>
#include <bitset>
#include <string>
#include <cmath>

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
};

struct Coliziune {
    bool esteSolid;
    bool esteDeclansator; // Trigger
};

// --- REGISTRU ECS (Data Oriented) ---
class Registru {
public:
    int indexEntitate = 0;
    std::vector<std::bitset<TipComponenta::NUMAR_TIPURI>> semnaturi;

    std::vector<Transformare> transformari;
    std::vector<CorpRigid> corpuri;
    std::vector<ComponentaSprite> spriteuri;
    std::vector<Coliziune> coliziuni;

    Registru() {
        semnaturi.resize(MAX_ENTITATI);
        transformari.resize(MAX_ENTITATI);
        corpuri.resize(MAX_ENTITATI);
        spriteuri.resize(MAX_ENTITATI);
        coliziuni.resize(MAX_ENTITATI);
    }

    int CreazaEntitate() {
        if (indexEntitate >= MAX_ENTITATI) return -1;
        int id = indexEntitate++;
        semnaturi[id].reset();
        return id;
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
        // Creare textura programatic (simulare sprite fara fisier extern)
        SDL_Surface* surf = SDL_CreateRGBSurface(0, w, h, 32, 0, 0, 0, 0);
        SDL_FillRect(surf, NULL, SDL_MapRGB(surf->format, r, g, b));
        SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
        SDL_FreeSurface(surf);

        spriteuri[id] = { tex, w, h };
        semnaturi[id].set(C_SPRITE);
    }

    void AdaugaColiziune(int id, bool solid) {
        coliziuni[id] = { solid, !solid };
        semnaturi[id].set(C_COLIZIUNE);
    }

    void AdaugaTagControl(int id) { semnaturi[id].set(C_CONTROL); }
    void AdaugaTagAI(int id) { semnaturi[id].set(C_AI_PATRULARE); }
};

// --- SISTEME ---

class SistemInput {
public:
    void Actualizeaza(Registru& reg) {
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        for (int i = 0; i < reg.indexEntitate; i++) {
            if (reg.semnaturi[i].test(C_CONTROL) && reg.semnaturi[i].test(C_CORP_RIGID)) {
                float v = reg.corpuri[i].vitezaMiscare;
                reg.corpuri[i].vx = 0;
                reg.corpuri[i].vy = 0;

                if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])    reg.corpuri[i].vy = -v;
                if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])  reg.corpuri[i].vy = v;
                if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])  reg.corpuri[i].vx = -v;
                if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) reg.corpuri[i].vx = v;
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
    void Actualizeaza(Registru& reg, float dt) {
        for (int i = 0; i < reg.indexEntitate; i++) {
            
            // AI Simplu
            if (reg.semnaturi[i].test(C_AI_PATRULARE) && reg.semnaturi[i].test(C_TRANSFORMARE)) {
                if (reg.transformari[i].x < 50) reg.corpuri[i].vx = abs(reg.corpuri[i].vx);
                if (reg.transformari[i].x > LATIME_ECRAN - 100) reg.corpuri[i].vx = -abs(reg.corpuri[i].vx);
            }

            // Miscare si Coliziune
            if (reg.semnaturi[i].test(C_TRANSFORMARE) && reg.semnaturi[i].test(C_CORP_RIGID)) {
                float vechiX = reg.transformari[i].x;
                float vechiY = reg.transformari[i].y;

                reg.transformari[i].x += reg.corpuri[i].vx * dt;
                reg.transformari[i].y += reg.corpuri[i].vy * dt;

                // Verificare limite ecran
                if (reg.transformari[i].x < 0) reg.transformari[i].x = 0;
                if (reg.transformari[i].y < 0) reg.transformari[i].y = 0;
                if (reg.transformari[i].x > LATIME_ECRAN) reg.transformari[i].x = LATIME_ECRAN;
                if (reg.transformari[i].y > INALTIME_ECRAN) reg.transformari[i].y = INALTIME_ECRAN;

                // Verificare Coliziuni
                if (reg.semnaturi[i].test(C_COLIZIUNE) && reg.semnaturi[i].test(C_SPRITE)) {
                    for (int j = 0; j < reg.indexEntitate; j++) {
                        if (i == j) continue;
                        if (reg.semnaturi[j].test(C_COLIZIUNE) && reg.semnaturi[j].test(C_SPRITE)) {
                            if (AABB(reg.transformari[i], reg.spriteuri[i], reg.transformari[j], reg.spriteuri[j])) {
                                
                                // Logica Coliziune Solid
                                if (reg.coliziuni[j].esteSolid) {
                                    reg.transformari[i].x = vechiX;
                                    reg.transformari[i].y = vechiY;
                                }
                                // Logica Jucator vs AI (Game Over Reset)
                                else if (reg.semnaturi[i].test(C_CONTROL) && reg.semnaturi[j].test(C_AI_PATRULARE)) {
                                    std::cout << "[Eveniment] Coliziune cu Inamic! Resetare pozitie." << std::endl;
                                    reg.transformari[i].x = 100; 
                                    reg.transformari[i].y = 300;
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
                SDL_Rect dest;
                dest.x = (int)reg.transformari[i].x;
                dest.y = (int)reg.transformari[i].y;
                dest.w = reg.spriteuri[i].latime;
                dest.h = reg.spriteuri[i].inaltime;

                // Randare textura (Sprite) in loc de FillRect
                SDL_RenderCopy(ren, reg.spriteuri[i].textura, NULL, &dest);
            }
        }
        SDL_RenderPresent(ren);
    }
};

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return -1;
    SDL_Window* fereastra = SDL_CreateWindow("Faza 2: ECS Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, LATIME_ECRAN, INALTIME_ECRAN, SDL_WINDOW_SHOWN);
    SDL_Renderer* randator = SDL_CreateRenderer(fereastra, -1, SDL_RENDERER_ACCELERATED);

    Registru reg;
    SistemInput sysInput;
    SistemFizica sysFizica;
    SistemRandare sysRandare;

    // 1. Jucator (Verde)
    int player = reg.CreazaEntitate();
    reg.AdaugaTransformare(player, 100, 300);
    reg.AdaugaCorpRigid(player, 0, 0, 300.0f);
    reg.AdaugaSprite(player, randator, 32, 32, 0, 255, 0); // Sprite 32x32
    reg.AdaugaColiziune(player, false);
    reg.AdaugaTagControl(player);

    // 2. Monstru (Rosu)
    int inamic = reg.CreazaEntitate();
    reg.AdaugaTransformare(inamic, 600, 300);
    reg.AdaugaCorpRigid(inamic, -150.0f, 0, 0);
    reg.AdaugaSprite(inamic, randator, 40, 40, 255, 0, 0);
    reg.AdaugaColiziune(inamic, true); // Trigger
    reg.AdaugaTagAI(inamic);

    // 3. Platforma (Albastru)
    int zid = reg.CreazaEntitate();
    reg.AdaugaTransformare(zid, 300, 400);
    reg.AdaugaSprite(zid, randator, 200, 32, 100, 100, 255);
    reg.AdaugaColiziune(zid, true); // Solid

    // 4. Obiect Colectabil (Galben)
    int banut = reg.CreazaEntitate();
    reg.AdaugaTransformare(banut, 700, 100);
    reg.AdaugaSprite(banut, randator, 20, 20, 255, 255, 0);
    reg.AdaugaColiziune(banut, true);

    bool ruleaza = true;
    Uint64 timpAnterior = SDL_GetPerformanceCounter();
    double timpAcumulat = 0;
    int cadre = 0;

    while (ruleaza) {
        Uint64 timpCurent = SDL_GetPerformanceCounter();
        float dt = (float)(timpCurent - timpAnterior) / SDL_GetPerformanceFrequency();
        timpAnterior = timpCurent;

        // Monitorizare FPS (Cerinta Faza 1 integrata in Faza 2)
        timpAcumulat += dt;
        cadre++;
        if (timpAcumulat >= 1.0) {
            std::string titlu = "ECS Engine | FPS: " + std::to_string(cadre) + " | Entitati: " + std::to_string(reg.indexEntitate);
            SDL_SetWindowTitle(fereastra, titlu.c_str());
            cadre = 0;
            timpAcumulat = 0;
        }

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) ruleaza = false;
        }

        sysInput.Actualizeaza(reg);
        sysFizica.Actualizeaza(reg, dt);
        sysRandare.Deseneaza(reg, randator);
    }

    // Curatare Texturi
    for(int i=0; i<reg.indexEntitate; i++) {
        if(reg.semnaturi[i].test(C_SPRITE)) SDL_DestroyTexture(reg.spriteuri[i].textura);
    }
    SDL_DestroyRenderer(randator);
    SDL_DestroyWindow(fereastra);
    SDL_Quit();

    return 0;
}
