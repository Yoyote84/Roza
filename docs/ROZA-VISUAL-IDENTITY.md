# Identité visuelle РO3A

## Intention

РO3A imagine le Cardputer ADV comme un instrument personnel que Roza Chanina aurait pu emporter avec elle si cette technologie avait existé dans les années 1940. Il ne s'agit pas d'un thème cyber ou d'un décor militaire générique : l'appareil doit évoquer un petit instrument de terrain, entretenu et utilisé au quotidien.

## Principes non négociables

- Préserver toutes les applications, leurs fonctions, raccourcis, réglages et compatibilités.
- Modifier la présentation, jamais la réalité fonctionnelle d'une application.
- Réutiliser les primitives de dessin et composants partagés existants avant d'en créer.
- Garder les écrans lisibles à 240 × 135 pixels ; la densité et la lisibilité priment sur l'ornement.
- Ne pas utiliser de textures ou animations coûteuses en RAM/Flash.
- Ne pas afficher de faux états matériels dans le firmware réel.
- Garder le fonctionnement hors ligne et le comportement radio existant.

## Palette de référence

| Rôle | Couleur | RGB |
|---|---|---|
| Fond / encre | Vert encré très sombre | #303629 |
| Panneaux | Olive sombre | #424A24 |
| Traits | Olive patiné | #59654A |
| Accent principal | Olive clair | #667044 |
| Texte principal | Ivoire vieilli | #E8DFC5 |
| Texte secondaire | Ivoire atténué | #C6B894 |
| Alerte | Bordeaux | #783D3D |

Les valeurs RGB565 sont centralisées dans `firmware/src/roza_theme.h`. Le bordeaux est réservé aux alertes et aux états qui le justifient, pas à la décoration.

## Vocabulaire graphique

- Plaques d'identification, filets fins, repères de gravure, index et graduations.
- Rose des vents, repères cartographiques et symboles d'instruments lorsque cela sert la fonction.
- Panneaux sobres et contrastes francs ; pas de néons, glow, grille numérique ou animation de décryptage.
- Français par défaut ; quelques inscriptions cyrilliques uniquement si la police et l'encodage les rendent lisibles.
- Les catégories restent identifiées par leur nom et leurs icônes ; elles ne changent plus de couleur comme un arc-en-ciel.

## Déploiement progressif

1. Fondation de palette et écran de démarrage / menu principal.
2. Barre d'état, titres et composants communs.
3. Applications locales (horloge, fichiers, calculatrice, lecteur).
4. Applications de navigation et d'observation (GNSS, carte, météo, ciel).
5. Radio, réseau et outils avancés.
6. Paramètres, dialogues et états d'erreur.
7. Revue visuelle complète dans `firmware/tools/uisim`.

Chaque étape doit conserver les fonctions et être compilée par les workflows PlatformIO et UI Viewer. Les captures du simulateur sont des rendus logiciels, pas une preuve de fonctionnement sur la carte réelle.
