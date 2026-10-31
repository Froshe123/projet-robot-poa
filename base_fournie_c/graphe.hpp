#ifndef GRAPHE_HPP
#define GRAPHE_HPP

#include "types.hpp"
#include "carte_robot.hpp"

constexpr char PAS_DE_CHEMIN = 'X'; // renvoye par prochain_pas si l'objet est inaccessible

bool est_a_cote(Pos a, Pos b);
char prochain_pas(const CarteRobot &carte, Pos depart, Pos objet);

#endif
