# Réglage de l'heure et profil GitHub

Dans **Paramètres > Fuseau horaire**, choisir **Central Europe**, puis appuyer sur **m** pour régler manuellement la date et l'heure.

- `, / : sélectionner année, mois, jour, heure ou minute.
- - / = : diminuer ou augmenter la valeur.
- Entrée : appliquer et mémoriser.
- Retour : annuler.

Le jour de la semaine est calculé automatiquement. Le fuseau CET/CEST applique automatiquement les changements d'heure d'hiver et d'été en France.

La dernière heure manuelle est mémorisée et restaurée au démarrage. Limite matérielle : le Cardputer ADV ne possède pas de RTC sauvegardée par pile. Après une extinction complète, il ne connaît pas la durée écoulée ; l'heure redémarre donc depuis la dernière valeur mémorisée et ne peut pas rester parfaitement exacte pendant l'arrêt. Une synchronisation GNSS reste la solution hors ligne la plus précise. NTP peut corriger l'heure lorsque le Wi-Fi est utilisé.

Dans l'application **GitHub**, appuyer sur **u**, saisir le pseudo (sans @), puis Entrée. Le pseudo est conservé dans le Cardputer. **r** actualise la heatmap des contributions. Aucun jeton API n'est requis pour un profil public ; le Wi-Fi et une horloge correcte sont nécessaires pour HTTPS.
