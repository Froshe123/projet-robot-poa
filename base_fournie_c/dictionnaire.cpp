#include "dictionnaire.hpp"
#include "reconfort_io.hpp"

// l'entree du dictionnaire qui contient ce mot, nullptr s'il n'y en a pas
const JsonValue *chercher_mot(const JsonValue &dictionnaire, const std::string &mot)
{
    const JsonValue *entrees = dictionnaire.get("entrees");
    if (entrees == nullptr) {
        return nullptr;
    }
    for (std::size_t i = 0; i < entrees->taille(); i++) {
        const JsonValue *entree = entrees->index(i);
        const JsonValue *formes = entree->get("formes");
        if (formes == nullptr) {
            continue;
        }
        for (std::size_t j = 0; j < formes->taille(); j++) {
            const std::string *forme = formes->index(j)->chaine();
            if (forme != nullptr && *forme == mot) {
                return entree;
            }
        }
    }
    return nullptr;
}

// emotion du message : le premier mot connu en lisant de gauche a droite (enonce 6.1)
Emotion lire_emotion(const JsonValue &dictionnaire, const std::string &message)
{
    Emotion resultat;

    for (const std::string &mot : normaliser(message)) {
        const JsonValue *entree = chercher_mot(dictionnaire, mot);
        if (entree != nullptr) {
            resultat.trouvee = true;
            resultat.emotion = entree->get_chaine("emotion", "");
            resultat.intensite = entree->get_chaine("intensite", "");
            break;
        }
    }

    return resultat;
}
