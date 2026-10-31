#ifndef CHOIX_CASIER_HPP
#define CHOIX_CASIER_HPP

#include <string>
#include <vector>
#include "interface.hpp"

// un casier que le robot peut essayer (enonce 6.2)
struct CasierPossible {
    Pos position; // (intensite, emotion)
    std::string repli; // "aucun", "intensite", "voisine_1" ou "voisine_2"
};

int numero_emotion(const JsonValue &dictionnaire, const std::string &emotion);
int numero_intensite(const JsonValue &dictionnaire, const std::string &intensite);
std::string etiquette_repli(int distance, bool meme_intensite);
std::vector<CasierPossible> casiers_a_essayer(int emotion, int intensite);
char pas_du_selecteur(Pos selecteur, Pos cible);
int prochain_casier(const std::vector<CasierPossible> &liste,
                    const bool vu[3][8], const std::string contenu[3][8]);

#endif