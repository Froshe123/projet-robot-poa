#include "memoire.hpp"

// au depart le robot connait le bord, l'armoire, le dictionnaire, les residents
// et sa case de depart (enonce 3.2), mais aucun mur
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

// a chaque tour on note ce que le robot voit
void Memoire::noter_perception(const PerceptionRobot &p)
{
    // les 4 voisins, dans l'ordre N S E O
    for (int i = 0; i < 4; i++) {
        Pos v = voisin(robot, DIRS[i]);
        if (p.voisins[i] == "libre") {
            carte.mettre_a_jour(v, TypeCase::LIBRE);
        } else if (!p.voisins[i].empty()) {
            carte.mettre_a_jour(v, TypeCase::OCCUPEE);  
        }
    }

    // le casier sous le selecteur, si le robot est devant l'armoire
    if (p.devant_armoire) {
        vu[selecteur.l][selecteur.c] = true;
        contenu[selecteur.l][selecteur.c] = p.contenu_casier.value_or("");
    }
}

// nouvelle demande : on oublie ce qu'on a vu dans l'armoire
void Memoire::oublier_armoire()
{
    for (int l = 0; l < 3; l++) {
        for (int c = 0; c < 8; c++) {
            vu[l][c] = false;
            contenu[l][c] = "";
        }
    }
}