# BadApple — préparer le fichier vidéo sur microSD

Le lecteur ROZA ne décode pas les MP4 directement. Le Cardputer ADV n'a pas de PSRAM et le lecteur natif utilise un format compact 1 bit par pixel, lu en continu depuis la microSD. Aucun fichier vidéo n'est inclus dans le dépôt : il faut convertir une vidéo que tu possèdes et dont tu as le droit de faire une copie.

## Format accepté

- Résolution fixe : 240 × 135 pixels
- Monochrome 1 bit par pixel, ordre des bits MSB-first
- 16 octets d'en-tête (`BAP1`, largeur, hauteur, FPS, réservé, nombre d'images)
- 4 050 octets par image, sans piste audio
- Emplacement exact sur la carte : `/video/badapple.dat`

À 30 images/s, le flux occupe environ 121 500 octets/s, soit environ 7,3 Mo par minute. À 20 images/s, il occupe environ 4,86 Mo par minute. Une carte microSD de 128 Go offre largement la capacité nécessaire.

## Conversion

Sur un ordinateur avec Python 3, ffmpeg, ffprobe et NumPy :

```sh
python3 -m pip install numpy
python3 firmware/tools/badapple_pack.py source.mp4 -o badapple.dat
```

Pour réduire le débit si l'écran n'arrive pas à suivre :

```sh
python3 firmware/tools/badapple_pack.py source.mp4 -o badapple.dat --fps 20
```

Le convertisseur redimensionne l'image, la passe en niveaux de gris puis en noir/blanc et écrit le format `BAP1`. Le format cible est silencieux : la piste audio du fichier d'origine n'est pas conservée.

## Installation sur la microSD

1. Créer un dossier `video` à la racine de la microSD.
2. Copier le fichier converti sous le nom exact `/video/badapple.dat`.
3. Insérer la carte et ouvrir BadApple dans ROZA.

Le lecteur signale `Missing /video/badapple.dat` si le fichier n'est pas présent. `Bad file header` ou `Unsupported format` indique un fichier qui ne respecte pas le format attendu. Le lecteur n'écrit pas et ne formate pas la carte.

## Limite actuelle

Ce lecteur est une voie de lecture optimisée pour une animation monochrome et silencieuse. Pour lire des MP4 couleur avec le son intégré, il faudrait un autre pipeline de décodage matériel/logiciel et des compromis importants sur la mémoire, le débit SPI et la fréquence d'image ; ce n'est pas une capacité du lecteur BadApple actuel.
