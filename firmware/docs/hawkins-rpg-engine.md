# Moteur RPG Hawkins — étapes 1 à 6

Le jeu est intégré au firmware ROZA existant. Il conserve les pilotes matériels de ROZA et ne remplace ni les fonctions GNSS, ni les fonctions LoRa, ni l'interface principale.
La cible reste le Cardputer ADV ESP32-S3FN8, Flash 8 Mo, sans PSRAM.

## Étape 1 — monde persistant déterministe

- hawkins_world.h/.cpp : état du monde, évolution journalière, statistiques et événements.
- Une graine identique et les mêmes dates donnent la même évolution.
- Aucun état de fin de campagne n'existe dans le moteur.
- Le rattrapage temporel est plafonné à 4096 jours par appel, sans plafonner la durée totale du monde.

## Étape 2 — sauvegarde et première interface

- hawkins_save_codec.h/.cpp : format explicite little-endian, versionné, 58 octets, CRC-32.
- hawkins_nvs.h/.cpp : deux emplacements NVS alternés ; la dernière sauvegarde valide est préservée pendant l'écriture de la suivante.
- hawkins_runtime.h/.cpp : restauration du monde, progression et sauvegarde.
- hawkins_app.h/.cpp : page Hawkins dans un groupe RPG séparé du menu ROZA.
- Tableau de bord : jour du monde, nombre d'événements, tension, anomalies, activité des factions et dernier événement.

## Étape 3 — exploration physique entièrement hors ligne

- hawkins_exploration.h/.cpp : conversion WGS-84 vers une grille locale de cellules de 250 m, identifiants déterministes, visites et découvertes.
- Le pilote GNSS existant du Cap LoRa-1262 est réutilisé ; aucune API web, géolocalisation IP, tuile distante ou connexion Wi-Fi n'est sollicitée par Hawkins.
- Un point GPS doit être valide, compter au moins cinq satellites et rester stable pendant 10 secondes avant d'être pris en compte.
- Le premier point fixe l'origine. Les nouveaux secteurs font progresser le calendrier d'une journée ; une découverte narrative est garantie tous les trois nouveaux secteurs.
- 128 identifiants de secteurs récents sont conservés dans une mémoire circulaire (contre 24 auparavant), afin de reconnaître davantage de retours. Le compteur global continue au-delà de cette fenêtre.
- Les sauvegardes d'exploration v1 (24 secteurs) sont migrées à la lecture vers le format v2 sans effacer la progression connue. Les deux emplacements NVS alternés restent protégés par CRC ; les archives SD acceptent également l'ancien bloc d'exploration.
- Les événements locaux sont générés de manière déterministe à l'entrée d'une maille GPS réelle et enregistrent l'identifiant haché de cette maille. Une découverte déclenche un signal ; les événements ordinaires dépendent des niveaux locaux de tension, d'anomalie et d'activité des factions.
- L'écran Terrain n'affiche un événement que lorsqu'il appartient à la maille actuellement sélectionnée. Les objectifs des missions donnent une consigne calculée depuis la mission active et la zone réellement visitée, sans adresse, bâtiment, lieu privé, destination fictive ou téléchargement de carte.
- Seuls des identifiants hachés de cellules sont sauvegardés. Les coordonnées latitude/longitude brutes ne sont ni affichées ni écrites en NVS.
- Les données d'exploration sont dans la NVS interne, sous deux emplacements expl0/expl1 avec CRC-32, vérification après écriture et conservation du précédent emplacement valide.
- Le Wi-Fi n'est jamais activé par Hawkins. Le SX1262 n'est pas utilisé pour cette étape.

## Étape 4 — personnage et progression

- hawkins_character.h/.cpp : création du personnage, six archétypes, six caractéristiques, points de vie, stress, expérience et niveaux.
- Classes proposées : Explorateur, Technicien, Enquêteur, Protecteur, Médecin et Sensitif.
- Création locale : nom ASCII de 1 à 12 caractères, puis choix de classe au clavier. Aucun compte, service distant ou identifiant réseau.
- Caractéristiques : Force, Endurance, Agilité, Intelligence, Perception et Charisme.
- Les points de vie max sont dérivés de l'Endurance ; la capacité de stress est dérivée de la Perception.
- Un secteur inédit rapporte 25 XP ; une découverte narrative rapporte 50 XP. Un passage de niveau ajoute un point de compétence, dépensable dans l'écran Compétences de l'étape 6.
- Le personnage doit être créé avant l'exploration récompensée : cela évite de perdre les XP si le joueur part en exploration sans profil.
- Le profil est sauvegardé dans la NVS sous char0 et char1, avec format binaire versionné, CRC-32, alternance des emplacements et vérification de lecture.
- Le profil est indépendant de l'exploration et du monde. La perte ou la corruption d'un emplacement ne déclenche aucun effacement automatique.
- Depuis Hawkins : P personnage, E Terrain/Monde, M missions, I inventaire, K compétences, Q rencontre scénarisée, D sauvegardes, S sauvegarde locale.
- Les trois bits réservés du format de sauvegarde du personnage conservent les récompenses de mission déjà créditées. Les anciennes sauvegardes (bits à zéro) restent lisibles par cette version ; la taille et la version du format ne changent pas.

## Étape 5 — missions et récompenses hors ligne

- `hawkins_missions.h/.cpp` : trois missions locales en chaîne : premier signal, cartographie de trois secteurs, puis piste de deux signaux.
- Le déverrouillage est séquentiel. Une mission suivante ne devient active qu'après l'achèvement de la précédente ; les compteurs déjà atteints sont pris en compte au déverrouillage.
- Les missions suivent uniquement les nouveaux secteurs confirmés par le GNSS local. Aucun serveur, Wi-Fi ou téléchargement n'est requis.
- La progression des missions est conservée dans la NVS interne sous `miss0` et `miss1`, avec format explicite little-endian, version, CRC-32, alternance et vérification après écriture.
- Le dernier identifiant haché de cellule est enregistré avec la progression : si l'appareil redémarre entre la sauvegarde de mission et celle de l'exploration, la reprise ne compte pas deux fois le même secteur.
- Récompenses : 100 XP pour le premier signal, 75 XP pour la cartographie et 150 XP pour la piste de l'anomalie.
- Le profil contient un masque de récompenses déjà créditées. Le moteur refuse de créditer deux fois le même identifiant de mission ; le masque est sauvegardé avec le personnage avant de poursuivre.
- La touche M affiche l'écran des missions, leurs états (verrouillée, active, terminée) et les compteurs de secteurs/signaux. P ouvre le personnage ; E revient au terrain.

### Progression hors ligne

La progression du personnage dépend des secteurs réellement découverts et du moteur local. L'activation future du Wi-Fi ne déclenche ni synchronisation obligatoire ni téléchargement de contenu. Aucun service réseau n'est utilisé pour créer, afficher, faire évoluer ou sauvegarder le personnage.

## Étape 6 — aventure, choix et sauvegarde microSD

- `hawkins_adventure.h/.cpp` ajoute un inventaire, quatre types d'objets, quatre compétences et une première rencontre narrative à choix.
- L'inventaire de départ comprend une trousse de soins, un thé calmant et une pile AA. Les trousses restaurent 8 PV (12 avec Premiers soins) ; le thé réduit le stress de 8 (12 avec Sang-froid). Les ressources de preuve et les piles sont conservées comme objets d'enquête.
- Les quatre compétences dépensent réellement un point gagné au passage de niveau et exigent une caractéristique minimale : Premiers soins (Endurance 7), Sang-froid (Perception 7), Bricolage (Intelligence 8), Observation (Perception 8).
- La première rencontre propose trois décisions : aider, enquêter ou se replier. Les attributs et compétences influencent la réussite ; XP, objets, PV et stress changent selon le choix. Le choix est enregistré et ne peut pas être rejoué pour obtenir plusieurs récompenses.
- La progression d'aventure est sauvegardée en NVS dans `adv0` et `adv1`, avec séquence, CRC-32, alternance des emplacements et relecture de vérification.
- L'option `D SD` ouvre le gestionnaire de sauvegarde. `A` active/désactive la copie automatique ; `B` crée une copie immédiate ; `R` puis `R` confirme la restauration ; `D` revient en jeu.
- La sauvegarde SD regroupe les cinq blocs du jeu : monde, exploration, personnage, missions et aventure (inventaire, compétences et choix). Les archives sont dans `/HAWKINS/SLOT0.HWS` et `/HAWKINS/SLOT1.HWS` ; une écriture temporaire est relue et contrôlée avant de remplacer le slot le plus ancien. Chaque bloc conserve aussi son propre CRC.
- L'option automatique est désactivée par défaut. L'activer crée et vérifie d'abord une sauvegarde complète ; ensuite chaque progression importante la recopie sur la carte. La touche B reste disponible pour une copie manuelle quand l'automatique est désactivée.
- La NVS reste la sauvegarde de fonctionnement. Une carte absente, pleine, illisible ou retirée ne bloque donc pas la partie ; une erreur SD est signalée sans effacer la copie interne. La restauration contrôle l'intégralité de l'archive avant de commencer les écritures NVS ; une panne NVS pendant la restauration peut néanmoins laisser une restauration partielle, signalée à l'écran.
- Aucune opération ne formate la carte ni ne supprime d'autres fichiers. ROZA doit d'abord monter la carte via son pilote SD existant ; une carte 128 Go non reconnue reste intacte et le jeu continue avec la NVS. Le type de système de fichiers et le fonctionnement de cette carte précise restent à confirmer sur le matériel.
- Le Wi-Fi démarre en arrière-plan au lancement seulement si ROZA a déjà mémorisé un réseau. La connexion est non bloquante et les tentatives réseau n'interviennent pas dans le moteur du RPG. Sans identifiants mémorisés, la radio Wi-Fi ne démarre pas. Jeu, GNSS, inventaire, missions et sauvegardes internes restent hors ligne.

## Sauvegarde et stockage

Les états restent d'abord dans la NVS interne, protégée par les codecs CRC existants et par `PrefsLock`. La microSD est une copie de sauvegarde complète facultative : elle n'est jamais nécessaire pour charger ou faire progresser une partie. La copie SD utilise deux slots alternés, un fichier temporaire et une relecture avant validation. Les fichiers étrangers au dossier `/HAWKINS` ne sont pas touchés.

Ne jamais formater automatiquement une carte de 128 Go. Si `sdMounted` est faux, Hawkins laisse la carte tranquille et garde la sauvegarde NVS.

## Tests logiciels

La suite native hawkinstest vérifie :
- l'évolution déterministe et le rattrapage plafonné ;
- la progression de journées sans horloge ;
- la conversion de coordonnées en cellules et le rejet de coordonnées invalides ;
- les visites, retours, mémoire circulaire de 128 secteurs, migration des sauvegardes v1, événements locaux rattachés à une maille GPS et objectifs dynamiques ;
- la création, les limites du nom, les attributs, les gains d'XP, les niveaux, les récompenses idempotentes et les sauvegardes CRC du personnage ;
- le déverrouillage séquentiel, les doublons de secteurs, les sauvegardes CRC et la restauration des missions ;
- les objets consommables, le plafonnement des effets, les prérequis et la dépense des points de compétence, les conséquences à choix unique et la validation CRC de l'état d'aventure.

La suite native ne simule pas le contrôleur SD ni une carte de 128 Go : les opérations de fichier doivent encore être validées sur la carte réelle et la compilation PlatformIO vérifie l'intégration ESP32.

La compilation native ne remplace pas les essais physiques. Le GPS peut ne pas obtenir de fix en intérieur ou dans un environnement masqué.

## Vérifications matérielles restantes

- Tester l'obtention du fix GNSS en extérieur avec le Cap LoRa-1262 et valider la stabilité du parcours.
- Vérifier que la carte microSD 128 Go est montée par le pilote ROZA. Le code ne la formate jamais ; le format actuellement présent sur la carte doit être reconnu par le pilote.
- Tester une sauvegarde automatique, une copie manuelle, un retrait de carte, puis une restauration. Vérifier aussi le cas d'une carte pleine et d'une coupure pendant l'écriture.
- Vérifier le démarrage Wi-Fi sur un réseau déjà enregistré, puis le fonctionnement complet du RPG sans point d'accès.
- Les combats, le multijoueur et une mise à jour de contenu du scénario par Wi-Fi ne font pas partie de cette étape ; le Wi-Fi est ici une disponibilité réseau facultative, pas une dépendance ni un mécanisme de téléchargement de scénario.
