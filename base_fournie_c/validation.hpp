#ifndef VALIDATION_HPP
#define VALIDATION_HPP

#include "json.hpp"

/* Validations de la section 5.6, à appeler juste après le chargement des
 * fichiers et AVANT la simulation. Chaque fonction lève ErreurFichier
 * (reconfort_io.hpp) avec un message clair au premier problème trouvé.
 *
 * Fichier absent, pas UTF-8, JSON invalide, format/version : déjà vérifiés
 * par les fonctions charger_*() de reconfort_io.cpp. */

void valider_carte(const JsonValue &carte);
void valider_armoire(const JsonValue &armoire);
void valider_dictionnaire(const JsonValue &dictionnaire);
void valider_scenario(const JsonValue &scenario, const JsonValue &carte);

#endif
