#ifndef TYPES_HPP
#define TYPES_HPP

// une case (ligne, colonne), (0, 0) en haut a gauche
struct Pos {
    int l; // ligne
    int c; // colonne
};


constexpr char DIRS[] = "NSEO"; // les 4 directions, toujours dans cet ordre


bool pos_egales(Pos a, Pos b);

Pos voisin(Pos p, char dir);

#endif
