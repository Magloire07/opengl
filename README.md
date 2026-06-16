

# Projet OpenGL - Scène Cubemap Dragon
**Membres du groupe:**
- _Aissatou CISE_
- _Kokou KOMBEDE_
- _Ounissa SADAOUI_


Ce projet est une application OpenGL en C++ qui affiche une scène 3D avec :

- un skybox construit à partir d'une cubemap dans le dossier `texture3`
- une sphère blanche au centre de la scène
- un dragon texturé positionné au-dessus de la sphère et tournant sur lui-même
- huit cubes alternant noir et blanc disposés autour de la scène
- un plan de sol (ground plane) avec motif de damier procédural pour faciliter la perception des déplacements de la caméra
- une caméra entièrement contrôlable au clavier et à la souris

## Contrôles

### Déplacement
- `W` / `S` : avancer / reculer (dans la direction du regard)
- `A` / `D` : déplacement latéral gauche / droite
- `Q` / `E` : monter / descendre verticalement

### Rotation de la caméra (Orientation)
- **Souris** : Maintenir le **clic droit** enfoncé et glisser pour orienter la vue (le curseur est masqué pendant la rotation).
- **Clavier** : 
  - Flèches gauche / droite : pivoter horizontalement (Yaw)
  - Flèches haut / bas : incliner verticalement (Pitch)

### Autres
- `ESC` : quitter l'application

## Fonctionnalités Implémentées

### 1. Illumination Avancée (Partie 1 du sujet)

- **Modèle Blinn-Phong** : Remplacement de l'éclairage Lambert simple par le modèle Blinn-Phong complet (diffus + spéculaire) utilisant le vecteur half-way `H = normalize(L + V)`.
- **Environment Mapping (Réflexion Cubemap)** : Les objets reflètent la cubemap environnante via un échantillonnage du vecteur de réflexion `R = reflect(-V, N)` dans le samplerCube, créant des reflets réalistes de l'environnement sur les surfaces.
- **Fresnel (approximation de Schlick)** : Implémentation de la formule `F = F0 + (1 - F0) * (1 - cos θ)^5` avec `F0 = 0.04` (matériau non-métallique). Le coefficient de Fresnel pondère la contribution entre réflexion spéculaire indirecte et diffus.
- **Correction Gamma (sRGB ↔ Linéaire)** :
  - Les couleurs de base et les textures cubemap sont converties de l'espace sRGB vers l'espace linéaire (`pow(color, 2.2)`) avant les calculs d'éclairage.
  - Le résultat final est reconverti en sRGB (`pow(color, 1/2.2)`) pour un affichage correct sur écran.
- **Transmission de `u_ViewPos`** : La position de la caméra est envoyée au shader pour les calculs de Blinn-Phong et de réflexion.
- **Liaison de la Cubemap aux objets** : La cubemap (texture unit 1) est liée au shader des objets pour l'environment mapping indirect.

### 2. Chargement Dynamique de Modèles OBJ (Partie 2 du sujet)

- **Intégration de TinyOBJLoader** : Téléchargement et intégration de la bibliothèque header-only `tiny_obj_loader.h` pour le parsing de fichiers `.obj`.
- **Fonction `LoadOBJ()`** : Fonction générique de chargement OBJ qui parse les positions, normales et coordonnées UV, crée les buffers OpenGL (VAO/VBO/EBO) et configure les attributs de vertex.
- **Chargement dynamique du dragon** : Le modèle du dragon est chargé depuis `dragon.obj` au lieu d'utiliser les données codées en dur de `DragonData.h`. Le fichier `dragon.obj` a été exporté à partir des données originales.

### 3. Plan de Sol Damier (Ground Plane)

- Ajout d'un grand plan subdivisé à `y = -1.0f` pour ancrer la scène et donner une forte impression de parallaxe lors des translations de la caméra.
- Implémentation d'un shader de damier procédural (`u_UseTexture == 2` dans `object.fs`) utilisant les coordonnées UV pour alterner des cases grises sans charger d'image de texture externe.



## Compilation et exécution

Se placer dans le dossier du projet et lancer :

```bash
g++ -std=c++17 main.cpp common/GLShader.cpp -o main -lGL -lglfw -ldl -pthread
./main
```

## Fichiers principaux

- `main.cpp` : logique de rendu, boucle principale, gestion des entrées, chargement OBJ et configuration de la scène
- `basic.vs` / `basic.fs` : shaders du skybox
- `object.vs` / `object.fs` : shaders pour les objets 3D (Blinn-Phong, Environment Mapping, Fresnel, Gamma)
- `tiny_obj_loader.h` : bibliothèque header-only pour le chargement de modèles OBJ
- `dragon.obj` : modèle 3D du dragon au format Wavefront OBJ
- `DragonData.h` : données géométriques du dragon (conservé comme référence)
- `texture3/` : images des 6 faces de la cubemap
- `dragon.png` : texture du dragon

