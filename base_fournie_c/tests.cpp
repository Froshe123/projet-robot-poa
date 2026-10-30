/* Tests automatiques de la partie agent (énoncé, section 8).
   Lancer avec : make test   (depuis base_fournie_c) */
#include <cstdio>
#include <string>
#include "graphe.hpp"
#include "dictionnaire.hpp"
#include "reconfort_io.hpp"
#include "choix_casier.hpp"
#include "memoire.hpp"
#include "interface.hpp"
#include "validation.hpp"

static int nb_tests = 0;
static int nb_echecs = 0;

/* Vérifie une condition : affiche "ok" ou "ECHEC", et compte les échecs. */
void verifier(bool condition, const std::string &message)
{
    nb_tests++;
    if (condition) {
        std::printf("  ok     %s\n", message.c_str());
    } else {
        std::printf("  ECHEC  %s\n", message.c_str());
        nb_echecs++;
    }
}

/* Fait avancer le robot pas à pas jusqu'à être à côté de l'objet,
   sans rien découvrir en route. Renvoie les directions prises, par
   exemple "SSEENE", avec un "X" à la fin si aucun chemin n'existe. */
std::string marcher(const CarteRobot &carte, Pos robot, Pos objet)
{
    std::string chemin;
    while (!est_a_cote(robot, objet) && chemin.size() < 100) {
        char d = prochain_pas(carte, robot, objet);
        if (d == PAS_DE_CHEMIN) {
            return chemin + "X";
        }
        chemin += d;
        robot = voisin(robot, d);
    }
    return chemin;
}

/* ------------------------------------------------------------------
   La mini-grille de l'énoncé (annexe B.3), avec l'armoire A en (1,4) :
       ######
       #R.#A#
       #.#..#
       #...##
       ######
   ------------------------------------------------------------------ */
const Pos ROBOT = {1, 1};
const Pos ARMOIRE = {1, 4};

/* Test 1 : planification sur une grille entièrement connue. */
void test_planification()
{
    std::printf("Test 1 : planification (grille connue)\n");
    CarteRobot carte(5, 6);
    const char *grille[] = { "######", "#..#A#", "#.#..#", "#...##", "######" };
    for (int l = 0; l < 5; l++) {
        for (int c = 0; c < 6; c++) {
            TypeCase type = (grille[l][c] == '.') ? TypeCase::LIBRE : TypeCase::OCCUPEE;
            carte.mettre_a_jour(Pos{l, c}, type);
        }
    }

    verifier(prochain_pas(carte, ROBOT, ARMOIRE) == 'S', "le 1er pas est S (pas E, qui mene a un cul-de-sac)");
    verifier(marcher(carte, ROBOT, ARMOIRE) == "SSEENE", "le chemin complet est S S E E N E (6 pas)");

    carte.mettre_a_jour(Pos{3, 2}, TypeCase::OCCUPEE);   /* on bouche le seul passage */
    verifier(prochain_pas(carte, ROBOT, ARMOIRE) == PAS_DE_CHEMIN, "passage bouche : aucun chemin (X)");
}

/* Test 2 : replanification, un mur découvert en route change le trajet. */
void test_replanification()
{
    std::printf("Test 2 : replanification (murs inconnus au depart)\n");
    CarteRobot carte(5, 6);                               /* seul le bord est connu */
    carte.mettre_a_jour(ARMOIRE, TypeCase::OCCUPEE);       /* et l'armoire */

    verifier(prochain_pas(carte, ROBOT, ARMOIRE) == 'E', "au depart, (1,3) est inconnue donc supposee libre : on part vers E");

    /* Le robot avance en (1,2) et voit : E = mur (1,3), S = mur (2,2). */
    Pos robot = {1, 2};
    carte.mettre_a_jour(Pos{1, 3}, TypeCase::OCCUPEE);
    carte.mettre_a_jour(Pos{2, 2}, TypeCase::OCCUPEE);

    verifier(prochain_pas(carte, robot, ARMOIRE) == 'O', "apres avoir vu le mur, le robot fait demi-tour (O)");
    verifier(marcher(carte, robot, ARMOIRE) == "OSSEENE", "le nouveau chemin passe par le bas (7 pas)");
}

/* Vérifie l'émotion et l'intensité lues dans un message. */
void verifier_emotion(const JsonValue &dico, const std::string &message,
                      const std::string &emotion, const std::string &intensite)
{
    Emotion e = lire_emotion(dico, message);
    verifier(e.trouvee && e.emotion == emotion && e.intensite == intensite,
             emotion + " / " + intensite + " <- " + message);
}

/* Test 3 : lecture du dictionnaire (énoncé 6.1). */
void test_dictionnaire()
{
    std::printf("Test 3 : lecture du dictionnaire\n");
    JsonValue dico;
    try {
        dico = charger_dictionnaire("donnees/dictionnaire.json");
    } catch (const ErreurFichier &e) {
        verifier(false, std::string("chargement du dictionnaire : ") + e.what());
        return;
    }

    /* un message par émotion */
    verifier_emotion(dico, "Ma fille a appelé, je suis vraiment heureux.", "joie", "moyenne");
    verifier_emotion(dico, "Je te fais confiance, tu sauras quoi faire.", "confiance", "moyenne");
    verifier_emotion(dico, "Je suis terrifiée, il y a quelqu'un dans l'entrée.", "peur", "forte");
    verifier_emotion(dico, "Je suis perplexe, cette porte était fermée ce matin.", "surprise", "faible");
    verifier_emotion(dico, "Je me sens tellement seule ce soir, personne n'est passé.", "tristesse", "moyenne");
    verifier_emotion(dico, "Cette odeur me dégoûte, c'est insupportable.", "degout", "moyenne");
    verifier_emotion(dico, "Je suis furieux, on a encore oublié mon repas.", "colere", "forte");
    verifier_emotion(dico, "Je reste vigilante, quelqu'un rôde dehors.", "anticipation", "forte");

    /* deux mots connus : le premier gagne ("sidere" avant "seule") */
    verifier_emotion(dico, "Je suis sidéré, la télévision s'est allumée toute seule.", "surprise", "forte");

    /* aucun mot connu */
    Emotion e = lire_emotion(dico, "Bonjour, il fait beau aujourd'hui.");
    verifier(!e.trouvee, "aucun mot connu -> emotion indeterminee");
}

/* Vrai si p est le casier [l, c]. */
bool meme_casier(Pos p, int l, int c)
{
    return p.l == l && p.c == c;
}

/* Les casiers à essayer (énoncé 6.2) et le sélecteur (annexe C.3). */
void test_casiers_a_essayer()
{
    std::printf("Casiers a essayer et selecteur\n");

    /* les numéros viennent du dictionnaire (énoncé 5.2) */
    try {
        JsonValue dico = charger_dictionnaire("donnees/dictionnaire.json");
        verifier(numero_emotion(dico, "tristesse") == 4, "tristesse est l'emotion numero 4");
        verifier(numero_intensite(dico, "forte") == 2, "forte est l'intensite numero 2");
        verifier(numero_emotion(dico, "bonheur") == -1, "mot absent de la liste : -1");
    } catch (const ErreurFichier &e) {
        verifier(false, std::string("chargement du dictionnaire : ") + e.what());
    }

    /* tristesse (4) / moyenne (1) : le tableau de l'énoncé, rang par rang */
    std::vector<CasierPossible> liste = casiers_a_essayer(4, 1);
    const int attendu[15][2] = { {1,4}, {0,4}, {2,4}, {1,5}, {1,3}, {0,5}, {0,3}, {2,5},
                                 {2,3}, {1,6}, {1,2}, {0,6}, {0,2}, {2,6}, {2,2} };
    bool ordre_ok = (liste.size() == 15);
    for (int rang = 0; ordre_ok && rang < 15; rang++) {
        ordre_ok = meme_casier(liste[rang].position, attendu[rang][0], attendu[rang][1]);
    }
    verifier(ordre_ok, "tristesse/moyenne : les 15 casiers dans l'ordre de 6.2");
    verifier(ordre_ok && liste[0].repli == "aucun" && liste[1].repli == "intensite"
             && liste[3].repli == "voisine_1" && liste[9].repli == "voisine_2",
             "etiquettes : aucun, intensite, voisine_1, voisine_2");

    /* joie (0) / faible (0) : la voisine antihoraire de joie est anticipation (7) */
    liste = casiers_a_essayer(0, 0);
    verifier(meme_casier(liste[3].position, 0, 1) && meme_casier(liste[4].position, 0, 7),
             "joie/faible : rang 4 = confiance [0,1], rang 5 = anticipation [0,7] (bouclage)");

    /* le sélecteur : vertical d'abord, puis le côté le plus court */
    verifier(pas_du_selecteur(Pos{1, 0}, Pos{1, 4}) == 'E', "[1,0] -> [1,4] : E (4 crans des deux cotes -> E)");
    verifier(pas_du_selecteur(Pos{2, 7}, Pos{0, 1}) == 'N', "[2,7] -> [0,1] : N d'abord");
    verifier(pas_du_selecteur(Pos{1, 0}, Pos{2, 6}) == 'S', "[1,0] -> [2,6] : S d'abord");
    verifier(pas_du_selecteur(Pos{2, 0}, Pos{2, 6}) == 'O', "[2,0] -> [2,6] : O (2 crans au lieu de 6)");
    verifier(pas_du_selecteur(Pos{0, 7}, Pos{0, 1}) == 'E', "[0,7] -> [0,1] : E (on passe par la colonne 0)");
}

/* L'armoire standard (donnees/armoire_standard.json) ; "" = casier vide. */
const char *ARMOIRE_STANDARD[3][8] = {
    { "tisane", "coussin", "veilleuse", "", "mouchoirs", "livre", "balle antistress", "magazine" },
    { "photo de famille", "plaid", "lampe torche", "", "couverture", "desodorisant", "casque audio", "calendrier" },
    { "guirlande lumineuse", "album souvenir", "bouton d appel", "", "telephone", "gants de menage", "tasse de camomille", "" },
};

/* Test 4 : */
void test_selecteur() {
    std::printf("Test 4 : test de selecteur\n");
    JsonValue carte;
    JsonValue armoire;
    try {
        carte = charger_carte("cartes/appartement_01.json");
        armoire = charger_armoire("donnees/armoire_standard.json");
    } catch (const ErreurFichier &e) {
        verifier(false, std::string("chargement de la carte et de l'armoire : ") + e.what());
        return;
    }
    Environnement env(carte,armoire);
    Action action;
    action.type = TypeAction::AVANCER;
    action.direction = 'E';
    env.executer(action);
    env.executer(action);
    verifier(pos_egales(env.robot(), Pos{6, 3}), "le robot est devant l'armoire");
    action.type = TypeAction::CHERCHER;
    //O
    action.direction = 'O';
    bool ok = env.executer(action);
    verifier(ok && pos_egales(env.selecteur(), Pos{1,7}), "CHERCHER O depuis (1,0) : bouclage vers (1,7)");
    //E
    action.direction = 'E';
    ok = env.executer(action);
    verifier(ok && pos_egales(env.selecteur(), Pos{1,0}), "CHERCHER E depuis (1,7) : bouclage vers (1,0)");
    //N
    action.direction = 'N';
    ok = env.executer(action);
    verifier(ok && pos_egales(env.selecteur(), Pos{0,0}), "CHERCHER N depuis (1,0) : monte vers (0,0)");
    //N
    action.direction = 'N';
    ok = env.executer(action);
    verifier(!ok && pos_egales(env.selecteur(), Pos{0,0}), "CHERCHER N depuis (0,0) : refuse, les lignes ne bouclent pas");
    //S
    action.direction = 'S';
    ok = env.executer(action);
    verifier(ok && pos_egales(env.selecteur(), Pos{1,0}), "CHERCHER S depuis (0,0) : descend vers (1,0)");
    //S
    action.direction = 'S';
    ok = env.executer(action);
    verifier(ok && pos_egales(env.selecteur(), Pos{2,0}), "CHERCHER S depuis (1,0) : descend vers (2,0)");
    //S
    action.direction = 'S';
    ok = env.executer(action);
    verifier(!ok && pos_egales(env.selecteur(), Pos{2,0}), "CHERCHER S depuis (2,0) : refuse, derniere ligne");
}

/* Test 5 : la règle de repli (énoncé 6.2). Au début, le robot a vu toute l'armoire. */
void test_repli()
{
    std::printf("Test 5 : regle de repli\n");
    bool vu[3][8];
    std::string contenu[3][8];
    for (int l = 0; l < 3; l++) {
        for (int c = 0; c < 8; c++) {
            vu[l][c] = true;
            contenu[l][c] = ARMOIRE_STANDARD[l][c];
        }
    }

    /* 1. casier exact garni */
    std::vector<CasierPossible> liste = casiers_a_essayer(4, 1);   /* tristesse / moyenne */
    int rang = prochain_casier(liste, vu, contenu);
    verifier(rang == 0 && liste[rang].repli == "aucun", "tristesse/moyenne : [1,4] couverture, repli aucun");

    /* 2. casier exact vide, même émotion garnie */
    liste = casiers_a_essayer(7, 2);                          /* anticipation / forte */
    rang = prochain_casier(liste, vu, contenu);
    verifier(rang == 1 && meme_casier(liste[rang].position, 1, 7) && liste[rang].repli == "intensite",
             "anticipation/forte : [2,7] vide -> [1,7] calendrier, repli intensite");

    /* 3. colonne entière vide : on passe à une voisine (annexe C.5) */
    liste = casiers_a_essayer(3, 2);                          /* surprise / forte */
    rang = prochain_casier(liste, vu, contenu);
    verifier(rang == 3 && meme_casier(liste[rang].position, 2, 4) && liste[rang].repli == "voisine_1",
             "surprise/forte : colonne surprise vide -> [2,4] telephone, repli voisine_1");

    /* 4. rien de garni à distance <= 2 : on vide les colonnes 2 à 6 */
    for (int l = 0; l < 3; l++) {
        for (int c = 2; c <= 6; c++) {
            contenu[l][c] = "";
        }
    }
    liste = casiers_a_essayer(4, 1);                          /* tristesse / moyenne */
    verifier(prochain_casier(liste, vu, contenu) == -1,
             "colonnes 2 a 6 vides : armoire vide (joie, confiance, anticipation sont trop loin)");

    /* 5. pas de repli sans fouille : le robot n'a vu que [1,0] [2,0] [2,1] [2,2] */
    for (int l = 0; l < 3; l++) {
        for (int c = 0; c < 8; c++) {
            vu[l][c] = false;
            contenu[l][c] = ARMOIRE_STANDARD[l][c];
        }
    }
    vu[1][0] = vu[2][0] = vu[2][1] = vu[2][2] = true;
    liste = casiers_a_essayer(3, 2);                          /* surprise / forte */
    rang = prochain_casier(liste, vu, contenu);
    verifier(rang == 0 && meme_casier(liste[rang].position, 2, 3),
             "bouton d'appel [2,2] croise mais pas pris : on va d'abord voir [2,3]");
}

bool est_refuse(const std::string &type, const std::string &chemin){
    try{
        if (type == "carte"){
            valider_carte(charger_carte(chemin));
        }
        else if (type == "armoire"){
            valider_armoire(charger_armoire(chemin));
        }
        else if (type == "dictionnaire"){
            valider_dictionnaire(charger_dictionnaire(chemin));
        }
        else if (type == "scenario"){
            valider_scenario(charger_scenario(chemin), charger_carte("cartes/appartement_01.json"));
        }
    }
    catch (const ErreurFichier &){
        return true;
    }
    return false;
    
}


/* Test 6 : test validation */
void test_validation(){
    std::printf("Test 6 : validation des fichiers\n");
    verifier(!est_refuse("carte", "cartes/appartement_01.json"), "carte valide : acceptee");
    verifier(!est_refuse("armoire", "donnees/armoire_standard.json"), "armoire valide : acceptee");
    verifier(!est_refuse("dictionnaire", "donnees/dictionnaire.json"), "dictionnaire valide : accepte");
    verifier(!est_refuse("scenario", "cartes/scenario_01.json"), "scenario valide : accepte");
    verifier(est_refuse("carte", "tests_casses/n_existe_pas.json"), "fichier absent : refuse");
    verifier(est_refuse("carte", "tests_casses/pas_utf8.json"), "fichier non UTF-8 : refuse");
    verifier(est_refuse("carte", "tests_casses/json_invalide.json"), "JSON invalide : refuse");
    verifier(est_refuse("carte", "tests_casses/mauvais_format.json"), "format inattendu : refuse");
    verifier(est_refuse("carte", "tests_casses/champ_absent.json"), "champ absent : refuse");
    verifier(est_refuse("carte", "tests_casses/mauvais_type.json"), "champ du mauvais type : refuse");
    verifier(est_refuse("carte", "tests_casses/grille_non_rectangulaire.json"), "grille non rectangulaire : refuse");
    verifier(est_refuse("carte", "tests_casses/dimensions_incoherentes.json"), "dimensions incoherentes : refuse");
    verifier(est_refuse("carte", "tests_casses/position_hors_grille.json"), "position hors de la grille : refuse");
    verifier(est_refuse("carte", "tests_casses/symbole_incorrect.json"), "symbole incorrect sous l'armoire : refuse");
    verifier(est_refuse("carte", "tests_casses/residents_meme_id.json"), "deux residents meme id : refuse");
    verifier(est_refuse("scenario", "tests_casses/scenario_autre_carte.json"), "scenario d'une autre carte : refuse");
    verifier(est_refuse("scenario", "tests_casses/scenario_resident_absent.json"), "demande a un resident absent : refuse");
    verifier(est_refuse("armoire", "tests_casses/casier_hors_armoire.json"), "casier hors de l'armoire : refuse");
    verifier(est_refuse("armoire", "tests_casses/casier_en_double.json"), "casier en double : refuse");
    verifier(est_refuse("armoire", "tests_casses/casier_mauvaise_etiquette.json"), "etiquette de casier incoherente : refuse");
    verifier(est_refuse("armoire", "tests_casses/casier_depart_hors_armoire.json"), "casier_depart hors de l'armoire : refuse");
    verifier(est_refuse("dictionnaire", "tests_casses/dico_emotion_inconnue.json"), "emotion inconnue : refuse");
    verifier(est_refuse("dictionnaire", "tests_casses/dico_mot_en_double.json"), "mot dans deux entrees : refuse");
}

/* Déroule une demande avec l'environnement et l'agent (même boucle que
   simuler_demande dans main.cpp, sans la trace), et renvoie le bilan. */
Bilan derouler_demande(Environnement &env, Agent &agent, const Connaissances &k, const JsonValue &demande)
{
    std::string resident = demande.get_chaine("resident");
    agent.nouvelle_demande((int)demande.get_nombre("numero"), resident, demande.get_chaine("message"));
    for (const ResidentConnu &r : k.residents) {
        if (r.id == resident) {
            env.viser_resident(r.pos);
        }
    }
    for (int pas = 0; pas < 500; pas++) {             /* garde-fou */
        PerceptionRobot p = env.percevoir();
        agent.percevoir(p);
        Action a = agent.decider();
        if (a.type == TypeAction::FIN) {
            break;
        }
        if (!env.executer(a)) {
            verifier(false, "action refusee par l'environnement");
            break;
        }
        agent.appliquer(a);
    }
    return agent.bilan();
}

/* Test 7 : cas inaccessible (énoncé 6.3 et annexe B), sur une grille écrite à la main
   (tests_cartes/resident_mure.json) : R2 est muré, R1 est accessible. */
void test_resident_inaccessible()
{
    std::printf("Test 7 : resident inaccessible\n");
    JsonValue carte, scenario, armoire, dico;
    try {
        carte = charger_carte("tests_cartes/resident_mure.json");
        scenario = charger_scenario("tests_cartes/scenario_resident_mure.json");
        armoire = charger_armoire("donnees/armoire_standard.json");
        dico = charger_dictionnaire("donnees/dictionnaire.json");
        valider_carte(carte);
        valider_scenario(scenario, carte);
    } catch (const ErreurFichier &e) {
        verifier(false, std::string("chargement du test 7 : ") + e.what());
        return;
    }
    verifier(true, "carte et scenario du test 7 valides");

    Environnement env(carte, armoire);
    Connaissances k = env.connaissances();
    Agent agent(k, dico);
    const JsonValue &demandes = *scenario.get("demandes");

    Bilan b = derouler_demande(env, agent, k, *demandes.index(0));
    verifier(!b.succes && b.motif_echec == "resident inaccessible",
             "demande 1 (R2 mure) : echec 'resident inaccessible'");

    b = derouler_demande(env, agent, k, *demandes.index(1));
    verifier(b.succes, "demande 2 (R1 accessible) : le scenario continue et reussit");
}

/* Test mémoire : carte mentale et armoire (énoncé 3.2 et 7), sur appartement_01. */
void test_memoire()
{
    std::printf("Test memoire : carte mentale et armoire\n");
    Connaissances k;
    k.hauteur = 9;
    k.largeur = 13;
    k.depart = Pos{6, 1};
    k.armoire = Pos{6, 4};
    k.dictionnaire = Pos{7, 6};
    k.residents = { {"R1", Pos{6, 11}}, {"R2", Pos{3, 3}} };
    k.casier_depart = Pos{1, 0};

    Memoire m(k);

    verifier(pos_egales(m.robot, Pos{6, 1}), "le robot est sur sa case de depart");
    verifier(pos_egales(m.selecteur, Pos{1, 0}), "le selecteur est sur le casier de depart");
    verifier(!m.carte.case_valide(k.armoire), "l'armoire est une case occupee");
    verifier(!m.carte.case_valide(Pos{3, 3}), "le resident R2 est une case occupee");
    verifier(m.carte.case_valide(Pos{5, 7}), "(5,7) jamais vue : supposee libre");

    m.robot = Pos{5, 6};
    PerceptionRobot p;
    p.voisins = { "mur", "libre", "mur", "libre" };
    m.noter_perception(p);
    verifier(!m.carte.case_valide(Pos{5, 7}), "E = mur : (5,7) devient occupee");
    verifier(!m.carte.case_valide(Pos{4, 6}), "N = mur : (4,6) devient occupee");
    verifier(m.carte.case_valide(Pos{6, 6}), "S = libre : (6,6) reste valide");

    p.devant_armoire = true;
    p.contenu_casier = "photo de famille";
    m.noter_perception(p);
    verifier(m.vu[1][0] && m.contenu[1][0] == "photo de famille", "casier (1,0) vu : photo de famille");

    m.selecteur = Pos{1, 3};
    p.contenu_casier = std::nullopt;
    m.noter_perception(p);
    verifier(m.vu[1][3] && m.contenu[1][3] == "", "casier (1,3) vu : vide");
    verifier(!m.vu[0][0], "casier (0,0) jamais vu");

    m.oublier_armoire();
    verifier(!m.vu[1][0] && !m.vu[1][3], "apres oublier_armoire : plus aucun casier vu");
    verifier(!m.carte.case_valide(Pos{5, 7}), "la carte, elle, est gardee");
}

int main()
{
    test_planification();
    test_replanification();
    test_dictionnaire();
    test_casiers_a_essayer();
    test_repli();
    test_memoire();
    test_selecteur();
    test_validation();
    test_resident_inaccessible();

    std::printf("\n%d tests, %d echec(s)\n", nb_tests, nb_echecs);
    return (nb_echecs == 0) ? 0 : 1;
}
