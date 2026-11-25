# ROZA — sécurité des mises à jour

**Architecture cible :** A/B OTA, ESP32-S3FN8, Flash 8 Mio, sans PSRAM.

## Ancien format et migration unique

L'ancienne table ROZA ne possédait qu'une partition factory à 0x10000. Elle ne permettait ni A/B OTA ni retour arrière. Une migration USB unique est donc nécessaire avant la première mise à jour OTA.

La nouvelle table conserve exactement NVS (0x9000, taille 0x5000) et PHY init (0xE000, taille 0x1000), puis ajoute :
- otadata : 0xF000, taille 0x2000 ;
- ota_0 : 0x20000, taille 0x3F0000 ;
- ota_1 : 0x410000, taille 0x3F0000.

Le script de migration vérifie d'abord SHA256SUMS-migration.txt pour les quatre fichiers du kit (bootloader, table, application et script), puis exige MIGRATE-ROZA. Il efface uniquement les 8 Kio de otadata (0xF000..0x10FFF), qui chevauchent l'ancienne application factory ; il écrit ensuite bootloader, table et application dans OTA_0. NVS et PHY restent à leurs adresses d'origine. Il n'utilise jamais de commande de chip erase. Les sommes SHA-256 détectent une corruption accidentelle, mais ne remplacent pas une signature d'authenticité du kit USB.

**La microSD n'est jamais utilisée par le mécanisme OTA.** Aucun montage, formatage, effacement ou écriture sur la carte n'est nécessaire.

## Chaîne OTA

1. Le manifeste est téléchargé en HTTPS avec validation du certificat. Son en-tête Content-Length doit être connu et inférieur ou égal à 2 Kio ; le client lit au maximum cette taille avant de parser le JSON.
2. Sa signature ECDSA P-256 est vérifiée avec la clé publique intégrée au firmware.
3. La cible, le schéma, l'URL autorisée, le numéro de build, la taille, le SHA Git et le SHA-256 sont contrôlés.
4. Le build proposé doit être plus récent que le firmware courant et que le numéro le plus élevé confirmé en NVS.
5. L'application est écrite dans le slot inactif. Update.setSHA256() et Update.end() contrôlent l'intégralité du binaire avant d'activer le nouvel emplacement.
6. Au premier démarrage, le bootloader place l'application en attente de confirmation. ROZA ne la confirme qu'après le contrôle essentiel écran/canvas. Si elle redémarre ou perd l'alimentation avant confirmation, le bootloader doit revenir au slot précédent.

Le rollback dépend de CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y, défini dans firmware/sdkconfig.defaults et transmis au build pioarduino par custom_sdkconfig.

## Signature et secret GitHub

La clé privée n'est jamais stockée dans le dépôt. Le workflow Pages attend le secret Actions ROZA_OTA_SIGNING_KEY_PEM. Sans ce secret, les images USB sont construites mais aucun manifeste OTA non signé n'est publié ; le client échoue fermé.

La signature porte sur une représentation canonique qui lie la cible, le build, le commit, la taille, le SHA-256 et l'URL exacte du binaire. Avant de produire un manifeste, l'outil de publication dérive la clé publique de la clé privée CI et compare son encodage DER à la clé publique embarquée ; une paire incohérente fait échouer la publication. Le manifeste seul ne peut donc pas être modifié pour pointer vers une autre image ou une autre version.

## Configuration initiale du secret GitHub Actions

La clé privée de signature correspond à la clé publique intégrée dans `firmware/src/ota_public_key.h`. Elle est fournie séparément au propriétaire du dépôt et ne doit jamais être commitée, publiée dans un artefact public ou envoyée dans une issue.

Dans GitHub :
1. Ouvrir le dépôt `Yoyote84/Roza`, puis **Settings → Secrets and variables → Actions**.
2. Choisir **New repository secret**.
3. Nom exact : `ROZA_OTA_SIGNING_KEY_PEM`.
4. Coller le contenu intégral du fichier PEM privé fourni séparément, en conservant les lignes BEGIN/END.
5. Enregistrer le secret.

Le workflow Pages le consomme uniquement pendant la création du manifeste signé. Sans ce secret, aucun manifeste OTA installable n'est publié. Le connecteur GitHub utilisé pour les modifications du code ne permet pas de créer des secrets de dépôt ; cette étape de configuration doit donc être faite par le propriétaire dans l'interface GitHub.

## Limites de sécurité

Secure Boot, chiffrement Flash et anti-rollback eFuse ne sont pas activés automatiquement : ces protections irréversibles nécessitent un provisioning matériel, une stratégie de récupération et des essais dédiés. La CI ne simule pas une coupure d'alimentation ni un Cardputer physique ; les essais de l'écran, du clavier, du GNSS, du SX1262 et du rollback réel restent à faire à l'étape 5/5.
