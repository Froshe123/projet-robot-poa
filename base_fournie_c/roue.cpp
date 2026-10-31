#include "interface.hpp"
#include <algorithm>

// structure cyclique : roue de Plutchik (annexe A) et colonnes de l'armoire

// avance de 'pas' crans depuis la case i sur une roue de n cases
// pas < 0 = sens antihoraire ; resultat dans [0, n-1]
// ex (n = 8) : (7, 1) -> 0, (0, -1) -> 7
int roue_avancer(int i, int pas, int n){
    pas = pas % n + n; // pas devient positif
    return (i + pas) % n;
}

// nombre de crans entre a et b par le cote le plus court, dans [0, n/2]
// ex (n = 8) : (7, 0) -> 1, (1, 6) -> 3
int roue_distance(int a, int b, int n){
    int ecart = (b - a + n) % n; // ecart dans le sens horaire
    return std::min(ecart, n - ecart);
}
