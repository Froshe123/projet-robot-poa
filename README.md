# Robot de réconfort — KRAMER Axel & MELON Eliott

Projet de semestre en C++17 (g++ 15, Linux/WSL). Pas de dépendance à part g++ et make.

## Lancer

```
make -C base_fournie_c robot
base_fournie_c/robot <carte> <scenario> <dossier_donnees> <trace_sortie>
```

Par exemple :

```
base_fournie_c/robot base_fournie_c/cartes/appartement_01.json base_fournie_c/cartes/scenario_01.json base_fournie_c/donnees sortie.json
```

Si un fichier est invalide, le programme affiche l'erreur, renvoie 1 et n'écrit pas de trace.
Avec `AFFICHAGE=1` devant la commande, on voit le robot bouger dans le terminal.

## Tester

```
make -C base_fournie_c test
```

Ça lance les tests 1 à 7 de l'énoncé (plus un test de la mémoire), 79 vérifications au total.
Les fichiers cassés du test 6 sont dans `tests_casses/`, la carte du test 7 dans `tests_cartes/`.

## Organisation du code

Tout est dans `base_fournie_c/` :

| Fichier | Rôle |
|---|---|
| `main.cpp` | lance tout et écrit la trace |
| `environnement.cpp` | le vrai monde (seul à lire la grille et les objets) |
| `validation.cpp` | vérif des fichiers (5.6) |
| `agent.cpp`, `memoire.cpp`, `carte_robot.cpp` | le robot et sa carte mentale |
| `graphe.cpp` | BFS pour le plus court chemin |
| `choix_casier.cpp`, `dictionnaire.cpp`, `roue.cpp` | choix du casier, lecture du message, roue des émotions |
| `json.cpp`, `reconfort_io.cpp` | base fournie |

## État

**Ce qui marche :** les 5 scénarios fournis (29 livraisons sur 29), les 3 cas d'échec, les fichiers invalides.

**Ce qui marche pas :** si le robot prend un objet et découvre ensuite que le résident est inaccessible, il le garde et le donne au résident suivant, même si ça correspond pas à son émotion.

## Vidéo

`demo.mp4`
