#include "validation.hpp"
#include "reconfort_io.hpp"
#include "types.hpp"
#include <set>
#include <string>
#include <vector>

// ===================== outils =====================

// renvoie le champ 'cle' de 'objet', erreur s'il est absent ou du mauvais type
// fichier : nom du fichier pour le message d'erreur
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

// lit la position [ligne, colonne] 'cle' de 'objet', erreur si elle est mal formee
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

// p est dans une grille hauteur x largeur ?
static bool dans_grille(Pos p, int hauteur, int largeur)
{
    return (p.l >= 0) && (p.l< hauteur) && (p.c  >= 0) && (p.c < largeur);
}

// symbole de la grille JSON en p (p doit etre dans la grille)
static char symbole(const JsonValue &grille, Pos p)
{
    return (*grille.index(p.l)->chaine())[p.c];
}

// le tableau JSON de chaines 'liste' contient 'mot' ?
static bool contient(const JsonValue &liste, const std::string &mot)
{
    for (size_t i = 0; i < liste.taille(); i++) {
        const std::string *s = liste.index(i)->chaine();
        if (s != nullptr && *s == mot) {
            return true;
        }
    }
    return false;
}

// "(l, c)" pour les messages d'erreur
static std::string texte(Pos p)
{
    return "(" + std::to_string(p.l) + ", " + std::to_string(p.c) + ")";
}

// ===================== carte =====================

// verifie les champs, la forme de la grille, les positions, les symboles R A D P
// et qu'il n'y a pas deux residents avec le meme id
void valider_carte(const JsonValue &carte)
{
    // 1. champs obligatoires et leur type
    champ(carte, "nom", JsonType::String, "carte"); // sert a valider_scenario
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

    // 2. grille rectangulaire et dimensions coherentes
    if ((int)grille.taille() != hauteur) {
        throw ErreurFichier("carte : la grille a " + std::to_string(grille.taille()) + " lignes alors que la hauteur annoncee est " + std::to_string(hauteur));
    }
    for (size_t i = 0; i < grille.taille(); i++) {
        if ((int)grille.index(i)->chaine()->size() != largeur) {
            throw ErreurFichier("carte : la ligne " + std::to_string(i) + " de la grille ne fait pas " + std::to_string(largeur) + " caracteres (grille non rectangulaire)");
        }
    }

    // 3. positions dans la grille, puis 4. bon symbole sous chaque position
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

    // 5. pas deux residents avec le meme id
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

// ===================== armoire =====================

// verifie les champs, que chaque casier est dans l'armoire, pas en double,
// avec la bonne etiquette, et que casier_depart est dans l'armoire
void valider_armoire(const JsonValue &armoire)
{
    // 1. champs obligatoires et leur type
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

        // 2. casier dans l'armoire
        if (!dans_grille(p, nb_lignes, nb_colonnes)) {
            throw ErreurFichier("armoire : le casier " + texte(p) + " est hors de l'armoire");
        }

        // 3. pas deux casiers au meme endroit
        if (deja_vu[p.l][p.c]) {
            throw ErreurFichier("armoire : deux casiers en " + texte(p));
        }
        deja_vu[p.l][p.c] = true;

        // 4. l'etiquette correspond a la position
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

    // 5. casier_depart dans l'armoire
    if (!dans_grille(depart, nb_lignes, nb_colonnes)) {
        throw ErreurFichier("armoire : casier_depart " + texte(depart) + " est hors de l'armoire");
    }
}

// ===================== dictionnaire =====================

// verifie les champs, que chaque entree a une emotion et une intensite connues,
// et qu'un mot n'est pas dans deux entrees
void valider_dictionnaire(const JsonValue &dictionnaire)
{
    // 1. champs obligatoires et leur type
    const JsonValue &emotions = champ(dictionnaire, "emotions", JsonType::Array, "dictionnaire");
    const JsonValue &intensites = champ(dictionnaire, "intensites", JsonType::Array, "dictionnaire");
    const JsonValue &entrees = champ(dictionnaire, "entrees", JsonType::Array, "dictionnaire");
    for (size_t i = 0; i < emotions.taille(); i++) {
        if (!emotions.index(i)->est_chaine()) {
            throw ErreurFichier("dictionnaire : l'emotion numero " + std::to_string(i) + " n'est pas une chaine");
        }
    }
    for (size_t i = 0; i < intensites.taille(); i++) {
        if (!intensites.index(i)->est_chaine()) {
            throw ErreurFichier("dictionnaire : l'intensite numero " + std::to_string(i) + " n'est pas une chaine");
        }
    }

    std::set<std::string> mots_vus; // tous les mots deja rencontres
    for (size_t i = 0; i < entrees.taille(); i++) {
        const JsonValue &e = *entrees.index(i);
        std::string numero = std::to_string(i);
        if (!e.est_objet()) {
            throw ErreurFichier("dictionnaire : l'entree numero " + numero + " n'est pas un objet");
        }
        const JsonValue &formes = champ(e, "formes", JsonType::Array, "dictionnaire");
        const std::string &emotion = *champ(e, "emotion", JsonType::String, "dictionnaire").chaine();
        const std::string &intensite = *champ(e, "intensite", JsonType::String, "dictionnaire").chaine();

        // 2. emotion et intensite connues
        if (!contient(emotions, emotion)) {
            throw ErreurFichier("dictionnaire : l'entree numero " + numero + " a une emotion inconnue '" + emotion + "'");
        }
        if (!contient(intensites, intensite)) {
            throw ErreurFichier("dictionnaire : l'entree numero " + numero + " a une intensite inconnue '" + intensite + "'");
        }

        // 3. un mot n'apparait pas dans deux entrees
        for (size_t j = 0; j < formes.taille(); j++) {
            const JsonValue *forme = formes.index(j);
            if (!forme->est_chaine()) {
                throw ErreurFichier("dictionnaire : une forme de l'entree numero " + numero + " n'est pas une chaine");
            }
            if (!mots_vus.insert(*forme->chaine()).second) {
                throw ErreurFichier("dictionnaire : le mot '" + *forme->chaine() + "' apparait dans deux entrees");
            }
        }
    }
}

// ===================== scenario =====================

// verifie les champs, que le scenario vise bien cette carte et que chaque
// demande vise un resident de la carte (carte deja validee)
void valider_scenario(const JsonValue &scenario, const JsonValue &carte)
{
    // 1. champs obligatoires et leur type
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

        // 3. la demande vise un resident de la carte
        const std::string &id_demande = *d.get("resident")->chaine();
        const JsonValue &residents = *carte.get("residents");
        bool trouve = false;
        for (size_t j = 0; j < residents.taille(); j++) {
            if (*residents.index(j)->get("id")->chaine() == id_demande) {
                trouve = true;
            }
        }
        if (!trouve) {
            throw ErreurFichier("scenario : la demande numero " + std::to_string(i) +
                                " vise le resident '" + id_demande + "', absent de la carte");
        }
    }

    // 2. le scenario est prevu pour cette carte
    const std::string &nom_carte = *carte.get("nom")->chaine();
    if ( *carte_visee.chaine() != nom_carte){
        throw ErreurFichier( "scenario : prevu pour la carte '" + *carte_visee.chaine() + "', mais la carte fournie est '" + nom_carte + "'");
    }
}
