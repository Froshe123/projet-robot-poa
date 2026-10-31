#ifndef MEMOIRE_HPP
#define MEMOIRE_HPP

#include <string>
#include "interface.hpp"
#include "carte_robot.hpp"

// la memoire du robot : tout ce qu'il sait, mis a jour a chaque perception
struct Memoire {
    CarteRobot carte; // ce qu'il sait de l'appartement
    Pos robot; // ou il est
    Pos selecteur; // ou est le selecteur de l'armoire
    bool vu[3][8]; // casiers deja vus pendant la demande en cours
    std::string contenu[3][8]; // leur contenu ("" = vide)

    Memoire(const Connaissances &k);
    void noter_perception(const PerceptionRobot &p);
    void oublier_armoire();
};

#endif