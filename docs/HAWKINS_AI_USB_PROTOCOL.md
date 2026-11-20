# Pont IA locale Hawkins par USB CDC

Cette livraison introduit le protocole de transport HKA/1 et ses tests natifs. Le pont ne modifie pas les sauvegardes ni l'état du personnage.

## Format

Les trames sont des lignes UTF-8 précédées de `HKA/1 `, terminées par LF. L'application compagnon doit utiliser la liaison USB série du Cardputer, pas le Wi-Fi.

- Téléphone vers Cardputer : `{"type":"hello","version":1}`
- Cardputer vers téléphone : `{"type":"narrate","id":N,"context":{...}}`
- Téléphone vers Cardputer : `{"type":"narration","id":N,"text":"..."}`
- Erreur : `{"type":"error","id":N,"message":"..."}`

Le firmware doit limiter la taille des trames, vérifier le numéro de requête, et ne jamais appliquer des actions de gameplay proposées par le modèle sans validation explicite.

## État

Cette livraison est une fondation testée en CI, et non une intégration complète du port USB ni l'application Android d'inférence. Le passage à l'étape 5 est prématuré tant que ces éléments ne sont pas livrés et compilés.
