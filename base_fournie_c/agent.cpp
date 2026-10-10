#include "interface.hpp"
#include "memoire.hpp"
#include "graphe.hpp"
#include "dictionnaire.hpp"
#include "choix_casier.hpp"

/* Les 3 étapes d'une demande (énoncé, section 4), puis FINIE. */
enum class Phase { DICTIONNAIRE, ARMOIRE, RESIDENT, FINIE };

/* Fabrique une action, par exemple action(TypeAction::AVANCER, 'E'). */
static Action action(TypeAction type, char direction = 0, const std::string &objet = "")
{
    Action a;
    a.type = type;
    a.direction = direction;
    a.objet = objet;
    return a;
}

/* Tout ce que l'agent retient (caché ici, dans agent.cpp). */
struct Agent::Etat {
    Connaissances k;
    const JsonValue &dictionnaire;
    Memoire memoire;

    int numero = 0;
    std::string resident;
    Pos pos_resident = {-1, -1};
    std::string message;

    std::string objet_en_main;

    Phase phase = Phase::FINIE;
    std::vector<CasierPossible> liste;
    int rang_choisi = -1;
    Bilan resultat;

    Etat(const Connaissances &connaissances, const JsonValue &dico)
        : k(connaissances), dictionnaire(dico), memoire(connaissances) {}

    bool resident_atteignable() const;
    Action aller_vers(Pos objet, const std::string &motif);
    Action echouer(const std::string &motif);
    Action etape_dictionnaire();
    Action etape_armoire();
    Action etape_resident();
    void bouger_selecteur(char direction);
    void apres_consulter();
    void apres_prendre(const std::string &objet);
};

/* Vrai si le carnet montre encore un chemin vers le résident (BFS, annexe B). */
bool Agent::Etat::resident_atteignable() const
{
    if (pos_resident.l < 0) {
        return false;
    }
    if (est_a_cote(memoire.robot, pos_resident)) {
        return true;
    }
    return prochain_pas(memoire.carte, memoire.robot, pos_resident) != PAS_DE_CHEMIN;
}

/* Un pas du BFS vers une case à côté de l'objet (annexe B).
   S'il n'existe aucun chemin, c'est la preuve qu'on ne peut pas y aller (B.6). */
Action Agent::Etat::aller_vers(Pos objet, const std::string &motif)
{
    char d = prochain_pas(memoire.carte, memoire.robot, objet);
    if (d == PAS_DE_CHEMIN) {
        return echouer(motif);
    }
    return action(TypeAction::AVANCER, d);
}

/* La demande échoue (énoncé 6.3) : on note le motif et on passe à la suivante. */
Action Agent::Etat::echouer(const std::string &motif)
{
    resultat.succes = false;
    resultat.motif_echec = motif;
    phase = Phase::FINIE;
    return action(TypeAction::FIN);
}

/* Étape 1 : rejoindre le dictionnaire, puis CONSULTER. */
Action Agent::Etat::etape_dictionnaire()
{
    if (est_a_cote(memoire.robot, k.dictionnaire)) {
        return action(TypeAction::CONSULTER);
    }
    return aller_vers(k.dictionnaire, "emotion indeterminee");
}

/* Étape 2 : rejoindre l'armoire, chercher dans l'ordre de 6.2, puis PRENDRE.
   Si le carnet prouve déjà que le résident est inaccessible, on ne prend rien (règle A). */
Action Agent::Etat::etape_armoire()
{
    if (!resident_atteignable()) {
        return echouer("resident inaccessible");
    }
    if (!est_a_cote(memoire.robot, k.armoire)) {
        return aller_vers(k.armoire, "armoire vide");
    }
    int rang = prochain_casier(liste, memoire.vu, memoire.contenu);
    if (rang == -1) {
        return echouer("armoire vide");
    }
    Pos cible = liste[rang].position;
    if (!pos_egales(memoire.selecteur, cible)) {
        return action(TypeAction::CHERCHER, pas_du_selecteur(memoire.selecteur, cible));
    }
    rang_choisi = rang;
    return action(TypeAction::PRENDRE, 0, memoire.contenu[cible.l][cible.c]);
}

/* Étape 3 : rejoindre le résident, puis DONNER. */
Action Agent::Etat::etape_resident()
{
    if (pos_resident.l < 0) {
        return echouer("resident inaccessible");
    }
    if (est_a_cote(memoire.robot, pos_resident)) {
        return action(TypeAction::DONNER, 0, objet_en_main);
    }
    return aller_vers(pos_resident, "resident inaccessible");
}

/* Le sélecteur bouge d'un casier : les lignes ne bouclent pas, les colonnes si (annexe C.2). */
void Agent::Etat::bouger_selecteur(char direction)
{
    if (direction == 'N') memoire.selecteur.l--;
    if (direction == 'S') memoire.selecteur.l++;
    if (direction == 'E') memoire.selecteur.c = roue_avancer(memoire.selecteur.c, +1, 8);
    if (direction == 'O') memoire.selecteur.c = roue_avancer(memoire.selecteur.c, -1, 8);
}

/* Après CONSULTER : le robot connaît l'émotion (6.1) et les 15 casiers à essayer (6.2).
   S'il porte encore l'objet d'une livraison ratée, il va le donner directement (règle B). */
void Agent::Etat::apres_consulter()
{
    Emotion e = lire_emotion(dictionnaire, message);
    if (!e.trouvee) {
        echouer("emotion indeterminee");
        return;
    }
    resultat.emotion = e.emotion;
    resultat.intensite = e.intensite;
    if (!objet_en_main.empty()) {
        resultat.objet = objet_en_main;
        phase = Phase::RESIDENT;
        return;
    }
    liste = casiers_a_essayer(numero_emotion(dictionnaire, e.emotion),
                              numero_intensite(dictionnaire, e.intensite));
    phase = Phase::ARMOIRE;
}

/* Après PRENDRE : le robot porte l'objet ; on retient le casier et le repli pour la trace (5.5). */
void Agent::Etat::apres_prendre(const std::string &objet)
{
    objet_en_main = objet;
    resultat.a_un_casier = true;
    resultat.casier_choisi = liste[rang_choisi].position;
    resultat.repli = liste[rang_choisi].repli;
    resultat.objet = objet;
    phase = Phase::RESIDENT;
}

/* Création de l'agent, au début de la simulation. */
Agent::Agent(const Connaissances &k, const JsonValue &dictionnaire)
    : etat(std::make_unique<Etat>(k, dictionnaire)) {}

Agent::~Agent() = default;

/* Une nouvelle demande arrive (énoncé, section 4). */
void Agent::nouvelle_demande(int numero, const std::string &resident, const std::string &message)
{
    etat->numero = numero;
    etat->resident = resident;
    etat->message = message;

    etat->pos_resident = Pos{-1, -1};
    for (const ResidentConnu &r : etat->k.residents) {
        if (r.id == resident) {
            etat->pos_resident = r.pos;
        }
    }

    etat->memoire.oublier_armoire();
    etat->phase = Phase::DICTIONNAIRE;
    etat->liste.clear();
    etat->rang_choisi = -1;
    etat->resultat = Bilan();
}

/* PERCEPTION -> MÉMOIRE, à chaque tour. */
void Agent::percevoir(const PerceptionRobot &p)
{
    etat->memoire.noter_perception(p);
}

/* DÉCISION : l'action de ce tour, selon l'étape de la demande. FIN = demande terminée. */
Action Agent::decider()
{
    switch (etat->phase) {
    case Phase::DICTIONNAIRE:
        return etat->etape_dictionnaire();
    case Phase::ARMOIRE:
        return etat->etape_armoire();
    case Phase::RESIDENT:
        return etat->etape_resident();
    default:
        return action(TypeAction::FIN);
    }
}

/* Après une action acceptée par l'environnement : on met le carnet à jour. */
void Agent::appliquer(const Action &a)
{
    switch (a.type) {
    case TypeAction::AVANCER:
        etat->memoire.robot = voisin(etat->memoire.robot, a.direction);
        break;
    case TypeAction::CHERCHER:
        etat->bouger_selecteur(a.direction);
        break;
    case TypeAction::CONSULTER:
        etat->apres_consulter();
        break;
    case TypeAction::PRENDRE:
        etat->apres_prendre(a.objet);
        break;
    case TypeAction::DONNER:
        etat->objet_en_main = "";
        etat->resultat.succes = true;
        etat->phase = Phase::FINIE;
        break;
    default:
        break;
    }
}

/* Le résultat de la demande, pour la trace (énoncé 5.5). */
Bilan Agent::bilan() const
{
    return etat->resultat;
}
