# Coupure de tronçon (Ag Open GPS)
Solution complète de coupure de tronçon utilisant le logiciel libre Ag Open GPS et un contrôleur Arduino. Cette solution permet de contrôler des tronçons automatiquement (via positionnement GPS) et manuellement sur pulvérisateur, semoir à distribution électrique ou bineuse (relevage rang à rang). Le coût est dérisoire (environ 300€ de matériel électronique + une tablette Windows) et le système s'adapte facilement sur toutes les machines. 
Illustration d'utilisation ici : https://www.youtube.com/watch?v=-jRzunTiOS0&feature=youtu.be

Comment ça fonctionne ?

Le logiciel (Ag Open GPS, installé sur une tablette exécutant Windows 10) reçoit des informations de positionnement via un module et une antenne GPS (de préférence double bande, L1 et L2). Le logiciel interprète le positionnement et donne l’ordre à l’Arduino d’ouvrir ou fermer les tronçons grâce à des relais. L’Arduino envoie également des informations au logiciel, notamment lorsque l’on appuie sur les boutons, ce qui permet de commander le logiciel (sans avoir besoin de toucher l’écran tactile). 


![Dessin_général](https://user-images.githubusercontent.com/65913566/109956758-56779f80-7ce4-11eb-8312-71eaa2b2a2a8.png)

A l’heure actuelle le logiciel supporte 16 tronçons, mais le code lui n’en supporte que 8.

* Vous trouverez dans le fichier « Boitier » la liste des pièces nécessaires à la réalisation du boitier comme le mien, ainsi que les schémas de branchement. Un tuto vidéo explique les étapes de réalisation du boitier : bientôt disponible. L'idéal est de réutiliser votre boitier de commande du pulvérisateur existant (attention il faut des intérupteurs momentannés ou des boutons).

* Dans le fichier « Code » vous trouverez le code Arduino (V0.9) à téléverser sur votre contrôleur ainsi qu’un tableur détaillant le format de communication des données entre le logiciel et le contrôleur (si vous souhaitez modifier le code). Un tuto vidéo explique le fonctionnement du code : bientôt disponible.
 
Remarque : des versions futures du code prendront en compte l'utilisation du boitier sans le logiciel et intégreront la gestion de 16 tronçons. 
