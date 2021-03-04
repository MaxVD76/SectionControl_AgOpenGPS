# SectionControl_AgOpenGPS
Solution complète de coupure de tronçon utilisant le logiciel libre Ag Open GPS et un contrôleur Arduino. Cette solution permet de contrôler des tronçons automatiquement (via positionnement GPS) et manuellement sur pulvérisateur, semoir à distribution électrique ou bineuse (relevage rang à rang). Le coût est dérisoire (environ 300€ de matériel électronique + une tablette Windows) et le système s'adapte facilement sur toutes les machines. 

Comment ça fonctionne ?

Le logiciel (Ag Open GPS, installé sur une tablette exécutant Windows 10) reçoit des informations de positionnement via un module et une antenne GPS (de préférence double bande, L1 et L2). Le logiciel interprète le positionnement et donne l’ordre à l’Arduino d’ouvrir ou fermer les tronçons grâce à des relais. L’Arduino envoie également des informations au logiciel, notamment lorsque l’on appuie sur les boutons, ce qui permet de commander le logiciel (sans avoir besoin de toucher l’écran tactile). 


![Dessin_général](https://user-images.githubusercontent.com/65913566/109956758-56779f80-7ce4-11eb-8312-71eaa2b2a2a8.png)
