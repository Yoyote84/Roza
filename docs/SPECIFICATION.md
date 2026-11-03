# РОЗА — spécification produit

## Principes non négociables

1. Fonctionnement local sans compte, serveur ou Internet.
2. Interface française ; cyrillique réservé aux titres, marques et détails historiques.
3. Wi-Fi activé uniquement par action explicite de l'utilisateur.
4. Bluetooth désactivé par défaut.
5. Économie d'énergie mesurée, jamais annoncée sur simple estimation.
6. Une application ne commande pas directement un périphérique : elle passe par un service.
7. Les données utilisateur ne sont pas stockées uniquement en RAM.
8. Les erreurs d'une application ne doivent pas faire redémarrer tout le système.
9. Aucun message radio n'est transmis sans action ou règle explicite.
10. Les mises à jour doivent prévoir une voie de récupération.

## Identité visuelle

- Palette : vert olive, ivoire/papier, noir d'encre, rouge bordeaux.
- Contraste élevé et lisibilité sur 240 × 135 pixels.
- Typographie bitmap compacte, lisible en français (accents compris).
- Quelques inscriptions cyrilliques, jamais pour les commandes essentielles.
- Pas de néon, de transparence décorative, de parallaxe ou d'animations coûteuses.

## Applications prévues

### Première tranche
- Accueil / lanceur
- Carnet et journal
- Agenda et tâches
- Calculatrice
- Gestionnaire de fichiers et lecteur texte
- Paramètres / état de la batterie

### Tranches suivantes
- GNSS : position, coordonnées, traces, points d'intérêt.
- Radio : messages LoRa, contacts, boîte de réception, historique.
- Bibliothèque : documents et cartes préchargés.
- Audio : enregistrement et lecture selon mémoire et autonomie disponibles.
- Wi-Fi facultatif : synchronisation locale, jamais requis pour le fonctionnement normal.

## Profils énergétiques

- Veille : écran et périphériques non nécessaires arrêtés ; réveil clavier selon capacités matérielles.
- Quotidien : réseau et GNSS arrêtés.
- Navigation : GNSS activé à la demande, fréquence d'acquisition configurable.
- Communication : SX1262 activé à la demande ; veille radio quand le protocole et le matériel le permettent.
- Maintenance : Wi-Fi activé manuellement, extinction dès la fin de session.

## Limites connues

L'ESP32-S3FN8 dispose de 8 Mo de Flash et de ressources RAM limitées. РОЗА n'est pas Android :
pas d'appels cellulaires natifs, pas de navigateur moderne complet, pas de multitâche lourd.
Les cartes devront être vectorielles ou rasterisées à faible résolution et chargées par tuiles.
