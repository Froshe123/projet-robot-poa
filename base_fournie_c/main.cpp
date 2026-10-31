// robot de reconfort : ./robot <carte.json> <scenario.json> <dossier_donnees> <trace.json>
// fait tourner l'environnement et l'agent tour par tour puis ecrit la trace
// code 0 si tout va bien, 1 si un fichier est mauvais (et aucune trace n'est ecrite)
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include "interface.hpp"
#include "reconfort_io.hpp"
#include "validation.hpp"
#include <unistd.h>

// les 4 fichiers d'entree, charges et valides
struct Entrees {
    JsonValue carte;
    JsonValue scenario;
    JsonValue dictionnaire;
    JsonValue armoire;
};

// ===================== Eliott : entrees et affichage =====================

// charge et valide les 4 fichiers, leve ErreurFichier au premier probleme
// l'ordre compte : le scenario a besoin de la carte, et c'est lui qui donne le nom de l'armoire
static Entrees charger_entrees(const std::string &chemin_carte, const std::string &chemin_scenario, const std::string &dossier)
{
    Entrees e;
    // carte
    e.carte = charger_carte(chemin_carte);
    valider_carte(e.carte);
    // scenario
    e.scenario = charger_scenario(chemin_scenario);
    valider_scenario(e.scenario,e.carte);
    // dictionnaire
    e.dictionnaire = charger_dictionnaire(dossier + "/dictionnaire.json");
    valider_dictionnaire(e.dictionnaire);
    // armoire : <dossier>/<champ "armoire" du scenario>.json
    e.armoire = charger_armoire(dossier + "/" + e.scenario.get_chaine("armoire") + ".json");
    valider_armoire(e.armoire);
    return e;
}

// met la perception (tableau N S E O) au format de la trace
static Perception perception_pour_trace(const PerceptionRobot &p)
{
    Perception t;
    t.n = p.voisins[0]; // nord
    t.s = p.voisins[1]; // sud
    t.e = p.voisins[2]; // est
    t.o = p.voisins[3]; // ouest
    return t;
}

// redessine la grille (si AFFICHAGE=1) et attend un peu pour voir le robot bouger
static void afficher_tour(const Environnement &env)
{
    std::printf("\033[2J\033[H"); // efface l'ecran
    env.afficher();
    std::fflush(stdout);
    usleep(150000); // 0,15 s
}

// ===================== Axel : actions et livraisons =====================

// nom de l'action pour la trace
static std::string nom_action(TypeAction type)
{
    switch (type) {
    case TypeAction::AVANCER:   return "AVANCER";
    case TypeAction::CONSULTER: return "CONSULTER";
    case TypeAction::CHERCHER:  return "CHERCHER";
    case TypeAction::PRENDRE:   return "PRENDRE";
    case TypeAction::DONNER:    return "DONNER";
    case TypeAction::FIN:       return "FIN";
    default:                    return "ATTENDRE";
    }
}

// argument pour la trace : la direction, l'objet, ou rien
static std::optional<std::string> argument_action(const Action &a)
{
    if (a.type == TypeAction::AVANCER || a.type == TypeAction::CHERCHER) {
        return std::string(1, a.direction);
    }
    if (a.type == TypeAction::PRENDRE || a.type == TypeAction::DONNER) {
        return a.objet;
    }
    return std::nullopt;
}

// position du resident 'id', (-1, -1) s'il n'existe pas
static Pos position_resident(const Connaissances &k, const std::string &id)
{
    for (const ResidentConnu &r : k.residents) {
        if (r.id == id) {
            return r.pos;
        }
    }
    return Pos{-1, -1};
}

// "" devient null dans la trace
static std::optional<std::string> texte_ou_null(const std::string &s)
{
    if (s.empty()) {
        return std::nullopt;
    }
    return s;
}

// ecrit dans la trace la livraison d'une demande a partir du bilan de l'agent
static void noter_livraison(Trace &trace, int numero, const std::string &resident, const Bilan &b)
{
    std::optional<int> ligne, colonne;
    if (b.a_un_casier) {
        ligne = b.casier_choisi.l;
        colonne = b.casier_choisi.c;
    }

    std::string repli = b.repli.empty() ? "aucun" : b.repli;

    std::optional<std::string> motif;
    if (!b.succes) {
        motif = b.motif_echec.empty() ? "blocage" : b.motif_echec;
    }

    trace.ajouter_livraison(numero, resident, texte_ou_null(b.emotion), texte_ou_null(b.intensite),
                            ligne, colonne, repli, texte_ou_null(b.objet), b.succes, motif);
}

// ===================== la boucle =====================

// deroule une demande tour par tour : percevoir, decider, executer, noter le pas
// jusqu'a ce que l'agent reponde FIN, puis note la livraison
static void simuler_demande(Environnement &env, Agent &agent, Trace &trace,
                            const Connaissances &k, const JsonValue &demande, bool affichage)
{
    int numero = (int)demande.get_nombre("numero");
    std::string resident = demande.get_chaine("resident");
    agent.nouvelle_demande(numero, resident, demande.get_chaine("message"));
    env.viser_resident(position_resident(k, resident));


    // garde-fou contre une boucle infinie, assez grand pour n'importe quelle carte :
    // chaque mur decouvert peut faire refaire au pire un chemin de la taille de la carte
    long long cases = (long long)k.hauteur * k.largeur;
    long long limite = cases * cases + 1000;
    for (long long pas = 0; pas < limite; pas++) {
        Pos pos = env.robot(); // la trace veut la position AVANT l'action
        Pos sel = env.selecteur();
        PerceptionRobot p = env.percevoir();
        agent.percevoir(p);
        Action a = agent.decider();
        if (a.type == TypeAction::FIN) {
            break;
        }
        bool ok = env.executer(a);
        trace.ajouter_pas(numero, pos.l, pos.c, sel.l, sel.c,
                        nom_action(a.type), argument_action(a),
                        perception_pour_trace(p), p.contenu_casier,
                        ok ? std::nullopt : std::optional<std::string>("action refusee"));
        if (!ok) { // action illegale : on arrete la demande
            break;
        }

        agent.appliquer(a);
        if (affichage) {
            afficher_tour(env);
        }
    }
    noter_livraison(trace, numero, resident, agent.bilan());
}

// ===================== main =====================

int main(int argc, char **argv)
{
    // 1. exactement 4 arguments
    if (argc != 5) {
        std::fprintf(stderr, "usage : %s <carte.json> <scenario.json> <dossier_donnees> <trace.json>\n", argv[0]);
        return 1;
    }

    // 2. chargement et validation : en cas d'erreur, message, code 1 et pas de trace
    Entrees e;
    try {
        e = charger_entrees(argv[1], argv[2], argv[3]);
    } catch (const ErreurFichier &err) {
        std::fprintf(stderr, "erreur : %s\n", err.what());
        return 1;
    }

    // 3. le monde, le robot, la trace
    Environnement env(e.carte, e.armoire);
    Connaissances k = env.connaissances();
    Agent agent(k, e.dictionnaire);
    Trace trace(e.carte.get_chaine("nom"), e.scenario.get_chaine("nom"),
                {"KRAMER Axel", "MELON Eliott"});

    // 4. les demandes dans l'ordre (AFFICHAGE=1 ./robot ... pour voir le robot bouger)
    bool affichage = std::getenv("AFFICHAGE") != nullptr;
    const JsonValue &demandes = *e.scenario.get("demandes");
    for (std::size_t i = 0; i < demandes.taille(); i++) {
        simuler_demande(env, agent, trace, k, *demandes.index(i), affichage);
    }

    // 5. ecrire la trace
    try {
        trace.ecrire(argv[4]);
    } catch (const ErreurFichier &err) {
        std::fprintf(stderr, "erreur : %s\n", err.what());
        return 1;
    }
    return 0;
}
