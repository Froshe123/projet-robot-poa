#include "interface.hpp"
#include <memory>
#include <optional>
#include <cstdlib>
#include <cstdio>

// le monde reel : seul l'environnement lit la grille et les objets
struct Environnement::Etat {
    std::vector<std::string> grille; // grille[l][c] = symbole de la case (l, c)

    Pos pos_robot; // position du robot dans la grille
    Pos pos_selecteur; // position du selecteur dans l'armoire
    Pos pos_armoire; // position de l'armoire dans la grille
    Pos pos_dictionnaire; // position du dictionnaire dans la grille
    Pos depart_robot; // ne bouge pas, sert a connaissances()
    Pos casier_depart; // ne bouge pas, sert a connaissances()
    std::vector<std::vector<bool>> vue; // vue[l][c] = case deja montree au robot (pour l'affichage)

    std::vector<ResidentConnu> residents; // id et position de chaque resident
    std::vector<std::vector<std::optional<std::string>>> armoire; // armoire[l][c] = objet du casier, vide si rien
    std::optional<std::string> objet_porte; // objet tenu par le robot, vide si aucun
    std::optional<Pos> resident_vise; // resident de la demande en cours
};

Environnement::~Environnement() = default;

// construit le monde a partir de la carte et de l'armoire (deja validees)
Environnement::Environnement(const JsonValue &carte, const JsonValue &armoire)
{   
    etat = std::make_unique<Etat>();

    // grille
    const JsonValue &grille_json = *carte.get("grille");
    for (size_t i = 0; i < grille_json.taille(); i++)
    {
        const JsonValue &ligne = *grille_json.index(i);
        etat->grille.push_back(*ligne.chaine());
    }

    // positions de depart
    carte.get("depart_robot")->paire_entiers(etat->depart_robot.l, etat->depart_robot.c);
    armoire.get("casier_depart")->paire_entiers(etat->casier_depart.l, etat->casier_depart.c);
    
    // positions fixes
    carte.get("armoire")->get("position")->paire_entiers(etat->pos_armoire.l, etat->pos_armoire.c);
    carte.get("dictionnaire")->get("position")->paire_entiers(etat->pos_dictionnaire.l, etat->pos_dictionnaire.c);

    // au debut le robot et le selecteur sont sur leur case de depart
    etat->pos_robot = etat->depart_robot;
    etat->pos_selecteur = etat->casier_depart;
    
    // residents
    const JsonValue &residents_json = *carte.get("residents");
    for (size_t i = 0; i < residents_json.taille(); i++)
    {
        const JsonValue &resident_json = *residents_json.index(i);
        ResidentConnu res;
        res.id = *resident_json.get("id")->chaine();
        resident_json.get("position")->paire_entiers(res.pos.l, res.pos.c);
        etat->residents.push_back(res);
        
    }

    // armoire vide de la bonne taille : une ligne par intensite, une colonne par emotion
    size_t n_ligne = armoire.get("intensites")->taille();
    size_t n_colonne = armoire.get("emotions")->taille();

    std::vector<std::optional<std::string>> ligne_armoire(n_colonne);
    etat->armoire.assign(n_ligne, ligne_armoire);
    
    // on range chaque objet a sa place, les casiers null restent vides
    const JsonValue &casiers_json = *armoire.get("casiers");
    for (size_t i = 0; i < casiers_json.taille(); i++)
    {
        const JsonValue &casier = *casiers_json.index(i);
        int l = casier.get_nombre("ligne"); 
        int c = casier.get_nombre("colonne");
        const JsonValue &objet = *casier.get("objet");
        if (objet.est_chaine())
        {
            etat->armoire[l][c] = *objet.chaine();

        }
    }

    // au debut le robot n'a vu que sa case de depart
    etat->vue.assign(etat->grille.size(), std::vector<bool>(etat->grille[0].size(), false));
    etat->vue[etat->depart_robot.l][etat->depart_robot.c] = true;
}

// position actuelle du robot
    Pos Environnement::robot() const {
        return etat->pos_robot;
    }

// position actuelle du selecteur dans l'armoire
    Pos Environnement::selecteur() const {
        return etat->pos_selecteur;
    }

// retient le resident a qui DONNER est autorise pour cette demande
    void Environnement::viser_resident(Pos resident) {
        etat->resident_vise = resident;
    }


// ce que le robot a le droit de savoir au depart : jamais la grille ni les objets
Connaissances Environnement::connaissances() const{
        Connaissances k;
        k.hauteur = etat->grille.size();
        k.largeur = etat->grille.empty() ? 0 : etat->grille[0].size();
        k.depart = etat->depart_robot;
        k.armoire = etat->pos_armoire;
        k.dictionnaire = etat->pos_dictionnaire;
        k.residents = etat->residents;
        k.casier_depart = etat->casier_depart;
        return k;
}

// traduit un symbole de la grille en mot de perception
std::string nom_case (char signe){
    switch (signe)
    {
    case '#' :
        return "mur";

    case '.' : 
        return "libre";
    
    case 'A' : 
        return "armoire";
    
    case 'D' : 
        return "dictionnaire";

    case 'P' : 
        return "resident";
        
    case 'R' : 
        return "libre"; // la case de depart est une case libre

    default:
        return "mur"; // on evite les cases inconnues
    }
}

// ce que voit le robot : ses 4 voisins (N, S, E, O), et le casier courant
// s'il touche l'armoire
PerceptionRobot Environnement::percevoir() const{
    char direction;
    PerceptionRobot p;
    etat->vue[ etat->pos_robot.l ][ etat->pos_robot.c ] = true; // pour l'affichage
    for (size_t i = 0; i <=3; i++){
        direction = DIRS[i]; // NSEO
        Pos case_voisine = voisin(etat->pos_robot,direction);
        etat->vue[case_voisine.l][case_voisine.c] = true;
        char char_grille = etat->grille[case_voisine.l][case_voisine.c];
        p.voisins[i] = nom_case(char_grille);
        if (p.voisins[i].compare("armoire") == 0){
            p.devant_armoire = true;
        }
    }
    if (p.devant_armoire){
        p.contenu_casier = etat ->armoire[etat->pos_selecteur.l][etat->pos_selecteur.c];
    }

    return p;
} 

// a et b sont voisins (pas de diagonale)
bool adjacent(Pos a, Pos b){
    int somme_ecart = abs(a.l - b.l) + abs(a.c - b.c);
    return somme_ecart == 1;
}

// applique l'action si elle est legale (enonce 3.4 et annexe C.2),
// renvoie false sinon et le monde ne bouge pas
bool Environnement::executer(const Action &action){
    switch (action.type)
    {
    case TypeAction::AVANCER :{
        Pos case_visee = voisin(etat->pos_robot,action.direction);
        char char_case_visee = etat->grille[case_visee.l][case_visee.c];
        if (nom_case(char_case_visee) != "libre"){
            return false;
        }
        else{
            etat->pos_robot = case_visee;
            return true;
        }
    }
    case TypeAction::ATTENDRE : {
        return true;
    }

    case TypeAction::CONSULTER :{
        return adjacent(etat->pos_robot,etat->pos_dictionnaire);
    }

    case TypeAction::CHERCHER :{
        if (!adjacent(etat->pos_robot,etat->pos_armoire)){
            return false;
        }
        else{
            switch (action.direction)
            {
            case 'N': // les lignes ne bouclent pas
                if (etat->pos_selecteur.l == 0){
                    return false;
                }
                else{
                    etat->pos_selecteur.l -= 1;
                    return true;
                }
            case 'S':
                if (etat->pos_selecteur.l == int(etat-> armoire.size())-1){
                    return false;
                }
                else{
                    etat->pos_selecteur.l += 1;
                    return true;
                }
            case 'E': // les colonnes bouclent (7 -> 0)
                etat->pos_selecteur.c = roue_avancer(etat->pos_selecteur.c, 1,(int)etat->armoire[0].size());
                return true;
                
            case 'O':
                etat->pos_selecteur.c = roue_avancer(etat->pos_selecteur.c, -1,(int)etat->armoire[0].size());
                return true;

            default:
                return false;
            }
        }
    }
    case TypeAction::PRENDRE :{
        std::optional<std::string> &casier = etat->armoire[etat->pos_selecteur.l][etat->pos_selecteur.c]; // & : on modifie le vrai casier
        if (!adjacent(etat->pos_robot,etat->pos_armoire)){ // pas a cote de l'armoire
            return false;
        }
        else if(!casier){ // casier vide
            return false;
        }
        else if(etat->objet_porte != std::nullopt){ // le robot a deja un objet
            return false;
        }
        else if(casier != action.objet){ // pas le bon objet
            return false;
        }
        etat->objet_porte = action.objet;
        casier = std::nullopt;
        return true;

    }

    case TypeAction::DONNER :{
        if(!etat->resident_vise){ // aucun resident vise
            return false;
        }
        if(!adjacent(etat->pos_robot,*etat->resident_vise)){ // pas a cote du resident
            return false;
        }
        else if(etat->objet_porte == std::nullopt){ // le robot n'a pas d'objet
            return false;
        }
        else if(etat->objet_porte != action.objet){ // pas le bon objet
            return false;
        }

        etat->objet_porte = std::nullopt;
        return true;
    }

    default:
        return false;
    }

}

// dessine la grille dans le terminal : '@' = robot, '?' = case jamais vue
void Environnement::afficher() const{
    for(size_t l = 0 ; l < etat->grille.size(); l++){
        std::string ligne = etat->grille[l]; // copie, on ne touche pas a la vraie grille
        for(size_t c = 0; c < ligne.size();c++){
            if (!etat->vue[l][c]){
                ligne[c] = '?';
            }
        }
        if (etat->pos_robot.l == (int)l){
            ligne[etat->pos_robot.c] = '@';
        }
        std::printf("%s\n"  , ligne.c_str());
    }
    std::printf("robot (%d, %d)   selecteur (%d, %d)   objet : %s\n",
            etat->pos_robot.l, etat->pos_robot.c,
            etat->pos_selecteur.l, etat->pos_selecteur.c,
            etat ->objet_porte ? etat->objet_porte->c_str() : "-");

}


