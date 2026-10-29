#include "validation.hpp"
#include "reconfort_io.hpp"
#include "types.hpp"
#include <string>
#include <vector>

/* ===================== Outils ===================== */

/* Renvoie le champ 'cle' de 'objet'. Lève ErreurFichier s'il est absent
 * ou s'il n'est pas du type attendu. 'fichier' sert au message d'erreur. */
static const JsonValue &champ(const JsonValue &objet, const char *cle,
                              JsonType type, const char *fichier)
{
    const JsonValue *p = objet.get(cle);
    if (p == nullptr){
        throw ErreurFichier(std::string(fichier) + " : champ obligatoire '" + cle + "' absent");
    }
    else if (p->type() != type){
        throw ErreurFichier(std::string(fichier) + " : le champ '" + cle + "' n'a pas le bon type");
    }
    else{
        return *p;
    }
}

/* Lit la position 'cle' de 'objet' ([ligne, colonne]). Lève ErreurFichier
 * si elle est absente ou si ce n'est pas un couple de deux nombres. */
static Pos position(const JsonValue &objet, const char *cle, const char *fichier)
{
    const JsonValue &tab = champ(objet, cle, JsonType::Array, fichier);
    Pos p;
    if(!tab.paire_entiers(p.l,p.c)){
        throw ErreurFichier(std::string(fichier) + " : la position '" + cle + "' doit etre [ligne, colonne]");
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

/* Le caractère de la grille (tableau JSON de chaînes) à la position p.
 * À n'appeler que si p est dans la grille. */
static char symbole(const JsonValue &grille, Pos p)
{
    return (*grille.index(p.l)->chaine())[p.c];
}

/* "(l, c)", pour les messages d'erreur. */
static std::string texte(Pos p)
{
    return "(" + std::to_string(p.l) + ", " + std::to_string(p.c) + ")";
}

/* ===================== Carte ===================== */

void valider_carte(const JsonValue &carte)
{
    champ(carte, "nom", JsonType::String, "carte");  
    const JsonValue &dimensions = champ(carte, "dimensions", JsonType::Object, "carte");
    int hauteur = (int)champ(dimensions, "hauteur", JsonType::Number, "carte").nombre();
    int largeur = (int)champ(dimensions, "largeur", JsonType::Number, "carte").nombre();
    const JsonValue &grille = champ(carte, "grille", JsonType::Array, "carte");
    Pos depart = position(carte, "depart_robot", "carte");
    Pos pos_armoire = position(champ(carte, "armoire", JsonType::Object, "carte"), "position", "carte");
    Pos pos_dico = position(champ(carte, "dictionnaire", JsonType::Object, "carte"), "position", "carte");
    const JsonValue &residents = champ(carte, "residents", JsonType::Array, "carte");

    for (size_t i = 0; i < grille.taille(); i++) {
        if (!grille.index(i)->est_chaine()) {
            throw ErreurFichier("carte : la ligne " + std::to_string(i) + " de la grille n'est pas une chaine");
        }
    }
    for (size_t i = 0; i < residents.taille(); i++) {
        const JsonValue &r = *residents.index(i);
        if (!r.est_objet()) {
            throw ErreurFichier("carte : le resident numero " + std::to_string(i) + " n'est pas un objet");
        }
        champ(r, "id", JsonType::String, "carte");
        position(r, "position", "carte");
    }

    /* 2. grille rectangulaire, dimensions cohérentes */
    if ((int)grille.taille() != hauteur) {
        throw ErreurFichier("carte : la grille a " + std::to_string(grille.taille()) + " lignes alors que la hauteur annoncee est " + std::to_string(hauteur));
    }
    for (size_t i = 0; i < grille.taille(); i++) {
        if ((int)grille.index(i)->chaine()->size() != largeur) {
            throw ErreurFichier("carte : la ligne " + std::to_string(i) + " de la grille ne fait pas " + std::to_string(largeur) + " caracteres (grille non rectangulaire)");
        }
    }

    /* 3. positions dans la grille, puis 4. bon symbole sous chaque position */
    if (!dans_grille(depart, hauteur, largeur)) {
        throw ErreurFichier("carte : depart_robot " + texte(depart) + " hors de la grille");
    }
    if (!dans_grille(pos_armoire, hauteur, largeur)) {
        throw ErreurFichier("carte : l'armoire " + texte(pos_armoire) + " est hors de la grille");
    }
    if (!dans_grille(pos_dico, hauteur, largeur)) {
        throw ErreurFichier("carte : le dictionnaire " + texte(pos_dico) + " est hors de la grille");
    }
    if (symbole(grille, depart) != 'R') {
        throw ErreurFichier("carte : depart_robot " + texte(depart) + " n'est pas sur un 'R'");
    }
    if (symbole(grille, pos_armoire) != 'A') {
        throw ErreurFichier("carte : l'armoire " + texte(pos_armoire) + " n'est pas sur un 'A'");
    }
    if (symbole(grille, pos_dico) != 'D') {
        throw ErreurFichier("carte : le dictionnaire " + texte(pos_dico) + " n'est pas sur un 'D'");
    }
    for (size_t i = 0; i < residents.taille(); i++) {
        const JsonValue &r = *residents.index(i);
        const std::string &id = *r.get("id")->chaine();
        Pos p = position(r, "position", "carte");
        if (!dans_grille(p, hauteur, largeur)) {
            throw ErreurFichier("carte : le resident " + id + " " + texte(p) + " est hors de la grille");
        }
        if (symbole(grille, p) != 'P') {
            throw ErreurFichier("carte : le resident " + id + " " + texte(p) + " n'est pas sur un 'P'");
        }
    }

    /* 5. pas deux résidents avec le même identifiant */
    for (size_t i = 0; i < residents.taille(); i++) {
        for (size_t j = i + 1; j < residents.taille(); j++) {
            const std::string &id_i = *residents.index(i)->get("id")->chaine();
            const std::string &id_j = *residents.index(j)->get("id")->chaine();
            if (id_i == id_j) {
                throw ErreurFichier("carte : deux residents ont l'identifiant " + id_i);
            }
        }
    }
}

/* ===================== Armoire ===================== */

void valider_armoire(const JsonValue &armoire)
{
    /* 1. champs obligatoires et leur type */
    const JsonValue &emotions = champ(armoire, "emotions", JsonType::Array, "armoire");  
    const JsonValue &intensites = champ(armoire, "intensites", JsonType::Array, "armoire");  
    for (size_t i = 0; i < emotions.taille() ; i++  ){
        if(!emotions.index(i)->est_chaine()){
            throw ErreurFichier("armoire : l'emotion numero " + std::to_string(i) + " n'est pas une chaine");
        }
    }
    for (size_t i = 0; i < intensites.taille() ; i++  ){
        if(!intensites.index(i)->est_chaine()){
            throw ErreurFichier("armoire : l'intensite numero " + std::to_string(i) + " n'est pas une chaine");
        } 
    }
    Pos depart = position(armoire, "casier_depart", "armoire");
    const JsonValue &casiers = champ(armoire, "casiers", JsonType::Array, "armoire");

    int nb_lignes = (int)intensites.taille();
    int nb_colonnes = (int)emotions.taille();
    std::vector<std::vector<bool>> deja_vu(nb_lignes, std::vector<bool>(nb_colonnes, false));

    for (size_t i = 0; i < casiers.taille(); i++) {
        const JsonValue &c = *casiers.index(i);
        std::string numero = std::to_string(i);
        if (!c.est_objet()) {
            throw ErreurFichier("armoire : le casier numero " + numero + " n'est pas un objet");
        }
        Pos p;
        p.l = (int)champ(c, "ligne", JsonType::Number, "armoire").nombre();
        p.c = (int)champ(c, "colonne", JsonType::Number, "armoire").nombre();
        const std::string &emotion = *champ(c, "emotion", JsonType::String, "armoire").chaine();
        const std::string &intensite = *champ(c, "intensite", JsonType::String, "armoire").chaine();
        const JsonValue *objet = c.get("objet");
        if (objet == nullptr) {
            throw ErreurFichier("armoire : le casier numero " + numero + " n'a pas de champ 'objet'");
        }
        if (!objet->est_chaine() && !objet->est_nul()) {
            throw ErreurFichier("armoire : l'objet du casier numero " + numero + " doit etre un texte ou null");
        }

        /* 2. casier dans l'armoire */
        if (!dans_grille(p, nb_lignes, nb_colonnes)) {
            throw ErreurFichier("armoire : le casier " + texte(p) + " est hors de l'armoire");
        }

        /* 3. pas deux casiers à la même position */
        if (deja_vu[p.l][p.c]) {
            throw ErreurFichier("armoire : deux casiers en " + texte(p));
        }
        deja_vu[p.l][p.c] = true;

        /* 4. l'étiquette correspond à la position */
        const std::string &emotion_attendue = *emotions.index(p.c)->chaine();
        const std::string &intensite_attendue = *intensites.index(p.l)->chaine();
        if (emotion != emotion_attendue) {
            throw ErreurFichier("armoire : le casier " + texte(p) + " porte l'emotion '" + emotion +
                                "' au lieu de '" + emotion_attendue + "'");
        }
        if (intensite != intensite_attendue) {
            throw ErreurFichier("armoire : le casier " + texte(p) + " porte l'intensite '" + intensite +
                                "' au lieu de '" + intensite_attendue + "'");
        }
    }

    /* 5. casier_depart dans l'armoire */
    if (!dans_grille(depart, nb_lignes, nb_colonnes)) {
        throw ErreurFichier("armoire : casier_depart " + texte(depart) + " est hors de l'armoire");
    }
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
    const JsonValue &carte_visee = champ (scenario, "carte", JsonType::String,"scenario");
    champ (scenario, "armoire", JsonType::String,"scenario");
    const JsonValue &demandes = champ (scenario, "demandes", JsonType::Array,"scenario");
    for (size_t i = 0; i < demandes.taille(); i++){
        const JsonValue &d = *demandes.index(i);
        if(d.type() != JsonType::Object){
            throw ErreurFichier("scenario : la demande numero " + std::to_string(i) + " n'est pas un objet");
        }
        champ(d, "numero", JsonType::Number, "scenario");
        champ(d, "resident", JsonType::String, "scenario");
        champ(d, "message", JsonType::String, "scenario");
    }
}
