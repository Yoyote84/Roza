# Moteur RPG Hawkins — étapes 1 à 5

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
- 24 identifiants de secteurs récents sont conservés pour reconnaître les retours. Le compteur global continue au-delà de cette fenêtre.
- Seuls des identifiants hachés de cellules sont sauvegardés. Les coordonnées latitude/longitude brutes ne sont ni affichées ni écrites en NVS.
- Les données d'exploration sont dans la NVS interne, sous deux emplacements expl0/expl1 avec CRC-32, vérification après écriture et conservation du précédent emplacement valide.
- Le Wi-Fi n'est jamais activé par Hawkins. Le SX1262 n'est pas utilisé pour cette étape.

## Étape 4 — personnage et progression

- hawkins_character.h/.cpp : création du personnage, six archétypes, six caractéristiques, points de vie, stress, expérience et niveaux.
- Classes proposées : Explorateur, Technicien, Enquêteur, Protecteur, Médecin et Sensitif.
- Création locale : nom ASCII de 1 à 12 caractères, puis choix de classe au clavier. Aucun compte, service distant ou identifiant réseau.
- Caractéristiques : Force, Endurance, Agilité, Intelligence, Perception et Charisme.
- Les points de vie max sont dérivés de l'Endurance ; la capacité de stress est dérivée de la Perception.
- Un secteur inédit rapporte 25 XP ; une découverte narrative rapporte 50 XP. Un passage de niveau ajoute un point de compétence non dépensé, réservé aux futurs arbres de compétences.
- Le personnage doit être créé avant l'exploration récompensée : cela évite de perdre les XP si le joueur part en exploration sans profil.
- Le profil est sauvegardé dans la NVS sous char0 et char1, avec format binaire versionné, CRC-32, alternance des emplacements et vérification de lecture.
- Le profil est indépendant de l'exploration et du monde. La perte ou la corruption d'un emplacement ne déclenche aucun effacement automatique.
- Depuis Hawkins, P ouvre le personnage, E alterne Terrain/Monde, M ouvre les missions et S force les sauvegardes.
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

## Sauvegarde et stockage

Les données du monde, de l'exploration et du personnage sont stockées dans la NVS interne. Le verrou PrefsLock existant est réutilisé pour éviter les accès concurrents au gestionnaire Preferences.

Aucun formatage, effacement ou écriture n'est effectué sur la microSD. Les fonctions existantes de partage SPI entre la SD et le SX1262 ne sont pas modifiées.

## Tests logiciels

La suite native hawkinstest vérifie :
- l'évolution déterministe et le rattrapage plafonné ;
- la progression de journées sans horloge ;
- la conversion de coordonnées en cellules et le rejet de coordonnées invalides ;
- les visites, retours et découvertes ;
- la création, les limites du nom, les attributs, les gains d'XP, les niveaux, les récompenses idempotentes et les sauvegardes CRC du personnage ;
- le déverrouillage séquentiel, les doublons de secteurs, les sauvegardes CRC et la restauration des missions.

La compilation native ne remplace pas les essais physiques. Le GPS peut ne pas obtenir de fix en intérieur ou dans un environnement masqué.

## Limites restantes

- Les missions scénarisées, les dialogues à choix, l'inventaire, les objets, les compétences dépensables, les combats et les interactions avec les factions restent à développer.
- Les points de compétence sont comptés mais pas encore dépensables.
- Les dégâts et le stress ne changent pas encore ; leur gestion est réservée au système de jeu suivant.
- Les essais physiques sur Cardputer ADV + Cap LoRa-1262 restent nécessaires.
