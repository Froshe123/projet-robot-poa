#include "graphe.hpp"
#include <cstdlib>
#include <vector>

// a et b sont voisins (pas de diagonale)
bool est_a_cote(Pos a, Pos b) {
    return (std::abs(a.l - b.l) + std::abs(a.c - b.c)) == 1;
}

// BFS sur la carte mentale : la direction du premier pas du plus court chemin
// vers une case a cote de l'objet, ou PAS_DE_CHEMIN s'il n'y en a pas (annexe B)
char prochain_pas(const CarteRobot &carte, Pos depart, Pos objet) {
    // premier_pas[l][c] = direction prise depuis le depart pour arriver en (l, c), 0 = pas encore vue
    std::vector<std::vector<char>> premier_pas(carte.hauteur(), std::vector<char>(carte.largeur(), 0));
    std::vector<Pos> file(carte.hauteur() * carte.largeur()); // file FIFO : debut = tete, fin = queue
    int debut = 0;
    int fin = 0;
    for (int i = 0; i < 4; i++) {
        Pos v = voisin(depart, DIRS[i]);
        if (carte.case_valide(v)) {
            premier_pas[v.l][v.c] = DIRS[i];
            file[fin] = v;
            fin++;
        }
    }
    while (debut < fin) { // les cases sortent dans l'ordre de distance
        Pos courant = file[debut];
        debut++;
        if (est_a_cote(courant, objet)) {
            return premier_pas[courant.l][courant.c];
        }

        for (int i = 0; i < 4; i++) {
            Pos v = voisin(courant, DIRS[i]);
            if (carte.case_valide(v) && premier_pas[v.l][v.c] == 0 && !pos_egales(v, depart)) {
                premier_pas[v.l][v.c] = premier_pas[courant.l][courant.c];
                file[fin] = v;
                fin++;
            }
        }
    }
    return PAS_DE_CHEMIN;
}
