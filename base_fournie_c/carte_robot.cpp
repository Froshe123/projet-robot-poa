#include "carte_robot.hpp"

CarteRobot::CarteRobot(int hauteur, int largeur)
    : hauteur_(hauteur), largeur_(largeur),
      cases_(hauteur, std::vector<TypeCase>(largeur, TypeCase::INCONNUE)) {
    for (int i = 0; i < hauteur; i++) {
        for (int j = 0; j < largeur; j++) {
            if (i==0 || i==hauteur-1 || j==0 || j==largeur-1) {
                cases_[i][j] = TypeCase::OCCUPEE; // le bord est toujours un mur
            } else {
                cases_[i][j] = TypeCase::INCONNUE; // le reste est inconnu
            }
        }
    }

}

// la case existe ?
bool CarteRobot::est_dans_carte(Pos pos) const
{
    if (pos.l >= 0 && pos.l < hauteur_ && pos.c >= 0 && pos.c < largeur_) {
        return true;
    } else {
        return false;
    }
}

// hypothese optimiste : une case jamais vue est supposee libre
bool CarteRobot::case_valide(Pos pos) const
{
    if (est_dans_carte(pos) && cases_[pos.l][pos.c] != TypeCase::OCCUPEE) {
        return true;
    } else {
        return false;
    }
}

// note ce que le robot vient de voir
void CarteRobot::mettre_a_jour(Pos pos, TypeCase type)
{
    if (est_dans_carte(pos)) {
        cases_[pos.l][pos.c] = type;
    }
}
