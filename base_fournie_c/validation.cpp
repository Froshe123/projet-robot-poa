#include "validation.hpp"
#include "reconfort_io.hpp"
#include "types.hpp"
#include <string>

/* ===================== Outils ===================== */

/* Renvoie le champ 'cle' de 'objet'. Lève ErreurFichier s'il est absent
 * ou s'il n'est pas du type attendu. 'fichier' sert au message d'erreur. */
static const JsonValue &champ(const JsonValue &objet, const std::string &cle,
                              JsonType type, const std::string &fichier)
{
    const JsonValue *p = objet.get(cle);
    if (p == nullptr){
        throw ErreurFichier(fichier + " : champ obligatoire '" + cle + "' absent");
    }
    else if (p->type() != type){
        throw ErreurFichier(fichier + " : le champ '" + cle + "' n'a pas le bon type");
    }
    else{
        return *p;
    }
}

/* Lit la position 'cle' de 'objet' ([ligne, colonne]). Lève ErreurFichier
 * si elle est absente ou si ce n'est pas un couple de deux nombres. */
static Pos position(const JsonValue &objet, const std::string &cle, const std::string &fichier)
{
    const JsonValue &tab = champ(objet, cle, JsonType::Array, fichier);
    Pos p;
    if(!tab.paire_entiers(p.l,p.c)){
        throw ErreurFichier(fichier + " : la position '" + cle + "' doit etre [ligne, colonne]");
    }
    else{
        return p;
    }
}

/* La position p est-elle dans une grille hauteur x largeur ? */
static bool dans_grille(Pos p, int hauteur, int largeur)
{
    return (p.l >= 0) && (p.l< hauteur) && (p.c  >= 0) && (p.c < largeur);
}

/* ===================== Carte ===================== */

void valider_carte(const JsonValue &carte)
{
    /* À compléter, dans cet ordre :
     *   1. champs obligatoires et leur type : dimensions (hauteur, largeur),
     *      grille, depart_robot, armoire.position, dictionnaire.position,
     *      residents (id, position)
     *   2. grille rectangulaire, et dimensions annoncées = taille de la grille
     *   3. chaque position est dans la grille
     *   4. le symbole de la grille correspond : R au départ, A à l'armoire,
     *      D au dictionnaire, P à chaque résident
     *   5. pas deux résidents avec le même id */
    
}

/* ===================== Armoire ===================== */

void valider_armoire(const JsonValue &armoire)
{
    /* À compléter :
     *   1. champs obligatoires et leur type : emotions, intensites,
     *      casier_depart, casiers (ligne, colonne, emotion, intensite)
     *   2. chaque casier est dans l'armoire (ligne < nb intensités,
     *      colonne < nb émotions)
     *   3. pas deux casiers à la même position
     *   4. l'étiquette correspond à la position :
     *      emotions[colonne] == emotion, intensites[ligne] == intensite
     *   5. casier_depart est dans l'armoire */
    (void)armoire;
}

/* ===================== Dictionnaire ===================== */

void valider_dictionnaire(const JsonValue &dictionnaire)
{
    /* À compléter :
     *   1. champs obligatoires et leur type : emotions, intensites,
     *      entrees (formes, emotion, intensite)
     *   2. chaque entrée a une émotion et une intensité connues
     *      (présentes dans les listes emotions / intensites)
     *   3. un même mot n'apparaît pas dans deux entrées */
    (void)dictionnaire;
}

/* ===================== Scénario ===================== */

void valider_scenario(const JsonValue &scenario, const JsonValue &carte)
{
    /* À compléter :
     *   1. champs obligatoires et leur type : carte, armoire,
     *      demandes (numero, resident, message)
     *   2. le scénario est prévu pour cette carte : scenario.carte == carte.nom
     *   3. chaque demande vise un résident présent dans la carte */
    (void)scenario; (void)carte;
}
