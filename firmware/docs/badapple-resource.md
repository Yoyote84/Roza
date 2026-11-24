# BadApple — installation de la ressource SD

Le firmware ROZA ne contient volontairement pas la vidéo : le Cardputer ADV n'a pas de PSRAM et le fichier d'images séquentielles occupe plusieurs mégaoctets. L'application lit une ressource locale depuis la microSD 128 Go.

1. Préparez une vidéo source que vous êtes autorisé à utiliser et installez Python, NumPy, FFmpeg et FFprobe sur votre ordinateur.
2. Depuis la racine du dépôt, exécutez :

   ```bash
   python3 -m pip install numpy
   python3 firmware/tools/badapple_pack.py source.mp4 -o badapple.dat --fps 30
   ```

3. Créez le dossier `video` à la racine de la microSD et copiez-y `badapple.dat` :
   `/video/badapple.dat`.
4. Insérez la carte dans le Cardputer ADV et ouvrez BadApple. Les anciennes cartes avec le dossier `/vidéo/badapple.dat` (accent) sont également reconnues.

Le format attendu est `BAP1`, 240×135, 1 bit par pixel, en-tête little-endian de 16 octets. ROZA vérifie l'en-tête et la taille minimale attendue avant lecture ; un fichier tronqué est refusé.

Le dépôt ne fournit pas de vidéo source ni de `badapple.dat` préfabriqué. Le message de ressource manquante ne peut donc être entièrement levé qu'après copie du fichier converti sur la carte SD.
