# Hawkins — Étape 5 : missions hors ligne
Le module `hawkins_missions.h/.cpp` fournit un noyau de missions sans réseau ni dépendance ESP32.
Missions définies : premier signal, cartographie de trois secteurs, piste de deux découvertes.
La progression s'appuie sur des compteurs de secteurs et de découvertes. Les récompenses XP sont définies dans le moteur.
Cette livraison expose le module et l'intègre à la compilation des tests natifs. L'interface Hawkins et le stockage NVS des missions ne sont pas encore raccordés : cela reste une sous-étape à terminer avant l'utilisation sur l'appareil.
