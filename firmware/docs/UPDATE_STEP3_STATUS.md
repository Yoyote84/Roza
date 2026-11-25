# ROZA — étape 3/5 : sécurisation des mises à jour

## État du dépôt (25 novembre 2026)

Le mécanisme A/B OTA, la signature ECDSA P-256 du manifeste, la validation TLS, SHA-256, le contrôle de version, l'installation sur le slot inactif, et le rollback ESP-IDF étaient déjà présents dans le dépôt. Cette étape ne les réécrit pas à l'aveugle : elle renforce leurs tests de garde.

Commit de renforcement : `37193e54a3a8dba20d4ba6d3fcaa6f993e3cbbab`.

## Protection déjà intégrée

- Manifeste limité en taille et en schéma, ciblé Cardputer ADV.
- URL du binaire fixée, manifeste signé ECDSA P-256 et vérification côté firmware avec la clé publique embarquée.
- SHA-256, taille, hash Git, numéro de build et signature contrôlés.
- Refus de versions égales/anciennes en s'appuyant sur build actuel et plus haut build confirmé en NVS.
- Relecture et revalidation du manifeste avant installation; vérification que les champs n'ont pas changé entre la consultation et l'installation.
- HTTPS avec CA validée; pas de TLS insecure.
- Flux binaire écrit par blocs, annulation et erreurs d'écriture gérées, validation SHA-256 par Update, abandon sur échec.
- Deux slots OTA de 0x3F0000, otadata, rollback ESP-IDF configuré.
- Confirmation du nouveau firmware après contrôle de démarrage essentiel; rollback sur échec.
- Mise à jour sans accès ni modification de la carte microSD.

## Renforcement de tests ajouté

Le script `firmware/tools/ota_security_test.py` ajoute des tests structurels qui vérifient :
- URL de téléchargement verrouillée et parser qui échoue fermé;
- taille et formats de hash/commit;
- revalidation du manifeste avant écriture;
- SHA-256, abandon et fin d'installation;
- TLS HTTPS et absence de `setInsecure()`;
- confirmation/rollback et monotonie du numéro de build;
- migration USB sans chip erase et confirmation explicite;
- conservation de NVS/PHY et non-intervention sur la SD.

La suite de sécurité existante vérifie aussi la signature cryptographique générée, l'altération du payload, la table de partitions, la clé publique et le rollback configuré.

## Migration initiale (important)

Les anciennes installations à partition factory unique doivent faire une migration USB unique vers la table A/B. Le script `tools/migrate_to_ota.sh` exige la chaîne `MIGRATE-ROZA`, ne fait aucun chip erase et efface seulement la zone otadata `0xF000..0x10FFF` avant d'écrire le bootloader, la table et l'application OTA_0. NVS et PHY restent à leurs adresses. La carte microSD n'est jamais touchée.

Cette migration initiale est une opération de flashage et doit être exécutée uniquement avec le kit de migration publié par une CI validée, après sauvegarde des réglages importants et lecture de la fiche d'installation. Ne pas flasher l'image full-flash si l'objectif est de conserver NVS : elle réinitialise les réglages.

## Éléments qui nécessitent encore confirmation

- [ ] CI du commit de renforcement `37193e5` : tests, build et packaging doivent être réussis.
- [ ] Configuration GitHub Actions du secret `ROZA_OTA_SIGNING_KEY_PEM` : inaccessible depuis le connecteur, et non confirmée. Sans ce secret, le pipeline ne publie pas de manifeste OTA installable signé; ne pas utiliser de manifeste non signé.
- [ ] Essai sur Cardputer ADV réel : migration USB, vérification, installation OTA, démarrage, retour arrière simulé si possible. À faire en étape 5.
- [ ] Secure Boot et chiffrement flash : volontairement non activés automatiquement, car leur provisioning peut rendre la récupération difficile ou irréversible.

## Conclusion

Le code de l'architecture de sécurité OTA est intégré; le commit de renforcement ajoute des garde-fous de test. La clôture complète de l'étape 3 dépend de la CI de ce commit, du secret de signature de publication et des essais matériels prévus à l'étape 5. Ne pas prétendre qu'une mise à jour OTA signée est publiquement disponible tant que le secret et la publication n'ont pas été vérifiés.
