#include "memoire.hpp"

/* Au départ, le robot connaît le bord, l'armoire, le dictionnaire,
   les résidents et sa case de départ (énoncé 3.2). Aucun mur. */
Memoire::Memoire(const Connaissances &k)
    : carte(k.hauteur, k.largeur), robot(k.depart), selecteur(k.casier_depart)
{
    carte.mettre_a_jour(k.armoire, TypeCase::OCCUPEE);
    carte.mettre_a_jour(k.dictionnaire, TypeCase::OCCUPEE);
    for (const ResidentConnu &r : k.residents) {
        carte.mettre_a_jour(r.pos, TypeCase::OCCUPEE);
    }
    carte.mettre_a_jour(k.depart, TypeCase::LIBRE);
    oublier_armoire();
}

/* À chaque tour, on écrit ce que le robot voit (énoncé 3.2 et 7). */
void Memoire::noter_perception(const PerceptionRobot &p)
{
    /* 1. les 4 cases voisines, dans l'ordre N, S, E, O */
    for (int i = 0; i < 4; i++) {
        Pos v = voisin(robot, DIRS[i]);
        if (p.voisins[i] == "libre") {
            carte.mettre_a_jour(v, TypeCase::LIBRE);
        } else if (!p.voisins[i].empty()) {
            carte.mettre_a_jour(v, TypeCase::OCCUPEE);  
        }
    }

    /* 2. le casier sous le sélecteur, si le robot est devant l'armoire */
    if (p.devant_armoire) {
        vu[selecteur.l][selecteur.c] = true;
        contenu[selecteur.l][selecteur.c] = p.contenu_casier.value_or("");
    }
}

/* Nouvelle demande : on oublie ce qu'on a vu dans l'armoire. */
void Memoire::oublier_armoire()
{
    for (int l = 0; l < 3; l++) {
        for (int c = 0; c < 8; c++) {
            vu[l][c] = false;
            contenu[l][c] = "";
        }
    }
}