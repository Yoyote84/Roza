# Tests natifs du moteur Hawkins LLM

Cette suite compile le moteur REZOR réellement importé dans `src/hawkins_llm/llm.cpp` en mode scalaire, avec des remplacements hôte des API heap_caps et FreeRTOS. Elle exécute des tests négatifs d'initialisation :

- longueur de fichier trop courte (y compris pointeur nul, qui doit être rejeté avant lecture) ;
- signature CRDP incorrecte ;
- version de format non prise en charge.

Ces tests ne remplacent pas un essai d'inférence avec un modèle réel. Les poids français seront préparés et testés à l'étape 3.

Les adaptateurs hôte proviennent du projet REZOR Cardputer AI, licence MIT ; voir `../../src/hawkins_llm/LICENSE`.
