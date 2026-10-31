#ifndef INTERFACE_HPP
#define INTERFACE_HPP

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "types.hpp"
#include "json.hpp"

// ce que le robot voit au debut de chaque tour (enonce 3.2)
struct PerceptionRobot {
    std::array<std::string, 4> voisins; // N S E O : mur, libre, armoire, dictionnaire ou resident
    bool devant_armoire = false; // le robot touche l'armoire ?
    std::optional<std::string> contenu_casier; // objet du casier courant, vide si rien ou si pas devant l'armoire
};

// les actions du robot (FIN = l'agent a fini la demande, ce n'est pas une vraie action)
enum class TypeAction { AVANCER, CONSULTER, CHERCHER, PRENDRE, DONNER, ATTENDRE, FIN };

struct Action {
    TypeAction type = TypeAction::ATTENDRE;
    char direction = 0; // pour AVANCER et CHERCHER
    std::string objet; // pour PRENDRE et DONNER
};

// ===================== ce que le robot sait au depart =====================

struct ResidentConnu {
    std::string id; // ex "R1"
    Pos pos;
};

// tout ce que le robot sait au depart : ni la grille, ni les objets
struct Connaissances {
    int hauteur, largeur;
    Pos depart, armoire, dictionnaire;
    std::vector<ResidentConnu> residents;
    Pos casier_depart; // selecteur de l'armoire au depart
};

// le bilan d'une demande pour la trace ("" sera ecrit null)
struct Bilan {
    std::string emotion;
    std::string intensite;
    bool a_un_casier = false; // false : casier_choisi ecrit null
    Pos casier_choisi;
    std::string repli; // aucun, intensite, voisine_1 ou voisine_2
    std::string objet;
    bool succes = false;
    std::string motif_echec; // resident inaccessible, armoire vide ou emotion indeterminee
};

// ===================== le robot (Axel, agent.cpp) =====================

class Agent {
public:
    Agent(const Connaissances &k, const JsonValue &dictionnaire);
    ~Agent();
    void nouvelle_demande(int numero, const std::string &resident, const std::string &message);
    void percevoir(const PerceptionRobot &p); // perception -> memoire
    Action decider(); // decision (FIN = demande finie)
    void appliquer(const Action &action); // memoire a jour apres une action acceptee
    Bilan bilan() const;

private:
    struct Etat; // defini dans agent.cpp
    std::unique_ptr<Etat> etat;
};

// ===================== le monde (Eliott, environnement.cpp) =====================

class Environnement {
public:
    Environnement(const JsonValue &carte, const JsonValue &armoire);
    ~Environnement();
    Connaissances connaissances() const; // ce que le robot a le droit de savoir
    void viser_resident(Pos resident); // resident de la demande en cours
    PerceptionRobot percevoir() const;
    bool executer(const Action &action); // false si l'action est illegale
    Pos robot() const;
    void afficher() const; // dessine la grille dans le terminal
    Pos selecteur() const;

private:
    struct Etat; // defini dans environnement.cpp
    std::unique_ptr<Etat> etat;
};

// ===================== la roue (Eliott, roue.cpp) =====================

int roue_avancer(int i, int pas, int n); // avance de 'pas' crans sur une roue de n cases
int roue_distance(int a, int b, int n); // crans par le cote le plus court

#endif
