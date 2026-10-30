/* Point d'entrée du robot de réconfort (énoncé, section 2 : contrat d'appel).
 *
 *     ./robot <carte.json> <scenario.json> <dossier_donnees> <trace.json>
 *
 * Relie l'Environnement (le monde) et l'Agent (le robot), tour par tour,
 * jusqu'à la fin du scénario, puis écrit la trace (section 5.5).
 * Code de retour : 0 si tout va bien, 1 en cas d'erreur de données (5.6),
 * et dans ce cas AUCUNE trace n'est écrite. */
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include "interface.hpp"
#include "reconfort_io.hpp"
#include "validation.hpp"
#include <unistd.h>

/* Les quatre fichiers d'entrée, chargés et validés. */
struct Entrees {
    JsonValue carte;
    JsonValue scenario;
    JsonValue dictionnaire;
    JsonValue armoire;
};

/* ===================== Eliott : entrées et monde ===================== */

/* [Eliott] Charge et valide les 4 fichiers, dans cet ordre :
 *   carte, scénario, dictionnaire (<dossier>/dictionnaire.json),
 *   armoire (<dossier>/<champ "armoire" du scénario>.json).
 * Lève ErreurFichier au premier problème (aucune trace ne doit être écrite). */
static Entrees charger_entrees(const std::string &chemin_carte, const std::string &chemin_scenario, const std::string &dossier)
{
    Entrees e;
    //Json carte
    e.carte = charger_carte(chemin_carte);
    valider_carte(e.carte);
    //Json scenario
    e.scenario = charger_scenario(chemin_scenario);
    valider_scenario(e.scenario,e.carte);
    //Json dictionnaire
    e.dictionnaire = charger_dictionnaire(dossier + "/dictionnaire.json");
    valider_dictionnaire(e.dictionnaire);
    //Json armoire
    e.armoire = charger_armoire(dossier + "/" + e.scenario.get_chaine("armoire") + ".json");
    valider_armoire(e.armoire);
    //tout les fichiers d'entrées
    return e;
}

/* [Eliott] Convertit la perception de l'environnement (tableau N, S, E, O)
 * au format attendu par la trace (struct Perception de reconfort_io.hpp). */
static Perception perception_pour_trace(const PerceptionRobot &p)
{
    Perception t;
    t.n = p.voisins[0];//Nord
    t.s = p.voisins[1];//Sud
    t.e = p.voisins[2];//Est
    t.o = p.voisins[3];//Ouest
    return t;
}

/* [Eliott] Redessine la grille dans le terminal (activé par AFFICHAGE=1),
 * puis attend un peu pour qu'on ait le temps de voir le robot bouger. */
static void afficher_tour(const Environnement &env)
{
    std::printf("\033[2J\033[H");   /* efface l'écran, curseur en haut à gauche */
    env.afficher();
    std::fflush(stdout);
    usleep(150000);                 /* 0,15 s */
}

/* ===================== Axel : actions et livraisons ===================== */

/* [Axel] Le nom d'une action pour la trace : "AVANCER", "CONSULTER", ... */
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

/* [Axel] L'argument d'une action pour la trace : la direction ("N", "S",
 * "E", "O") pour AVANCER et CHERCHER, l'objet pour PRENDRE et DONNER,
 * std::nullopt sinon. */
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

/* [Axel] La position du résident 'id', d'après les connaissances du robot. */
static Pos position_resident(const Connaissances &k, const std::string &id)
{
    for (const ResidentConnu &r : k.residents) {
        if (r.id == id) {
            return r.pos;
        }
    }
    return Pos{-1, -1};
}

/* [Axel] Une chaîne vide devient null dans la trace. */
static std::optional<std::string> texte_ou_null(const std::string &s)
{
    if (s.empty()) {
        return std::nullopt;
    }
    return s;
}

/* [Axel] Ajoute à la trace la livraison d'une demande, à partir du Bilan
 * de l'agent (Trace::ajouter_livraison, reconfort_io.hpp). */
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

/* ===================== Ensemble : la boucle ===================== */

/* [Ensemble] Déroule UNE demande, tour par tour :
 *   1. agent.nouvelle_demande(...) et env.viser_resident(...)
 *   2. tant que l'agent ne répond pas FIN :
 *        - noter position et casier AVANT l'action (env.robot(), env.selecteur())
 *        - PerceptionRobot p = env.percevoir();  agent.percevoir(p);
 *        - Action a = agent.decider();  si FIN -> sortir
 *        - bool ok = env.executer(a);
 *        - trace.ajouter_pas(...)  (perception et contenu_casier RÉELS, de p)
 *        - si ok : agent.appliquer(a) ; sinon : action illégale (à décider)
 *        - garde-fou : un nombre maximal de pas, contre les boucles infinies
 *   3. noter_livraison(trace, ..., agent.bilan())
 * Si 'affichage' est vrai : afficher_tour(env) après chaque action. */
static void simuler_demande(Environnement &env, Agent &agent, Trace &trace,
                            const Connaissances &k, const JsonValue &demande, bool affichage)
{
    int numero = (int)demande.get_nombre("numero");
    std::string resident = demande.get_chaine("resident");
    agent.nouvelle_demande(numero, resident, demande.get_chaine("message"));
    env.viser_resident(position_resident(k, resident));


    for (int pas = 0; pas < 500; pas++) {             /* garde-fou */
        Pos pos = env.robot();
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
        if (!ok) {
            break;
        }

        agent.appliquer(a);
        if (affichage) {
            afficher_tour(env);
        }
    }
    noter_livraison(trace, numero, resident, agent.bilan());
}

/* ===================== main ===================== */

int main(int argc, char **argv)
{
    /* 1. exactement 4 arguments */
    if (argc != 5) {
        std::fprintf(stderr, "usage : %s <carte.json> <scenario.json> <dossier_donnees> <trace.json>\n", argv[0]);
        return 1;
    }

    /* 2. chargement et validation : en cas d'erreur, message, code 1, pas de trace */
    Entrees e;
    try {
        e = charger_entrees(argv[1], argv[2], argv[3]);
    } catch (const ErreurFichier &err) {
        std::fprintf(stderr, "erreur : %s\n", err.what());
        return 1;
    }

    /* 3. le monde, le robot, la trace */
    Environnement env(e.carte, e.armoire);
    Connaissances k = env.connaissances();
    Agent agent(k, e.dictionnaire);
    Trace trace(e.carte.get_chaine("nom"), e.scenario.get_chaine("nom"),
                {"KRAMER Axel", "MELON Eliott"});

    /* 4. les demandes, dans l'ordre ; AFFICHAGE=1 ./robot ... pour voir le robot bouger */
    bool affichage = std::getenv("AFFICHAGE") != nullptr;
    const JsonValue &demandes = *e.scenario.get("demandes");
    for (std::size_t i = 0; i < demandes.taille(); i++) {
        simuler_demande(env, agent, trace, k, *demandes.index(i), affichage);
    }

    /* 5. écrire la trace */
    try {
        trace.ecrire(argv[4]);
    } catch (const ErreurFichier &err) {
        std::fprintf(stderr, "erreur : %s\n", err.what());
        return 1;
    }
    return 0;
}
