#include "types.hpp"

// a et b sont la meme case ?
bool pos_egales(Pos a, Pos b) {
    return a.l == b.l && a.c == b.c;
}

// la case d'a cote de p dans la direction dir
Pos voisin(Pos p, char dir) {
    switch (dir) {
        case 'N': p.l--; break; // en haut
        case 'S': p.l++; break; // en bas
        case 'E': p.c++; break; // a droite
        case 'O': p.c--; break; // a gauche
    }
    return p;
}
