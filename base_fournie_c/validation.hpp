#ifndef VALIDATION_HPP
#define VALIDATION_HPP

#include "json.hpp"

// validations de l'enonce 5.6, a appeler apres le chargement et avant la simulation
// chaque fonction leve ErreurFichier avec un message clair au premier probleme
// (fichier absent, pas UTF-8, JSON invalide, format : deja faits par charger_*())

void valider_carte(const JsonValue &carte);
void valider_armoire(const JsonValue &armoire);
void valider_dictionnaire(const JsonValue &dictionnaire);
void valider_scenario(const JsonValue &scenario, const JsonValue &carte);

#endif
