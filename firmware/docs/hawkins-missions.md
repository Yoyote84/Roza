# Hawkins — Étape 5 : missions hors ligne

Le module est maintenant raccordé à l'application Hawkins, à l'exploration GNSS, au profil du personnage et à la NVS de ROZA.

## Missions

1. **Le premier signal** — découvrir un premier signal anormal ; récompense : 100 XP.
2. **Cartographie** — explorer trois secteurs distincts ; récompense : 75 XP.
3. **Piste de l'anomalie** — trouver deux signaux ; récompense : 150 XP.

Les missions sont débloquées dans cet ordre. Les événements sont enregistrés uniquement après validation d'un nouveau secteur par le moteur d'exploration.

## Sauvegarde et reprise

- Deux emplacements NVS `miss0` / `miss1`, avec numéro de séquence, format explicite, CRC-32, alternance et vérification de lecture.
- Le dernier identifiant de cellule haché permet de dédupliquer un événement répété après une coupure entre la sauvegarde de la mission et celle de l'exploration.
- Les récompenses sont protégées par un masque de trois bits sauvegardé dans le profil du personnage. Une mission déjà payée ne crédite pas une deuxième fois ses XP au redémarrage.
- Si une sauvegarde de mission échoue, l'état d'exploration en mémoire est restauré et le secteur pourra être retenté.

## Commandes

- `M` : ouvrir / fermer les missions.
- `P` : ouvrir le personnage.
- `E` : revenir au terrain.
- `S` : forcer la sauvegarde locale.

## Réseau et matériel

Tout le moteur de missions est local. Le GNSS fonctionne par réception satellite ; aucune connexion Wi-Fi n'est activée. Le SX1262 n'est pas sollicité par les missions. Aucun accès ou formatage de la microSD n'est effectué.

## Vérification restante

Les tests natifs sont intégrés à `firmware/tools/hawkinstest/build.sh`, appelé par le workflow ROZA Firmware Build. Une compilation PlatformIO et un essai sur Cardputer ADV + Cap LoRa-1262 restent nécessaires avant le flash réel.
