#!/usr/bin/env python3
"""Génère suivi/AVANCEMENT.pdf (Eliott + Axel) à partir des listes ci-dessous.
Pour mettre à jour : changer l'état (FAIT / EN COURS / A FAIRE / BLOQUÉ), la note
et les prochaines étapes, puis relancer :  python3 suivi/avancement.py
Aucune dépendance : le PDF est écrit à la main (polices standard Helvetica)."""
import datetime, os, textwrap

FAIT, COURS, TODO, BLOQUE = "FAIT", "EN COURS", "A FAIRE", "BLOQUÉ"

# (section, tâche, état, note)
AXEL = [
 ("Carte & mémoire", "Carte mentale (hypothèse optimiste)", FAIT, "carte_robot.cpp/.hpp : INCONNUE/LIBRE/OCCUPEE, mise à jour par case"),
 ("Carte & mémoire", "Mémoire du robot (struct Memoire)", COURS, "memoire.hpp écrit (carte, robot, sélecteur, vu/contenu) ; memoire.cpp à écrire"),
 ("Carte & mémoire", "Mise à jour à chaque perception", COURS, "noter_perception / oublier_armoire déclarés, non implémentés"),
 ("Planification", "BFS + file (plus court chemin)", FAIT, "graphe.cpp : prochain_pas, file écrite à la main, sans lib"),
 ("Planification", "Replanification à chaque pas", FAIT, "un pas puis recalcul ; PAS_DE_CHEMIN si inaccessible"),
 ("Décision", "Lecture du dictionnaire (6.1)", FAIT, "dictionnaire.cpp : premier mot reconnu, sinon indéterminée"),
 ("Décision", "Choix du casier (6.2) : 15 candidats triés", FAIT, "choix_casier.cpp : casiers_a_essayer, prochain_casier, étiquettes de repli"),
 ("Décision", "Itinéraire du sélecteur (annexe C)", FAIT, "pas_du_selecteur : vertical d'abord + bouclage des colonnes (C.3, accepté par C.4)"),
 ("Décision", "Enchaînement des 4 étapes par demande", TODO, "agent.cpp : classe Agent de interface.hpp (decider, appliquer, bilan)"),
 ("Décision", "Gestion des échecs (6.3)", TODO, "résident inaccessible, armoire vide, émotion indéterminée"),
 ("Tests", "Test 1 : planification petite grille", FAIT, "tests.cpp : mini-grille B.3, chemin SSEENE, puis X si bouché"),
 ("Tests", "Test 2 : replanification (mur découvert)", FAIT, "tests.cpp : murs vus en (1,2), nouveau chemin OSSEENE"),
 ("Tests", "Test 3 : lecture du dictionnaire", FAIT, "tests.cpp : 8 émotions + 2 mots connus + aucun mot"),
 ("Tests", "Test 5 : règle de repli", FAIT, "tests.cpp : exact, intensité, voisine, armoire vide, ordre respecté"),
]

ELIOTT = [
 ("Roue", "roue_avancer : bouclage dans [0, n-1]", FAIT, "roue.cpp : pas négatifs gérés"),
 ("Roue", "roue_distance : plus court côté", FAIT, "roue.cpp : résultat dans [0, n/2]"),
 ("Environnement", "struct Environnement::Etat", FAIT, "grille, robot, sélecteur, armoire 2D (optional), positions fixes, résidents, objet porté, résident visé, départs"),
 ("Environnement", "Makefile : environnement.o dans ROBOT", FAIT, "conflit de fusion réparé"),
 ("Environnement", "Destructeur ~Environnement()", FAIT, "= default, après la struct"),
 ("Environnement", "Constructeur (lecture carte + armoire)", FAIT, "grille, 6 positions, résidents, armoire 3x8 (casiers null gérés)"),
 ("Environnement", "connaissances()", FAIT, "tout SAUF la grille et les objets"),
 ("Environnement", "percevoir()", FAIT, "4 voisins N/S/E/O, devant_armoire, contenu_casier"),
 ("Environnement", "viser_resident(), robot(), selecteur()", FAIT, ""),
 ("Environnement", "executer() : légalité des actions", FAIT, "les 6 actions ; FIN refusée (default)"),
 ("Validation 5.6", "Fichier absent / illisible / pas UTF-8, JSON invalide, format/version", FAIT, "déjà assuré par charger_*() dans reconfort_io.cpp"),
 ("Validation 5.6", "Champ obligatoire absent ou mauvais type", FAIT, "valider_carte / armoire / dictionnaire / scenario"),
 ("Validation 5.6", "Grille non rectangulaire, dimensions, position hors grille", FAIT, "valider_carte"),
 ("Validation 5.6", "Symboles P / A / D / R cohérents avec les positions", FAIT, "valider_carte"),
 ("Validation 5.6", "Résidents en double, autre carte, résident absent", FAIT, "valider_carte + valider_scenario"),
 ("Validation 5.6", "Casiers : hors armoire, doublon, étiquette, casier_depart", FAIT, "valider_armoire"),
 ("Validation 5.6", "Dictionnaire : émotion/intensité inconnue, mot en double", FAIT, "valider_dictionnaire"),
 ("Validation 5.6", "Erreur -> stderr, code non nul, aucune trace", COURS, "à brancher dans main.cpp (try/catch avant la simulation)"),
 ("Affichage", "Grille redessinée à chaque pas", TODO, "robot + murs découverts (débogage, vidéo)"),
 ("Tests", "Test 4 : sélecteur, bouclage colonne 7 <-> 0", FAIT, "tests.cpp : test_selecteur, 8 vérifications"),
 ("Tests", "Test 6 : un fichier cassé par catégorie de 5.6", FAIT, "tests.cpp : test_validation, 4 fichiers valides + 19 cassés (tests_casses/)"),
 ("Tests", "Test 7 : résident muré (avec Axel)", TODO, ""),
]

ENSEMBLE = [
 ("Intégration", "main.cpp : boucle Environnement <-> Agent", TODO, "trace, livraisons, résumé"),
 ("Relecture", "Relecture croisée", TODO, "Eliott : agent/graphe/carte_robot ; Axel : environnement/roue"),
 ("Rendu", "Rapport 3 pages", TODO, "architecture, replanification, repli, 3 échecs"),
 ("Rendu", "Vidéo 1 min 30", TODO, "robot qui se cogne et replanifie"),
 ("Rendu", "README + rendu.json", TODO, "commande de test ; C++, make, commande, vidéo"),
]

PROCHAINES = [
 ("Axel", "memoire.cpp, puis agent.cpp (decider, appliquer, bilan)."),
 ("Eliott", "main.cpp avec Axel (y brancher la validation), puis affichage texte, puis test 7."),
]

# --------------------------------------------------------------------------
# Rendu PDF minimal (A4, Helvetica, encodage WinAnsi)
# --------------------------------------------------------------------------
PRET = {FAIT: 1.0, COURS: 0.5, TODO: 0.0, BLOQUE: 0.0}
COUL = {FAIT: (0.18, 0.49, 0.20), COURS: (0.94, 0.56, 0.0), TODO: (0.62, 0.62, 0.62), BLOQUE: (0.78, 0.16, 0.16)}
W, H, M = 595, 842, 45
L = W - 2 * M


def pourcentage(taches):
    return round(100 * sum(PRET[t[2]] for t in taches) / len(taches))


def esc(s):
    return s.replace("\\", "\\\\").replace("(", "\\(").replace(")", "\\)")


class Doc:
    def __init__(self):
        self.pages, self.y = [], 0
        self.nouvelle_page()

    def nouvelle_page(self):
        self.pages.append([])
        self.y = H - M

    def place(self, h):
        if self.y - h < M:
            self.nouvelle_page()

    def texte(self, x, y, s, t=10, gras=False, c=(0, 0, 0)):
        self.pages[-1].append("BT %g %g %g rg /%s %g Tf %g %g Td (%s) Tj ET"
                              % (c[0], c[1], c[2], "F2" if gras else "F1", t, x, y, esc(s)))

    def rect(self, x, y, w, h, c):
        self.pages[-1].append("%g %g %g rg %g %g %g %g re f" % (c[0], c[1], c[2], x, y, w, h))

    def trait(self, y, g=0.75):
        self.pages[-1].append("%g G 0.5 w %g %g m %g %g l S" % (g, M, y, W - M, y))

    def ecrire(self, chemin):
        objs = [None, None,
                b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>",
                b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold /Encoding /WinAnsiEncoding >>"]
        kids = []
        for ops in self.pages:
            data = "\n".join(ops).encode("cp1252", "replace")
            objs.append(b"<< /Length %d >>\nstream\n" % len(data) + data + b"\nendstream")
            objs.append(("<< /Type /Page /Parent 2 0 R /MediaBox [0 0 %d %d] /Resources << /Font "
                         "<< /F1 3 0 R /F2 4 0 R >> >> /Contents %d 0 R >>" % (W, H, len(objs))).encode())
            kids.append(len(objs))
        objs[0] = b"<< /Type /Catalog /Pages 2 0 R >>"
        objs[1] = ("<< /Type /Pages /Kids [%s] /Count %d >>"
                   % (" ".join("%d 0 R" % k for k in kids), len(kids))).encode()
        out, offs = bytearray(b"%PDF-1.4\n"), []
        for i, o in enumerate(objs, 1):
            offs.append(len(out))
            out += b"%d 0 obj\n" % i + o + b"\nendobj\n"
        xref = len(out)
        out += b"xref\n0 %d\n0000000000 65535 f \n" % (len(objs) + 1)
        out += b"".join(b"%010d 00000 n \n" % o for o in offs)
        out += b"trailer\n<< /Size %d /Root 1 0 R >>\nstartxref\n%d\n%%%%EOF\n" % (len(objs) + 1, xref)
        with open(chemin, "wb") as f:
            f.write(out)


def barre(d, pct):
    d.rect(M, d.y, L, 7, (0.88, 0.88, 0.88))
    if pct:
        d.rect(M, d.y, L * pct / 100, 7, COUL[FAIT])


def section(d, titre, role, taches):
    d.place(70)
    nb = {k: sum(1 for t in taches if t[2] == k) for k in (FAIT, COURS, TODO, BLOQUE)}
    pct = pourcentage(taches)
    d.texte(M, d.y, titre, 15, gras=True)
    d.y -= 15
    d.texte(M, d.y, "%s - %d %% : %d fait, %d en cours, %d à faire, %d bloqué"
            % (role, pct, nb[FAIT], nb[COURS], nb[TODO], nb[BLOQUE]), 9, c=(0.35, 0.35, 0.35))
    d.y -= 14
    barre(d, pct)
    d.y -= 18
    precedente = None
    for s, t, e, note in taches:
        lignes = textwrap.wrap(note, 70) if note else []
        d.place(16 + 11 * len(lignes) + (18 if s != precedente else 0))
        if s != precedente:
            d.texte(M, d.y, s, 10, gras=True, c=(0.15, 0.20, 0.22))
            d.y -= 4
            d.trait(d.y)
            d.y -= 13
            precedente = s
        d.rect(M, d.y - 2, 52, 11, COUL[e])
        d.texte(M + 4, d.y + 1, e, 7, gras=True, c=(1, 1, 1))
        d.texte(M + 60, d.y, t, 9.5)
        d.y -= 11
        for l in lignes:
            d.texte(M + 60, d.y, l, 8, c=(0.4, 0.4, 0.4))
            d.y -= 10
        d.y -= 4
    d.y -= 14


def main():
    d = Doc()
    total = pourcentage(AXEL + ELIOTT + ENSEMBLE)
    d.texte(M, d.y, "Robot de Réconfort - avancement du binôme", 19, gras=True)
    d.y -= 18
    d.texte(M, d.y, "Mis à jour le %s - projet global : %d %%"
            % (datetime.date.today().strftime("%d/%m/%Y"), total), 10, c=(0.35, 0.35, 0.35))
    d.y -= 12
    barre(d, total)
    d.y -= 22
    d.texte(M, d.y, "Prochaines étapes", 11, gras=True)
    d.y -= 14
    for qui, quoi in PROCHAINES:
        for i, l in enumerate(textwrap.wrap(quoi, 85)):
            if i == 0:
                d.texte(M, d.y, qui + " :", 9.5, gras=True)
            d.texte(M + 45, d.y, l, 9.5)
            d.y -= 12
    d.y -= 14
    section(d, "Axel (B)", "Agent & décision", AXEL)
    section(d, "Eliott (A)", "Monde & validation", ELIOTT)
    section(d, "Ensemble", "À la fin, à deux", ENSEMBLE)
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "AVANCEMENT.pdf")
    d.ecrire(out)
    print(out, "Axel", pourcentage(AXEL), "% / Eliott", pourcentage(ELIOTT), "%")


main()
