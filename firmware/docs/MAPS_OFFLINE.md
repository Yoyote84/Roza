# Cartes GNSS hors ligne — France et Europe

ROZA conserve le firmware dans la mémoire interne du Cardputer ADV. Les données cartographiques détaillées sont séparées et résident sur la microSD.

## Format attendu

Le lecteur actuel décode des images **JPEG de 256 × 256 pixels**. Il ne décode pas les tuiles PNG standard d'OpenStreetMap France sur cette cible sans PSRAM.

Arborescence recommandée :

```text
/map/
  wgs84
  attribution.txt
  5/
    16/
      11.jpg
  14/
    8290/
      5620.jpg
```

Le nommage est celui des tuiles Web Mercator XYZ : `zoom/x/y.jpg`. Le marqueur vide `/map/wgs84` indique que les coordonnées du paquet sont en WGS-84 (OSM), sans décalage GCJ-02. Le lecteur accepte aussi `/tiles/{z}/{x}/{y}.jpg`, `/map/{z}/{x}_{y}.jpg` et l'extension `.jpeg`.

## Préparer la microSD

1. Sauvegarder son contenu avant toute manipulation.
2. Utiliser une carte microSD de test, en FAT32 pour le premier essai.
3. Copier le dossier `map` à la racine de la carte.
4. Créer le fichier vide `/map/wgs84`.
5. Copier un paquet de tuiles JPEG XYZ couvrant la France ou la zone souhaitée.
6. Ajouter `/map/attribution.txt` indiquant la source et sa licence. Pour des données OpenStreetMap, conserver la mention « © OpenStreetMap contributors » et le lien https://www.openstreetmap.org/copyright.

Ne pas utiliser la fonction « Format SD » de ROZA pour préparer la carte : elle supprime les fichiers et ne crée pas un système de fichiers.

## Comportement dans ROZA

- La microSD est consultée avant toute source distante.
- Le profil standard désactive le téléchargement distant et utilise WGS-84.
- Si une tuile est absente, ROZA laisse un emplacement vide ; si aucune tuile ne peut être chargée, l'écran revient au fond mondial simplifié.
- Aucun fond cartographique détaillé n'est inclus dans le firmware : la taille de la carte dépend du paquet copié sur la microSD.
- L'écran affiche l'attribution OpenStreetMap lorsque des tuiles de la microSD sont utilisées.

## Respect des licences et des serveurs

Ne pas aspirer massivement les serveurs publics de tuiles. Respecter les conditions d'utilisation, les limites de débit et les obligations d'attribution du fournisseur. Le service public standard d'OpenStreetMap n'est pas un service de téléchargement en masse ni un fournisseur de paquets hors ligne. Pour une utilisation hors ligne, utiliser un paquet légalement obtenu ou généré à partir de données dont la licence et les conditions autorisent cet usage.

## Limites connues

- Ce changement prépare ROZA à afficher des cartes WGS-84 stockées sur SD ; il ne télécharge pas et ne fabrique pas automatiquement un paquet France.
- La génération d'un paquet JPEG à partir des tuiles PNG doit être faite hors du Cardputer, puis les fichiers copiés sur la microSD.
- La lecture, le temps de chargement et la compatibilité de la carte physique seront validés après le premier flash.
