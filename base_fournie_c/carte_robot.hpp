#ifndef CARTE_ROBOT_HPP
#define CARTE_ROBOT_HPP

#include <vector>
#include "types.hpp"

// ce que le robot croit d'une case
enum class TypeCase { INCONNUE, LIBRE, OCCUPEE };

// la carte mentale du robot, distincte de la vraie grille
class CarteRobot {
public:
    CarteRobot(int hauteur, int largeur);       // bords = murs, le reste inconnu
    void mettre_a_jour(Pos pos, TypeCase type);
    bool est_dans_carte(Pos pos) const;
    bool case_valide(Pos pos) const;            // on peut y passer ? (inconnue = libre)
    int hauteur() const { return hauteur_; }
    int largeur() const { return largeur_; }

private:
    int hauteur_, largeur_;
    std::vector<std::vector<TypeCase>> cases_;
};

#endif
