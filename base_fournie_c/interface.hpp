#ifndef INTERFACE_HPP
#define INTERFACE_HPP

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "types.hpp"
#include "json.hpp"

/* Ce que le robot perçoit au début de chaque tour (énoncé 3.2). */
struct PerceptionRobot {
    std::array<std::string, 4> voisins;          /* dans l'ordre N, S, E, O : "mur", "libre", "armoire",
                                                    "dictionnaire" ou "resident" */
    bool devant_armoire = false;                 /* le robot touche-t-il l'armoire ? */
    std::optional<std::string> contenu_casier;   /* objet du casier courant ; nullopt s'il est vide
                                                    ou si le robot n'est pas devant l'armoire */
};

/* Les actions du robot */
enum class TypeAction { AVANCER, CONSULTER, CHERCHER, PRENDRE, DONNER, ATTENDRE, FIN };

struct Action {
    TypeAction type = TypeAction::ATTENDRE;
    char direction = 0;   /* 'N', 'S', 'E' ou 'O' pour AVANCER et CHERCHER */
    std::string objet;    /* nom de l'objet pour PRENDRE et DONNER */
};

/* ===================== Ce que le robot sait au départ ===================== */

/* Un résident tel que le robot le connaît : son identifiant et sa position. */
struct ResidentConnu {
    std::string id;   /* par exemple "R1" */
    Pos pos;
};

/* Tout ce que le robot sait AU DÉPART (énoncé 3.2). Ni la grille, ni les objets. */
struct Connaissances {
    int hauteur, largeur;
    Pos depart, armoire, dictionnaire;
    std::vector<ResidentConnu> residents;
    Pos casier_depart;   /* position du sélecteur de l'armoire au départ */
};

/* Le bilan d'une demande, écrit dans la trace (énoncé 5.5). Une chaîne vide "" = null. */
struct Bilan {
    std::string emotion;
    std::string intensite;
    bool a_un_casier = false;   /* false : "casier_choisi" sera écrit null */
    Pos casier_choisi;
    std::string repli;          /* "aucun", "intensite", "voisine_1" ou "voisine_2" */
    std::string objet;
    bool succes = false;
    std::string motif_echec;    /* "resident inaccessible", "armoire vide", "emotion indeterminee" */
};

/* ===================== Le robot (Axel, agent.cpp) ===================== */

class Agent {
public:
    Agent(const Connaissances &k, const JsonValue &dictionnaire);
    ~Agent();
    void nouvelle_demande(int numero, const std::string &resident, const std::string &message);
    void percevoir(const PerceptionRobot &p);   /* PERCEPTION -> MÉMOIRE */
    Action decider();                           /* DÉCISION (FIN = demande finie) */
    void appliquer(const Action &action);       /* mise à jour après l'action */
    Bilan bilan() const;

private:
    struct Etat;                    /* le contenu de Etat est écrit dans agent.cpp */
    std::unique_ptr<Etat> etat;
};

/* ===================== Le monde (Eliott, environnement.cpp) ===================== */

class Environnement {
public:
    Environnement(const JsonValue &carte, const JsonValue &armoire);
    ~Environnement();
    Connaissances connaissances() const;        /* ce que le robot a le droit de savoir */
    void viser_resident(Pos resident);          /* résident de la demande en cours */
    PerceptionRobot percevoir() const;
    bool executer(const Action &action);        /* false si l'action est illégale */
    Pos robot() const;
    void afficher() const;                      /* dessine la grille et le robot dans le terminal */
    Pos selecteur() const;

private:
    struct Etat;                    /* le contenu de Etat est écrit dans environnement.cpp */
    std::unique_ptr<Etat> etat;
};

/* ===================== La roue (Eliott, roue.cpp ; utilisée par les deux) ===================== */

int roue_avancer(int i, int pas, int n);   /* avance de 'pas' crans sur une roue de n cases */
int roue_distance(int a, int b, int n);    /* nombre de crans par le côté le plus court */

#endif
